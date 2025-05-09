enum CarDoorState
{
	DOORS_MISSING,
	DOORS_OPEN,
	DOORS_CLOSED
}

enum CarHeadlightBulbsState
{
	NONE,
	LEFT,
	RIGHT,
	BOTH
}

enum CarRearLightType
{
	NONE,
	BRAKES_ONLY,
	REVERSE_ONLY,
	BRAKES_AND_REVERSE
}

enum ECarOperationalState
{
	OK = 0,
	RUINED = 1,
	NO_FUEL = 2,
	NO_IGNITER = 4,
	NO_BATTERY = 8,
}

enum CarEngineSoundState
{
	NONE,
	STARTING,
	START_OK,
	START_NO_FUEL,
	START_NO_BATTERY,
	START_NO_SPARKPLUG,
	STOP_OK,
	STOP_NO_FUEL
}

enum ECarHornState
{
	OFF 	= 0,
	SHORT	= 1,
	LONG	= 2
}

#ifdef DIAG_DEVELOPER
enum EVehicleDebugOutputType
{
	NONE,
	DAMAGE_APPLIED = 1,
	DAMAGE_CONSIDERED = 2,
	CONTACT = 4
	//OTHER = 8
	//OTHER = 16
}

class CrashDebugData
{
	static ref array<ref CrashDebugData> m_CrashData = new array<ref CrashDebugData>;
	static ref CrashDebugData m_CrashDataPoint;
	//data is recorded on server, upon request, sent to the client
	static void SendData(PlayerBase player)
	{
		/*
		m_CrashData.Clear();
		CrashDebugData fakeData = new CrashDebugData();
		fakeData.m_VehicleType = "FakeVehicle";
		m_CrashData.Insert(fakeData);
		*/
		GetGame().RPCSingleParam(player, ERPCs.DIAG_VEHICLES_DUMP_CRASH_DATA_CONTENTS, new Param1<array<ref CrashDebugData>>(m_CrashData), true, player.GetIdentity());
	}
	
	//this is client requesting to dump the data it previously received from the server
	static void DumpDataArray(array<ref CrashDebugData> dataArray)
	{
		Print("Vehicle; DamageType; Damage; Zone; Momentum; Momentum Prev; Momentum Delta; Speedometer; SpeedWorld; SpeedWorld Prev; SpeedWorld Delta; Velocity; Velocity Prev; Velocity Dot; TimeStamp (ms); CrewDamageBase; ShockTemp; DMGHealth; DMGShock");
		foreach (CrashDebugData data:dataArray)
		{
			DumpData(data);
		}
	}
	
	static void DumpData(CrashDebugData data)
	{
		string output = data.m_VehicleType+";"+data.m_DamageType+";"+data.m_Damage+";"+data.m_Zone+";"+data.m_MomentumCurr+";"+data.m_MomentumPrev+";"+data.m_MomentumDelta+";"+data.m_Speedometer;
		output += ";"+data.m_SpeedWorld+";"+data.m_SpeedWorldPrev+";"+data.m_SpeedWorldDelta+";"+data.m_VelocityCur;
		output += ";"+data.m_VelocityPrev+";"+data.m_VelocityDot+";"+data.m_Time+";"+data.m_CrewDamageBase+";"+data.m_ShockTemp+";"+data.m_DMGHealth+";"+data.m_DMGShock;
		Print(output);
	}
		
	string m_VehicleType;
	string m_DamageType;
	float m_Damage;
	string m_Zone;
	float m_MomentumCurr;
	float m_MomentumPrev;
	float m_MomentumDelta;
	float m_Speedometer;
	float m_SpeedWorld;
	float m_SpeedWorldPrev;
	float m_SpeedWorldDelta;
	vector m_VelocityCur;
	vector m_VelocityPrev;
	float m_VelocityDot;
	float m_Time;
	float m_CrewDamageBase;
	float m_ShockTemp;
	float m_DMGHealth;
	float m_DMGShock;
}

#endif
class CarContactData
{
	vector localPos;
	IEntity other;
	float impulse;
	
	void CarContactData(vector _localPos, IEntity _other, float _impulse)
	{
		localPos	= _localPos;
		other		= _other;
		impulse		= _impulse;
	}
}

typedef map<string, ref array<ref CarContactData>> CarContactCache

class CarScriptOwnerState : CarOwnerState
{
	float m_fTime;
	
	protected override event void Write(PawnStateWriter ctx)
	{
		ctx.Write(m_fTime);
	}
	
	protected override event void Read(PawnStateReader ctx)
	{
		ctx.Read(m_fTime);
	}
};

class CarScriptMove : CarMove
{
};

#ifdef DIAG_DEVELOPER 
CarScript _car;
#endif

/*!
	Base script class for cars
*/
class CarScript extends Car
{
	#ifdef DIAG_DEVELOPER
	static EVehicleDebugOutputType DEBUG_OUTPUT_TYPE;
	bool m_ContactCalled;
	#endif
	static ref map<typename, ref TInputActionMap> m_CarTypeActionsMap = new map<typename, ref TInputActionMap>;
	TInputActionMap m_InputActionMap;
	bool	m_ActionsInitialize;
	protected float m_MomentumPrevTick;
	protected vector m_VelocityPrevTick;
	ref CarContactCache m_ContactCache;

	protected float m_Time;

	static float DROWN_ENGINE_THRESHOLD = 0.5;
	static float DROWN_ENGINE_DAMAGE = 350.0;
	
	static const string MEMORY_POINT_NAME_CAR_HORN 	= "pos_carHorn";
	
	//! keeps ammount of each fluid
	protected float m_FuelAmmount;
	protected float m_CoolantAmmount;
	protected float m_OilAmmount;
	protected float m_BrakeAmmount;

	//!
	//protected float m_dmgContactCoef = 0.023;
	protected float m_dmgContactCoef = 0.058;
	protected float m_EnviroHeatComfortOverride;

	//!
	protected float m_DrownTime;
	static vector m_DrownEnginePos;
	
	//!
	protected float m_EngineHealth;
	protected float m_RadiatorHealth;
	protected float m_FuelTankHealth;
	protected float m_BatteryHealth;
	protected float m_PlugHealth;
	
	protected EntityAI m_Radiator;
	
	protected float	m_BatteryConsume = 15; 				//Battery energy consumption upon engine start
	protected float m_BatteryContinuousConsume = 0.25; 	//Battery consumption with lights on and engine is off
	protected float m_BatteryRecharge = 0.15; 			//Battery recharge rate when engine is on
	private	  float m_BatteryTimer = 0; 				//Used to factor energy consumption / recharging
	private const float BATTERY_UPDATE_DELAY = 100;

	//! Particles
	protected ref EffVehicleSmoke m_coolantFx;
	protected ref EffVehicleSmoke m_engineFx;
	protected ref EffVehicleSmoke m_exhaustFx;
		
	protected int m_enginePtcFx;
	protected int m_coolantPtcFx;
	protected int m_exhaustPtcFx;
	
	protected vector m_exhaustPtcPos;
	protected vector m_exhaustPtcDir;
	protected vector m_enginePtcPos;
	protected vector m_coolantPtcPos;
	
	protected vector m_enginePos;
	protected vector m_frontPos;
	protected vector m_backPos;
	protected vector m_side_1_1Pos;
	protected vector m_side_1_2Pos;
	protected vector m_side_2_1Pos;
	protected vector m_side_2_2Pos;	
	
	//!Sounds
	string m_EngineStartOK 				= "";
	string m_EngineStartBattery 		= "";
	string m_EngineStartPlug 			= "";
	string m_EngineStartFuel 			= "";
	string m_EngineStop	 				= "";
	string m_EngineStopFuel 			= "";
	
	string m_CarDoorOpenSound 			= "";
	string m_CarDoorCloseSound 			= "";
	string m_CarSeatShiftInSound 		= "";
	string m_CarSeatShiftOutSound 		= "";
	
	string m_CarHornShortSoundName 		= "";
	string m_CarHornLongSoundName		= "";
	
	ref EffectSound m_CrashSoundLight;
	ref EffectSound m_CrashSoundHeavy;
	ref EffectSound m_WindowSmall;
	ref EffectSound m_WindowLarge;
	private ref EffectSound m_PreStartSound;

	protected ref EffectSound m_CarHornSoundEffect;
	protected ref NoiseParams m_NoisePar;
	protected NoiseSystem m_NoiseSystem;
	
	protected bool m_PlayCrashSoundLight;
	protected bool m_PlayCrashSoundHeavy;
	
	protected bool m_HeadlightsOn;
	protected bool m_HeadlightsState;
	protected bool m_BrakesArePressed;
	protected bool m_RearLightType;
	
	protected bool m_ForceUpdateLights;
	protected bool m_EngineStarted;
	protected bool m_EngineDestroyed;
	
	protected int m_CarHornState;
	
	CarLightBase 		m_Headlight;
	CarRearLightBase 	m_RearLight;
	
	// Memory points
	static string m_ReverseLightPoint				= "light_reverse";
	static string m_LeftHeadlightPoint				= "light_left";
	static string m_RightHeadlightPoint				= "light_right";
	static string m_LeftHeadlightTargetPoint		= "light_left_dir";
	static string m_RightHeadlightTargetPoint		= "light_right_dir";
	static string m_DrownEnginePoint 				= "drown_engine";
	
	// Model selection IDs for texture/material changes
	// If each car needs different IDs, then feel free to remove the 'static' flag and overwrite these numbers down the hierarchy
	static const int SELECTION_ID_FRONT_LIGHT_L 	= 0;
	static const int SELECTION_ID_FRONT_LIGHT_R 	= 1;
	static const int SELECTION_ID_BRAKE_LIGHT_L 	= 2;
	static const int SELECTION_ID_BRAKE_LIGHT_R 	= 3;
	static const int SELECTION_ID_REVERSE_LIGHT_L 	= 4;
	static const int SELECTION_ID_REVERSE_LIGHT_R 	= 5;
	static const int SELECTION_ID_TAIL_LIGHT_L 		= 6;
	static const int SELECTION_ID_TAIL_LIGHT_R 		= 7;
	static const int SELECTION_ID_DASHBOARD_LIGHT 	= 8;
		
	protected ref array<ref EffWheelSmoke> m_WheelSmokeFx;
	protected ref array<int> m_WheelSmokePtcFx;

	protected int m_CarEngineSoundState;
	protected int m_CarEngineLastSoundState;
	
	#ifdef DEVELOPER 
	private const int DEBUG_MESSAGE_CLEAN_TIME_SECONDS = 10;
	private float m_DebugMessageCleanTime;
	private string m_DebugContactDamageMessage;
	#endif
	
	void CarScript()
	{
#ifdef DIAG_DEVELOPER 
		_car = this;
#endif

		SetEventMask(EntityEvent.POSTSIMULATE);
		SetEventMask(EntityEvent.POSTFRAME);
		
		m_ContactCache = new CarContactCache;
		
		m_Time = 0;
		// sets max health for all components at init
		m_EngineHealth = 1;
		m_FuelTankHealth = 1;
		m_RadiatorHealth = -1;
		m_BatteryHealth = -1;
		m_PlugHealth = -1;

		m_enginePtcFx = -1;
		m_coolantPtcFx = -1;
		m_exhaustPtcFx = -1;

		m_EnviroHeatComfortOverride = 0;

		m_PlayCrashSoundLight = false;
		m_PlayCrashSoundHeavy = false;
		
		m_CarHornState = ECarHornState.OFF;
		m_CarEngineSoundState = CarEngineSoundState.NONE;
				
		RegisterNetSyncVariableBool("m_HeadlightsOn");
		RegisterNetSyncVariableBool("m_BrakesArePressed");
		RegisterNetSyncVariableBool("m_ForceUpdateLights");
		RegisterNetSyncVariableBoolSignal("m_PlayCrashSoundLight");
		RegisterNetSyncVariableBoolSignal("m_PlayCrashSoundHeavy");
		RegisterNetSyncVariableInt("m_CarHornState", ECarHornState.OFF, ECarHornState.LONG);
		RegisterNetSyncVariableInt("m_CarEngineSoundState", CarEngineSoundState.NONE, CarEngineSoundState.STOP_NO_FUEL);
		
		if ( MemoryPointExists("ptcExhaust_end") )
		{
			m_exhaustPtcPos = GetMemoryPointPos("ptcExhaust_end");
			if ( MemoryPointExists("ptcExhaust_start") )
			{
				vector exhaustStart = GetMemoryPointPos("ptcExhaust_start");
				vector tempOri = vector.Direction( exhaustStart, m_exhaustPtcPos);

				m_exhaustPtcDir[0] = -tempOri[2];
				m_exhaustPtcDir[1] = tempOri[1];
				m_exhaustPtcDir[2] = tempOri[0];
				
				m_exhaustPtcDir = m_exhaustPtcDir.Normalized().VectorToAngles();
			}
		}
		else
		{
			m_exhaustPtcPos = "0 0 0";
			m_exhaustPtcDir = "1 1 1";
		}
	
		if ( MemoryPointExists("ptcEnginePos") )
			m_enginePtcPos = GetMemoryPointPos("ptcEnginePos");
		else
			m_enginePtcPos = "0 0 0";

		if ( MemoryPointExists("ptcCoolantPos") )
			m_coolantPtcPos = GetMemoryPointPos("ptcCoolantPos");
		else
			m_coolantPtcPos = "0 0 0";
		
		if ( MemoryPointExists("drown_engine") )
			m_DrownEnginePos = GetMemoryPointPos("drown_engine");
		else
			m_DrownEnginePos = "0 0 0";	

		if ( MemoryPointExists("dmgZone_engine") )
			m_enginePos = GetMemoryPointPos("dmgZone_engine");
		else
			m_enginePos = "0 0 0";

		if ( MemoryPointExists("dmgZone_front") )
			m_frontPos = GetMemoryPointPos("dmgZone_front");
		else
			m_frontPos = "0 0 0";

		if ( MemoryPointExists("dmgZone_back") )
			m_backPos = GetMemoryPointPos("dmgZone_back");
		else
			m_backPos = "0 0 0";		
				
		if ( MemoryPointExists("dmgZone_fender_1_1") )
			m_side_1_1Pos = GetMemoryPointPos("dmgZone_fender_1_1");
		else
			m_side_1_1Pos = "0 0 0";

		if ( MemoryPointExists("dmgZone_fender_1_2") )
			m_side_1_2Pos = GetMemoryPointPos("dmgZone_fender_1_2");
		else
			m_side_1_2Pos = "0 0 0";
		
		if ( MemoryPointExists("dmgZone_fender_2_1") )
			m_side_2_1Pos = GetMemoryPointPos("dmgZone_fender_2_1");
		else
			m_side_2_1Pos = "0 0 0";

		if ( MemoryPointExists("dmgZone_fender_2_2") )
			m_side_2_2Pos = GetMemoryPointPos("dmgZone_fender_2_2");
		else
			m_side_2_2Pos = "0 0 0";
		
		if (!GetGame().IsDedicatedServer())
		{
			m_WheelSmokeFx = new array<ref EffWheelSmoke>;
			m_WheelSmokeFx.Resize(WheelCount());
			m_WheelSmokePtcFx = new array<int>;
			m_WheelSmokePtcFx.Resize(WheelCount());
			for (int i = 0; i < m_WheelSmokePtcFx.Count(); i++)
			{
				m_WheelSmokePtcFx.Set(i, -1);
			}
		}
	}
	
	override void EEInit()
	{		
		super.EEInit();

		if (GetGame().IsServer())
		{
			m_NoiseSystem = GetGame().GetNoiseSystem();
			if (m_NoiseSystem && !m_NoisePar)
			{
				m_NoisePar = new NoiseParams();
				m_NoisePar.LoadFromPath("cfgVehicles " + GetType() + " NoiseCarHorn");
			}
		}
	}
	
	#ifdef DIAG_DEVELOPER
	
	override void FixEntity()
	{
		if (GetGame().IsServer())
		{
			FillUpCarFluids();
			//server and single
			
			for (int i = 5; i > 0; i--)//there is a problem with wheels when performed only once, this solves it
				super.FixEntity();
			if (!GetGame().IsMultiplayer())
			{
				//single
				SEffectManager.DestroyEffect(m_engineFx);
			}
		}
		else
		{
			//MP client
			SEffectManager.DestroyEffect(m_engineFx);	
		}
	}
	#endif
	
	override string GetVehicleType()
	{
		return "VehicleTypeCar";
	}
	
	vector GetEnginePosWS()
	{
		return ModelToWorld( m_DrownEnginePos );
	}
	
	vector GetCoolantPtcPosWS()
	{
		return ModelToWorld( m_coolantPtcPos );
	}

	vector GetEnginePointPosWS()
	{	
		return ModelToWorld( m_enginePos );
	}
	vector GetFrontPointPosWS()
	{	
		return ModelToWorld( m_frontPos );
	}
	vector GetBackPointPosWS()
	{	
		return ModelToWorld( m_backPos );
	}
	vector Get_1_1PointPosWS()
	{	
		return ModelToWorld( m_side_1_1Pos );
	}
	vector Get_1_2PointPosWS()
	{	
		return ModelToWorld( m_side_1_2Pos );
	}
	vector Get_2_1PointPosWS()
	{	
		return ModelToWorld( m_side_2_1Pos );
	}
	vector Get_2_2PointPosWS()
	{	
		return ModelToWorld( m_side_2_2Pos );
	}
	
	override float GetLiquidThroughputCoef()
	{
		return LIQUID_THROUGHPUT_CAR_DEFAULT;
	}

	//here we should handle the damage dealt in OnContact event, but maybe we will react even in that event 
	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		ForceUpdateLightsStart();
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ForceUpdateLightsEnd, 100, false);
	}

	override void EEDelete(EntityAI parent)
	{
		#ifndef SERVER
		CleanupEffects();
		#endif
	}
	
	void ~CarScript()
	{
		#ifndef SERVER
		CleanupEffects();
		#endif
	}
	
	void CleanupEffects()
	{
		for (int i = 0; i < m_WheelSmokeFx.Count(); i++ )
		{
			Effect ps = m_WheelSmokeFx.Get(i);
			if (ps)
			{
				SEffectManager.DestroyEffect(ps);
			}
		}
		
		m_WheelSmokeFx.Clear();
		m_WheelSmokePtcFx.Clear();
		
		SEffectManager.DestroyEffect(m_coolantFx);
		SEffectManager.DestroyEffect(m_exhaustFx);
		SEffectManager.DestroyEffect(m_engineFx);
			
		if (m_Headlight)
			m_Headlight.Destroy();
			
		if (m_RearLight)
			m_RearLight.Destroy();
			
		SEffectManager.DestroyEffect(m_CrashSoundLight);
		SEffectManager.DestroyEffect(m_CrashSoundHeavy);
		SEffectManager.DestroyEffect(m_WindowSmall);
		SEffectManager.DestroyEffect(m_WindowLarge);
		CleanupSound(m_CarHornSoundEffect);
	}
	
	void CleanupSound(EffectSound sound)
	{
		SEffectManager.DestroyEffect(sound);
	}

	override void GetDebugActions(out TSelectableActionInfoArrayEx outputList)
	{
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_HORN_START_SHORT, "Car Horn Start Short", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_HORN_START_LONG, "Car Horn Start Long", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_HORN_STOP, "Car Horn Stop", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.SEPARATOR, "___________________________", FadeColors.RED));

		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.SEPARATOR, "Car Fuel", FadeColors.RED));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_FUEL_FULL, "Full", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_FUEL_EMPTY, "Empty", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_FUEL_INCREASE, "10% increase", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_FUEL_DECREASE, "10% decrease", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.SEPARATOR, "___________________________", FadeColors.RED));

		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.SEPARATOR, "Car Cooler", FadeColors.RED));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_COOLANT_FULL, "Full", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_COOLANT_EMPTY, "Empty", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_COOLANT_INCREASE, "10% increase", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.CAR_COOLANT_DECREASE, "10% decrease", FadeColors.LIGHT_GREY));
		outputList.Insert(new TSelectableActionInfoWithColor(SAT_DEBUG_ACTION, EActions.SEPARATOR, "___________________________", FadeColors.RED));
		
		super.GetDebugActions(outputList);
	}
	
	override bool OnAction(int action_id, Man player, ParamsReadContext ctx)
	{
		if (super.OnAction(action_id, player, ctx))
			return true;

		if (!GetGame().IsServer())
		{
			return false;
		}

		switch (action_id)
		{
			case EActions.CAR_HORN_START_SHORT:
				SetCarHornState(ECarHornState.SHORT);
				return true;
			case EActions.CAR_HORN_START_LONG:
				SetCarHornState(ECarHornState.LONG);
				return true;
			case EActions.CAR_HORN_STOP:
				SetCarHornState(ECarHornState.OFF);
				return true;

			case EActions.CAR_FUEL_FULL:
				Fill(CarFluid.FUEL, GetFluidCapacity(CarFluid.FUEL));
				return true;
			case EActions.CAR_FUEL_EMPTY:
				LeakAll(CarFluid.FUEL);
				return true;
			case EActions.CAR_FUEL_INCREASE:
				Fill(CarFluid.FUEL, GetFluidCapacity(CarFluid.FUEL) * 0.1);
				return true;
			case EActions.CAR_FUEL_DECREASE:
				Leak(CarFluid.FUEL, GetFluidCapacity(CarFluid.FUEL) * 0.1);
				return true;

			case EActions.CAR_COOLANT_FULL:
				Fill(CarFluid.COOLANT, GetFluidCapacity(CarFluid.COOLANT));
				return true;
			case EActions.CAR_COOLANT_EMPTY:
				LeakAll(CarFluid.COOLANT);
				return true;
			case EActions.CAR_COOLANT_INCREASE:
				Fill(CarFluid.COOLANT, GetFluidCapacity(CarFluid.COOLANT) * 0.1);
				return true;
			case EActions.CAR_COOLANT_DECREASE:
				Leak(CarFluid.COOLANT, GetFluidCapacity(CarFluid.COOLANT) * 0.1);
				return true;
		}
	
		return false;
	}
	
	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();
				
		if (GetCrashHeavySound())
		{
			PlayCrashHeavySound();
		}
		else if (GetCrashLightSound())
		{
			PlayCrashLightSound();
		}
		
		HandleCarHornSound(m_CarHornState);
		
		if (!IsOwner() && m_CarEngineSoundState != m_CarEngineLastSoundState)
			HandleEngineSound(m_CarEngineSoundState);
		
		UpdateLights();
	}
	
	void CreateCarDestroyedEffect()
	{
		if ( !SEffectManager.IsEffectExist( m_enginePtcFx ) && GetGame().GetWaterDepth( GetEnginePosWS() ) <= 0 )
		{
			m_engineFx = new EffEngineSmoke();
			m_engineFx.SetParticleStateHeavy();
			m_enginePtcFx = SEffectManager.PlayOnObject( m_engineFx, this, m_enginePtcPos, Vector(0,0,0));
		}
	}
	
	override void EEItemAttached(EntityAI item, string slot_name)
	{
		super.EEItemAttached(item, slot_name);

		switch (slot_name)
		{
			case "Reflector_1_1":
				if (GetGame().IsServer())
				{
					SetHealth("Reflector_1_1", "Health", item.GetHealth());
				}
				break;
			case "Reflector_2_1":
				if (GetGame().IsServer())
				{
					SetHealth("Reflector_2_1", "Health", item.GetHealth());
				}
				break;
			case "CarBattery":
			case "TruckBattery":
				if (GetGame().IsServer())
				{
					m_BatteryHealth = item.GetHealth01();
				}
				break;
			case "SparkPlug":
			case "GlowPlug":
				if (GetGame().IsServer())
				{
					m_PlugHealth = item.GetHealth01();
				}
				break;
			case "CarRadiator":
				if (GetGame().IsServer())
				{
					m_RadiatorHealth = item.GetHealth01();
				}
				
				m_Radiator = item;
				break;
		}
		
		if (GetGame().IsServer())
		{
			Synchronize();
		}

		UpdateHeadlightState();
		UpdateLights();
	}
	
	// Updates state of attached headlight bulbs for faster access
	void UpdateHeadlightState()
	{
		EntityAI bulb_L = FindAttachmentBySlotName("Reflector_1_1");
		EntityAI bulb_R = FindAttachmentBySlotName("Reflector_2_1");
		
		if (bulb_L && !bulb_L.IsRuined() && bulb_R && !bulb_R.IsRuined())
		{
			m_HeadlightsState = CarHeadlightBulbsState.BOTH;
		}
		else if (bulb_L && !bulb_L.IsRuined())
		{
			m_HeadlightsState = CarHeadlightBulbsState.LEFT;
		}
		else if (bulb_R && !bulb_R.IsRuined())
		{
			m_HeadlightsState = CarHeadlightBulbsState.RIGHT;
		}
		else if ((!bulb_L || bulb_L.IsRuined()) && (!bulb_R || bulb_R.IsRuined()))
		{
			m_HeadlightsState = CarHeadlightBulbsState.NONE;
		}
	}

	override void EEItemDetached(EntityAI item, string slot_name)
	{
		switch (slot_name)
		{
		case "CarBattery":
		case "TruckBattery":
			m_BatteryHealth = -1;
			if (IsServerOrOwner())
			{
				if (EngineIsOn())
				{
					EngineStop();
				}
			}

			if (GetGame().IsServer())
			{	
				if (IsScriptedLightsOn())
				{
					ToggleHeadlights();
				}

				UpdateBattery(ItemBase.Cast(item));
			}
		break;
		case "SparkPlug":
		case "GlowPlug":
			m_PlugHealth = -1;
			if (GetGame().IsServer() && EngineIsOn())
			{
				EngineStop();
			}
		break;
		case "CarRadiator":
			m_Radiator = null;
			if (IsServerOrOwner())
			{
				LeakAll(CarFluid.COOLANT);
			}

			if (GetGame().IsServer())
			{
				if (m_DamageZoneMap.Contains("Radiator"))
				{
					SetHealth("Radiator", "Health", 0);
				}
			}
		break;
		}
		
		if (GetGame().IsServer())
		{
			Synchronize();
		}
		
		UpdateHeadlightState();
		UpdateLights();
	}
	
	override void OnAttachmentRuined(EntityAI attachment)
	{
		super.OnAttachmentRuined(attachment);
		
		UpdateHeadlightState();
		UpdateLights();
	}
	
	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (!super.CanReceiveAttachment(attachment, slotId))
			return false;
		
		InventoryLocation attachmentInventoryLocation = new InventoryLocation();
		attachment.GetInventory().GetCurrentInventoryLocation(attachmentInventoryLocation);
		if (attachmentInventoryLocation.GetParent() == null)
		{
			return true;
		}
		
		if (attachment && attachment.Type().IsInherited(CarWheel))
		{
			string slotSelectionName;
			InventorySlots.GetSelectionForSlotId(slotId, slotSelectionName);

			switch (slotSelectionName)
			{
				case "wheel_spare_1":
				case "wheel_spare_2":
					return CanManipulateSpareWheel(slotSelectionName);
					break;
			}
		}
		
		return true;
	}

	override bool CanReleaseAttachment(EntityAI attachment)
	{
		if (!super.CanReleaseAttachment(attachment))
		{
			return false;
		}

		if (EngineIsOn() && IsMoving())
		{
			return false;
		}

		if (attachment && attachment.Type().IsInherited(CarWheel))
		{
			InventoryLocation attachmentInventoryLocation = new InventoryLocation();
			attachment.GetInventory().GetCurrentInventoryLocation(attachmentInventoryLocation);

			string slotSelectionName;
			InventorySlots.GetSelectionForSlotId(attachmentInventoryLocation.GetSlot(), slotSelectionName);

			switch (slotSelectionName)
			{
				case "wheel_spare_1":
				case "wheel_spare_2":
					return CanManipulateSpareWheel(slotSelectionName);
					break;
			}
		}

		return true;
	}
	
	protected bool CanManipulateSpareWheel(string slotSelectionName)
	{
		return false;
	}

	override void EOnPostSimulate(IEntity other, float timeSlice)
	{
		m_Time += timeSlice;
		
		if (GetGame().IsServer())
		{
			#ifdef DIAG_DEVELOPER
			if (DEBUG_OUTPUT_TYPE & EVehicleDebugOutputType.CONTACT)
			{
				if (m_ContactCalled)
				{
					Debug.Log("Momentum delta: " + (GetMomentum() - m_MomentumPrevTick));
					Debug.Log("--------------------------------------------------------------------");
					m_ContactCalled = false;
				}
			}
			#endif
			
			
			CheckContactCache();
			m_VelocityPrevTick = GetVelocity(this);
			m_MomentumPrevTick = GetMomentum();
			#ifdef DEVELOPER
			m_DebugMessageCleanTime += timeSlice;
			if (m_DebugMessageCleanTime >= DEBUG_MESSAGE_CLEAN_TIME_SECONDS)
			{
				m_DebugMessageCleanTime = 0;
				m_DebugContactDamageMessage = "";
			}
			#endif
		}
		
		if ( m_Time >= GameConstants.CARS_FLUIDS_TICK )
		{
			m_Time = 0;

			CarPartsHealthCheck();

			bool isServerOrOwner = IsServerOrOwner();

			//First of all check if the car should stop the engine
			if (isServerOrOwner && EngineIsOn())
			{
				if (IsDamageDestroyed() || GetFluidFraction(CarFluid.FUEL) <= 0 || m_EngineHealth <= 0)
					EngineStop();

				//CheckVitalItem(IsVitalCarBattery(), CarBattery.SLOT_ID);
				//CheckVitalItem(IsVitalTruckBattery(), TruckBattery.SLOT_ID);
				CheckVitalItem(IsVitalSparkPlug(), "SparkPlug");
				CheckVitalItem(IsVitalGlowPlug(), "GlowPlug");
			}
			
			if (GetGame().IsServer())
			{
				if (IsVitalFuelTank())
				{
					if (m_FuelTankHealth == GameConstants.DAMAGE_RUINED_VALUE && m_EngineHealth > GameConstants.DAMAGE_RUINED_VALUE)
					{
						SetHealth("Engine", "Health", GameConstants.DAMAGE_RUINED_VALUE);
					}
				}
			}

			//! actions runned when the engine on
			if ( EngineIsOn() )
			{
				if ( GetGame().IsServer() )
				{
					float dmg;

					if ( EngineGetRPM() >= EngineGetRPMRedline() )
					{
						if (EngineGetRPM() > EngineGetRPMMax())
							AddHealth( "Engine", "Health", -GetMaxHealth("Engine", "") * 0.05 ); //CAR_RPM_DMG
							
						// only called on server, don't use deterministic RandomFloat
						dmg = EngineGetRPM() * 0.001 * Math.RandomFloat( 0.02, 1.0 );  //CARS_TICK_DMG_MIN; //CARS_TICK_DMG_MAX
						ProcessDirectDamage(DamageType.CUSTOM, null, "Engine", "EnviroDmg", vector.Zero, dmg);
						SetEngineZoneReceivedHit(true);
					}
					else
					{
						SetEngineZoneReceivedHit(false);
					}
				}

				if (isServerOrOwner)
				{
					//! leaking of coolant from radiator when damaged
					if ( IsVitalRadiator() )
					{
						if ( GetFluidFraction(CarFluid.COOLANT) > 0 && m_RadiatorHealth < 0.5 ) //CARS_LEAK_THRESHOLD
							LeakFluid( CarFluid.COOLANT );
					}

					if ( GetFluidFraction(CarFluid.FUEL) > 0 && m_FuelTankHealth < GameConstants.DAMAGE_DAMAGED_VALUE )
						LeakFluid( CarFluid.FUEL );

					if ( GetFluidFraction(CarFluid.BRAKE) > 0 && m_EngineHealth < GameConstants.DAMAGE_DAMAGED_VALUE )
						LeakFluid( CarFluid.BRAKE );

					if ( GetFluidFraction(CarFluid.OIL) > 0 && m_EngineHealth < GameConstants.DAMAGE_DAMAGED_VALUE )
						LeakFluid( CarFluid.OIL );

					if ( m_EngineHealth < 0.25 )
						LeakFluid( CarFluid.OIL );
				}
				
				if ( GetGame().IsServer() )
				{
					if ( IsVitalRadiator() )
					{
						if ( GetFluidFraction( CarFluid.COOLANT ) < 0.5 && GetFluidFraction( CarFluid.COOLANT ) >= 0 )
						{
							// only called on server, don't use deterministic RandomFloat
							dmg = ( 1 - GetFluidFraction(CarFluid.COOLANT) ) * Math.RandomFloat( 0.02, 10.00 );  //CARS_DMG_TICK_MIN_COOLANT; //CARS_DMG_TICK_MAX_COOLANT
							AddHealth( "Engine", "Health", -dmg );
							SetEngineZoneReceivedHit(true);
						}
					}
				}

				//FX only on Client and in Single
				if (!GetGame().IsDedicatedServer())
				{
					if (!SEffectManager.IsEffectExist(m_exhaustPtcFx))
					{
						m_exhaustFx = new EffExhaustSmoke();
						m_exhaustPtcFx = SEffectManager.PlayOnObject( m_exhaustFx, this, m_exhaustPtcPos, m_exhaustPtcDir );
						m_exhaustFx.SetParticleStateLight();
					}
				
					if (IsVitalRadiator() && GetFluidFraction(CarFluid.COOLANT) < 0.5)
					{
						if (!SEffectManager.IsEffectExist(m_coolantPtcFx))
						{
							m_coolantFx = new EffCoolantSteam();
							m_coolantPtcFx = SEffectManager.PlayOnObject(m_coolantFx, this, m_coolantPtcPos, vector.Zero);
						}

						if (m_coolantFx)
						{
							if (GetFluidFraction(CarFluid.COOLANT) > 0)
								m_coolantFx.SetParticleStateLight();
							else
								m_coolantFx.SetParticleStateHeavy();
						}
					}
					else
					{
						if (SEffectManager.IsEffectExist(m_coolantPtcFx))
							SEffectManager.Stop(m_coolantPtcFx);
					}
				}
			}
			else
			{
				//FX only on Client and in Single
				if ( !GetGame().IsDedicatedServer() )
				{
					if (SEffectManager.IsEffectExist(m_exhaustPtcFx))
					{
						SEffectManager.Stop(m_exhaustPtcFx);
						m_exhaustPtcFx = -1;
					}
					
					if (SEffectManager.IsEffectExist(m_coolantPtcFx))
					{
						SEffectManager.Stop(m_coolantPtcFx);
						m_coolantPtcFx = -1;
					}
				}
			}
			
		}
			
			//FX only on Client and in Single
			if ( !GetGame().IsDedicatedServer() )
			{
				float carSpeed = GetVelocity(this).Length();
				for (int i = 0; i < WheelCount(); i++)
				{
					EffWheelSmoke eff = m_WheelSmokeFx.Get(i);
					int ptrEff = m_WheelSmokePtcFx.Get(i);
					bool haveParticle = false;
	
					CarWheel wheel = CarWheel.Cast(WheelGetEntity(i));
					if (wheel && WheelHasContact(i))
					{
						float wheelSpeed = WheelGetAngularVelocity(i) * wheel.GetRadius();
					
						vector wheelPos = WheelGetContactPosition(i);
						vector wheelVel = dBodyGetVelocityAt(this, wheelPos);

						vector transform[3];
						transform[2] = WheelGetDirection(i);
						transform[1] = vector.Up;
						transform[0] = transform[2] * transform[1];

						wheelVel = wheelVel.InvMultiply3(transform);

						float bodySpeed = wheelVel[2];
					
						bool applyEffect = false;
						if ((wheelSpeed > 0 && bodySpeed > 0) || (wheelSpeed < 0 && bodySpeed < 0))
						{
							applyEffect = Math.AbsFloat(wheelSpeed) > Math.AbsFloat(bodySpeed) + EffWheelSmoke.WHEEL_SMOKE_THRESHOLD;
						}
						else
						{
							applyEffect = Math.AbsFloat(wheelSpeed) > EffWheelSmoke.WHEEL_SMOKE_THRESHOLD;
						}
						
						if (applyEffect)
						{
							haveParticle = true;

							string surface;
							GetGame().SurfaceGetType(wheelPos[0], wheelPos[2], surface);
							wheelPos = WorldToModel(wheelPos);
	
							if (!SEffectManager.IsEffectExist(ptrEff))
							{
								eff = new EffWheelSmoke();
								eff.SetSurface(surface);
								ptrEff = SEffectManager.PlayOnObject(eff, this, wheelPos, "0 1 -1");
								eff.SetCurrentLocalPosition(wheelPos);
								m_WheelSmokeFx.Set(i, eff);
								m_WheelSmokePtcFx.Set(i, ptrEff);
							}
							else
							{
								if (!eff.IsPlaying() && Surface.GetWheelParticleID(surface) != 0)
									eff.Start();
								eff.SetSurface(surface);
								eff.SetCurrentLocalPosition(wheelPos);
							}
						}
					}
					
					if (!haveParticle)
					{
						if (eff && eff.IsPlaying())
							eff.Stop();
					}
				}
			}
	}
	
	void OnBrakesPressed()
	{
		UpdateLights();
	}
	
	void OnBrakesReleased()
	{
		UpdateLights();
	}

	override void OnDriverExit(Human player)
	{
		super.OnDriverExit(player);
		
		if (GetGear() != GetNeutralGear())
		{
			EngineStop();
		}
	}
	
	// Server side event for jump out processing 
	void OnVehicleJumpOutServer(GetOutTransportActionData gotActionData)
	{
		PlayerBase player = gotActionData.m_Player;
			
		array<ClothingBase> equippedClothes = new array<ClothingBase>;
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("LEGS")));
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("BACK")));
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("VEST")));
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("HeadGear")));
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("Mask")));
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("BODY")));
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("FEET")));
		equippedClothes.Insert(ClothingBase.Cast(player.GetItemOnSlot("GLOVES")));

		// -----------------------------------------------
		float shockTaken = (gotActionData.m_Speed * gotActionData.m_Speed) / ActionGetOutTransport.DMG_FACTOR;
		
		//Lower shock taken if player uses a helmet
		ItemBase headGear = ClothingBase.Cast(player.GetItemOnHead());
		HelmetBase helmet;
		if (Class.CastTo(helmet, headGear))
			shockTaken *= 0.5;

		// -----------------------------------------------
		
		int randNum; //value used for probability evaluation
		randNum = Math.RandomInt(0, 100);
		if (gotActionData.m_Speed < ActionGetOutTransport.LOW_SPEED_VALUE)
		{
			if (randNum < 20)
				player.GiveShock(-shockTaken); //To inflict shock, a negative value must be passed

			randNum = Math.RandomIntInclusive(0, PlayerBase.m_BleedingSourcesLow.Count() - 1);
			
			if (player.m_BleedingManagerServer)
				player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection(PlayerBase.m_BleedingSourcesLow[randNum]);
		}
		else if (gotActionData.m_Speed >= ActionGetOutTransport.LOW_SPEED_VALUE && gotActionData.m_Speed < ActionGetOutTransport.HIGH_SPEED_VALUE)
		{
			if (randNum < 50)
				player.GiveShock(-shockTaken);

			randNum = Math.RandomInt(0, PlayerBase.m_BleedingSourcesUp.Count() - 1);
			
			if (player.m_BleedingManagerServer)
				player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection(PlayerBase.m_BleedingSourcesUp[randNum]);
		}
		else if (gotActionData.m_Speed >= ActionGetOutTransport.HIGH_SPEED_VALUE)
		{
			if (!headGear && player.m_BleedingManagerServer)
				player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Head");

			if (randNum < 75)
				player.GiveShock(-shockTaken);
		}
		
		float dmgTaken = (gotActionData.m_Speed * gotActionData.m_Speed) / ActionGetOutTransport.SHOCK_FACTOR;
		
		//Damage all currently equipped clothes
		foreach (ClothingBase cloth : equippedClothes)
		{
			//If no item is equipped on slot, slot is ignored
			if (cloth == null)
				continue;

			cloth.DecreaseHealth(dmgTaken, false);
		}
		
		vector posMS = gotActionData.m_Player.WorldToModel(gotActionData.m_Player.GetPosition());
		gotActionData.m_Player.DamageAllLegs(dmgTaken); //Additionnal leg specific damage dealing
		
		float healthCoef = Math.InverseLerp(ActionGetOutTransport.HEALTH_LOW_SPEED_VALUE, ActionGetOutTransport.HEALTH_HIGH_SPEED_VALUE, gotActionData.m_Speed);
		healthCoef = Math.Clamp(healthCoef, 0.0, 1.0);
		gotActionData.m_Player.ProcessDirectDamage(DamageType.CUSTOM, gotActionData.m_Player, "", "FallDamageHealth", posMS, healthCoef);
	}
	
	protected override bool DetectFlipped(VehicleFlippedContext ctx)
	{
		if (!DetectFlippedUsingWheels(ctx, GameConstants.VEHICLE_FLIP_WHEELS_LIMITED))
			return false;
		if (!DetectFlippedUsingSurface(ctx, GameConstants.VEHICLE_FLIP_ANGLE_TOLERANCE))
			return false;
		return true;
	}
	
	override void OnUpdate( float dt )
    {
		Human driver = CrewDriver();
		if (driver && !driver.IsControllingVehicle())
		{
			// likely unconscious
			if (driver.IsAlive())
			{
				SetBrake(0.5);
			}
		}

		if (GetGame().IsServer())
		{
			ItemBase battery = GetBattery();
			if (battery)
			{
				m_BatteryTimer += dt;
				if (m_BatteryTimer >= BATTERY_UPDATE_DELAY)
				{
					UpdateBattery(battery);
				}
			}
			
			if ( GetGame().GetWaterDepth( GetEnginePosWS() ) > 0 )
			{
				m_DrownTime += dt;
				if ( m_DrownTime > DROWN_ENGINE_THRESHOLD )
				{
					// *dt to get damage per second
					AddHealth( "Engine", "Health", -DROWN_ENGINE_DAMAGE * dt);
					SetEngineZoneReceivedHit(true);
				}
			}
			else
			{
				m_DrownTime = 0;
			}
		}

		// For visualisation of brake lights for all players
		float brake_coef = GetBrake();
		if ( brake_coef > 0 )
		{
			if ( !m_BrakesArePressed )
			{
				m_BrakesArePressed = true;
				SetSynchDirty();
				OnBrakesPressed();
			}
		}
		else
		{
			if ( m_BrakesArePressed )
			{
				m_BrakesArePressed = false;
				SetSynchDirty();
				OnBrakesReleased();
			}
		}
		
		if ( (!GetGame().IsDedicatedServer()) && m_ForceUpdateLights )
		{
			UpdateLights();
			m_ForceUpdateLights = false;
		}
    }
	
	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);
		m_EngineDestroyed = true;
	}
	
	//! WARNING: Can be called very frequently in one frame, use with caution
	override void OnContact( string zoneName, vector localPos, IEntity other, Contact data )
	{
		if (GetGame().IsServer())
		{
			#ifdef DIAG_DEVELOPER
			m_ContactCalled = true;
			if (DEBUG_OUTPUT_TYPE & EVehicleDebugOutputType.CONTACT)
			{
				string output = "Zone: " + zoneName + " | Impulse:" + data.Impulse;
				Debug.Log(output);
			}
			#endif
			if (m_ContactCache.Count() == 0)
			{
				array<ref CarContactData> ccd = new array<ref CarContactData>;
				m_ContactCache.Insert(zoneName, ccd);
				float momentumDelta = GetMomentum() - m_MomentumPrevTick;
				float dot = vector.Dot(m_VelocityPrevTick.Normalized(), GetVelocity(this).Normalized());
				if (dot < 0)
				{
					momentumDelta = m_MomentumPrevTick;
				}
				
				ccd.Insert(new CarContactData(localPos, other, momentumDelta));
			}
		}
	}
	
	//! Responsible for damaging the car according to contact events from OnContact
	void CheckContactCache()
	{
		
		int contactZonesCount = m_ContactCache.Count();
		
		if (contactZonesCount == 0)
			return;
		
		
		for (int i = 0; i < contactZonesCount; ++i)
		{
			string zoneName = m_ContactCache.GetKey(i);
			array<ref CarContactData> data = m_ContactCache[zoneName];

			float dmg = Math.AbsInt(data[0].impulse * m_dmgContactCoef);
			float crewDmgBase = Math.AbsInt((data[0].impulse / dBodyGetMass(this)) * 1000 * m_dmgContactCoef);// calculates damage as if the object's weight was 1000kg instead of its actual weight

			#ifdef DIAG_DEVELOPER
			CrashDebugData.m_CrashDataPoint = new CrashDebugData();
			CrashDebugData.m_CrashDataPoint.m_VehicleType = GetDisplayName();
			CrashDebugData.m_CrashDataPoint.m_Damage = dmg;
			CrashDebugData.m_CrashDataPoint.m_Zone = zoneName;
			CrashDebugData.m_CrashDataPoint.m_MomentumCurr = GetMomentum();
			CrashDebugData.m_CrashDataPoint.m_MomentumPrev = m_MomentumPrevTick;
			CrashDebugData.m_CrashDataPoint.m_MomentumDelta = data[0].impulse;
			CrashDebugData.m_CrashDataPoint.m_SpeedWorld = GetVelocity(this).Length() * 3.6;
			CrashDebugData.m_CrashDataPoint.m_SpeedWorldPrev = m_VelocityPrevTick.Length() * 3.6;
			CrashDebugData.m_CrashDataPoint.m_SpeedWorldDelta = (m_VelocityPrevTick.Length() - GetVelocity(this).Length()) * 3.6;
			CrashDebugData.m_CrashDataPoint.m_VelocityCur = GetVelocity(this);
			CrashDebugData.m_CrashDataPoint.m_VelocityPrev = m_VelocityPrevTick;
			CrashDebugData.m_CrashDataPoint.m_VelocityDot = vector.Dot(m_VelocityPrevTick.Normalized(), GetVelocity(this).Normalized());
			CrashDebugData.m_CrashDataPoint.m_Time = GetGame().GetTime();
			

			
			if (DEBUG_OUTPUT_TYPE & EVehicleDebugOutputType.DAMAGE_CONSIDERED)
			{
				Debug.Log("--------------------------------------------------");
				Debug.Log("Vehicle:" + GetDisplayName());
				Debug.Log("DMG: " + dmg);
				Debug.Log("zoneName : "+ zoneName);
				Debug.Log("momentumCurr : "+ GetMomentum());
				Debug.Log("momentumPrev : "+ m_MomentumPrevTick);
				Debug.Log("momentumDelta : "+ data[0].impulse);
				Debug.Log("speed(km/h): "+ GetVelocity(this).Length() * 3.6);
				Debug.Log("speedPrev(km/h): "+ m_VelocityPrevTick.Length() * 3.6);
				Debug.Log("speedDelta(km/h) : "+ (m_VelocityPrevTick.Length() - GetVelocity(this).Length()) * 3.6);
				Debug.Log("velocityCur.): "+ GetVelocity(this));
				Debug.Log("velocityPrev.): "+ m_VelocityPrevTick);
				Debug.Log("velocityDot): "+ vector.Dot(m_VelocityPrevTick.Normalized(), GetVelocity(this).Normalized()));
				Debug.Log("GetGame().GetTime(): "+ GetGame().GetTime());
				Debug.Log("--------------------------------------------------");
			}
			#endif
			if ( dmg < GameConstants.CARS_CONTACT_DMG_MIN )
				continue;

			int pddfFlags;
			#ifdef DIAG_DEVELOPER
			CrashDebugData.m_CrashData.Insert(CrashDebugData.m_CrashDataPoint);
			CrashDebugData.m_CrashDataPoint.m_Speedometer =  GetSpeedometer();
			//Print("Crash data recorded");
			#endif
			if (dmg < GameConstants.CARS_CONTACT_DMG_THRESHOLD)
			{			
				#ifdef DIAG_DEVELOPER
				CrashDebugData.m_CrashDataPoint.m_DamageType = "Small";
				if (DEBUG_OUTPUT_TYPE & EVehicleDebugOutputType.DAMAGE_APPLIED)
					Debug.Log(string.Format("[Vehiles:Damage]:: DMG %1 to the %2 zone is SMALL (threshold: %3), SPEEDOMETER: %4, TIME: %5", dmg, zoneName, GameConstants.CARS_CONTACT_DMG_MIN, GetSpeedometer(), GetGame().GetTime() ));
				#endif
				SynchCrashLightSound(true);
				pddfFlags = ProcessDirectDamageFlags.NO_TRANSFER;
			}
			else
			{
				#ifdef DIAG_DEVELOPER
				CrashDebugData.m_CrashDataPoint.m_DamageType = "Big";
				if (DEBUG_OUTPUT_TYPE & EVehicleDebugOutputType.DAMAGE_APPLIED)
					Debug.Log(string.Format("[Vehiles:Damage]:: DMG %1 to the %2 zone is BIG (threshold: %3), SPEED: %4, TIME: %5", dmg, zoneName, GameConstants.CARS_CONTACT_DMG_THRESHOLD, GetSpeedometer(), GetGame().GetTime() ));
				#endif
				DamageCrew(crewDmgBase);
				SynchCrashHeavySound(true);
				pddfFlags = 0;
			}
			
			#ifdef DEVELOPER
			m_DebugContactDamageMessage += string.Format("%1: %2\n", zoneName, dmg);
			#endif
			
			ProcessDirectDamage(DamageType.CUSTOM, null, zoneName, "EnviroDmg", "0 0 0", dmg, pddfFlags);
			
			//if (data[0].impulse > TRESHOLD)
			//{
				Object targetEntity = Object.Cast(data[0].other);
				if (targetEntity && targetEntity.IsTree())
				{
					SEffectManager.CreateParticleServer(targetEntity.GetPosition(), new TreeEffecterParameters("TreeEffecter", 1.0, 0.1));
				}
			//}
		}
		
		UpdateHeadlightState();
		UpdateLights();
		
		m_ContactCache.Clear();
		
	}
	
	//! Responsible for damaging crew in a car crash
	void DamageCrew(float dmg)
	{
		for ( int c = 0; c < CrewSize(); ++c )
		{
			Human crew = CrewMember( c );
			if ( !crew )
				continue;

			PlayerBase player;
			if ( Class.CastTo(player, crew ) )
			{
				if ( dmg > GameConstants.CARS_CONTACT_DMG_KILLCREW )
				{		
					#ifdef DIAG_DEVELOPER
					CrashDebugData.m_CrashDataPoint.m_CrewDamageBase = dmg;
					CrashDebugData.m_CrashDataPoint.m_DMGHealth = -100;
					if (DEBUG_OUTPUT_TYPE & EVehicleDebugOutputType.DAMAGE_APPLIED)
					{
						Debug.Log("--------------------------------------------------");
						Debug.Log("Killing the player");
						Debug.Log("Crew DMG Base: " + dmg);
						Debug.Log("--------------------------------------------------");

					}
					#endif
					player.SetHealth(0.0);
				}
				else
				{
					float shockTemp = Math.InverseLerp(GameConstants.CARS_CONTACT_DMG_THRESHOLD, GameConstants.CARS_CONTACT_DMG_KILLCREW, dmg);
					shockTemp = Math.Clamp(shockTemp,0,1);
					float shock = Math.Lerp( 50, 150, shockTemp );
					float hp = Math.Lerp( 2, 100, shockTemp );

					#ifdef DIAG_DEVELOPER
					CrashDebugData.m_CrashDataPoint.m_CrewDamageBase = dmg;
					CrashDebugData.m_CrashDataPoint.m_ShockTemp = shockTemp;
					CrashDebugData.m_CrashDataPoint.m_DMGHealth = hp;
					CrashDebugData.m_CrashDataPoint.m_DMGShock = shock;
					if (DEBUG_OUTPUT_TYPE & EVehicleDebugOutputType.DAMAGE_APPLIED)
					{
						Debug.Log("--------------------------------------------------");
						Debug.Log("Crew DMG Base: " + dmg);
						Debug.Log("Crew shockTemp: " + shockTemp);
						Debug.Log("Crew DMG Health: " + hp);
						Debug.Log("Crew DMG Shock: " + shock);
						Debug.Log("--------------------------------------------------");
					}
					#endif
					
					player.AddHealth("", "Shock", -shock );
					player.AddHealth("", "Health", -hp );
				}
			}
		}
	}

	/*!
		Gets called every sound simulation step.
		In this callback, user can modify behaviour of sound controllers
		The higher the return value is the more muted sound is.
	*/
	override float OnSound( CarSoundCtrl ctrl, float oldValue )
	{
		switch (ctrl)
		{
		case CarSoundCtrl.ENGINE:
			if (!m_EngineStarted)
			{
				return 0.0;
			}
		break;
		}
		
		return oldValue;
	}
	
	override void OnAnimationPhaseStarted(string animSource, float phase)
	{
		#ifndef SERVER
		HandleDoorsSound(animSource, phase);
		HandleSeatAdjustmentSound(animSource, phase);
		#endif
	}
	
	protected EffectSound CreateSoundForAnimationSource(string animSource)
	{
		vector position = vector.Zero;
		int pivotIndex = -1;
		
		string selectionName = GetSelectionFromAnimSource(animSource);
		if (selectionName != "")
		{			
			position = GetSelectionBasePositionLS(selectionName);
			
			int level = GetViewGeometryLevel();
		
			array<int> pivots = new array<int>();
							
			pivots.Clear();
			GetBonePivotsForAnimationSource(level, animSource, pivots);
				
			if (pivots.Count())
			{
				pivotIndex = pivots[0];
			}
		}

		WaveKind waveKind = WaveKind.WAVEEFFECTEX;
		
		EffectSound sound = new EffectSound();
		
		PlayerBase player;
		if (Class.CastTo(player, GetGame().GetPlayer()))
		{
			if (player.IsCameraInsideVehicle())
			{
				waveKind = WaveKind.WAVEEFFECT;
			}
		}

		sound.SetAutodestroy(true);
		sound.SetParent(this, pivotIndex);
		sound.SetPosition(ModelToWorld(position));
		sound.SetLocalPosition(position);
		sound.SetSoundWaveKind(waveKind);

		return sound;
	}
	
	protected void HandleDoorsSound(string animSource, float phase)
	{
		switch (animSource)
		{
			case "doorsdriver":
			case "doorscodriver":
			case "doorscargo1":
			case "doorscargo2":
			case "doorshood":
			case "doorstrunk":
				EffectSound sound = CreateSoundForAnimationSource(animSource);
				
				if (phase == 0)
					sound.SetSoundSet(m_CarDoorOpenSound);
				else
					sound.SetSoundSet(m_CarDoorCloseSound);

				SEffectManager.EffectRegister(sound);
				sound.SoundPlay();

				break;
		}
	}
	
	protected void HandleSeatAdjustmentSound(string animSource, float phase)
	{
		switch (animSource)
		{
			case "seatdriver":
			case "seatcodriver":
				EffectSound sound = CreateSoundForAnimationSource(animSource);

				if (phase == 0)
					sound.SetSoundSet(m_CarSeatShiftInSound);
				else
					sound.SetSoundSet(m_CarSeatShiftOutSound);
				
				SEffectManager.EffectRegister(sound);
				sound.SoundPlay();

				break;
		}
	}
	
	
	protected void HandleCarHornSound(ECarHornState pState)
	{
		switch (pState)
		{
		case ECarHornState.SHORT:
			PlaySoundSet(m_CarHornSoundEffect, m_CarHornShortSoundName, 0, 0, false);
		break;
		case ECarHornState.LONG:
			PlaySoundSet(m_CarHornSoundEffect, m_CarHornLongSoundName, 0, 0, true);
		break;
		default:
			CleanupSound(m_CarHornSoundEffect);
		break;
		}
	}
	
	// Only used for sound states which happen before engine start
	void SetCarEngineSoundState(CarEngineSoundState pState)
	{
		m_CarEngineSoundState = pState;
		SetSynchDirty();
	}
	
	void HandleEngineSound(CarEngineSoundState state)
	{
		SetCarEngineSoundState(state);
		m_CarEngineLastSoundState 	= state;

		#ifndef SERVER
		PlayerBase player = null;
		EffectSound sound = null;
		WaveKind waveKind = WaveKind.WAVEEFFECTEX;
		
		bool doInside = false;

		m_CarEngineLastSoundState = state;
		
		switch (state)
		{
			case CarEngineSoundState.STARTING:
				sound = new EffectSound();
				sound.SetSoundSet("Offroad_02_Starting_SoundSet");
				sound.SetSoundFadeOut(0.15);

				m_PreStartSound = sound;
				break;
			case CarEngineSoundState.START_OK:
				doInside = true;
		
				sound = new EffectSound();
				sound.SetSoundSet(m_EngineStartOK);
				sound.SetAutodestroy(true);
				
				//! postpone the engine sound played from c++ on soundcontroller (via OnSound override)
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(SetEngineStarted, 1000, false, true);
				break;
				
			case CarEngineSoundState.START_NO_FUEL:
				sound = new EffectSound();
				sound.SetSoundSet("offroad_engine_failed_start_fuel_SoundSet");
				sound.SetAutodestroy(true);
				break;
				
			case CarEngineSoundState.START_NO_BATTERY:
				sound = new EffectSound();
				sound.SetSoundSet("offroad_engine_failed_start_battery_SoundSet");
				sound.SetAutodestroy(true);
				break;
				
			case CarEngineSoundState.START_NO_SPARKPLUG:
				sound = new EffectSound();
				sound.SetSoundSet("offroad_engine_failed_start_sparkplugs_SoundSet");
				sound.SetAutodestroy(true);
				break;
				
			case CarEngineSoundState.STOP_OK:
				doInside = true;
					
				sound = new EffectSound();
				sound.SetSoundSet(m_EngineStop);
				sound.SetAutodestroy(true);
				break;
			case CarEngineSoundState.STOP_NO_FUEL:
				doInside = true;
					
				sound = new EffectSound();
				sound.SetSoundSet(m_EngineStopFuel);
				sound.SetAutodestroy(true);
				break;
			
			default: 
				break;
		}
		
		// play different sound based on selected camera
		if (doInside && Class.CastTo(player, GetGame().GetPlayer()))
		{
			if (player.IsCameraInsideVehicle())
			{
				waveKind = WaveKind.WAVEEFFECT;
			}
		}

		if (sound)
		{
			vector enginePos = GetEnginePos();
			
			sound.SetParent(this);
			sound.SetPosition(ModelToWorld(enginePos));
			sound.SetLocalPosition(enginePos);
			sound.SetSoundWaveKind(waveKind);
			SEffectManager.EffectRegister(sound);
			
			sound.SoundPlay();
		}

		#endif
	}

	override void MarkCrewMemberUnconscious(int crewMemberIndex)
	{
		if (!IsAuthority())
			return;

		if (crewMemberIndex == DayZPlayerConstants.VEHICLESEAT_DRIVER)
		{
			EngineStop();
		}
	}

	override void MarkCrewMemberDead(int crewMemberIndex)
	{
		if (!IsAuthority())
			return;

		if (crewMemberIndex == DayZPlayerConstants.VEHICLESEAT_DRIVER)
		{
			EngineStop();
		}
	}

	/*!
		Gets called everytime when the specified vehicle's fluid
		changes its current value eg. when vehicle is consuming fuel.

		This callback is called on server.
	*/
	override void OnFluidChanged(CarFluid fluid, float newValue, float oldValue)
	{
		switch ( fluid )
		{
		case CarFluid.FUEL:
			m_FuelAmmount = newValue;
			break;

		case CarFluid.OIL:
			m_OilAmmount = newValue;
			break;

		case CarFluid.BRAKE:
			m_BrakeAmmount = newValue;
			break;

		case CarFluid.COOLANT:
			m_CoolantAmmount = newValue;
			break;
		}
	}
	
	/*!
		Gets called everytime the game wants to start the engine.

		\return true if the engine can start, false otherwise.
	*/
	override bool OnBeforeEngineStart()
	{
		ECarOperationalState state = CheckOperationalRequirements();
		SetCarEngineSoundState(CarEngineSoundState.NONE);
		return state == ECarOperationalState.OK;
	}

	void OnIgnition()
	{
		ECarOperationalState state = CheckOperationalRequirements();

		if (state == ECarOperationalState.RUINED)
		{
			return;
		}

		if (state & ECarOperationalState.NO_BATTERY)
		{
			HandleEngineSound(CarEngineSoundState.START_NO_BATTERY);
			return;
		}

		if (state & ECarOperationalState.NO_IGNITER)
		{
			HandleEngineSound(CarEngineSoundState.START_NO_SPARKPLUG);
			return;
		}

		if (state & ECarOperationalState.NO_FUEL)
		{
			HandleEngineSound(CarEngineSoundState.START_NO_FUEL);
			return;
		}

		HandleEngineSound(CarEngineSoundState.STARTING);
	}
	
	// Whether the car engine can be started
	int CheckOperationalRequirements()
	{
		int state = ECarOperationalState.OK;

		EntityAI item = null;

		if (IsDamageDestroyed() || GetHealthLevel("Engine") >= GameConstants.STATE_RUINED)
		{
			state |= ECarOperationalState.RUINED;
		}
		
		if (GetFluidFraction(CarFluid.FUEL) <= 0)
		{
			state |= ECarOperationalState.NO_FUEL;
		}

		if (IsVitalCarBattery() || IsVitalTruckBattery())
		{
			item = GetBattery();
			float batteryConsume = GetBatteryConsumption();
			if (!item || (item && (item.IsRuined() || item.GetCompEM().GetEnergy() < batteryConsume)))
				state |= ECarOperationalState.NO_BATTERY;
		}

		if (IsVitalSparkPlug())
		{
			item = FindAttachmentBySlotName("SparkPlug");
			if (!item || (item && item.IsRuined()))
				state |= ECarOperationalState.NO_IGNITER;
		}
		
		if (IsVitalGlowPlug())
		{
			item = FindAttachmentBySlotName("GlowPlug");
			if (!item || (item && item.IsRuined()))
				state |= ECarOperationalState.NO_IGNITER;
		}
		
		return state;
	}

	bool CheckOperationalState()
	{
		return CheckOperationalRequirements() == ECarOperationalState.OK;
	}

	override void OnGearChanged(int newGear, int oldGear)
	{
		//Debug.Log(string.Format("OnGearChanged newGear=%1,oldGear=%2", newGear, oldGear));
		UpdateLights(newGear);
	}
	
	//! Gets called everytime the engine starts.
	override void OnEngineStart()
	{
		ItemBase battery = GetBattery();
		if (GetGame().IsServer() && battery)
		{
			float batteryConsume = GetBatteryConsumption();
			battery.GetCompEM().ConsumeEnergy(batteryConsume);

			UpdateBattery(battery);
		}
		
		UpdateLights();
		
		HandleEngineSound(CarEngineSoundState.START_OK);
	}

	//! Gets called everytime the engine stops.
	override void OnEngineStop()
	{
		ItemBase battery = GetBattery();
		if (GetGame().IsServer() && battery)
		{
			UpdateBattery(battery);
		}

		UpdateLights();
		
		CarEngineSoundState stopSoundState = CarEngineSoundState.STOP_OK;
		if (GetFluidFraction(CarFluid.FUEL) <= 0)
			stopSoundState = CarEngineSoundState.STOP_NO_FUEL;

		HandleEngineSound(stopSoundState);
		
		SetEngineZoneReceivedHit(false);
	}
	
	//! Proper way to get if light is swiched on. Use instead of IsLightsOn().
	bool IsScriptedLightsOn()
	{
		return m_HeadlightsOn;
	}
	
	//! Switches headlights on/off, including the illumination of the control panel and synchronizes this change to all clients.
	void ToggleHeadlights()
	{
		// TODO(kumarjac): Call 'UpdateBattery' here. Probably can't right now

		m_HeadlightsOn = !m_HeadlightsOn;
		SetSynchDirty();
	}
	
	void UpdateLights(int new_gear = -1)
	{
		#ifndef SERVER
		UpdateLightsClient(new_gear);
		#endif
		UpdateLightsServer(new_gear);
	}
	
	void UpdateLightsClient(int newGear = -1)
	{
		int gear;
		
		if (newGear == -1)
		{
			gear = GetGear();
		}
		else
		{
			gear = newGear;
		}
		
		if (m_HeadlightsOn)
		{
			if (!m_Headlight && m_HeadlightsState != CarHeadlightBulbsState.NONE)
			{
				m_Headlight = CreateFrontLight();
			}
			
			if (m_Headlight)
			{
				switch (m_HeadlightsState)
				{
				case CarHeadlightBulbsState.LEFT:
					m_Headlight.AttachOnMemoryPoint(this, m_LeftHeadlightPoint, m_LeftHeadlightTargetPoint);
					m_Headlight.SegregateLight();
					break;
				case CarHeadlightBulbsState.RIGHT:
					m_Headlight.AttachOnMemoryPoint(this, m_RightHeadlightPoint, m_RightHeadlightTargetPoint);
					m_Headlight.SegregateLight();
					break;
				case CarHeadlightBulbsState.BOTH:
					vector local_pos_left = GetMemoryPointPos(m_LeftHeadlightPoint);
					vector local_pos_right = GetMemoryPointPos(m_RightHeadlightPoint);
					
					vector local_pos_middle = (local_pos_left + local_pos_right) * 0.5;
					m_Headlight.AttachOnObject(this, local_pos_middle);
					m_Headlight.AggregateLight();
					break;
				default:
					m_Headlight.FadeOut();
					m_Headlight = null;
				}
			}
		}
		else
		{
			if (m_Headlight)
			{
				m_Headlight.FadeOut();
				m_Headlight = null;
			}
		}
			
		// brakes & reverse
		switch (gear)
		{
			case CarGear.REVERSE:
			case CarAutomaticGearboxMode.R:
				if (m_BrakesArePressed)
					m_RearLightType = CarRearLightType.BRAKES_AND_REVERSE;
				else
					m_RearLightType = CarRearLightType.REVERSE_ONLY;
				break;
			default:
				if (m_BrakesArePressed)
					m_RearLightType = CarRearLightType.BRAKES_ONLY;
				else
					m_RearLightType = CarRearLightType.NONE;
		}

		//Debug.Log(string.Format("m_BrakesArePressed=%1, m_RearLightType=%2", m_BrakesArePressed.ToString(), EnumTools.EnumToString(CarRearLightType, m_RearLightType)));

		if (!m_RearLight && m_RearLightType != CarRearLightType.NONE && m_HeadlightsState != CarHeadlightBulbsState.NONE)
		{
			if (EngineIsOn() || m_RearLightType == CarRearLightType.BRAKES_ONLY || m_RearLightType == CarRearLightType.BRAKES_AND_REVERSE)
			{	
				m_RearLight = CreateRearLight();
				vector localPos = GetMemoryPointPos(m_ReverseLightPoint);
				m_RearLight.AttachOnObject(this, localPos, "180 0 0");
			}
		}

		// rear lights
		if (m_RearLight && m_RearLightType != CarRearLightType.NONE && m_HeadlightsState != CarHeadlightBulbsState.NONE)
		{
			switch (m_RearLightType)
			{
				case CarRearLightType.BRAKES_ONLY:
					BrakesRearLight();
					break;
				case CarRearLightType.REVERSE_ONLY:
					if (EngineIsOn())
						ReverseRearLight();
					else
						NoRearLight();

					break;
				case CarRearLightType.BRAKES_AND_REVERSE:
					if (EngineIsOn())
						BrakeAndReverseRearLight();
					else
						BrakesRearLight();
	
					break;
				default:
					NoRearLight();
			}
		}
		else
		{
			if (m_RearLight)
			{
				NoRearLight();
			}
		}
	}
	
	void UpdateLightsServer(int newGear = -1)
	{
		int gear;
		
		if (newGear == -1)
		{
			gear = GetGear();
			if (GearboxGetType() == CarGearboxType.AUTOMATIC)
			{
				gear = GearboxGetMode();
			}
		}
		else
		{
			gear = newGear;
		}
		
		if (m_HeadlightsOn)
		{
			DashboardShineOn();
			TailLightsShineOn();
			
			switch (m_HeadlightsState)
			{
			case CarHeadlightBulbsState.LEFT:
				LeftFrontLightShineOn();
				RightFrontLightShineOff();
				break;
			case CarHeadlightBulbsState.RIGHT:
				LeftFrontLightShineOff();
				RightFrontLightShineOn();
				break;
			case CarHeadlightBulbsState.BOTH:
				LeftFrontLightShineOn();
				RightFrontLightShineOn();
				break;
			default:
				LeftFrontLightShineOff();
				RightFrontLightShineOff();
			}
			
			//Debug.Log(string.Format("m_HeadlightsOn=%1, m_HeadlightsState=%2", m_HeadlightsOn.ToString(), EnumTools.EnumToString(CarHeadlightBulbsState, m_HeadlightsState)));
		}
		else
		{
			TailLightsShineOff();
			DashboardShineOff();
			LeftFrontLightShineOff();
			RightFrontLightShineOff();
		}
			

		// brakes & reverse
		switch (gear)
		{
			case CarGear.REVERSE:
			case CarAutomaticGearboxMode.R:
				if (m_BrakesArePressed)
					m_RearLightType = CarRearLightType.BRAKES_AND_REVERSE;
				else
					m_RearLightType = CarRearLightType.REVERSE_ONLY;
				break;
			default:
				if (m_BrakesArePressed)
					m_RearLightType = CarRearLightType.BRAKES_ONLY;
				else
					m_RearLightType = CarRearLightType.NONE;
		}

		//Debug.Log(string.Format("m_BrakesArePressed=%1, m_RearLightType=%2", m_BrakesArePressed.ToString(), EnumTools.EnumToString(CarRearLightType, m_RearLightType)));
			
		
		// rear lights
		if (m_RearLightType != CarRearLightType.NONE && m_HeadlightsState != CarHeadlightBulbsState.NONE)
		{
			switch (m_RearLightType)
			{
				case CarRearLightType.BRAKES_ONLY:
					ReverseLightsShineOff();
					BrakeLightsShineOn();
					break;
				case CarRearLightType.REVERSE_ONLY:
					if (EngineIsOn())
					{
						ReverseLightsShineOn();
						BrakeLightsShineOff();
					}
					else
					{
						ReverseLightsShineOff();
						BrakeLightsShineOff();
					}
					break;
				case CarRearLightType.BRAKES_AND_REVERSE:
					if (EngineIsOn())
					{
						BrakeLightsShineOn();
						ReverseLightsShineOn();
					}
					else
					{
						ReverseLightsShineOff();
						BrakeLightsShineOn();
					}
					break;
				default:
					ReverseLightsShineOff();
					BrakeLightsShineOff();
			}
		}
		else
		{
			ReverseLightsShineOff();
			BrakeLightsShineOff();
		}
	}
	
	protected void BrakesRearLight()
	{
		m_RearLight.SetAsSegregatedBrakeLight();
	}
	
	protected void ReverseRearLight()
	{
		m_RearLight.SetAsSegregatedReverseLight();
	}
	
	protected void BrakeAndReverseRearLight()
	{
		m_RearLight.AggregateLight();
		m_RearLight.SetFadeOutTime(1);
	}
	
	protected void NoRearLight()
	{
		m_RearLight.FadeOut();
		m_RearLight = null;
	}
	
	protected void LeftFrontLightShineOn()
	{
		string material = ConfigGetString("frontReflectorMatOn");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_FRONT_LIGHT_L, material);
		}
	}
	
	protected void RightFrontLightShineOn()
	{
		string material = ConfigGetString("frontReflectorMatOn");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_FRONT_LIGHT_R, material);
		}
	}
	
	protected void LeftFrontLightShineOff()
	{
		string material = ConfigGetString("frontReflectorMatOff");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_FRONT_LIGHT_L, material);
		}
	}
	
	protected void RightFrontLightShineOff()
	{
		string material = ConfigGetString("frontReflectorMatOff");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_FRONT_LIGHT_R, material);
		}
	}
	
	protected void ReverseLightsShineOn()
	{
		string material = ConfigGetString("ReverseReflectorMatOn");
			
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_REVERSE_LIGHT_L, material);
			SetObjectMaterial(SELECTION_ID_REVERSE_LIGHT_R, material);
		}
	}
	
	protected void ReverseLightsShineOff()
	{
		string material = ConfigGetString("ReverseReflectorMatOff");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_REVERSE_LIGHT_L, material);
			SetObjectMaterial(SELECTION_ID_REVERSE_LIGHT_R, material);
		}
	}
	
	protected void BrakeLightsShineOn()
	{
		string material = ConfigGetString("brakeReflectorMatOn");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_BRAKE_LIGHT_L, material);
			SetObjectMaterial(SELECTION_ID_BRAKE_LIGHT_R, material);
		}
	}
	
	protected void BrakeLightsShineOff()
	{
		string material = ConfigGetString("brakeReflectorMatOff");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_BRAKE_LIGHT_L, material);
			SetObjectMaterial(SELECTION_ID_BRAKE_LIGHT_R, material);
		}
	}
	
	protected void TailLightsShineOn()
	{
		string material = ConfigGetString("TailReflectorMatOn");
		string materialOff = ConfigGetString("TailReflectorMatOff");
		
		if (material && materialOff)
		{
			if (m_HeadlightsState == CarHeadlightBulbsState.LEFT)
			{
				SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_L, material);
				SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_R, materialOff);
			}
			else if (m_HeadlightsState == CarHeadlightBulbsState.RIGHT)
			{
				SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_L, materialOff);
				SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_R, material);
			}
			else if (m_HeadlightsState == CarHeadlightBulbsState.BOTH)
			{
				SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_L, material);
				SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_R, material);
			}
		}
	}
	
	protected void TailLightsShineOff()
	{
		string material = ConfigGetString("TailReflectorMatOff");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_L, material);
			SetObjectMaterial(SELECTION_ID_TAIL_LIGHT_R, material);
		}
	}
	
	protected void DashboardShineOn()
	{
		string material = ConfigGetString("dashboardMatOn");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_DASHBOARD_LIGHT, material);
		}
	}
	
	protected void DashboardShineOff()
	{
		string material = ConfigGetString("dashboardMatOff");
		
		if (material)
		{
			SetObjectMaterial(SELECTION_ID_DASHBOARD_LIGHT, material);
		}
	}
	
	// Override this for a car-specific light type
	protected CarRearLightBase CreateRearLight()
	{
		return CarRearLightBase.Cast(ScriptedLightBase.CreateLight(OffroadHatchbackFrontLight));
	}
	
	// Override this for a car-specific light type
	protected CarLightBase CreateFrontLight()
	{
		return CarLightBase.Cast(ScriptedLightBase.CreateLight(OffroadHatchbackFrontLight));
	}
	
	protected void CheckVitalItem(bool isVital, int slotId)
	{
		if ( !isVital || !GetInventory() )
			return;

		EntityAI item = GetInventory().FindAttachment(slotId);

		if (!item || item.IsRuined())
		{
			EngineStop();
		}
	}
	
	protected void CheckVitalItem(bool isVital, string slot_name)
	{
		if ( !isVital )
			return;

		int slotId = InventorySlots.GetSlotIdFromString(slot_name);
		CheckVitalItem(isVital, slotId);
	}

	protected void LeakFluid(CarFluid fluid)
	{
		float ammount = 0;
		
		switch (fluid)
		{
			case CarFluid.COOLANT:
				ammount = (1 - m_RadiatorHealth) * RandomFloat(0.02, 0.05);//CARS_LEAK_TICK_MIN; CARS_LEAK_TICK_MAX
				Leak(fluid, ammount);
			break;
			
			case CarFluid.OIL:
				ammount = (1 - m_EngineHealth) * RandomFloat(0.02, 1.0);//CARS_LEAK_OIL_MIN; CARS_LEAK_OIL_MAX
				Leak(fluid, ammount);
			break;
			
			case CarFluid.FUEL:
				ammount = (1 - m_FuelTankHealth) * RandomFloat(0.02, 0.05);//CARS_LEAK_TICK_MIN; CARS_LEAK_TICK_MAX
				Leak(fluid, ammount);
			break;
		}
	}

	protected void CarPartsHealthCheck()
	{
		if (HasRadiator())
		{
			EntityAI radiator = GetRadiator();
			int radiatorHealthLevel = radiator.GetHealthLevel("");
			m_RadiatorHealth = GetHealthLevelValue(radiatorHealthLevel, "");
		}
		else
		{
			m_RadiatorHealth = 0;
		}

		int engineHealthLevel = GetHealthLevel("Engine");
		m_EngineHealth = GetHealthLevelValue(engineHealthLevel, "Engine");

		int fuelTankHealthLevel = GetHealthLevel("FuelTank");
		m_FuelTankHealth = GetHealthLevelValue(fuelTankHealthLevel, "FuelTank");
	}
	
	bool GetCrashLightSound()
	{
		return m_PlayCrashSoundLight;
	}
	
	void SynchCrashLightSound(bool play)
	{
		if (m_PlayCrashSoundLight != play)
		{
			m_PlayCrashSoundLight = play;
			SetSynchDirty();
		}
	}

	void PlayCrashLightSound()
	{
		PlaySoundEx("offroad_hit_light_SoundSet", m_CrashSoundLight, m_PlayCrashSoundLight);
	}

	bool GetCrashHeavySound()
	{
		return m_PlayCrashSoundHeavy;
	}

	void SynchCrashHeavySound(bool play)
	{
		if (m_PlayCrashSoundHeavy != play)
		{
			m_PlayCrashSoundHeavy = play;
			SetSynchDirty();
		}
	}
	
	void PlayCrashHeavySound()
	{
		PlaySoundEx("offroad_hit_heavy_SoundSet", m_CrashSoundHeavy, m_PlayCrashSoundHeavy);
	}
	
	void PlaySoundEx(string soundset, EffectSound sound, out bool soundbool)
	{
		#ifndef SERVER
		if (!sound)
		{
			sound =	SEffectManager.PlaySoundCachedParams(soundset, GetPosition());
			if( sound )
			{
				sound.SetAutodestroy(true);
			}
		}
		else
		{
			if (!sound.IsSoundPlaying())
			{
				sound.SetPosition(GetPosition());
				sound.SoundPlay();
			}
		}
		
		soundbool = false;
		#endif
	}
	
	void PlaySound(string soundset, EffectSound sound, out bool soundbool)
	{
		PlaySoundEx(soundset, sound, soundbool);
	}
	
	string GetAnimSourceFromSelection( string selection )
	{
		return "";
	}

	string GetSelectionFromAnimSource( string animSource )
	{
		// Brute force for vehicles that aren't set up
		
		TStringArray allSelections = new TStringArray();
		GetSelectionList(allSelections);
		
		foreach (string selectionAll : allSelections)
		{
			string animSrc = GetAnimSourceFromSelection(selectionAll);
			animSrc.ToLower();
			if (animSrc != animSource)
				continue;
			
			if (animSrc)
			{
				return selectionAll;
			}
		}

		return "";
	}
	
	string GetDoorConditionPointFromSelection( string selection )
	{
		switch( selection )
		{
			case "seat_driver":
				return "seat_con_1_1";
			break;
			case "seat_codriver":
				return "seat_con_2_1";
			break;
			case "seat_cargo1":
				return "seat_con_1_2";
			break;
			case "seat_cargo2":
				return "seat_con_2_2";
			break;
		}
		
		return "";
	}
	
	string GetDoorSelectionNameFromSeatPos(int posIdx)
	{
		return "";
	}
	
	string GetDoorInvSlotNameFromSeatPos(int posIdx)
	{
		return "";
	}
	
	int GetCrewIndex( string selection )
	{
		return -1;
	}

	override bool CanReachSeatFromDoors( string pSeatSelection, vector pFromPos, float pDistance = 1.0 )
	{
		string conPointName = GetDoorConditionPointFromSelection(pSeatSelection);
		if (conPointName.Length() > 0)
		{
			if ( MemoryPointExists(conPointName) )
			{
				vector conPointMS = GetMemoryPointPos(conPointName);
				vector conPointWS = ModelToWorld(conPointMS);
				
				//! skip the height for now
				conPointWS[1] = 0;
				pFromPos[1] = 0;
				
				if (vector.Distance(pFromPos, conPointWS) <= pDistance)
				{
					return true;
				}
			}
		}

		return false;		
	}

	bool IsVitalCarBattery()
	{
		return true;
	}
	
	bool IsVitalTruckBattery()
	{
		return true;
	}
		
	bool IsVitalGlowPlug()
	{
		return true;
	}
	
	bool IsVitalEngineBelt()
	{
		return true;
	}
	
	bool IsVitalRadiator()
	{
		return true;
	}
	
	bool IsVitalFuelTank()
	{
		return true;
	}
	
	bool HasRadiator()
	{
		return m_Radiator != null;
	}
	
	EntityAI GetRadiator()
	{
		return m_Radiator;
	}
	
	bool IsMoving()
	{
		return GetSpeedometerAbsolute() > 3.5;
	}
	
	bool IsHandbrakeActive()
	{
		return GetHandbrake() > 0.0;
	}

	//! camera type
	override int Get3rdPersonCameraType()
	{
		return DayZPlayerCameras.DAYZCAMERA_3RD_VEHICLE;

	}

	void SetEngineStarted(bool started)
	{
		m_EngineStarted = started;
	}

	int GetCarDoorsState(string slotType)
	{
		return CarDoorState.DOORS_MISSING;
	}
	
	CarDoorState TranslateAnimationPhaseToCarDoorState(string animation)
	{
		if (GetAnimationPhase(animation) > 0.5)
		{
			return CarDoorState.DOORS_OPEN;
		}
		else
		{
			return CarDoorState.DOORS_CLOSED;
		}
	}

	string GetActionCompNameCoolant()
	{
		return "radiator";
	}

	float GetActionDistanceCoolant()
	{
		return 2.0;
	}
	
	string GetActionCompNameOil()
	{
		return "carradiator";
	}

	float GetActionDistanceOil()
	{
		return 2.0;
	}
	
	string GetActionCompNameBrakes()
	{
		return "carradiator";
	}

	float GetActionDistanceBrakes()
	{
		return 2.0;
	}
	
	override bool CanPutIntoHands(EntityAI parent)
	{
		return false;
	}
	
	void InitializeActions()
	{
		m_InputActionMap = m_CarTypeActionsMap.Get( this.Type() );
		if (!m_InputActionMap)
		{
			TInputActionMap iam = new TInputActionMap;
			m_InputActionMap = iam;
			SetActions();
			m_CarTypeActionsMap.Insert(this.Type(), m_InputActionMap);
		}
	}
	
	override void GetActions(typename action_input_type, out array<ActionBase_Basic> actions)
	{
		if (!m_ActionsInitialize)
		{
			m_ActionsInitialize = true;
			InitializeActions();
		}
		
		actions = m_InputActionMap.Get(action_input_type);
	}
	
	void SetActions()
	{
		AddAction(ActionOpenCarDoorsOutside);
		AddAction(ActionCloseCarDoorsOutside);
		AddAction(ActionGetInTransport);
		AddAction(ActionSwitchLights);
		AddAction(ActionCarHornShort);
		AddAction(ActionCarHornLong);
		AddAction(ActionPushCar);
	}
	
	void AddAction(typename actionName)
	{
		ActionBase action = ActionManagerBase.GetAction(actionName);

		if (!action)
		{
			Debug.LogError("Action " + actionName + " dosn't exist!");
			return;
		}		
		
		typename ai = action.GetInputType();
		if (!ai)
		{
			m_ActionsInitialize = false;
			return;
		}
		array<ActionBase_Basic> action_array = m_InputActionMap.Get(ai);
		
		if (!action_array)
		{
			action_array = new array<ActionBase_Basic>;
			m_InputActionMap.Insert(ai, action_array);
		}
		
		if ( LogManager.IsActionLogEnable() )
		{
			Debug.ActionLog(action.ToString() + " -> " + ai, this.ToString() , "n/a", "Add action" );
		}
		action_array.Insert(action);
	}
	
	void RemoveAction(typename actionName)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		ActionBase action = player.GetActionManager().GetAction(actionName);
		typename ai = action.GetInputType();
		array<ActionBase_Basic> action_array = m_InputActionMap.Get(ai);
		
		if (action_array)
		{
			action_array.RemoveItem(action);
		}
	}
	
	override bool IsInventoryVisible()
	{
		return ( GetGame().GetPlayer() && ( !GetGame().GetPlayer().GetCommand_Vehicle() || GetGame().GetPlayer().GetCommand_Vehicle().GetTransport() == this ) );
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		super.EEHealthLevelChanged(oldLevel,newLevel,zone);
		
		if (newLevel == GameConstants.STATE_RUINED && oldLevel != newLevel)
		{
			bool dummy;
			switch ( zone )
			{
			case "WindowLR":
			case "WindowRR":
				if (m_Initialized)
				{
					PlaySoundEx("offroad_hit_window_small_SoundSet", m_WindowSmall, dummy);
				}
			break;
			
			case "WindowFront":
			case "WindowBack":
			case "WindowFrontLeft":
			case "WindowFrontRight":
				if (m_Initialized)
				{
					PlaySoundEx("offroad_hit_window_large_SoundSet", m_WindowLarge, dummy);
				}
			break;

			case "Engine":
				#ifndef SERVER
				CreateCarDestroyedEffect();
				#endif
			break;
			}
		}
	}
	
	override void EEOnCECreate()
	{

		float maxVolume = GetFluidCapacity( CarFluid.FUEL );
		float amount = Math.RandomFloat(0.0, maxVolume * 0.35 );

		Fill( CarFluid.FUEL, amount );
	}
	
	/*override void EOnPostFrame(IEntity other, int extra)
	{
		//Prepared for fix particle simulation when player is not in vehicle
	}*/
	
	void ForceUpdateLightsStart()
	{
		if (!m_ForceUpdateLights)
		{
			m_ForceUpdateLights = true;
			SetSynchDirty();
		}
	}
	
	void ForceUpdateLightsEnd()
	{
		if (m_ForceUpdateLights)
		{
			m_ForceUpdateLights = false;
			SetSynchDirty();
		}
	}
	
	//Get the engine start battery consumption
	float GetBatteryConsumption()
	{
		return m_BatteryConsume;
	}
	
	float GetBatteryRuntimeConsumption()
	{
		return m_BatteryContinuousConsume;
	}

	float GetBatteryRechargeRate()
	{
		return -m_BatteryRecharge;
	}
	
	ItemBase GetBattery()
	{
		if (IsVitalCarBattery()) 
		{
			return ItemBase.Cast(GetInventory().FindAttachment(CarBattery.SLOT_ID));
		}
		else if (IsVitalTruckBattery())
		{
			return ItemBase.Cast(GetInventory().FindAttachment(TruckBattery.SLOT_ID));
		}
		
		return null;
	}

	protected void UpdateBattery(ItemBase battery)
	{
		if (!battery)
		{
			m_BatteryTimer = 0;
			return;
		}
		
		// unlikely
		if (m_BatteryTimer < 0)
		{
			m_BatteryTimer = 0;
		}

		bool engineOn = EngineIsOn();
		bool lightsOn = IsScriptedLightsOn();

		if (engineOn)
		{
			// alternator
			battery.GetCompEM().ConsumeEnergy(GetBatteryRechargeRate() * m_BatteryTimer);
		}
		else if (!engineOn && lightsOn)
		{
			battery.GetCompEM().ConsumeEnergy(GetBatteryRuntimeConsumption() * m_BatteryTimer);
		}

		if (lightsOn && battery.GetCompEM().GetEnergy() <= 0)
		{
			// lights currently don't automatically turn back on if the headlight was last turned on, so we just keep them on if the engine is on
			if (!engineOn)
			{
				ToggleHeadlights();
			}
		}

		m_BatteryTimer = 0;
	}
	
	void SetCarHornState(int pState)
	{
		m_CarHornState = pState;
		SetSynchDirty();
		
		if (GetGame().IsServer())
		{
			GenerateCarHornAINoise(pState);
		}
	}
	
	protected void GenerateCarHornAINoise(int pState)
	{
		if (pState != ECarHornState.OFF)
		{
			if (m_NoiseSystem && m_NoisePar)
			{
				float noiseMultiplier = 1.0;
				if (pState == ECarHornState.LONG)
					noiseMultiplier = 2.0;

				noiseMultiplier *= NoiseAIEvaluate.GetNoiseReduction(GetGame().GetWeather());
				
				m_NoiseSystem.AddNoiseTarget(GetPosition(), 5, m_NoisePar, noiseMultiplier);
			}
		}
	}

	override vector GetDefaultHitPosition()
	{
		return vector.Zero;
	}
	
	float GetPushForceCoefficientMultiplier()
	{
		return 1.0;
	}

#ifdef DEVELOPER
	override protected string GetDebugText()
	{
		string debug_output = super.GetDebugText();
		if (GetGame().IsServer())
		{
			debug_output += m_DebugContactDamageMessage + "\n";
		}
		debug_output += "Entity momentum: " + GetMomentum();
		debug_output += "\nIsEngineON: " + EngineIsOn();

		return debug_output;	
	}
#endif

	protected void SpawnUniversalParts()
	{
		GetInventory().CreateInInventory("HeadlightH7");
		GetInventory().CreateInInventory("HeadlightH7");
		GetInventory().CreateInInventory("HeadlightH7");
		GetInventory().CreateInInventory("HeadlightH7");

		if (IsVitalCarBattery())
		{
			GetInventory().CreateInInventory("CarBattery");
			GetInventory().CreateInInventory("CarBattery");
		}

		if (IsVitalTruckBattery())
		{
			GetInventory().CreateInInventory("TruckBattery");
			GetInventory().CreateInInventory("TruckBattery");
		}

		if (IsVitalRadiator())
		{
			GetInventory().CreateInInventory("CarRadiator");
			GetInventory().CreateInInventory("CarRadiator");
		}

		if (IsVitalSparkPlug())
		{
			GetInventory().CreateInInventory("SparkPlug");
			GetInventory().CreateInInventory("SparkPlug");
		}

		if (IsVitalGlowPlug())
		{
			GetInventory().CreateInInventory("GlowPlug");
			GetInventory().CreateInInventory("GlowPlug");
		}
	}

	protected void SpawnAdditionalItems()
	{
		GetInventory().CreateInInventory("Wrench");
		GetInventory().CreateInInventory("LugWrench");
		GetInventory().CreateInInventory("Screwdriver");
		GetInventory().CreateInInventory("EpoxyPutty");

		GetInventory().CreateInInventory("CanisterGasoline");

		EntityAI ent;
		ItemBase container;
		ent = GetInventory().CreateInInventory("CanisterGasoline");
		if (Class.CastTo(container, ent))
		{
			container.SetLiquidType(LIQUID_WATER, true);
		}

		ent = GetInventory().CreateInInventory("Blowtorch");
		if (ent)
		{
			ent.GetInventory().CreateInInventory("LargeGasCanister");
		}

		ent = GetInventory().CreateInInventory("Blowtorch");
		if (ent)
		{
			ent.GetInventory().CreateInInventory("LargeGasCanister");
		}
	}

	protected void FillUpCarFluids()
	{
		Fill(CarFluid.FUEL, 200.0);
		Fill(CarFluid.COOLANT, 200.0);
		Fill(CarFluid.OIL, 200.0);
	}

	protected override event typename GetOwnerStateType()
	{
		return CarScriptOwnerState;
	}

	protected override event typename GetMoveType()
	{
		return CarScriptMove;
	}

	protected override event void ObtainState(/*inout*/ PawnOwnerState pState)
	{
		auto state = CarScriptOwnerState.Cast(pState);
		state.m_fTime = m_Time;
	}

	protected override event void RewindState(PawnOwnerState pState, /*inout*/ PawnMove pMove, inout NetworkRewindType pRewindType)
	{
		auto state = CarScriptOwnerState.Cast(pState);
		m_Time = state.m_fTime;
	}

	// Cars that use the old networking only perform this part of the simulation on the server and not the clients
	// TODO(kumarjac): Obsolete this function once new networking is permanently enabled
	bool IsServerOrOwner()
	{
		bool isServer = GetGame().IsServer();
		if (isServer || GetNetworkMoveStrategy() != NetworkMoveStrategy.PHYSICS)
		{
			return isServer;
		}
		
		return IsOwner();
	}
		
	//! DEPRECATED
	protected float m_BatteryEnergyStartMin = 5.0;

	/*!
		DEPRECATED Gets called everytime the game wants to switch the lights.
		\return true when lights can be switched, false otherwise.
	*/
	[Obsolete("no replacement")]
	bool OnBeforeSwitchLights(bool toOn)
	{
		return true;
	}

	[Obsolete("no replacement")]
	float GetEnviroHeatComfortOverride();
}

class Truck_01_Base extends CarScript
{
	protected ref UniversalTemperatureSource m_UTSource;
	protected ref UniversalTemperatureSourceSettings m_UTSSettings;
	protected ref UniversalTemperatureSourceLambdaEngine m_UTSLEngine;

	void Truck_01_Base()
	{
		//m_dmgContactCoef 		= 0.018;
		m_enginePtcPos 			= "0 1.346 2.205";
		
		m_EngineStartOK 		= "Truck_01_engine_start_SoundSet";
		m_EngineStartBattery	= "Truck_01_engine_failed_start_battery_SoundSet";
		m_EngineStartPlug 		= "Truck_01_engine_failed_start_sparkplugs_SoundSet";
		m_EngineStartFuel 		= "Truck_01_engine_failed_start_fuel_SoundSet";
		m_EngineStop 			= "Truck_01_engine_stop_SoundSet";
		m_EngineStopFuel 		= "Truck_01_engine_stop_fuel_SoundSet";

		m_CarDoorOpenSound 		= "Truck_01_door_open_SoundSet";
		m_CarDoorCloseSound 	= "Truck_01_door_close_SoundSet";
		
		m_CarHornShortSoundName = "Truck_01_Horn_Short_SoundSet";
		m_CarHornLongSoundName	= "Truck_01_Horn_SoundSet";
		
		SetEnginePos("0 1.4 2.25");
	}

	override void EEInit()
	{		
		super.EEInit();
		
		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
 			m_UTSSettings 						= new UniversalTemperatureSourceSettings();
			m_UTSSettings.m_ManualUpdate 		= true;
			m_UTSSettings.m_TemperatureItemCap 	= GameConstants.ITEM_TEMPERATURE_NEUTRAL_ZONE_MIDDLE;
			m_UTSSettings.m_TemperatureCap		= 0;
			m_UTSSettings.m_RangeFull			= 0.5;
			m_UTSSettings.m_RangeMax			= 2;
			
			m_UTSLEngine						= new UniversalTemperatureSourceLambdaEngine();
			m_UTSource							= new UniversalTemperatureSource(this, m_UTSSettings, m_UTSLEngine);
		}		
	}
	
	override void OnEngineStart()
	{
		super.OnEngineStart();

		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			m_UTSource.SetDefferedActive(true, 20.0);
		}
	}
	
	override void OnEngineStop()
	{
		super.OnEngineStop();

		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			m_UTSource.SetDefferedActive(false, 10.0);
		}
	}
	
	override void EOnPostSimulate(IEntity other, float timeSlice)
	{
		if (GetGame().IsServer() || !GetGame().IsMultiplayer())
		{
			if (m_UTSource.IsActive())
			{
				m_UTSource.Update(m_UTSSettings, m_UTSLEngine);
			}
		}
	}

	override float GetTransportCameraDistance()
	{
		return 6.5;
	}
	
	override vector GetTransportCameraOffset()
	{
		return "0 3.2 0";
	}
	
	override int GetAnimInstance()
	{
		return VehicleAnimInstances.V3S;
	}

	override int GetSeatAnimationType(int posIdx)
	{
		switch (posIdx)
		{
		case 0:
			return DayZPlayerConstants.VEHICLESEAT_DRIVER;
		case 1:
			return DayZPlayerConstants.VEHICLESEAT_CODRIVER;
		}

		return 0;
	}

	// Override for car-specific light type
	override CarLightBase CreateFrontLight()
	{
		return CarLightBase.Cast(ScriptedLightBase.CreateLight(Truck_01FrontLight));
	}
	
	// Override for car-specific light type
	override CarRearLightBase CreateRearLight()
	{
		return CarRearLightBase.Cast(ScriptedLightBase.CreateLight(Truck_01RearLight));
	}
	
	override bool CanReleaseAttachment(EntityAI attachment)
	{
		if (!super.CanReleaseAttachment(attachment))
		{
			return false;
		}
		
		if (EngineIsOn() && attachment.GetType() == "TruckBattery")
		{
			return false;
		}

		return true;
	}
	
	override protected bool CanManipulateSpareWheel(string slotSelectionName)
	{
		if (slotSelectionName == "wheel_spare_1")
		{
			return GetAnimationPhase("wheelSidePlate1") == 1.0;
		}
		
		if (slotSelectionName == "wheel_spare_2")
		{
			return GetAnimationPhase("wheelSidePlate2") == 1.0;
		}
		
		return super.CanManipulateSpareWheel(slotSelectionName);
	}

	override bool CrewCanGetThrough( int posIdx )
	{
		CarDoor carDoor;
		switch( posIdx )
		{
			case 0:
				if ( GetCarDoorsState("Truck_01_Door_1_1") == CarDoorState.DOORS_CLOSED )
					return false;

				return true;
			
			case 1:
				if ( GetCarDoorsState("Truck_01_Door_2_1") == CarDoorState.DOORS_CLOSED )
					return false;

				return true;
		}

		return false;
	}

	override string GetDoorSelectionNameFromSeatPos(int posIdx)
	{
		switch( posIdx )
		{
			case 0:
				return "doors_driver";
			break;
			case 1:
				return "doors_codriver";
			break;
		}

		return super.GetDoorSelectionNameFromSeatPos(posIdx);
	}

	override string GetDoorInvSlotNameFromSeatPos(int posIdx)
	{
		switch( posIdx )
		{
			case 0:
				return "Truck_01_Door_1_1";
			break;
			case 1:
				return "Truck_01_Door_2_1";
			break;
		}

		return super.GetDoorInvSlotNameFromSeatPos(posIdx);
	}

	override float OnSound(CarSoundCtrl ctrl, float oldValue)
	{
		switch (ctrl)
		{
		case CarSoundCtrl.DOORS:
			float newValue = 0;

			//-----
			if (GetCarDoorsState("Truck_01_Door_1_1") == CarDoorState.DOORS_CLOSED)
			{
				newValue += 0.4;
			}

			if (GetCarDoorsState("Truck_01_Door_2_1") == CarDoorState.DOORS_CLOSED)
			{
				newValue += 0.4;
			}

			return Math.Clamp(newValue, 0, 1);
		break;
		}

		return super.OnSound(ctrl, oldValue);
	}
	
	override void OnAnimationPhaseStarted(string animSource, float phase)
	{
		super.OnAnimationPhaseStarted(animSource, phase);

		#ifndef SERVER
		switch (animSource)
		{
		case "wheelsideplate1":
		case "wheelsideplate2":
			EffectSound sound = CreateSoundForAnimationSource(animSource);
			if (phase == 0)
			{
				sound.SetSoundSet("Truck_01_Gear_Open_Side_Plate_SoundSet");
			}
			else
			{
				sound.SetSoundSet("Truck_01_Gear_Close_Side_Plate_SoundSet");
			}

			sound.SetSoundFadeIn(0.1);
			sound.SetSoundFadeOut(0.1);
			
			SEffectManager.EffectRegister(sound);
			sound.SoundPlay();
		break;
		}
		#endif
	}
	
	override int GetCarDoorsState( string slotType )
	{
		CarDoor carDoor;
		Class.CastTo(carDoor, FindAttachmentBySlotName(slotType));
		if (!carDoor)
		{
			return CarDoorState.DOORS_MISSING;
		}
		
		switch (slotType)
		{
		case "Truck_01_Door_1_1":
			return TranslateAnimationPhaseToCarDoorState("DoorsDriver");
		case "Truck_01_Door_2_1":
			return TranslateAnimationPhaseToCarDoorState("DoorsCoDriver");
		case "Truck_01_Hood":
			return TranslateAnimationPhaseToCarDoorState("DoorsHood");
		}
		
		return CarDoorState.DOORS_MISSING;
	}

	override string GetAnimSourceFromSelection( string selection )
	{
		switch( selection )
		{
		case "doors_driver":
			return "DoorsDriver";
		case "doors_codriver":
			return "DoorsCoDriver";
		case "doors_hood":
			return "DoorsHood";
		case "doors_trunk":
			return "DoorsTrunk";			
		case "wheelsideplate1":
			return "WheelSidePlate1";
		case "wheelsideplate2":
			return "WheelSidePlate2";
		}

		return "";
	}
	
	override bool CanReachSeatFromSeat(int currentSeat, int nextSeat)
	{
		switch (currentSeat)
		{
		case 0:
			return nextSeat == 1;
		case 1:
			return nextSeat == 0;
		}

		return false;
	}

	override bool CanReachDoorsFromSeat(string pDoorsSelection, int pCurrentSeat)
	{
		switch (pCurrentSeat)
		{
			case 0:
				return pDoorsSelection == "DoorsDriver";
	
			case 1:
				return pDoorsSelection == "DoorsCoDriver";
		}
		
		return false;		
	}
		
	override int GetSeatIndexFromDoor( string pDoorSelection )
	{
		switch (pDoorSelection)
		{
			case "DoorsDriver":
				return 0;

			case "DoorsCoDriver":
				return 1;
		}

		return -1;
	}
	

	override bool IsVitalCarBattery()
	{
		return false;
	}

	override bool IsVitalSparkPlug()
	{
		return false;
	}

	override bool IsVitalGlowPlug()
	{
		return false;
	}

	override bool IsVitalEngineBelt()
	{
		return false;
	}
	
	override bool IsVitalRadiator()
	{
		return false;
	}

	override void SetActions()
	{
		super.SetActions();

		AddAction(ActionSidePlateInteract);
		AddAction(ActionDetachFromTarget_SpecificSlot_WoodenLogs);
		AddAction(ActionDetachFromTarget_SpecificSlot_WoodenPlanks);
		AddAction(ActionDetachFromTarget_SpecificSlot_MetalSheets);
		AddAction(ActionDetachFromTarget_SpecificSlotsCategory_Barrel);
		AddAction(ActionDetachFromTarget_SpecificSlotsCategory_WoodenCrate);
	}

	override void OnDebugSpawn()
	{
		SpawnUniversalParts();
		SpawnAdditionalItems();
		FillUpCarFluids();

		GetInventory().CreateInInventory("Truck_01_Wheel");
		GetInventory().CreateInInventory("Truck_01_Wheel");

		GetInventory().CreateInInventory("Truck_01_WheelDouble");
		GetInventory().CreateInInventory("Truck_01_WheelDouble");
		GetInventory().CreateInInventory("Truck_01_WheelDouble");
		GetInventory().CreateInInventory("Truck_01_WheelDouble");

		GetInventory().CreateInInventory("Truck_01_Door_1_1");
		GetInventory().CreateInInventory("Truck_01_Door_2_1");
		GetInventory().CreateInInventory("Truck_01_Hood");
			
		//-----IN CAR CARGO
		GetInventory().CreateInInventory("Truck_01_Wheel");
		GetInventory().CreateInInventory("Truck_01_Wheel");
		GetInventory().CreateInInventory("Truck_01_WheelDouble");
		GetInventory().CreateInInventory("Truck_01_WheelDouble");
	}
};
class Sedan_02 extends CarScript
{
	protected ref UniversalTemperatureSource m_UTSource;
	protected ref UniversalTemperatureSourceSettings m_UTSSettings;
	protected ref UniversalTemperatureSourceLambdaEngine m_UTSLEngine;

	void Sedan_02()
	{
		m_dmgContactCoef		= 0.0790;

		m_EngineStartOK			= "Sedan_02_engine_start_SoundSet";
		m_EngineStartBattery	= "Sedan_02_engine_failed_start_battery_SoundSet";
		m_EngineStartPlug		= "Sedan_02_engine_failed_start_sparkplugs_SoundSet";
		m_EngineStartFuel		= "Sedan_02_engine_failed_start_fuel_SoundSet";
		m_EngineStop			= "Sedan_02_engine_stop_SoundSet";
		m_EngineStopFuel		= "Sedan_02_engine_stop_fuel_SoundSet";

		m_CarDoorOpenSound		= "offroad_door_open_SoundSet";
		m_CarDoorCloseSound		= "offroad_door_close_SoundSet";
		
		m_CarHornShortSoundName = "Sedan_02_Horn_Short_SoundSet";
		m_CarHornLongSoundName	= "Sedan_02_Horn_SoundSet";
		
		SetEnginePos("0 0.7 -1.7");
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

	override int GetAnimInstance()
	{
		return VehicleAnimInstances.S120;
	}

	override int GetSeatAnimationType(int posIdx)
	{
		switch (posIdx)
		{
		case 0:
			return DayZPlayerConstants.VEHICLESEAT_DRIVER;
		case 1:
			return DayZPlayerConstants.VEHICLESEAT_CODRIVER;
		case 2:
			return DayZPlayerConstants.VEHICLESEAT_PASSENGER_L;
		case 3:
			return DayZPlayerConstants.VEHICLESEAT_PASSENGER_R;
		}

		return 0;
	}

	// Override for car-specific light type
	override CarLightBase CreateFrontLight()
	{
		return CarLightBase.Cast(ScriptedLightBase.CreateLight(Sedan_02FrontLight));
	}
	
	// Override for car-specific light type
	override CarRearLightBase CreateRearLight()
	{
		return CarRearLightBase.Cast(ScriptedLightBase.CreateLight(Sedan_02RearLight));
	}
	
	override bool CanReleaseAttachment( EntityAI attachment )
	{
		if (!super.CanReleaseAttachment(attachment))
		{
			return false;
		}
		
		string attType = attachment.GetType();
		switch (attType)
		{
		case "CarBattery": 
			if (GetCarDoorsState("Sedan_02_Trunk") == CarDoorState.DOORS_CLOSED || EngineIsOn())
			{
				return false;
			}
			break;
		
		case "SparkPlug":
			if (GetCarDoorsState("Sedan_02_Trunk") == CarDoorState.DOORS_CLOSED || EngineIsOn())
			{
				return false;
			}
			break;

		case "CarRadiator":
			if (GetCarDoorsState("Sedan_02_Hood") == CarDoorState.DOORS_CLOSED || EngineIsOn())
			{
				return false;
			}
			break;
		}

		return true;
	}

	override protected bool CanManipulateSpareWheel(string slotSelectionName)
	{
		return GetCarDoorsState("Sedan_02_Hood") != CarDoorState.DOORS_CLOSED;
	}

	override bool CanDisplayAttachmentCategory(string category_name)
	{
		if ( !super.CanDisplayAttachmentCategory(category_name))
		{
			return false;
		}
	
		category_name.ToLower();
		if (category_name.Contains("engine"))
		{
			if (GetCarDoorsState("Sedan_02_Trunk") == CarDoorState.DOORS_CLOSED)
			{
				return false;
			}
		}
		
		if (category_name.Contains("trunk"))
		{
			if (GetCarDoorsState("Sedan_02_Hood") == CarDoorState.DOORS_CLOSED)
			{
				return false;
			}
		}

		return true;
	}
	
	override bool CanDisplayCargo()
	{
		if ( !super.CanDisplayCargo() )
			return false;
		
		if ( GetCarDoorsState("Sedan_02_Hood") == CarDoorState.DOORS_CLOSED )
			return false;
		
		return true;
	}

	override int GetCarDoorsState(string slotType)
	{
		CarDoor carDoor;

		Class.CastTo( carDoor, FindAttachmentBySlotName( slotType ) );
		if (!carDoor)
		{
			return CarDoorState.DOORS_MISSING;
		}
	
		switch (slotType)
		{
		case "Sedan_02_Door_1_1":
			return TranslateAnimationPhaseToCarDoorState("DoorsDriver");
			
		case "Sedan_02_Door_2_1":
			return TranslateAnimationPhaseToCarDoorState("DoorsCoDriver");
		
		case "Sedan_02_Door_1_2":
			return TranslateAnimationPhaseToCarDoorState("DoorsCargo1");
		
		case "Sedan_02_Door_2_2":
			return TranslateAnimationPhaseToCarDoorState("DoorsCargo2");
		
		case "Sedan_02_Hood":
			return TranslateAnimationPhaseToCarDoorState("DoorsHood");

		case "Sedan_02_Trunk":
			return TranslateAnimationPhaseToCarDoorState("DoorsTrunk");
		}

		return CarDoorState.DOORS_MISSING;
	}
	

	override bool CrewCanGetThrough( int posIdx )
	{
		switch( posIdx )
		{
			case 0:
				if ( GetCarDoorsState("Sedan_02_Door_1_1") == CarDoorState.DOORS_CLOSED )
					return false;

				return true;
			break;
			
			case 1:
				if ( GetCarDoorsState("Sedan_02_Door_2_1") == CarDoorState.DOORS_CLOSED )
					return false;

				return true;
			break;

			case 2:
				if ( GetCarDoorsState("Sedan_02_Door_1_2") == CarDoorState.DOORS_CLOSED )
					return false;

				return true;
			break;

			case 3:
				if ( GetCarDoorsState("Sedan_02_Door_2_2") == CarDoorState.DOORS_CLOSED )
					return false;

				return true;
			break;
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
		case 2:
			return "doors_cargo1";
		break;
		case 3:
			return "doors_cargo2";
		break;
		}
		
		return super.GetDoorSelectionNameFromSeatPos(posIdx);
	}

	override string GetDoorInvSlotNameFromSeatPos(int posIdx)
	{
		switch( posIdx )
		{
		case 0:
			return "Sedan_02_Door_1_1";
		break;
		case 1:
			return "Sedan_02_Door_2_1";
		break;
		case 2:
			return "Sedan_02_Door_1_2";
		break;
		case 3:
			return "Sedan_02_Door_2_2";
		break;
		}
		
		return super.GetDoorInvSlotNameFromSeatPos(posIdx);
	}
	
	override float OnSound(CarSoundCtrl ctrl, float oldValue)
	{
		switch ( ctrl )
		{
		case CarSoundCtrl.DOORS:
			float newValue = 0;
			if (GetCarDoorsState("Sedan_02_Door_1_1") == CarDoorState.DOORS_CLOSED)
			{
				newValue += 0.25;
			}

			if (GetCarDoorsState("Sedan_02_Door_2_1") == CarDoorState.DOORS_CLOSED)
			{
				newValue += 0.25;
			}
		
			if (GetCarDoorsState("Sedan_02_Door_1_2") == CarDoorState.DOORS_CLOSED)
			{
				newValue += 0.25;
			}

			if (GetCarDoorsState("Sedan_02_Door_2_2") == CarDoorState.DOORS_CLOSED)
			{
				newValue += 0.25;
			}
			
			return Math.Clamp(newValue, 0, 1);
		break;
		}

		return super.OnSound(ctrl, oldValue);
	}
	
	override string GetAnimSourceFromSelection( string selection )
	{
		switch( selection )
		{
		case "doors_driver":
			return "DoorsDriver";
		case "doors_codriver":
			return "DoorsCoDriver";
		case "doors_cargo1":
			return "DoorsCargo1";
		case "doors_cargo2":
			return "DoorsCargo2";
		case "doors_hood":
			return "DoorsHood";
		case "doors_trunk":
			return "DoorsTrunk";
		}

		return "";
	}


	override bool IsVitalTruckBattery()
	{
		return false;
	}

	override bool IsVitalGlowPlug()
	{
		return false;
	}
	
	override bool CanReachSeatFromSeat(int currentSeat, int nextSeat)
	{
		switch (currentSeat)
		{
			case 0:
				return nextSeat == 1;
	
			case 1:
				return nextSeat == 0;
	
			case 2:
				return nextSeat == 3;
	
			case 3:
				return nextSeat == 2;
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
	
			case 2:
				return pDoorsSelection == "DoorsCargo1";
				
			case 3:
				return pDoorsSelection == "DoorsCargo2";
		}
		
		return false;		
	}

	override void OnDebugSpawn()
	{
		SpawnUniversalParts();
		SpawnAdditionalItems();
		FillUpCarFluids();

		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");

		GetInventory().CreateInInventory("Sedan_02_Door_1_1");
		GetInventory().CreateInInventory("Sedan_02_Door_1_2");
		GetInventory().CreateInInventory("Sedan_02_Door_2_1");
		GetInventory().CreateInInventory("Sedan_02_Door_2_2");
		GetInventory().CreateInInventory("Sedan_02_Hood");
		GetInventory().CreateInInventory("Sedan_02_Trunk");

		//-----IN CAR CARGO
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
	}
}

class Sedan_02_Red extends Sedan_02
{
	override void OnDebugSpawn()
	{
		SpawnUniversalParts();
		SpawnAdditionalItems();
		FillUpCarFluids();

		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");

		GetInventory().CreateInInventory("Sedan_02_Door_1_1_Red");
		GetInventory().CreateInInventory("Sedan_02_Door_1_2_Red");
		GetInventory().CreateInInventory("Sedan_02_Door_2_1_Red");
		GetInventory().CreateInInventory("Sedan_02_Door_2_2_Red");
		GetInventory().CreateInInventory("Sedan_02_Hood_Red");
		GetInventory().CreateInInventory("Sedan_02_Trunk_Red");

		//-----IN CAR CARGO
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
	}
}

class Sedan_02_Grey extends Sedan_02
{
	override void OnDebugSpawn()
	{
		SpawnUniversalParts();
		SpawnAdditionalItems();
		FillUpCarFluids();

		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");

		GetInventory().CreateInInventory("Sedan_02_Door_1_1_Grey");
		GetInventory().CreateInInventory("Sedan_02_Door_1_2_Grey");
		GetInventory().CreateInInventory("Sedan_02_Door_2_1_Grey");
		GetInventory().CreateInInventory("Sedan_02_Door_2_2_Grey");
		GetInventory().CreateInInventory("Sedan_02_Hood_Grey");
		GetInventory().CreateInInventory("Sedan_02_Trunk_Grey");

		//-----IN CAR CARGO
		GetInventory().CreateInInventory("Sedan_02_Wheel");
		GetInventory().CreateInInventory("Sedan_02_Wheel");
	}
}

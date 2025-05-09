class ActionDisinfectPlantCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousDisinfectPlant(UAQuantityConsumed.GARDEN_DISINFECT_PLANT);
	}
};

class ActionDisinfectPlant: ActionContinuousBase
{
	PlantBase m_Plant;
	
	void ActionDisinfectPlant()
	{
		m_CallbackClass = ActionDisinfectPlantCB;
		m_SpecialtyWeight = UASoftSkillsWeight.PRECISE_LOW;
		
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_SPRAYPLANT;
		//m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH;
		m_FullBody = true;
		
		m_Text = "#apply";
	}
	
	override void CreateConditionComponents()  
	{	
		m_ConditionTarget = new CCTNonRuined(UAMaxDistances.DEFAULT);
		m_ConditionItem = new CCINotRuinedAndEmpty;
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		GardenBase garden_base;
		if ( Class.CastTo(garden_base, target.GetObject()))
		{
			Slot slot;
			
			array<string> selections = new array<string>;
			garden_base.GetActionComponentNameList(target.GetComponentIndex(), selections);
			string selection;

			for (int s = 0; s < selections.Count(); s++)
			{
				selection = selections[s];
				slot = garden_base.GetSlotBySelection( selection );
				if (slot)
					break;
			}
			
			if ( slot && slot.GetPlant() )
			{
				m_Plant = PlantBase.Cast(slot.GetPlant());
				if (m_Plant.IsSprayable())
				{	
					if ( m_Plant.GetPlantStateIndex() < 1 )
						return false;
					
					if ( item.GetQuantity() > 0 )
					{
						return true;
					}
				}
			}
		}
		return false;
	}
};
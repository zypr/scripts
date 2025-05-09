class ActionCraftBoneKnifeEnvCB : ActionContinuousBaseCB
{
	private const float TIME_TO_CRAFT_KNIFE = 5;
	
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousRepeat(TIME_TO_CRAFT_KNIFE);
	}
};

class ActionCraftBoneKnifeEnv: ActionContinuousBase
{
	void ActionCraftBoneKnifeEnv()
	{
		
		m_CallbackClass = ActionCraftBoneKnifeEnvCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_CRAFTING;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT;
		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_HIGH;
		m_Text = "#STR_CraftBoneKnife0";
	}
	
	override void CreateConditionComponents()  
	{		
		m_ConditionTarget = new CCTCursor(UAMaxDistances.DEFAULT);
		m_ConditionItem = new CCINonRuined;
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{	
		//Action not allowed if player has broken legs
		if (player.GetBrokenLegs() == eBrokenLegs.BROKEN_LEGS)
			return false;
		
		Object targetObject = target.GetObject();
		if ( targetObject.IsRock() )
		{
			return true;
		}
		return false;
	}
	
	override void OnFinishProgressServer( ActionData action_data )
	{	
		EntityAI item_ingredient = action_data.m_MainItem;
		EntityAI knife = action_data.m_Player.SpawnEntityOnGroundRaycastDispersed("BoneKnife");
		action_data.m_MainItem.AddQuantity(-1);
		
		MiscGameplayFunctions.TransferItemProperties(item_ingredient, knife);

	}
	
	override string GetSoundCategory(ActionData action_data)
	{
		return "BoneKnife_stone";
	}
	
};
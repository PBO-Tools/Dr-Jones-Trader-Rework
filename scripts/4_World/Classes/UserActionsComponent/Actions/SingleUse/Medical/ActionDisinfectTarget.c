modded class ActionDisinfectTarget
{	
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if (player.m_Trader_IsInSafezone)
            	return false;

		return super.ActionCondition(player, target, item);
	}
}
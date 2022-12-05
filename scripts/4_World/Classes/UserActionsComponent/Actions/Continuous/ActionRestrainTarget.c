modded class ActionRestrainTarget
{
    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
        if (player.m_Trader_IsInSafezone)
            return false;

        PlayerBase targetPlayer = PlayerBase.Cast(target.GetObject());
        if (!targetPlayer)
			return false;

        if (targetPlayer.m_Trader_IsInSafezone)
            return false;

		return super.ActionCondition(player, target, item);
	}
}
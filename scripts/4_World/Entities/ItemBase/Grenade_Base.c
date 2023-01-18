modded class Grenade_Base extends InventoryItemSuper
{
    override protected void ExplodeGrenade(EGrenadeType grenade_type)
	{
        if (isInSafezone())
        {
            OnExplode(); // deletes Grenade from Server
            return;
        }
		
        super.ExplodeGrenade(grenade_type);
	}

    bool isInSafezone()
    {
        //PlayerBase player = getRandomValidPlayer();
        PlayerBase player = PlayerBase.Cast(getRandomValidPlayer());


        if (!player)
            return false;

        if (!this)
            return false;

        for (int k = 0; k < player.m_Trader_TraderPositions.Count(); k++)
        {
            vector grenadePos = this.GetPosition();
            vector safezonePos = player.m_Trader_TraderPositions.Get(k);
            float distanceToSafezone = vector.Distance(grenadePos, safezonePos);
            float distanceToSafezoneMax = player.m_Trader_TraderSafezones.Get(k);

            if (distanceToSafezone <= distanceToSafezoneMax)
                return true;
        }

        return false;
    }
    PlayerBase getRandomValidPlayer()
    {
    if( GetGame().IsServer() )
    {
        ref array<Man> m_Players = new array<Man>;
        GetGame().GetWorld().GetPlayerList(m_Players);

        for (int j = 0; j < m_Players.Count(); j++)
        {
            Player player = Player.Cast(m_Players.Get(j));
            PlayerBase playerBase = PlayerBase.Cast(player);
            if (!playerBase)
                continue;

            if (!playerBase.IsAlive())
                continue;

            if (!playerBase.m_Trader_RecievedAllData)
                continue;

            return playerBase;
        }
    }
    else
    {
        return PlayerBase.Cast(GetGame().GetPlayer());
    }

    return null;
}

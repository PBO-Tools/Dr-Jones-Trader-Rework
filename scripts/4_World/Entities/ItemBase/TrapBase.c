modded class TrapBase
{
    override void StartActivate( PlayerBase player )
	{
        if (player)
        {
            if (player.m_Trader_IsInSafezone)
               return;
        }


        super.StartActivate(player);
	}
}
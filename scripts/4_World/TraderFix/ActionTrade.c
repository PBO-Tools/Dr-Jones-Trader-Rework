#ifdef TRADER
modded class ActionTrade: ActionInteractBase
{
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
        if (GetGame().IsServer())
            return true;

		if(!target || !target.GetObject() || !player)
			return false;		

		bool isTraderNPCObject = false;
		if(player.m_Trader_NPCDummyClasses)
		{	
			for ( int i = 0; i < player.m_Trader_NPCDummyClasses.Count(); i++ )
			{
				if (target.GetObject().GetType() == player.m_Trader_NPCDummyClasses.Get(i))
					isTraderNPCObject = true;
			}
		}
		PlayerBase ntarget = PlayerBase.Cast(target.GetObject());
		bool isTraderNPCCharacter = false;
		if(ntarget)
			isTraderNPCCharacter = ntarget.m_Trader_IsTrader;
					
		if (!isTraderNPCCharacter && !isTraderNPCObject)
			return false;

		vector playerPosition = player.GetPosition();

		if (player.m_Trader_RecievedAllData == false)
		{
			return false;
		}

		// only call these after we made sure the client has all trader data loaded!
		int traderUID = getNearbyTraderUID(playerPosition);
		bool canOpenTraderMenu = getCanOpenTraderMenu(playerPosition, traderUID);

		if (!canOpenTraderMenu)
			return false;

        return true;
	}
};
#endif
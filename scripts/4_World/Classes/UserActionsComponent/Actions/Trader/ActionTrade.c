class ActionTrade: ActionInteractBase
{
	void ActionTrade()
	{
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_INTERACTONCE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		m_HUDCursorIcon = CursorIcons.CloseHood;
	}

    override void CreateConditionComponents()  
	{
		m_ConditionTarget = new CCTObject(10);//CCTMan(10);
		m_ConditionItem = new CCINone;
	}

	override string GetText()
	{
		return "Trade";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
    {
        if ( GetGame().IsServer() )
            return true;

		if ( player.m_Trader_RecievedAllData == false )
        {            
            // player.MessageStatus("");                
            return false;
        }
        
        bool isTraderNPCCharacter = false;

        PlayerBase ntarget = PlayerBase.Cast( target.GetObject() );
        
        if ( ntarget )
            isTraderNPCCharacter = ntarget.m_Trader_IsTrader;

        bool isTraderNPCObject = false;
        for ( int i = 0; i < player.m_Trader_NPCDummyClasses.Count(); i++ )
        {
            if ( target.GetObject().GetType() == player.m_Trader_NPCDummyClasses.Get(i) )
                isTraderNPCObject = true;
        }

        if ( !isTraderNPCCharacter && !isTraderNPCObject )
            return false;

        vector playerPosition = player.GetPosition();

        // only call these after we made sure the client has all trader data loaded!
        int traderUID = getNearbyTraderUID(playerPosition);
        bool canOpenTraderMenu = getCanOpenTraderMenu( playerPosition, traderUID );

        if ( !canOpenTraderMenu )
            return false;

        return true;
    }
    
    override void OnStartClient(ActionData action_data)
    {
        handleTraderMenuOpenRequest();
    }

    void handleTraderMenuOpenRequest()
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		vector playerPosition = player.GetPosition();

		if (player.m_Trader_RecievedAllData == false)
		{			
			// player.MessageStatus("");				
			return;
		}

		// only call these after we made sure the client has all trader data loaded!
		int traderUID = getNearbyTraderUID(playerPosition);
		bool canOpenTraderMenu = getCanOpenTraderMenu(playerPosition, traderUID);

		if (!canOpenTraderMenu)
			return;

		openTraderMenu(traderUID);
	}

    bool getCanOpenTraderMenu(vector position, int traderUID)
	{		
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		bool playerIsInSafezoneRange = getIsInSafezoneRange(position);
		
		if (traderUID == -1 && playerIsInSafezoneRange)
		{
			//TraderMessage.PlayerWhite("#tm_no_trader_nearby", player, 5);
			return false;
		}

		if (!playerIsInSafezoneRange)
			return false;	

		return true;
	}

    int getNearbyTraderUID(vector position)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		for ( int traderUID = 0; traderUID < player.m_Trader_TraderPositions.Count(); traderUID++ )
		{
			if (getIsTraderNearby(position, traderUID))
				return traderUID;
		}

		return -1;
	}

    bool getIsInSafezoneRange(vector position)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		for ( int traderUID = 0; traderUID < player.m_Trader_TraderPositions.Count(); traderUID++ )
		{
			if (getDistanceToTrader(position, traderUID) <= player.m_Trader_TraderSafezones.Get(traderUID) || getIsTraderNearby(position, traderUID))
				return true;
		}

		return false;
	}

    int getTraderID(int traderUID)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		return player.m_Trader_TraderIDs.Get(traderUID);
	}

    float getDistanceToTrader(vector position, int traderUID)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		return vector.Distance(position, player.m_Trader_TraderPositions.Get(traderUID));
	}

    bool getIsTraderNearby(vector position, int traderUID)
	{
		return getDistanceToTrader(position, traderUID) <= 1.7;
	}

    vector getTraderVehicleSpawnPosition(int traderUID)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		return player.m_Trader_TraderVehicleSpawns.Get(traderUID);
	}

	vector getTraderVehicleSpawnOrientation(int traderUID)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		return player.m_Trader_TraderVehicleSpawnsOrientation.Get(traderUID);
	}

	void initializeTraderMenu(int traderUID)
	{
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		player.m_TraderMenu = new TraderMenu;
		player.m_TraderMenu.m_TraderID = getTraderID(traderUID);
		player.m_TraderMenu.m_TraderUID = traderUID;
		player.m_TraderMenu.m_TraderVehicleSpawn = getTraderVehicleSpawnPosition(traderUID);
		player.m_TraderMenu.m_TraderVehicleSpawnOrientation = getTraderVehicleSpawnOrientation(traderUID);
		player.m_TraderMenu.m_buySellTime = player.m_Trader_BuySellTimer;
		player.m_TraderMenu.Init();
	}

	void openTraderMenu(int traderUID)
	{
        PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());

		if ( g_Game.GetUIManager().GetMenu() == NULL )
		{					
			initializeTraderMenu(traderUID);

			GetGame().GetUIManager().ShowScriptedMenu( player.m_TraderMenu, NULL );
		}
	}
}
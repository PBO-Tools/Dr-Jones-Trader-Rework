modded class DayZPlayerImplement
{
	static const string m_Trader_ConfigFilePath = "$profile:Trader/TraderConfig.txt";
	static const string m_Trader_ObjectsFilePath = "$profile:Trader/TraderObjects.txt";
	static const string m_Trader_VehiclePartsFilePath = "$profile:Trader/TraderVehicleParts.txt";

	ref TraderNotifications m_Trader_TraderNotifications;
	
	float m_Trader_WelcomeMessageTimer = 25.0;
	float m_Trader_WelcomeMessageHandled = false;
	bool m_Trader_TraderModIsLoaded = false;
	bool m_Trader_TraderModIsLoadedHandled = false;
	
	bool m_Trader_IsInSafezone = false;
	float m_Trader_IsInSafezoneTimeout = 0;
	int m_Trader_InfluenzaEnteredSafeZone;
	
	bool m_Trader_RecievedAllData = false;
	
	string m_Trader_CurrencyName;
	ref array<string> m_Trader_CurrencyClassnames;
	ref array<int> m_Trader_CurrencyValues;
	int m_Player_CurrencyAmount;

	int m_Trader_LastSelledTime = 0;
	int m_Trader_LastBuyedTime = 0;
	
	ref array<string> m_Trader_TraderNames;
	ref array<vector> m_Trader_TraderPositions;
	ref array<int> m_Trader_TraderIDs;
	ref array<int> m_Trader_TraderSafezones;
	ref array<vector> m_Trader_TraderVehicleSpawns;
	ref array<vector> m_Trader_TraderVehicleSpawnsOrientation;
	
	ref array<string> m_Trader_Categorys;
	ref array<int> m_Trader_CategorysTraderKey;
	
	ref array<int> m_Trader_ItemsTraderId;
	ref array<int> m_Trader_ItemsCategoryId;
	ref array<string> m_Trader_ItemsClassnames;
	ref array<int> m_Trader_ItemsQuantity;
	ref array<int> m_Trader_ItemsBuyValue;
	ref array<int> m_Trader_ItemsSellValue;	

	ref array<string> m_Trader_Vehicles;
	ref array<string> m_Trader_VehiclesParts;
	ref array<int> m_Trader_VehiclesPartsVehicleId;

	ref array<string> m_Trader_NPCDummyClasses;

	float m_Trader_BuySellTimer = 0.3;

	string itemDisplayNameClient;
	bool m_Trader_IsSelling;

	string m_Trader_PlayerUID;
	ref array<string> m_Trader_AdminPlayerUIDs;
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// OVERRIDES
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		
		if (GetGame().IsServer())
			handleServerRPCs(sender, rpc_type, ctx);
		else
			handleClientRPCs(sender, rpc_type, ctx);
	}

	override void SetSuicide(bool state)
	{
		super.SetSuicide(state);

		if (state && m_Trader_IsInSafezone && GetGame().IsServer())
			SetAllowDamage(true);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// SERVER RPC HANDLING
	void handleServerRPCs(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		if (rpc_type == TRPCs.RPC_TRADER_MOD_IS_LOADED && !m_Trader_TraderModIsLoaded)
				handleTraderModIsLoadedRPC(sender, rpc_type, ctx);

		if (rpc_type == TRPCs.RPC_BUY)
			handleBuyRPC(sender, rpc_type, ctx);

		if (rpc_type == TRPCs.RPC_SELL)
			handleSellRPC(sender, rpc_type, ctx);
	}

	void handleTraderModIsLoadedRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		Param1<PlayerBase> rp0 = new Param1<PlayerBase>( NULL );
		ctx.Read(rp0);
		
		PlayerBase player = rp0.param1;
		
		m_Trader_TraderModIsLoaded = true;
		
		GetGame().RPCSingleParam(player, TRPCs.RPC_TRADER_MOD_IS_LOADED_CONFIRM, new Param1<PlayerBase>( player ), true, player.GetIdentity());
	}

	void handleBuyRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		Param3<int, int, string> rpb = new Param3<int, int, string>( -1, -1, "" );
		ctx.Read(rpb);

		int traderUID = rpb.param1;
		int itemID = rpb.param2;
		itemDisplayNameClient = rpb.param3;

		m_Trader_IsSelling = false;

		if (GetGame().GetTime() - m_Trader_LastBuyedTime < m_Trader_BuySellTimer * 1000)
			return;
		m_Trader_LastBuyedTime = GetGame().GetTime();

		if (itemID >= m_Trader_ItemsClassnames.Count() || itemID < 0 || traderUID >= m_Trader_TraderPositions.Count() || traderUID < 0)
			return;

		string itemType = m_Trader_ItemsClassnames.Get(itemID);
		int itemQuantity = m_Trader_ItemsQuantity.Get(itemID);
		int itemCosts = m_Trader_ItemsBuyValue.Get(itemID);

		vector playerPosition = this.GetPosition();	

		if (vector.Distance(playerPosition, m_Trader_TraderPositions.Get(traderUID)) > 1.7)
		{
			traderServerLog("tried to access the Trader out of Range! This could be an Hacker!");
			return;
		}

		m_Player_CurrencyAmount = getPlayerCurrencyAmount();

		if (itemCosts < 0)
		{
			TraderMessage.PlayerWhite("#tm_cant_be_bought", PlayerBase.Cast(this));

			return;
		}

		if (m_Player_CurrencyAmount < itemCosts)
		{
			TraderMessage.PlayerWhite("#tm_cant_afford", PlayerBase.Cast(this));
			return;
		}

		int vehicleKeyHash = 0;

		bool isDuplicatingKey = false;
		if (itemQuantity == -7) // is Key duplication
		{
			VehicleKeyBase vehicleKeyinHands = VehicleKeyBase.Cast(this.GetHumanInventory().GetEntityInHands());

			if (!vehicleKeyinHands)
			{
				TraderMessage.PlayerWhite("Put the Key you\nwant to duplicate\nin your Hands!", PlayerBase.Cast(this));
				return;
			}

			isDuplicatingKey = true;
			vehicleKeyHash = vehicleKeyinHands.GetHash();
			itemType = vehicleKeyinHands.GetType();
			itemQuantity = 1;
		}

		bool isLostKey = false;
		if (itemQuantity == -8) // is Lost Key
		{
			array<Transport> foundVehicles = GetVehicleToGetKeyFor(traderUID);

			if (foundVehicles.Count() < 1)
			{
				TraderMessage.PlayerWhite("There is no Vehicle\nin the Spawn Area!\nMake sure you was the last Driver!", PlayerBase.Cast(this));
				return;
			}

			if (foundVehicles.Count() > 1)
			{
				TraderMessage.PlayerWhite("Multiple Vehicles found\nin the Spawn Area!", PlayerBase.Cast(this));
				return;
			}

			CarScript carScript;
			Class.CastTo(carScript, foundVehicles.Get(0));

			vehicleKeyHash = carScript.m_Trader_VehicleKeyHash;

			if (canCreateItemInPlayerInventory("VehicleKeyBase", 1))
			{
				TraderMessage.PlayerWhite(getItemDisplayName("VehicleKey") + "\n " + "#tm_added_to_inventory", PlayerBase.Cast(this));
				vehicleKeyHash = createVehicleKeyInPlayerInventory(vehicleKeyHash);
			}
			else
			{
				TraderMessage.PlayerWhite("#tm_inventory_full" + "\n" + getItemDisplayName("VehicleKey") + "\n" + "#tm_was_placed_on_ground", PlayerBase.Cast(this));
				vehicleKeyHash = spawnVehicleKeyOnGround(vehicleKeyHash);
				GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>(false), true, this.GetIdentity());
			}

			deductPlayerCurrency(itemCosts);
			
			carScript.m_Trader_HasKey = true;
			carScript.m_Trader_VehicleKeyHash = vehicleKeyHash;
			carScript.SynchronizeValues();

			isLostKey = true;
			itemType = "VehicleKeyLost";
			itemQuantity = 1;
		}

		traderServerLog("bought " + getItemDisplayName(itemType) + "(" + itemType + ")");

		if (itemQuantity == -2 || itemQuantity == -6) // Is a Vehicle
		{
			string blockingObject = isVehicleSpawnFree(traderUID);

			if (blockingObject != "FREE")
			{
				TraderMessage.PlayerWhite(getItemDisplayName(blockingObject) + " " + "#tm_way_blocked", PlayerBase.Cast(this));
				return;
			}

			if (itemQuantity == -2)
			{
				if (canCreateItemInPlayerInventory("VehicleKeyBase", 1))
				{
					TraderMessage.PlayerWhite(getItemDisplayName("VehicleKey") + "\n " + "#tm_added_to_inventory", PlayerBase.Cast(this));
					
					vehicleKeyHash = createVehicleKeyInPlayerInventory();
				}
				else
				{
					TraderMessage.PlayerWhite("#tm_inventory_full" + "\n" + getItemDisplayName("VehicleKey") + "\n" + "#tm_was_placed_on_ground", PlayerBase.Cast(this));
										
					vehicleKeyHash = spawnVehicleKeyOnGround();
					
					GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>( false ), true, this.GetIdentity());
				}
				//TraderMessage.PlayerWhite("KeyHash:\n" + vehicleKeyHash, PlayerBase.Cast(this));
			}

			deductPlayerCurrency(itemCosts);

			TraderMessage.PlayerWhite("" + itemDisplayNameClient + "\n" + "#tm_parked_next_to_you", PlayerBase.Cast(this));

			spawnVehicle(traderUID, itemType, vehicleKeyHash);

			GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>( false ), true, this.GetIdentity());
		}
		else if (itemType != "VehicleKeyLost") // Is not a Vehicle
		{		
			deductPlayerCurrency(itemCosts);

			if (canCreateItemInPlayerInventory(itemType, itemQuantity))
			{
				TraderMessage.PlayerWhite("" + itemDisplayNameClient + "\n" + "#tm_added_to_inventory", PlayerBase.Cast(this));
				
				if (isDuplicatingKey)
					createVehicleKeyInPlayerInventory(vehicleKeyHash, itemType);
				else
					createItemInPlayerInventory(itemType, itemQuantity);
			}
			else
			{
				TraderMessage.PlayerWhite("#tm_inventory_full" + "\n " + itemDisplayNameClient + "\n" + "#tm_was_placed_on_ground", PlayerBase.Cast(this));
									
				if (isDuplicatingKey)
					spawnVehicleKeyOnGround(vehicleKeyHash, itemType);
				else			
					spawnItemOnGround(itemType, itemQuantity, playerPosition);
				
				GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>( false ), true, this.GetIdentity());
			}
		}

		//deductPlayerCurrency(itemCosts);
	}

	void handleSellRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		Param3<int, int, string> rps = new Param3<int, int, string>( -1, -1, "" );
		ctx.Read(rps);

		int traderUID = rps.param1;
		int itemID = rps.param2;
		itemDisplayNameClient = rps.param3;

		m_Trader_IsSelling = true;

		if (GetGame().GetTime() - m_Trader_LastSelledTime < m_Trader_BuySellTimer * 1000)
			return;
		m_Trader_LastSelledTime = GetGame().GetTime();

		if (itemID >= m_Trader_ItemsClassnames.Count() || itemID < 0 || traderUID >= m_Trader_TraderPositions.Count() || traderUID < 0)
			return;

		string itemType = m_Trader_ItemsClassnames.Get(itemID);
		int itemQuantity = m_Trader_ItemsQuantity.Get(itemID);
		int itemSellValue = m_Trader_ItemsSellValue.Get(itemID);

		vector playerPosition = this.GetPosition();	

		if (vector.Distance(playerPosition, m_Trader_TraderPositions.Get(traderUID)) > 1.7)
		{
			traderServerLog("tried to access the Trader out of Range! This could be an Hacker!");
			return;
		}


		Object vehicleToSell = GetVehicleToSell(traderUID, itemType);
		bool isValidVehicle = ((itemQuantity == -2 || itemQuantity == -6) && vehicleToSell);

		if (itemSellValue < 0)
		{
			TraderMessage.PlayerWhite("#tm_cant_be_sold", PlayerBase.Cast(this));
			return;
		}

		if (!isInPlayerInventory(itemType, itemQuantity) && !isValidVehicle)
		{
			TraderMessage.PlayerWhite("#tm_you_cant_sell", PlayerBase.Cast(this));

			if (itemQuantity == -2 || itemQuantity == -6)
				TraderMessage.PlayerWhite("#tm_cant_sell_vehicle", PlayerBase.Cast(this));
				//TraderMessage.PlayerWhite("Turn the Engine on and place it inside the Traffic Cones!", PlayerBase.Cast(this));

			return;
		}

		traderServerLog("#tm_sold" + " " + getItemDisplayName(itemType) + " (" + itemType + ")");

		TraderMessage.PlayerWhite("" + itemDisplayNameClient + "\n" + "#tm_was_sold", PlayerBase.Cast(this));

		if (isValidVehicle)
			deleteObject(vehicleToSell);
		else
			removeFromPlayerInventory(itemType, itemQuantity);
		
		increasePlayerCurrency(itemSellValue);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// CLIENT RPC HANDLING
	void handleClientRPCs(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		switch(rpc_type)
		{
			case TRPCs.RPC_SEND_TRADER_CURRENCYNAME_ENTRY:
				handleSendTraderCurrencyNameEntryRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_TRADER_CURRENCY_ENTRY:
				handleSendTraderCurrencyEntryRPC(sender, rpc_type, ctx);
			break;
			
			case TRPCs.RPC_SEND_TRADER_NAME_ENTRY:
				handleSendTraderNameEntryRPC(sender, rpc_type, ctx);
			break;
			
			case TRPCs.RPC_SEND_TRADER_CATEGORY_ENTRY:
				handleSendTraderCategoryEntryRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_TRADER_NPCDUMMY_ENTRY:
				handleSendTraderNPCDummyEntryRPC(sender, rpc_type, ctx);
			break;
			
			case TRPCs.RPC_SEND_TRADER_ITEM_ENTRY:
				handleSendTraderItemEntryRPC(sender, rpc_type, ctx);
			break;
			
			case TRPCs.RPC_SEND_TRADER_MARKER_ENTRY:
				handleSendTraderMarkerEntryRPC(sender, rpc_type, ctx);
			break;
			
			case TRPCs.RPC_SEND_TRADER_DATA_CONFIRMATION:
				handleSendTraderDataConfirmationRPC(sender, rpc_type, ctx);
			break;
			
			case TRPCs.RPC_SEND_TRADER_CLEAR:
				handleSendTraderClearRPC(sender, rpc_type, ctx);
			break;
			
			case TRPCs.RPC_TRADER_MOD_IS_LOADED_CONFIRM:
				handleTraderModIsLoadedConfirmRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_TRADER_IS_IN_SAFEZONE:
				handleSendTraderIsInSafezoneRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SYNC_OBJECT_ORIENTATION:
				handleSyncObjectOrientationRPC(sender, rpc_type, ctx);
			break;

			/*case TRPCs.RPC_SYNC_CARSCRIPT_ISINSAFEZONE:
				handleSyncCarscriptIsInSafezoneRPC(sender, rpc_type, ctx);
			break;*/

			case TRPCs.RPC_SEND_MENU_BACK:
				handleSendMenuBackRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_MESSAGE_WHITE:
				handleSendMessageWhiteRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_MESSAGE_RED:
				handleSendMessageRedRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_MESSAGE_SAFEZONE:
				handleSendMessageSafezoneRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_DELETE_SAFEZONE_MESSAGES:					
				handleDeleteSafezoneMessagesRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_TRADER_VARIABLES_ENTRY:
				handleSendTraderVariablesEntryRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_TRADER_PLAYERUID:
				handleSendTraderPlayerUIDRPC(sender, rpc_type, ctx);
			break;

			case TRPCs.RPC_SEND_TRADER_ADMINS_ENTRY:
				handleSendTraderAdminsEntryRPC(sender, rpc_type, ctx);
			break;
		}
	}

	void handleSendTraderCurrencyNameEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<string> currencyName_rp = new Param1<string>( "" );
		ctx.Read( currencyName_rp );
		
		m_Trader_CurrencyName = currencyName_rp.param1;
	}

	void handleSendTraderCurrencyEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param2<string, int> currency_rp = new Param2<string, int>( "", -1 );
		ctx.Read( currency_rp );
		
		m_Trader_CurrencyClassnames.Insert(currency_rp.param1);
		m_Trader_CurrencyValues.Insert(currency_rp.param2);
	}
	
	void handleSendTraderNameEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<string> tradername_rp = new Param1<string>( "" );
		ctx.Read( tradername_rp );
		
		m_Trader_TraderNames.Insert(tradername_rp.param1);
	}
	
	void handleSendTraderCategoryEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param2<string, int> category_rp = new Param2<string, int>( "", 0 );
		ctx.Read( category_rp );					
		
		m_Trader_Categorys.Insert(category_rp.param1);
		m_Trader_CategorysTraderKey.Insert(category_rp.param2);
	}

	void handleSendTraderNPCDummyEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<string> npcDummy_rp = new Param1<string>( "" );
		ctx.Read( npcDummy_rp );					
		
		m_Trader_NPCDummyClasses.Insert(npcDummy_rp.param1);
	}
	
	void handleSendTraderItemEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		Param6<int, int, string, int, int, int> itemEntry_rp = new Param6<int, int, string, int, int, int>( 0, 0, "", 0, 0, 0);
		ctx.Read( itemEntry_rp );
		
		m_Trader_ItemsTraderId.Insert(itemEntry_rp.param1);
		m_Trader_ItemsCategoryId.Insert(itemEntry_rp.param2);
		m_Trader_ItemsClassnames.Insert(itemEntry_rp.param3);
		m_Trader_ItemsQuantity.Insert(itemEntry_rp.param4);
		m_Trader_ItemsBuyValue.Insert(itemEntry_rp.param5);
		m_Trader_ItemsSellValue.Insert(itemEntry_rp.param6);
	}
	
	void handleSendTraderMarkerEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param5<int, vector, int, vector, vector> markerEntry = new Param5<int, vector, int, vector, vector>( 0, "0 0 0", 0, "0 0 0", "0 0 0" );
		ctx.Read( markerEntry );					
		
		m_Trader_TraderIDs.Insert(markerEntry.param1);
		m_Trader_TraderPositions.Insert(markerEntry.param2);
		m_Trader_TraderSafezones.Insert(markerEntry.param3);
		m_Trader_TraderVehicleSpawns.Insert(markerEntry.param4);
		m_Trader_TraderVehicleSpawnsOrientation.Insert(markerEntry.param5);
	}
	
	void handleSendTraderDataConfirmationRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<bool> conf_rp = new Param1<bool>( false );
		ctx.Read( conf_rp );
		
		m_Trader_RecievedAllData = conf_rp.param1;
	}
	
	void handleSendTraderClearRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		m_Trader_RecievedAllData = false;	
		m_Trader_CurrencyName = "";
		m_Trader_CurrencyClassnames = new array<string>;
		m_Trader_CurrencyValues = new array<int>;
		m_Trader_TraderNames = new array<string>;
		m_Trader_TraderPositions = new array<vector>;
		m_Trader_TraderIDs = new array<int>;
		m_Trader_TraderSafezones = new array<int>;
		m_Trader_TraderVehicleSpawns = new array<vector>;
		m_Trader_TraderVehicleSpawnsOrientation = new array<vector>;
		m_Trader_Categorys = new array<string>;
		m_Trader_CategorysTraderKey = new array<int>;
		m_Trader_ItemsTraderId = new array<int>;
		m_Trader_ItemsCategoryId = new array<int>;
		m_Trader_ItemsClassnames = new array<string>;
		m_Trader_ItemsQuantity = new array<int>;
		m_Trader_ItemsBuyValue = new array<int>;
		m_Trader_ItemsSellValue = new array<int>;
		m_Trader_Vehicles = new array<string>;
		m_Trader_VehiclesParts = new array<string>;
		m_Trader_VehiclesPartsVehicleId = new array<int>;
		m_Trader_PlayerUID = "";
		m_Trader_AdminPlayerUIDs = new array<string>;
		m_Trader_NPCDummyClasses = new array<string>;
	}
	
	void handleTraderModIsLoadedConfirmRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		m_Trader_TraderModIsLoaded = true;
		m_Trader_TraderModIsLoadedHandled = true;
	}

	void handleSendTraderIsInSafezoneRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<bool> safezone_rp = new Param1<bool>( false );
		ctx.Read( safezone_rp );
		
		m_Trader_IsInSafezone = safezone_rp.param1;

		PlayerBase player = PlayerBase.Cast(this);
		if(IsRestrained())
		{
			player.SetRestrained(false);

			EntityAI item_in_hands = GetHumanInventory().GetEntityInHands();
			if(item_in_hands)
			{
				MiscGameplayFunctions.TransformRestrainItem(item_in_hands, null, null, player);
			}
		}

		player.GetInputController().OverrideRaise(m_Trader_IsInSafezone, false);
	}

	void handleSyncObjectOrientationRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param2<Object, vector> syncObject_rp = new Param2<Object, vector>( NULL, "0 0 0" );
		ctx.Read( syncObject_rp );
		
		Object objectToSync = syncObject_rp.param1;
		vector objectToSyncOrientation  = syncObject_rp.param2;

		objectToSync.SetOrientation(objectToSyncOrientation);
	}

	/*void handleSyncCarscriptIsInSafezoneRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param2<CarScript, bool> synccarsscript_rp = new Param2<CarScript, bool>( NULL, false );
		ctx.Read( synccarsscript_rp );
		
		CarScript carToSync = synccarsscript_rp.param1;

		carToSync.m_Trader_IsInSafezone = synccarsscript_rp.param2;
	}*/

	void handleSendMenuBackRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		GetGame().GetUIManager().Back();
	}

	void handleSendMessageWhiteRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param2<string, float> sendMessageWhite_rp = new Param2<string, float>( "", 0 );
		ctx.Read( sendMessageWhite_rp );
		
		showTraderMessage(sendMessageWhite_rp.param1, sendMessageWhite_rp.param2);
	}

	void handleSendMessageRedRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param2<string, float> sendMessageRed_rp = new Param2<string, float>( "", 0 );
		ctx.Read( sendMessageRed_rp );
		
		showTraderMessage(sendMessageRed_rp.param1, sendMessageRed_rp.param2, 0xFFFA6B6B);
	}

	void handleSendMessageSafezoneRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<float> sendMessageSafezone_rp = new Param1<float>( 0 );
		ctx.Read( sendMessageSafezone_rp );
		
		showSafezoneMessage(sendMessageSafezone_rp.param1);
	}

	void handleDeleteSafezoneMessagesRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{					
		deleteAllSafezoneMessages();
	}

	void handleSendTraderVariablesEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<float> traderVariables_rp = new Param1<float>( 0 );
		ctx.Read( traderVariables_rp );
		
		m_Trader_BuySellTimer = traderVariables_rp.param1;
	}

	void handleSendTraderPlayerUIDRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<string> traderPlayerUID_rp = new Param1<string>( "" );
		ctx.Read( traderPlayerUID_rp );
		
		m_Trader_PlayerUID = traderPlayerUID_rp.param1;
	}

	void handleSendTraderAdminsEntryRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		ref Param1<string> traderAdmins_rp = new Param1<string>( "" );
		ctx.Read( traderAdmins_rp );
		
		m_Trader_AdminPlayerUIDs.Insert(traderAdmins_rp.param1);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// LOGS & MESSAGES
	void traderServerLog(string message)
	{
		TraderMessage.ServerLog("[TRADER] Player: (" + this.GetIdentity().GetName() + ") " + this.GetIdentity().GetId() + " " + message);
	}

	void showTraderMessage(string message, float time, int color = 0)
	{
		if (!m_Trader_TraderNotifications)
			m_Trader_TraderNotifications = new ref TraderNotifications();

		m_Trader_TraderNotifications.ShowMessage(message, time, color);
	}

	void showSafezoneMessage(float time)
	{
		if (!m_Trader_TraderNotifications)
			m_Trader_TraderNotifications = new ref TraderNotifications();

		m_Trader_TraderNotifications.ShowSafezoneMessage(time);
	}

	void deleteAllSafezoneMessages()
	{
		if (!m_Trader_TraderNotifications)
			return;

		m_Trader_TraderNotifications.DeleteAllSafezoneMessages();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// ITEM & INVENTORY
	string TrimUntPrefix(string str) // duplicate
	{
		str.Replace("$UNT$", "");
		return str;
	}

	string getItemDisplayName(string itemClassname) // duplicate
	{
		TStringArray itemInfos = new TStringArray;
		
		string cfg = "CfgVehicles " + itemClassname + " displayName";
		string displayName;
		GetGame().ConfigGetText(cfg, displayName);
	
		if (displayName == "")
		{
			cfg = "CfgAmmo " + itemClassname + " displayName";
			GetGame().ConfigGetText(cfg, displayName);
		}
		
		if (displayName == "")
		{
			cfg = "CfgMagazines " + itemClassname + " displayName";
			GetGame().ConfigGetText(cfg, displayName);
		}
		
		if (displayName == "")
		{
			cfg = "cfgWeapons " + itemClassname + " displayName";
			GetGame().ConfigGetText(cfg, displayName);
		}
	
		if (displayName == "")
		{
			cfg = "CfgNonAIVehicles " + itemClassname + " displayName";
			GetGame().ConfigGetText(cfg, displayName);
		}
		
		
		if (displayName != "")
			return TrimUntPrefix(displayName);
		else
			return itemClassname;
	}

	int GetItemMaxQuantity(string itemClassname) // duplicate
	{
		TStringArray searching_in = new TStringArray;
		searching_in.Insert( CFG_MAGAZINESPATH  + " " + itemClassname + " count");
		//searching_in.Insert( CFG_WEAPONSPATH );
		searching_in.Insert( CFG_VEHICLESPATH + " " + itemClassname + " varQuantityMax");

		for ( int s = 0; s < searching_in.Count(); ++s )
		{
			string path = searching_in.Get( s );

			if ( GetGame().ConfigIsExisting( path ) )
			{
				return g_Game.ConfigGetInt( path );
			}
		}

		return 0;
	}

	int getItemAmount(ItemBase item) // duplicate
	{
		Magazine mgzn = Magazine.Cast(item);
				
		int itemAmount = 0;
		if( item.IsMagazine() )
		{
			itemAmount = mgzn.GetAmmoCount();
		}
		else
		{
			itemAmount = QuantityConversions.GetItemQuantity(item);
		}
		
		return itemAmount;
	}

	bool SetItemAmount(ItemBase item, int amount)
	{
		if (!item)
			return false;

		if (amount == -1)
			amount = GetItemMaxQuantity(item.GetType());

		if (amount == -3)
			amount = 0;

		if (amount == -4)
			amount = 0;

		if (amount == -5)
			amount = Math.RandomIntInclusive(GetItemMaxQuantity(item.GetType()) * 0.5, GetItemMaxQuantity(item.GetType()));

		Magazine mgzn = Magazine.Cast(item);
				
		if( item.IsMagazine() )
		{
			if (!mgzn)
				return false;

			mgzn.ServerSetAmmoCount(amount);
		}
		else
		{
			item.SetQuantity(amount);
		}

		return true;
	}

	bool isInPlayerInventory(string itemClassname, int amount) // duplicate
	{
		itemClassname.ToLower();
		
		bool isMagazine = false;
		if (amount == -3)
			isMagazine = true;

		bool isWeapon = false;
		if (amount == -4)
			isWeapon = true;

		bool isSteak = false;
		if (amount == -5)
			isSteak = true;

		array<EntityAI> itemsArray = new array<EntityAI>;		
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		
		//TraderMessage.PlayerWhite("--------------");

		ItemBase item;		
		for (int i = 0; i < itemsArray.Count(); i++)
		{
			Class.CastTo(item, itemsArray.Get(i));
			string itemPlayerClassname = "";

			if (!item)
				continue;

			if (item.IsRuined())
				continue;

			if (isAttached(item))
				continue;

			itemPlayerClassname = item.GetType();
			itemPlayerClassname.ToLower();

			//TraderMessage.PlayerWhite("I: " + itemPlayerClassname + " == " + itemClassname);

			if(itemPlayerClassname == itemClassname && ((getItemAmount(item) >= amount && !isMagazine && !isWeapon && !isSteak) || isMagazine || isWeapon || (isSteak && (getItemAmount(item) >= GetItemMaxQuantity(itemPlayerClassname) * 0.5)))) // && m_Trader_LastSelledItemID != item.GetID())
			{
				return true;
			}
		}
		
		return false;
	}

	array<ItemBase> getMergeableItemFromPlayerInventory(string itemType, int amount, bool absolute = false)
	{
		array<ItemBase> mergableItems = new array<ItemBase>;

		array<EntityAI> itemsArray = new array<EntityAI>;		
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);

		ItemBase itemToCombine = ItemBase.Cast(GetGame().CreateObject(itemType, "0 0 0"));

		if (!itemToCombine)
			return new array<ItemBase>;

		SetItemAmount(itemToCombine, amount);

		ItemBase item;		
		for (int i = 0; i < itemsArray.Count(); i++)
		{
			Class.CastTo(item, itemsArray.Get(i));

			if (!item)
				continue;

			if (item.IsRuined())
				continue;

			if (itemToCombine.GetType() != item.GetType())
				continue;

			if (item.CanBeCombined(itemToCombine) && getItemAmount(item) < GetItemMaxQuantity(item.GetType()))
			{
				amount -= GetItemMaxQuantity(item.GetType()) - getItemAmount(item);
				SetItemAmount(itemToCombine, amount);
				mergableItems.Insert(item);
			}
		}

		GetGame().ObjectDelete(itemToCombine);

		if (absolute && amount > 0)
			return new array<ItemBase>;

		return mergableItems;
	}

	bool canCreateItemInPlayerInventory(string itemType, int amount)
	{
		array<ItemBase> mergeableItems = getMergeableItemFromPlayerInventory(itemType, amount);
		if (mergeableItems.Count() > 0)
			return true;

		EntityAI item = EntityAI.Cast(GetGame().CreateObject(itemType, "0 0 0"));
		if (!item)
			return false;

		SetItemAmount(ItemBase.Cast(item), amount);
		if(this.GetInventory().CanAddEntityToInventory(item))
		{
			GetGame().ObjectDelete(item);
			return true;
		}
		GetGame().ObjectDelete(item);

		EntityAI entityInHands = this.GetHumanInventory().GetEntityInHands();
		if (!entityInHands)
			return true;

		return false;			
	}

	int createVehicleKeyInPlayerInventory(int hash = 0, string classname = "")
	{
		ref array<string> vehicleKeyClasses = {"VehicleKeyRed", "VehicleKeyBlack", "VehicleKeyGrayCyan", "VehicleKeyYellow", "VehicleKeyPurple"};

		if (classname == "")
			classname = vehicleKeyClasses.Get(vehicleKeyClasses.GetRandomIndex());

		EntityAI entity;
		entity = this.GetHumanInventory().CreateInInventory(classname);

		if (!entity)
			return 0;

		VehicleKeyBase vehicleKey;
		Class.CastTo(vehicleKey, entity);

		if (!vehicleKey)
			return 0;

		//GetHive().CharacterSave(this);

		if (hash <= 0)
			hash = vehicleKey.GenerateNewHash();
		else
			hash = vehicleKey.SetNewHash(hash);

		return hash;
	}

	int spawnVehicleKeyOnGround(int hash = 0, string classname = "")
	{
		ref array<string> vehicleKeyClasses = {"VehicleKeyRed", "VehicleKeyBlack", "VehicleKeyGrayCyan", "VehicleKeyYellow", "VehicleKeyPurple"};

		if (classname == "")
			classname = vehicleKeyClasses.Get(vehicleKeyClasses.GetRandomIndex());

		EntityAI entity;
		entity = this.SpawnEntityOnGroundPos(classname, this.GetPosition());

		if (!entity)
			return 0;

		VehicleKeyBase vehicleKey;
		Class.CastTo(vehicleKey, entity);

		if (!vehicleKey)
			return 0;

		//GetHive().CharacterSave(this);

		if (hash <= 0)
			hash = vehicleKey.GenerateNewHash();
		else
			hash = vehicleKey.SetNewHash(hash);

		return hash;
	}

	void createItemInPlayerInventory(string itemType, int amount)
	{		
		EntityAI entity;

		array<ItemBase> mergeableItems = getMergeableItemFromPlayerInventory(itemType, amount);

		bool isMergingPossible = mergeableItems.Count() > 0;
		bool itemsAreLeftOverMerging = getMergeableItemFromPlayerInventory(itemType, amount, true).Count() == 0;

		if (isMergingPossible)
		{
			if (itemsAreLeftOverMerging)
			{
				entity = this.SpawnEntityOnGroundPos(itemType, this.GetPosition());
				
				if (m_Trader_IsSelling)
					TraderMessage.PlayerWhite("#tm_some_currency_on_ground", PlayerBase.Cast(this));
				else
					TraderMessage.PlayerWhite("#tm_some" + " " + itemDisplayNameClient + "\n" + "#tm_were_placed_on_ground", PlayerBase.Cast(this));

				GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>( false ), true, this.GetIdentity());
			}
			else
			{
				entity = this.SpawnEntityOnGroundPos(itemType, this.GetPosition());
			}
		}
		else
		{
			entity = this.GetHumanInventory().CreateInInventory(itemType);
		}

		if (!entity)
			return;

		ItemBase item;
		Class.CastTo(item, entity);

		if (!item)
			return;
		
		SetItemAmount(item, amount);

		for (int i = 0; i < mergeableItems.Count(); i++)
		{
			int mergeQuantity;

			if (getItemAmount(mergeableItems.Get(i)) + getItemAmount(item) <= GetItemMaxQuantity(mergeableItems.Get(i).GetType()))
				mergeQuantity = getItemAmount(item);
			else
				mergeQuantity = GetItemMaxQuantity(mergeableItems.Get(i).GetType()) - getItemAmount(mergeableItems.Get(i));

			//TraderMessage.PlayerWhite("MERGED " + mergeableItems.Get(i).GetType() + "; QTY: " + mergeQuantity, PlayerBase.Cast(this));

			SetItemAmount(mergeableItems.Get(i), getItemAmount(mergeableItems.Get(i)) + mergeQuantity);
			SetItemAmount(item, getItemAmount(item) - mergeQuantity);
			amount -= mergeQuantity;

			if (item)
			{
				if (getItemAmount(item) <= 0 || amount <= 0)
				{
					deleteItem(item);
					this.UpdateInventoryMenu(); // RPC-Call needed?

					return;
				}
			}
		}

		this.UpdateInventoryMenu(); // RPC-Call needed?
	}

	void spawnItemOnGround(string itemType, int amount, vector position)
	{		
		EntityAI entity = this.SpawnEntityOnGroundPos(itemType, position);

		if (!entity)
			return;

		ItemBase item;
		Class.CastTo(item, entity);

		if (!item)
			return;

		SetItemAmount(item, amount);
	}

	bool removeFromPlayerInventory(string itemClassname, int amount)
	{
		itemClassname.ToLower();

		bool isMagazine = false;
		if (amount == -3)
			isMagazine = true;

		bool isWeapon = false;
		if (amount == -4)
			isWeapon = true;

		bool isSteak = false;
		if (amount == -5)
			isSteak = true;
		

		string itemPlayerClassname = "";
		int itemAmount = -1;

		ItemBase item = ItemBase.Cast(this.GetHumanInventory().GetEntityInHands());
		if (item)
		{
			itemPlayerClassname = item.GetType();
			itemPlayerClassname.ToLower();

			if(!isAttached(item) && !item.IsRuined() && itemPlayerClassname == itemClassname && ((getItemAmount(item) >= amount && !isMagazine && !isWeapon && !isSteak) || isMagazine || isWeapon || (isSteak && (getItemAmount(item) >= GetItemMaxQuantity(itemPlayerClassname) * 0.5))))
			{
				itemAmount = getItemAmount(item);
				
				if (itemAmount == amount || isMagazine || isWeapon || isSteak)
				{
					deleteItem(item);
					
					this.UpdateInventoryMenu(); // RPC-Call needed?
					return true;
				}
				else
				{
					SetItemAmount(item, itemAmount - amount);
				
					this.UpdateInventoryMenu(); // RPC-Call needed?
					return true;
				}
			}
		}


		array<EntityAI> itemsArray = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		
		for (int i = 0; i < itemsArray.Count(); i++)
		{
			Class.CastTo(item, itemsArray.Get(i));
			itemPlayerClassname = "";

			if (!item)
				continue;

			if (item.IsRuined())
				continue;

			if (isAttached(item))
				continue;

			itemPlayerClassname = item.GetType();
			itemPlayerClassname.ToLower();

			if(itemPlayerClassname == itemClassname && ((getItemAmount(item) >= amount && !isMagazine && !isWeapon && !isSteak) || isMagazine || isWeapon || (isSteak && (getItemAmount(item) >= GetItemMaxQuantity(itemPlayerClassname) * 0.5))))
			{
				itemAmount = getItemAmount(item);
				
				if (itemAmount == amount || isMagazine || isWeapon || isSteak)
				{
					deleteItem(item);
					
					this.UpdateInventoryMenu(); // RPC-Call needed?
					return true;
				}
				else
				{
					SetItemAmount(item, itemAmount - amount);
				
					this.UpdateInventoryMenu(); // RPC-Call needed?
					return true;
				}
			}
		}
		
		this.UpdateInventoryMenu(); // RPC-Call needed?
		return false;
	}

	void deleteItem(ItemBase item)
	{
		if (item)
			GetGame().ObjectDelete(item);
			//item.Delete();

		//TraderMessage.PlayerWhite("DELETED " + item.GetType() + "; QTY: " + getItemAmount(item), PlayerBase.Cast(this));
	}

	bool isAttached(ItemBase item) // duplicate
	{
		EntityAI parent = item.GetHierarchyParent();

		if (!parent)
			return false;

		if (parent.IsWeapon() || parent.IsMagazine())
			return true;

		return false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// CURRENCY
	int getPlayerCurrencyAmount() // duplicate
	{
		PlayerBase m_Player = PlayerBase.Cast(this);
		
		int currencyAmount = 0;
		
		array<EntityAI> itemsArray = new array<EntityAI>;
		m_Player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);

		ItemBase item;
		
		for (int i = 0; i < itemsArray.Count(); i++)
		{
			Class.CastTo(item, itemsArray.Get(i));

			if (!item)
				continue;

			for (int j = 0; j < m_Player.m_Trader_CurrencyClassnames.Count(); j++)
			{
				if(item.GetType() == m_Player.m_Trader_CurrencyClassnames.Get(j))
				{
					currencyAmount += getItemAmount(item) * m_Player.m_Trader_CurrencyValues.Get(j);
				}
			}
		}
		
		return currencyAmount;
	}

	void increasePlayerCurrency(int currencyAmount)
	{
		if (currencyAmount == 0)
			return;

		EntityAI entity;
		ItemBase item;		
		
		for (int i = m_Trader_CurrencyClassnames.Count() - 1; i < m_Trader_CurrencyClassnames.Count(); i--)
		{
			int itemMaxAmount = GetItemMaxQuantity(m_Trader_CurrencyClassnames.Get(i));

			while (currencyAmount / m_Trader_CurrencyValues.Get(i) > 0)
			{
				if (currencyAmount > itemMaxAmount * m_Trader_CurrencyValues.Get(i))
				{
					if (canCreateItemInPlayerInventory(m_Trader_CurrencyClassnames.Get(i), itemMaxAmount))
					{
						createItemInPlayerInventory(m_Trader_CurrencyClassnames.Get(i), itemMaxAmount);
					}
					else
					{
						TraderMessage.PlayerWhite("#tm_inventory_full" + "\n" + "#tm_your_currency_on_ground", PlayerBase.Cast(this));

						GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>(true), true, this.GetIdentity());

						entity = this.SpawnEntityOnGroundPos(m_Trader_CurrencyClassnames.Get(i), this.GetPosition());						

						Class.CastTo(item, entity);
						SetItemAmount(item, itemMaxAmount);
					}

					currencyAmount -= itemMaxAmount * m_Trader_CurrencyValues.Get(i);
				}
				else
				{		
					if (canCreateItemInPlayerInventory(m_Trader_CurrencyClassnames.Get(i), currencyAmount / m_Trader_CurrencyValues.Get(i)))
					{
						createItemInPlayerInventory(m_Trader_CurrencyClassnames.Get(i), currencyAmount / m_Trader_CurrencyValues.Get(i));
					}
					else
					{		
						TraderMessage.PlayerWhite("#tm_inventory_full" + "\n" + "#tm_your_currency_on_ground", PlayerBase.Cast(this));
						GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>(true), true, this.GetIdentity());

						entity = this.SpawnEntityOnGroundPos(m_Trader_CurrencyClassnames.Get(i), this.GetPosition());	

						Class.CastTo(item, entity);
						SetItemAmount(item, currencyAmount / m_Trader_CurrencyValues.Get(i));		
					}

					currencyAmount -= (currencyAmount / m_Trader_CurrencyValues.Get(i) * m_Trader_CurrencyValues.Get(i));
				}

				if (currencyAmount == 0)
					return;
			}
		}
	}

	void deductPlayerCurrency(int currencyAmount)
	{		
	if (currencyAmount == 0)
		return;

	array<EntityAI> itemsArray = new array<EntityAI>;
	ItemBase item;
	this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
	
	for (int i = 0; i < m_Trader_CurrencyClassnames.Count(); i++)
	{
		for (int j = 0; j < itemsArray.Count(); j++)
		{
			item = ItemBase.Cast(itemsArray.Get(j));
			
			if (!item)
				continue;

			if(item.GetType() == m_Trader_CurrencyClassnames.Get(i))
			{
				int itemAmount = getItemAmount(item);

				if(itemAmount * m_Trader_CurrencyValues.Get(i) > currencyAmount)
				{
					if (currencyAmount >= m_Trader_CurrencyValues.Get(i))
					{
						SetItemAmount(item, itemAmount - (currencyAmount / m_Trader_CurrencyValues.Get(i)));

						this.UpdateInventoryMenu(); // RPC-Call needed?
						
						currencyAmount -= (currencyAmount / m_Trader_CurrencyValues.Get(i)) * m_Trader_CurrencyValues.Get(i);
					}


					if (currencyAmount < m_Trader_CurrencyValues.Get(i))
					{
						exchangeCurrency(item, currencyAmount, m_Trader_CurrencyValues.Get(i));

						return;
					}
				}
				else
				{
					deleteItem(itemsArray.Get(j));
					
					this.UpdateInventoryMenu(); // RPC-Call needed?
					
					currencyAmount -= itemAmount * m_Trader_CurrencyValues.Get(i);
				}
			}
		}
	}
	void exchangeCurrency(ItemBase item, int currencyAmount, int currencyValue)
	{
		if (!item)
			return;

		if (currencyAmount == 0)
			return;

		//TraderMessage.PlayerWhite("EXCHANGE " + item.GetType() + " [" + currencyValue + "] " + currencyAmount, PlayerBase.Cast(this));

		int itemAmount = getItemAmount(item);

		if (itemAmount <= 1)
			deleteItem(item);
		else
			SetItemAmount(item, itemAmount - 1);

		increasePlayerCurrency(currencyValue - currencyAmount);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// VEHICLE
	string isVehicleSpawnFree(int traderUID)
	{
		vector size = "3 5 9";
		array<Object> excluded_objects = new array<Object>;
		array<Object> nearby_objects = new array<Object>;

		GetGame().IsBoxColliding( m_Trader_TraderVehicleSpawns.Get(traderUID), m_Trader_TraderVehicleSpawnsOrientation.Get(traderUID), size, excluded_objects, nearby_objects);
		if (nearby_objects.Count() > 0)
			return nearby_objects.Get(0).GetType();

		return "FREE";
	}

	array<Transport> GetVehicleToGetKeyFor(int traderUID)
	{
		vector size = "3 5 9";
		array<Object> excluded_objects = new array<Object>;
		array<Object> nearby_objects = new array<Object>;

		array<Transport> found_vehicles = new array<Transport>;
		Transport transport;

		if (GetGame().IsBoxColliding(m_Trader_TraderVehicleSpawns.Get(traderUID), m_Trader_TraderVehicleSpawnsOrientation.Get(traderUID), size, excluded_objects, nearby_objects))
		{
			for (int i = 0; i < nearby_objects.Count(); i++)
			{
				transport = NULL;
				Class.CastTo(transport, nearby_objects.Get(i));
				if (transport)
				{
					// Check if Player was last Driver:
					CarScript carsScript = CarScript.Cast(transport);
					if(!carsScript)
						continue;
					
					if(carsScript.m_Trader_LastDriverId != this.GetIdentity().GetId())
						continue;	

					found_vehicles.Insert(transport);
				}
			}
		}

		return found_vehicles;
	}

	Object GetVehicleToSell(int traderUID, string vehicleClassname) // duplicate
	{
		vector size = "3 5 9";
		array<Object> excluded_objects = new array<Object>;
		array<Object> nearby_objects = new array<Object>;

		if (GetGame().IsBoxColliding( m_Trader_TraderVehicleSpawns.Get(traderUID), m_Trader_TraderVehicleSpawnsOrientation.Get(traderUID), size, excluded_objects, nearby_objects))
		{
			for (int i = 0; i < nearby_objects.Count(); i++)
			{
				if (nearby_objects.Get(i).GetType() == vehicleClassname)
				{
					// Check if there is any Player in the Vehicle:
					bool vehicleIsEmpty = true;

					Transport transport;
					Class.CastTo(transport, nearby_objects.Get(i));
					if (transport)
					{
						int crewSize = transport.CrewSize();
						for (int c = 0; c < crewSize; c++)
						{
							if (transport.CrewMember(c))
								vehicleIsEmpty = false;
						}
					}
					else
					{
						continue;
					}

					if (!vehicleIsEmpty)
						continue;

					// Check if Player was last Driver:
					CarScript carsScript = CarScript.Cast(nearby_objects.Get(i));
					if(!carsScript)
						continue;
					
					if(carsScript.m_Trader_LastDriverId != this.GetIdentity().GetId())
						continue;					

					// Check if Engine is running:
					/*Car car;
					Class.CastTo(car, nearby_objects.Get(i));
					if (car && vehicleIsEmpty)
					{
						if (car.EngineIsOn())
							return nearby_objects.Get(i);
					}*/

					//TODO: Check if Vehicle Owner is Player or Vehicle Owner is not set.

					//if (m_Trader_LastSelledItemID == nearby_objects.Get(i).GetID())
					//	continue;

					//m_Trader_LastSelledItemID = nearby_objects.Get(i).GetID();

					return nearby_objects.Get(i);		
				}					
			}
		}

		return NULL;
	}

	void spawnVehicle(int traderUID, string vehicleType, int vehicleKeyHash)
	{
		vector objectPosition = m_Trader_TraderVehicleSpawns.Get(traderUID);
		vector objectDirection = m_Trader_TraderVehicleSpawnsOrientation.Get(traderUID);

		// Get all Players to synchronize Things:
		ref array<Man> m_Players = new array<Man>;
		GetGame().GetWorld().GetPlayerList(m_Players);
		PlayerBase currentPlayer;

		// Spawn:
		Object obj = GetGame().CreateObject( vehicleType, objectPosition, false, false, true );

		obj.SetOrientation(objectDirection);
		obj.SetDirection(obj.GetDirection());

		for (int i = 0; i < m_Players.Count(); i++)
		{
			currentPlayer = PlayerBase.Cast(m_Players.Get(i));

			if ( !currentPlayer )
				continue;

			GetGame().RPCSingleParam(currentPlayer, TRPCs.RPC_SYNC_OBJECT_ORIENTATION, new Param2<Object, vector>( obj, objectDirection ), true, currentPlayer.GetIdentity());
		}
		
		// Attach Parts:
		EntityAI vehicle;
		Class.CastTo(vehicle, obj);

		int vehicleId = -1;
		for (i = 0; i < m_Trader_Vehicles.Count(); i++)
		{
			if (vehicleType == m_Trader_Vehicles.Get(i))
				vehicleId = i;
		}

		for (int j = 0; j < m_Trader_VehiclesParts.Count(); j++)
		{
			if (m_Trader_VehiclesPartsVehicleId.Get(j) == vehicleId)
				vehicle.GetInventory().CreateAttachment(m_Trader_VehiclesParts.Get(j));
		}

		// Try to fill Fuel, Oil, Brakeliquid, Coolantliquid and lock Vehicle:
		Car car;
		Class.CastTo(car, vehicle);
		if (car)
		{
			car.Fill( CarFluid.FUEL, car.GetFluidCapacity( CarFluid.FUEL ));
			car.Fill( CarFluid.OIL, car.GetFluidCapacity( CarFluid.OIL ));
			car.Fill( CarFluid.BRAKE, car.GetFluidCapacity( CarFluid.BRAKE ));
			car.Fill( CarFluid.COOLANT, car.GetFluidCapacity( CarFluid.COOLANT ));

			car.Fill( CarFluid.USER1, car.GetFluidCapacity( CarFluid.USER1 ));
			car.Fill( CarFluid.USER2, car.GetFluidCapacity( CarFluid.USER2 ));
			car.Fill( CarFluid.USER3, car.GetFluidCapacity( CarFluid.USER3 ));
			car.Fill( CarFluid.USER4, car.GetFluidCapacity( CarFluid.USER4 ));

			CarScript carScript;
			if (Class.CastTo(carScript, vehicle))
			{
				carScript.m_Trader_IsInSafezone = true;

				if (vehicleKeyHash != 0)
				{
					carScript.m_Trader_Locked = true;
					carScript.m_Trader_HasKey = true;
					carScript.m_Trader_VehicleKeyHash = vehicleKeyHash;
				}

				carScript.SynchronizeValues();

				car.SetAllowDamage(false);
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// OBJECTS
	void deleteObject(Object obj)
	{
		if (obj)
			GetGame().ObjectDelete(obj);
	}
}
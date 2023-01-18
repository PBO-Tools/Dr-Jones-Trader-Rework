#ifdef TRADER
modded class DayZPlayerImplement
{
	void CreateItemInInventory(string itemType, int amount)
	{
		array<EntityAI> itemsArray = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		string itemLower = itemType;
		itemLower.ToLower();

		int currentAmount = amount;
		ItemBase item;
		Ammunition_Base ammoItem;
		//if item doesn't have count, quantity or it has quantitybar then we should spawn one item instead of trying to stack it
		//if item has quantitybar we should spawn one with full bar? maybe make it an option for stuff like gasoline canister
		bool hasSomeQuant = (TR_ItemHasCount(itemType) || TR_ItemHasQuantity(itemType)) && !TR_HasQuantityBar(itemType) && amount >= 0;		
		int itemHasSpawnedOrStacked = 0;
		//autostacking
		//check if we have any stackable items of the type itemType
		//if we do, then add to each stack until no more stacks found or out of amount
		//we should keep count of how many items we spawned
		if (hasSomeQuant)
		{
			for (int i = 0; i < itemsArray.Count(); i++)
			{
				if (currentAmount <= 0)
					break;
				Class.CastTo(item, itemsArray.Get(i));
				string itemPlayerClassname = "";
				if (item)
				{
					if (item.IsRuined())
						continue;

					itemPlayerClassname = item.GetType();
					itemPlayerClassname.ToLower();
					if (itemLower == itemPlayerClassname && !item.IsFullQuantity() && !item.IsMagazine())
					{
						currentAmount = item.AddQuantityTR(currentAmount);
						itemDisplayNameClient = item.GetDisplayName();
						itemHasSpawnedOrStacked++;
					}
				}

				Class.CastTo(ammoItem, itemsArray.Get(i));
				if (ammoItem)
				{
					if (ammoItem.IsRuined())
						continue;
					itemPlayerClassname = ammoItem.GetType();
					itemPlayerClassname.ToLower();
					if (itemLower == itemPlayerClassname && ammoItem.IsAmmoPile())
					{
						currentAmount = ammoItem.AddQuantityTR(currentAmount);
						itemDisplayNameClient = ammoItem.GetDisplayName();
						itemHasSpawnedOrStacked++;
					}
				}
			}
		}
		else
		{
			currentAmount = 1;
		}

		if (itemHasSpawnedOrStacked > 0 && !itemType.Contains("Ruble"))
			TraderMessage.PlayerWhite("#tm_some" + " " + itemDisplayNameClient + "\n" + "#tm_added_to_inventory", PlayerBase.Cast(this));

		//any leftover or new stacks
		if (currentAmount > 0 || !hasSomeQuant)
		{
			EntityAI newItem = EntityAI.Cast(this.GetInventory().CreateInInventory(itemType));
			if (!newItem)
			{
				for (int j = 0; j < itemsArray.Count(); j++)
				{
					Class.CastTo(item, itemsArray.Get(j));
					if (!item)
						continue;
					newItem = EntityAI.Cast(item.GetInventory().CreateInInventory(itemType)); //CreateEntityInCargo	
					if (newItem)
						break;
				}
			}
			if (newItem)
			{
				TraderMessage.PlayerWhite(newItem.GetDisplayName() + "\n" + "#tm_added_to_inventory", PlayerBase.Cast(this));
			}
			if (!newItem)
			{
				newItem = EntityAI.Cast(GetGame().CreateObjectEx(itemType, GetPosition(), ECE_PLACE_ON_SURFACE));
				TraderMessage.PlayerWhite(newItem.GetDisplayName() + "\n" + "#tm_was_placed_on_ground", PlayerBase.Cast(this));
				//GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>(true), true, this.GetIdentity());
				if (!newItem)
				{
					Error("[TraderFix] Failed to spawn entity "+itemType+" , make sure the classname exists and item can be spawned");
					return;
				}
			}
			
			Magazine newMagItem = Magazine.Cast(newItem);
			Ammunition_Base newammoItem = Ammunition_Base.Cast(newItem);
			if(newMagItem && !newammoItem)					
			{	
				newMagItem.ServerSetAmmoCount(amount);
				currentAmount = 0;
				this.UpdateInventoryMenu();
				return;
			}
			if (hasSomeQuant)
			{
				if (newammoItem)
				{
					currentAmount = newammoItem.SetQuantityTR(currentAmount);
					this.UpdateInventoryMenu();
					return;
				}	
				ItemBase newItemBase;
				if (Class.CastTo(newItemBase, newItem))
				{
					currentAmount = newItemBase.SetQuantityTR(currentAmount);
				}
			}
		}
		this.UpdateInventoryMenu(); // RPC-Call needed?
	}

	void handleBuyRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		Param3<int, int, string> rpb = new Param3<int, int, string>(-1, -1, "");
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
				vehicleKeyHash = createVehicleKeyInPlayerInventory();

			deductPlayerCurrency(itemCosts);

			TraderMessage.PlayerWhite("" + itemDisplayNameClient + "\n" + "#tm_parked_next_to_you", PlayerBase.Cast(this));

			spawnVehicle(traderUID, itemType, vehicleKeyHash);

			GetGame().RPCSingleParam(this, TRPCs.RPC_SEND_MENU_BACK, new Param1<bool>(false), true, this.GetIdentity());
		}
		else if (itemType != "VehicleKeyLost")// Is not a Vehicle
		{
			deductPlayerCurrency(itemCosts);
			if (isDuplicatingKey)
				createVehicleKeyInPlayerInventory(vehicleKeyHash, itemType);
			else
				CreateItemInInventory(itemType, itemQuantity);
		}
	}

	bool isAttached(ItemBase item) // duplicate
	{
		EntityAI parent = item.GetHierarchyParent();

		if (!parent)
			return false;

		if (item.GetInventory().IsAttachment() || item.GetNumberOfItems() > 0)
			return true;

		if (parent.IsWeapon() || parent.IsMagazine())
			return true;

		return false;
	}

	int createVehicleKeyInPlayerInventory(int hash = 0, string classname = "")
	{
		ref array<string> vehicleKeyClasses ={ "VehicleKeyRed", "VehicleKeyBlack", "VehicleKeyGrayCyan", "VehicleKeyYellow", "VehicleKeyPurple" };

		if (classname == "")
			classname = vehicleKeyClasses.Get(vehicleKeyClasses.GetRandomIndex());

		array<EntityAI> itemsArray = new array<EntityAI>;
		this.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		EntityAI entity = EntityAI.Cast(this.GetInventory().CreateInInventory(classname));
		ItemBase item;
		if (!entity)
		{
			for (int j = 0; j < itemsArray.Count(); j++)
			{
				Class.CastTo(item, itemsArray.Get(j));
				if (!item)
					continue;
				entity = EntityAI.Cast(item.GetInventory().CreateInInventory(classname)); //CreateEntityInCargo	
				if (entity)
					break;
			}
		}
		if (entity)
			TraderMessage.PlayerWhite(getItemDisplayName("VehicleKey") + "\n " + "#tm_added_to_inventory", PlayerBase.Cast(this));
		if (!entity)
		{
			entity = EntityAI.Cast(GetGame().CreateObjectEx(classname, GetPosition(), ECE_PLACE_ON_SURFACE));
			TraderMessage.PlayerWhite("#tm_inventory_full" + "\n" + getItemDisplayName("VehicleKey") + "\n" + "#tm_was_placed_on_ground", PlayerBase.Cast(this));
			if (!entity)
			{
				Error("failed to spawn entity "+classname+" , make sure the classname exists and item can be spawned");
				return 0;
			}
		}

		VehicleKeyBase vehicleKey;
		Class.CastTo(vehicleKey, entity);

		if (!vehicleKey)
			return 0;

		if (hash <= 0)
			hash = vehicleKey.GenerateNewHash();
		else
			hash = vehicleKey.SetNewHash(hash);

		return hash;
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
					CreateItemInInventory(m_Trader_CurrencyClassnames.Get(i), itemMaxAmount);
					currencyAmount -= itemMaxAmount * m_Trader_CurrencyValues.Get(i);
				}
				else
				{
					CreateItemInInventory(m_Trader_CurrencyClassnames.Get(i), currencyAmount / m_Trader_CurrencyValues.Get(i));
					currencyAmount -= (currencyAmount / m_Trader_CurrencyValues.Get(i) * m_Trader_CurrencyValues.Get(i));
				}

				if (currencyAmount == 0)
					return;
			}
		}
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
					DeleteItemAndOfferRewardsForAllAttachments(item);
					
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
					DeleteItemAndOfferRewardsForAllAttachments(item);
					
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

	void DeleteItemAndOfferRewardsForAllAttachments(ItemBase item)
	{
		int totalSellValue = 0;
		for ( int i = 0; i < item.GetInventory().AttachmentCount(); i++ )
		{
			EntityAI attachment = item.GetInventory().GetAttachmentFromIndex ( i );
			int itemID = m_Trader_ItemsClassnames.Find(attachment.GetType());
			if ( itemID != -1 )
			{
				int itemQuantity = m_Trader_ItemsQuantity.Get(itemID);
				int itemSellValue = m_Trader_ItemsSellValue.Get(itemID);
				if (itemSellValue < 0)
					continue;
				totalSellValue += itemSellValue;
			}
		}
		increasePlayerCurrency(totalSellValue);
		item.Delete();
	}
};
#endif
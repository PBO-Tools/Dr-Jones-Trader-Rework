class TraderMenu extends UIScriptedMenu
{
    MultilineTextWidget m_InfoBox;
    ButtonWidget m_BtnBuy;
	ButtonWidget m_BtnSell;
	ButtonWidget m_BtnCancel;
	TextListboxWidget m_ListboxItems;
	TextWidget m_Saldo;
	TextWidget m_SaldoValue;
	TextWidget m_TraderName;
	XComboBoxWidget m_XComboboxCategorys;
	ItemPreviewWidget m_ItemPreviewWidget;
	protected EntityAI previewItem;
	MultilineTextWidget m_ItemDescription;
	TextWidget m_ItemWeight;
	float m_UiUpdateTimer = 0;
	float m_UiSellTimer = 0;
	float m_UiBuyTimer = 0;
	float m_buySellTime = 0.3;

	bool m_active = false;
	
	int m_TraderID = -1;
	int m_TraderUID = -1;
	vector m_TraderVehicleSpawn = "0 0 0";
	vector m_TraderVehicleSpawnOrientation = "0 0 0";
	
	int m_Player_CurrencyAmount;
	int m_ColorBuyable;
	int m_ColorTooExpensive;
	int m_CategorysCurrentIndex;

	int m_LastRowIndex = -1;
	int m_LastCategoryCurrentIndex = -1;
	
	bool updateListbox = false;
	
	ref array<string> m_Categorys; // unnoetig ????
	ref array<int> m_CategorysTraderKey;
	ref array<int> m_CategorysKey;
	
	ref array<string> m_ListboxItemsClassnames;
	ref array<int> m_ListboxItemsQuantity;
	ref array<int> m_ListboxItemsBuyValue;
	ref array<int> m_ListboxItemsSellValue;

	ref array<int> m_ItemIDs;
	
	ref TStringArray m_FileContent;

	void TraderMenu()
	{		
		m_Player_CurrencyAmount = 0;
		m_ColorBuyable = 0;
		m_ColorTooExpensive = 1;
	}	
	
	void ~TraderMenu()
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		player.GetInputController().SetDisabled(false);

		if ( previewItem ) 
		{
			GetGame().ObjectDelete( previewItem );
		}
	}

    override Widget Init()
    {
		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "TM/Trader/scripts/layouts/TraderMenu.layout" );

        m_BtnBuy = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "btn_buy" ) );
		m_BtnSell = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "btn_sell" ) );
		m_BtnCancel = ButtonWidget.Cast( layoutRoot.FindAnyWidget( "btn_cancel" ) );
		m_ListboxItems = TextListboxWidget.Cast(layoutRoot.FindAnyWidget("txtlist_items") );
		m_Saldo = TextWidget.Cast(layoutRoot.FindAnyWidget("text_saldo") );
		m_SaldoValue = TextWidget.Cast(layoutRoot.FindAnyWidget("text_saldoValue") );
		m_TraderName = TextWidget.Cast(layoutRoot.FindAnyWidget("title_text") );
		m_XComboboxCategorys = XComboBoxWidget.Cast( layoutRoot.FindAnyWidget( "xcombobox_categorys" ) );
		m_ItemDescription = MultilineTextWidget.Cast( layoutRoot.FindAnyWidget( "ItemDescWidget" ) );
		m_ItemWeight = TextWidget.Cast(layoutRoot.FindAnyWidget("ItemWeight") );

		m_active = true;
		
		m_Categorys = new array<string>;
		m_CategorysTraderKey = new array<int>;
		m_CategorysKey = new array<int>;
        m_ListboxItemsClassnames = new array<string>;
		m_ListboxItemsQuantity = new array<int>;
		m_ListboxItemsBuyValue = new array<int>;
		m_ListboxItemsSellValue = new array<int>;
		m_ItemIDs = new array<int>;
		
		LoadFileValues();
		m_CategorysCurrentIndex = 0;
		
		updateItemListboxContent();		
		m_ListboxItems.SelectRow(0);

		updatePlayerCurrencyAmount();
		updateItemListboxColors();
		
        return layoutRoot;
    }
	
	override void Update(float timeslice)
	{
		super.Update(timeslice);

		if (m_UiSellTimer > 0)
			m_UiSellTimer -= timeslice;

		if (m_UiBuyTimer > 0)
			m_UiBuyTimer -= timeslice;

		
		if (m_UiUpdateTimer >= 0.05)
		{
			updatePlayerCurrencyAmount();				
			updateItemListboxColors();

			local int row_index = m_ListboxItems.GetSelectedRow();
			if ((m_LastRowIndex != row_index) || (m_LastCategoryCurrentIndex != m_CategorysCurrentIndex))
			{
				m_LastRowIndex = row_index;
				m_LastCategoryCurrentIndex = m_CategorysCurrentIndex;

				string itemType = m_ListboxItemsClassnames.Get(row_index);
				updateItemPreview(itemType);
			}

			PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
			float playerDistanceToTrader = vector.Distance(player.GetPosition(), player.m_Trader_TraderPositions.Get(m_TraderUID));
			if (playerDistanceToTrader > 1.7)
				GetGame().GetUIManager().Back();

			m_UiUpdateTimer = 0;
		}
		else
		{
			m_UiUpdateTimer = m_UiUpdateTimer + timeslice;
		}

		if (!m_active)
			GetGame().GetUIManager().Back();
	}

	override void OnShow()
	{
		super.OnShow();

		PPEffects.SetBlurMenu(0.5);

		GetGame().GetInput().ChangeGameFocus(1);

		SetFocus( layoutRoot );
	}

	override void OnHide()
	{
		super.OnHide();

		PPEffects.SetBlurMenu(0);

		GetGame().GetInput().ResetGameFocus();

		if ( previewItem ) 
		{
			GetGame().ObjectDelete( previewItem );
		}

		Close();
	}

	override bool OnClick( Widget w, int x, int y, int button )
	{
		super.OnClick(w, x, y, button);

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		
		local int row_index = m_ListboxItems.GetSelectedRow();
		string itemType = m_ListboxItemsClassnames.Get(row_index);
		int itemQuantity = m_ListboxItemsQuantity.Get(row_index);
		
		if ( w == m_BtnBuy )
		{
			if(!previewItem)
			{
				TraderMessage.PlayerWhite("Item dosent exist!\nWrong Classname?", m_Player);
				return true;
			}

			if (m_UiBuyTimer > 0)
			{
				TraderMessage.PlayerWhite("#tm_not_that_fast", m_Player);
				return true;
			}
			m_UiBuyTimer = m_buySellTime;

			GetGame().RPCSingleParam(m_Player, TRPCs.RPC_BUY, new Param3<int, int, string>( m_TraderUID, m_ItemIDs.Get(row_index), getItemDisplayName(m_ListboxItemsClassnames.Get(row_index))), true);
			
			return true;
		}
		
		if ( w == m_BtnSell )
		{
			if(!previewItem)
			{
				TraderMessage.PlayerWhite("Item dosent exist!\nWrong Classname?", m_Player);
				return true;
			}
			
			if (m_UiSellTimer > 0)
			{
				TraderMessage.PlayerWhite("#tm_not_that_fast", m_Player);
				return true;
			}
			m_UiSellTimer = m_buySellTime;

			GetGame().RPCSingleParam(m_Player, TRPCs.RPC_SELL, new Param3<int, int, string>( m_TraderUID, m_ItemIDs.Get(row_index), getItemDisplayName(m_ListboxItemsClassnames.Get(row_index))), true);
			
			return true;
		}
		
		if ( w == m_BtnCancel )
		{
			GetGame().GetUIManager().Back();

			return true;
		}
		
		if (w == m_XComboboxCategorys)
		{
			if (updateListbox)
			{
				updateListbox = false;
				
				updateItemListboxContent();
				m_ListboxItems.SelectRow(0);

				updatePlayerCurrencyAmount();
				updateItemListboxColors();
			}
		}

		return false;
	}
	
	override bool OnChange( Widget w, int x, int y, bool finished )
	{
		super.OnChange(w, x, y, finished);
		if (!finished) return false;
		
		m_CategorysCurrentIndex = m_XComboboxCategorys.GetCurrentItem();
		updateListbox = true;
		
		return false;
	}
	
	void updateItemListboxContent()
	{		
		//------------------------------------------------------
		
		LoadItemsFromFile();
		
		//------------------------------------------------------
		
		m_ListboxItems.ClearItems();
		
		for ( int i = 0; i < m_ListboxItemsClassnames.Count(); i++ )
		{
			m_ListboxItems.AddItem(getItemDisplayName(m_ListboxItemsClassnames.Get(i)), NULL, 0 );	
			m_ListboxItems.SetItem( i, "" + m_ListboxItemsBuyValue.Get(i), 				NULL, 1 );
			m_ListboxItems.SetItem( i, "" + m_ListboxItemsSellValue.Get(i), 			NULL, 2 );
		}
	}
	
	void updateItemListboxColors()
	{
		for (int i = 0; i < m_ListboxItems.GetNumItems(); i++)
		{
			int itemCosts = m_ListboxItemsBuyValue.Get(i);
			
			if (itemCosts < 0)
			{
				m_ListboxItems.SetItemColor(i, 1, ARGBF(0, 1, 1, 1) );
			}
			else if (m_Player_CurrencyAmount >= itemCosts)
			{
				m_ListboxItems.SetItemColor(i, 1, ARGBF(1, 1, 1, 1) );
			}
			else
			{
				m_ListboxItems.SetItemColor(i, 1, ARGBF(1, 1, 0, 0) );
			}
			
			string itemClassname = m_ListboxItemsClassnames.Get(i);
			int itemQuantity = m_ListboxItemsQuantity.Get(i);
			
			if (m_ListboxItemsSellValue.Get(i) < 0)
			{
				m_ListboxItems.SetItemColor(i, 2, ARGBF(0, 1, 1, 1) );
			}
			else if (isInPlayerInventory(itemClassname, itemQuantity) || (itemQuantity == -2 && GetVehicleToSell(itemClassname)))
			{
				m_ListboxItems.SetItemColor(i, 2, ARGBF(1, 0, 1, 0) );
			}
			else
			{
				m_ListboxItems.SetItemColor(i, 2, ARGBF(1, 1, 1, 1) );
			}

			EntityAI entityInHands = g_Game.GetPlayer().GetHumanInventory().GetEntityInHands();
			if (entityInHands)
			{
				if (IsAttached(entityInHands, itemClassname))
					m_ListboxItems.SetItemColor(i, 0, ARGBF(1, 0.4, 0.4, 1) );
				else if (IsAttachment(entityInHands, itemClassname))
					m_ListboxItems.SetItemColor(i, 0, ARGBF(1, 0.7, 0.7, 1) );
				else
					m_ListboxItems.SetItemColor(i, 0, ARGBF(1, 1, 1, 1) );
			}
		}
	}

	void updateItemPreview(string itemType)
	{
		if ( !m_ItemPreviewWidget )
			{
				Widget preview_frame = layoutRoot.FindAnyWidget("ItemFrameWidget");

				if ( preview_frame ) 
				{
					float width;
					float height;
					preview_frame.GetSize(width, height);

					m_ItemPreviewWidget = ItemPreviewWidget.Cast( GetGame().GetWorkspace().CreateWidget(ItemPreviewWidgetTypeID, 0, 0, 1, 1, WidgetFlags.VISIBLE, ARGB(255, 255, 255, 255), 10, preview_frame) );
				}
			}

			if ( previewItem )
				GetGame().ObjectDelete( previewItem );

			previewItem = GetGame().CreateObject( itemType, "0 0 0", true, false, true );

			m_ItemPreviewWidget.SetItem( previewItem );
			m_ItemPreviewWidget.SetModelPosition( Vector(0,0,0.5) );

			float itemx, itemy;		
			m_ItemPreviewWidget.GetPos(itemx, itemy);

			m_ItemPreviewWidget.SetSize( 1.5, 1.5 );

			// align to center 
			m_ItemPreviewWidget.SetPos( -0.225, -0.225 );

			// update Item Description:
			InventoryItem iItem = InventoryItem.Cast( previewItem );

			if (iItem)
			{
				m_ItemWeight.SetText(GetItemWeightText());
				m_ItemDescription.SetText(TrimUntPrefix(iItem.GetTooltip()));
			}
			else
			{
				m_ItemWeight.SetText("");
				m_ItemDescription.SetText("...");
			}
	}

	string GetItemWeightText()
	{
		ItemBase item_IB = ItemBase.Cast( previewItem );

		int weight = item_IB.GetSingleInventoryItemWeight();
		//string weightStr = "";
		
		if (weight >= 1000)
		{
			int kilos = Math.Round(weight / 1000.0);
			return  "#inv_inspect_about" + " " + kilos.ToString() + " " + "#inv_inspect_kg";
		}
		else if (weight >= 500)
		{
			return "#inv_inspect_under_1";
		} 
		else if (weight >= 250)
		{
			return "#inv_inspect_under_05";
		}
		else 
		{
			return "#inv_inspect_under_025";
		}

		return "ERROR";
	}
	
	void updatePlayerCurrencyAmount()
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());

		m_Player_CurrencyAmount = 0;
		m_Player_CurrencyAmount = getPlayerCurrencyAmount();
		m_SaldoValue.SetText(" " + m_Player_CurrencyAmount);
	}
	
	int getPlayerCurrencyAmount() // duplicate
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		
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
	
	bool isInPlayerInventory(string itemClassname, int amount) // duplicate
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
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
		m_Player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		
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

			if(itemPlayerClassname == itemClassname && ((getItemAmount(item) >= amount && !isMagazine && !isWeapon && !isSteak) || isMagazine || isWeapon || (isSteak && (getItemAmount(item) >= GetItemMaxQuantity(itemPlayerClassname) * 0.5))))
			{
				return true;
			}
		}
		
		return false;
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

	Object GetVehicleToSell(string vehicleClassname) // duplicate
	{
		vector size = "3 5 9";
		array<Object> excluded_objects = new array<Object>;
		array<Object> nearby_objects = new array<Object>;

		if (GetGame().IsBoxColliding( m_TraderVehicleSpawn, m_TraderVehicleSpawnOrientation, size, excluded_objects, nearby_objects))
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

					// Check if Engine is running:
					/*Car car;
					Class.CastTo(car, nearby_objects.Get(i));
					if (car && vehicleIsEmpty)
					{
						if (car.EngineIsOn())
							return nearby_objects.Get(i);
					}*/

					return nearby_objects.Get(i);		
				}					
			}
		}

		return NULL;
	}

	bool IsAttached(EntityAI parentEntity, string attachmentClassname)
	{
		for ( int i = 0; i < parentEntity.GetInventory().AttachmentCount(); i++ )
		{
			EntityAI attachment = parentEntity.GetInventory().GetAttachmentFromIndex ( i );
			if ( attachment.IsKindOf ( attachmentClassname ) )
				return true;
		}

		return false;
	}

	bool isAttached(ItemBase item) // duplicate
	{
		EntityAI parent = item.GetHierarchyParent();

		if (!parent)
			return false;

		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());

		if (parent.IsWeapon() || parent.IsMagazine())
			return true;

		return false;
	}

	bool IsAttachment(EntityAI parentEntity, string attachmentClassname)
	{
		// Check chamberable Ammunitions
		string type_name = parentEntity.GetType();
		TStringArray cfg_chamberables = new TStringArray;

		GetGame().ConfigGetTextArray(CFG_WEAPONSPATH + " " + type_name + " chamberableFrom", cfg_chamberables);

		for (int i = 0; i < cfg_chamberables.Count(); i++)
		{
			if (attachmentClassname == cfg_chamberables[i])
				return true;
		}


		// Check Magazines
		TStringArray cfg_magazines = new TStringArray;

		GetGame().ConfigGetTextArray(CFG_WEAPONSPATH + " " + type_name + " magazines", cfg_magazines);

		for (i = 0; i < cfg_magazines.Count(); i++)
		{
			if (attachmentClassname == cfg_magazines[i])
				return true;
		}
		return false;

	bool LoadFileValues()
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		
		m_TraderName.SetText(m_Player.m_Trader_TraderNames.Get(m_TraderID));
		m_Saldo.SetText(m_Player.m_Trader_CurrencyName + ": ");

		m_XComboboxCategorys.ClearAll();
		m_Categorys = new array<string>;
		m_CategorysTraderKey = new array<int>;
		m_CategorysKey = new array<int>;
		
		for ( int i = 0; i < m_Player.m_Trader_Categorys.Count(); i++ )
		{
			if (m_TraderID != m_Player.m_Trader_CategorysTraderKey.Get(i))
				continue;
			
			m_XComboboxCategorys.AddItem(m_Player.m_Trader_Categorys.Get(i));
			m_Categorys.Insert(m_Player.m_Trader_Categorys.Get(i));
			m_CategorysTraderKey.Insert(m_Player.m_Trader_CategorysTraderKey.Get(i));
			m_CategorysKey.Insert(i);
		}
		
		return true;
	}
	
	bool LoadItemsFromFile()
	{
		PlayerBase player = PlayerBase.Cast(g_Game.GetPlayer());
		
		m_ListboxItemsClassnames = new array<string>;
		m_ListboxItemsQuantity = new array<int>;
		m_ListboxItemsBuyValue = new array<int>;
		m_ListboxItemsSellValue = new array<int>;
		m_ItemIDs = new array<int>;
			
		for ( int i = 0; i < m_Player.m_Trader_ItemsClassnames.Count(); i++ )
		{			
			if ( m_Player.m_Trader_ItemsCategoryId.Get(i) == m_CategorysKey.Get(m_CategorysCurrentIndex) )
			{
				m_ListboxItemsClassnames.Insert(m_Player.m_Trader_ItemsClassnames.Get(i));
				m_ListboxItemsQuantity.Insert(m_Player.m_Trader_ItemsQuantity.Get(i));
				m_ListboxItemsBuyValue.Insert(m_Player.m_Trader_ItemsBuyValue.Get(i));
				m_ListboxItemsSellValue.Insert(m_Player.m_Trader_ItemsSellValue.Get(i));
				m_ItemIDs.Insert(i);
			}
		}
		
		return true;
	}

	string TrimUntPrefix(string str) // duplicate
	{
		str.Replace("$UNT$", "");
		return str;
	}
}
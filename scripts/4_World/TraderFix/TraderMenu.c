#ifdef TRADER
class TraderItem
{
    string ClassName;
	int BuyValue;
	int SellValue;
	int Quantity;
	int IndexId;
};

modded class TraderMenu extends UIScriptedMenu
{
    private int                 m_PreviewWidgetRotationX;
	private int                 m_PreviewWidgetRotationY;
	private bool                m_SellablesOnly = false;
	private vector              m_PreviewWidgetOrientation;	
	private int 				m_characterScaleDelta;
	TextWidget 					m_ItemQuantity;
    private string              m_SearchFilter = "";
    private string              m_OldSearchFilter = "";
    private EditBoxWidget		m_SearchBox; 
    private CheckBoxWidget		m_SellablesCheckbox; 
	ref array<ref TraderItem> m_FilteredListOfTraderItems;
	ref array<ref TraderItem> m_ListOfCategoryTraderItems;
	ref array<ref TraderItem> m_ListOfTraderItems;
	string m_QuantString = "Quantity: ";

    EntityAI previewItemKit;

	override Widget Init()
    {
		layoutRoot = GetGame().GetWorkspace().CreateWidgets( "TM/Trader/Layout/TraderMenu.layout" );

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
		m_ItemQuantity = TextWidget.Cast(layoutRoot.FindAnyWidget("ItemQuantity"));
		m_SearchBox    = EditBoxWidget.Cast( layoutRoot.FindAnyWidget( "SearchBox" ) );
		m_SellablesCheckbox = CheckBoxWidget.Cast(layoutRoot.FindAnyWidget( "SellablesCheckBox" ) );
		m_SellablesCheckbox.SetChecked(false);
		m_active = true;
		
		m_Categorys = new array<string>;
		m_CategorysTraderKey = new array<int>;
		m_CategorysKey = new array<int>;
        m_ListboxItemsClassnames = new array<string>;		
        m_FilteredListOfTraderItems = new ref array<ref TraderItem>;
        m_ListOfCategoryTraderItems = new ref array<ref TraderItem>;
        m_ListOfTraderItems = new ref array<ref TraderItem>;
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
	
	override void OnShow()
	{
		PPEffects.SetBlurMenu(0.5);
		GetGame().GetInput().ChangeGameFocus(1);
		SetFocus( layoutRoot );		
        GetGame().GetMission().PlayerControlDisable(INPUT_EXCLUDE_ALL);
        GetGame().GetUIManager().ShowUICursor( true );
	}
	
	override void OnHide()
	{
		PPEffects.SetBlurMenu(0);

		GetGame().GetInput().ResetGameFocus();

		if ( previewItem ) 
		{
			GetGame().ObjectDelete( previewItem );
		}
		GetGame().GetMission().PlayerControlEnable(false);
        GetGame().GetUIManager().ShowUICursor( false );

		Close();
	}
    
	override bool OnClick( Widget w, int x, int y, int button )
	{
		PlayerBase m_Player = g_Game.GetPlayer();

		if ( w == m_SellablesCheckbox )
		{
			m_SellablesCheckbox.SetChecked( !m_SellablesOnly );
			m_SellablesOnly = !m_SellablesOnly;
			SearchForItems();
		}

		local int row_index = m_ListboxItems.GetSelectedRow();
		if(!m_FilteredListOfTraderItems.Get(row_index))
			return false;

		string itemType = m_FilteredListOfTraderItems.Get(row_index).ClassName;
		int itemQuantity = m_FilteredListOfTraderItems.Get(row_index).Quantity;
		
		if ( w == m_BtnBuy )
		{
            if(!previewItem)                
			{
				TraderMessage.PlayerWhite("You cannot buy this item. Item doesn't exist.", m_Player);
				return true;
			}
			if (m_UiBuyTimer > 0)
			{
				TraderMessage.PlayerWhite("#tm_not_that_fast", m_Player);
				return true;
			}
			m_UiBuyTimer = m_buySellTime;

			GetGame().RPCSingleParam(m_Player, TRPCs.RPC_BUY, new Param3<int, int, string>( m_TraderUID, m_FilteredListOfTraderItems.Get(row_index).IndexId, getItemDisplayName(itemType)), true);
			
			return true;
		}
		
		if ( w == m_BtnSell )
		{
			if (m_UiSellTimer > 0)
			{
				TraderMessage.PlayerWhite("#tm_not_that_fast", m_Player);
				return true;
			}
			m_UiSellTimer = m_buySellTime;

			GetGame().RPCSingleParam(m_Player, TRPCs.RPC_SELL, new Param3<int, int, string>( m_TraderUID, m_FilteredListOfTraderItems.Get(row_index).IndexId, getItemDisplayName(itemType)), true);
			
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

	override void updateItemListboxColors()
	{
		for (int i = 0; i < m_ListboxItems.GetNumItems(); i++)
		{
			int itemCosts = m_FilteredListOfTraderItems.Get(i).BuyValue;
			
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
			
			string itemClassname = m_FilteredListOfTraderItems.Get(i).ClassName;
			int itemQuantity = m_FilteredListOfTraderItems.Get(i).Quantity;
			
			if (m_FilteredListOfTraderItems.Get(i).SellValue < 0)
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
			string lower = itemType;
			int leng = -1;
			string itemName = "";
            lower.ToLower();
			if(KitIgnoreArray.Find(itemType) == -1 && lower.Contains("kit_"))
            {
                itemName = itemType.Substring(4,itemType.Length());  
                previewItemKit = GetGame().CreateObject( itemName, "0 0 0", true, false, true );
                m_ItemPreviewWidget.SetItem( previewItemKit );
            }
            else if(KitIgnoreArray.Find(itemType) == -1 && lower.Contains("_kit"))
            {
                leng = itemType.Length() - 4;
                itemName = itemType.Substring(0,leng);
				if(lower.Contains("md_camonetshelter"))
					itemName = "Land_" + itemName;
                previewItemKit = GetGame().CreateObject( itemName, "0 0 0", true, false, true );
                m_ItemPreviewWidget.SetItem( previewItemKit );
            }
			else if(KitIgnoreArray.Find(itemType) == -1 && lower.Contains("kit"))
            {
                leng = itemType.Length() - 3;      
                itemName = itemType.Substring(0,leng);  
                previewItemKit = GetGame().CreateObject( itemName, "0 0 0", true, false, true );
                m_ItemPreviewWidget.SetItem( previewItemKit );
            }
            else
            {
                m_ItemPreviewWidget.SetItem( previewItem );
            }
            
			m_ItemPreviewWidget.SetModelPosition( Vector(1.0,1.0,0.5) );
			m_ItemPreviewWidget.SetModelOrientation( Vector(0,0,0) );

			float itemx, itemy;		
			m_ItemPreviewWidget.GetPos(itemx, itemy);
			m_ItemPreviewWidget.SetSize( 1.0, 1.0 );
			m_ItemPreviewWidget.SetPos( 0, 0 );

			if (previewItem)
			{
				local int row_index = m_ListboxItems.GetSelectedRow();
				int itemQuantity = m_FilteredListOfTraderItems.Get(row_index).Quantity;
				string itemQuant = m_QuantString + "1";				
				if(itemQuantity > 0)
					itemQuant = m_QuantString + itemQuantity.ToString();
				ItemBase itemBaseItem = ItemBase.Cast(previewItem);
				if(itemBaseItem && itemBaseItem.HasQuantityBar())			
					itemQuant = m_QuantString + "1";
				m_ItemWeight.SetText(GetItemWeightText());
				m_ItemQuantity.SetText(itemQuant);
                if(KitIgnoreArray.Find(itemType) == -1 && lower.Contains("kit") && previewItemKit)
                {
				    m_ItemDescription.SetText(string.Format("This item is a kit. %1",TrimUntPrefix(GetEntityAITooltip(previewItemKit))));
                }
                else
                {
				    m_ItemDescription.SetText(TrimUntPrefix(GetEntityAITooltip(previewItem)));
                }
			}
			else
			{
				m_ItemWeight.SetText("");
				m_ItemDescription.SetText("ERROR FINDING ITEM. DO NOT BUY THIS ITEM.");
				m_ItemQuantity.SetText("");
			}
	}

	override void updateItemListboxContent()
	{		
		LoadItemsFromFile();	
		SearchForItems();
	}

	void ResetMenu()
	{
		if(previewItemKit)
			previewItemKit.Delete();
		if(previewItem)
			previewItem.Delete();
		m_ItemWeight.SetText("");
		m_ItemQuantity.SetText("");
		m_ItemDescription.SetText("");
	}

	override void Update(float timeslice)
	{
		if (m_UiSellTimer > 0)
			m_UiSellTimer -= timeslice;

		if (m_UiBuyTimer > 0)
			m_UiBuyTimer -= timeslice;

		if (m_UiUpdateTimer >= 0.05)
		{			           
			m_SearchFilter = m_SearchBox.GetText();
			if(m_SearchFilter != m_OldSearchFilter)
				SearchForItems();


			updatePlayerCurrencyAmount();				
			updateItemListboxColors();

			local int row_index = m_ListboxItems.GetSelectedRow();
			if ((m_LastRowIndex != row_index) || (m_LastCategoryCurrentIndex != m_CategorysCurrentIndex))
			{
				m_LastRowIndex = row_index;
				m_LastCategoryCurrentIndex = m_CategorysCurrentIndex;
				ResetMenu();
				if(!m_FilteredListOfTraderItems.Get(row_index))
				{
					m_UiUpdateTimer = 0;
					return;
				}

				string itemType = m_FilteredListOfTraderItems.Get(row_index).ClassName;
				updateItemPreview(itemType);
			}

			PlayerBase player = g_Game.GetPlayer();
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

	override bool LoadItemsFromFile()
	{
		PlayerBase m_Player = g_Game.GetPlayer();

		m_ListOfTraderItems.Clear();
		m_ListOfCategoryTraderItems.Clear();
		m_FilteredListOfTraderItems.Clear();

		for ( int i = 0; i < m_Player.m_Trader_ItemsClassnames.Count(); i++ )
		{	
			if(m_Player.m_Trader_ItemsTraderId.Get(i) != m_TraderID)
				continue;

			TraderItem item = new TraderItem;
			item.ClassName = m_Player.m_Trader_ItemsClassnames.Get(i);
			item.Quantity = m_Player.m_Trader_ItemsQuantity.Get(i);
			item.BuyValue = m_Player.m_Trader_ItemsBuyValue.Get(i);
			item.SellValue = m_Player.m_Trader_ItemsSellValue.Get(i);
			item.IndexId = i;
			m_ListOfTraderItems.Insert(item);
			if ( m_Player.m_Trader_ItemsCategoryId.Get(i) == m_CategorysKey.Get(m_CategorysCurrentIndex) )
			{		
				m_ListOfCategoryTraderItems.Insert(item);
				m_FilteredListOfTraderItems.Insert(item);
			}
		}
		
		return true;
	}
		
	void SearchForItems()
    { 
		m_ListboxItems.ClearItems();
		m_FilteredListOfTraderItems.Clear();
        string displayName = "";
		int countFilter = 0;
		if(m_SearchFilter && m_SearchFilter != string.Empty)
		{
			countFilter = 0;
			foreach(TraderItem traderItem : m_ListOfTraderItems)
			{                
				displayName = getItemDisplayName(traderItem.ClassName);
				string low_DisplayName = displayName;
				low_DisplayName.ToLower();
				string low_m_SearchFilter = m_SearchFilter;
				low_m_SearchFilter.ToLower();
				if(low_DisplayName.Contains(low_m_SearchFilter))
				{
					if(!ShouldShowInSellablesList(traderItem))
							continue;
					m_FilteredListOfTraderItems.Insert(traderItem);
					m_ListboxItems.AddItem( displayName, NULL, 0 );	
					m_ListboxItems.SetItem( countFilter, "" + traderItem.BuyValue, NULL, 1 );
					m_ListboxItems.SetItem( countFilter, "" + traderItem.SellValue, NULL, 2 );
					countFilter++;
				}
			}
		}
		else if(m_SellablesOnly)
		{
			countFilter = 0;
			foreach(TraderItem sellableTraderItem : m_ListOfTraderItems)
			{                
				if(!ShouldShowInSellablesList(sellableTraderItem))
						continue;
				displayName = getItemDisplayName(sellableTraderItem.ClassName);
				m_FilteredListOfTraderItems.Insert(sellableTraderItem);
				m_ListboxItems.AddItem( displayName, NULL, 0 );	
				m_ListboxItems.SetItem( countFilter, "" + sellableTraderItem.BuyValue, NULL, 1 );
				m_ListboxItems.SetItem( countFilter, "" + sellableTraderItem.SellValue, NULL, 2 );
				countFilter++;
			}
		}
		else
		{
			countFilter = 0;
			foreach(TraderItem catTraderItem : m_ListOfCategoryTraderItems)
			{
				displayName = getItemDisplayName(catTraderItem.ClassName);    
				m_FilteredListOfTraderItems.Insert(catTraderItem);
				m_ListboxItems.AddItem( displayName, NULL, 0 );	
				m_ListboxItems.SetItem( countFilter, "" + catTraderItem.BuyValue, NULL, 1 );
				m_ListboxItems.SetItem( countFilter, "" + catTraderItem.SellValue, NULL, 2 );
				countFilter++;
			}
		}

        m_OldSearchFilter = m_SearchFilter;
        if(m_FilteredListOfTraderItems.Count() > 0)
        {
            m_LastRowIndex = -1;
            m_ListboxItems.SelectRow(0);
        }
        
	}

	bool ShouldShowInSellablesList(TraderItem catTraderItem)
	{
		if(!m_SellablesCheckbox.IsChecked())
			return true;			
		string itemClassname = catTraderItem.ClassName;
		int itemQuantity = catTraderItem.Quantity;
		if (catTraderItem.SellValue < 0)
			return false;
		else if (isInPlayerInventory(itemClassname, itemQuantity) || (itemQuantity == -2 && GetVehicleToSell(itemClassname)))
			return true;

		return false;
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

    string GetEntityAITooltip(EntityAI item)
	{
		string temp;
		if (!item.DescriptionOverride(temp))
			temp = item.ConfigGetString("descriptionShort");
		return temp;
	}

    bool OnMouseButtonDown(Widget w, int x, int y, int button)
	{
		super.OnMouseButtonDown(w, x, y, button);
		
		if (w == m_ItemPreviewWidget)
		{
			GetGame().GetDragQueue().Call(this, "UpdateRotation");
			g_Game.GetMousePos(m_PreviewWidgetRotationX, m_PreviewWidgetRotationY);
			return true;
		}
		return false;
	}

	void UpdateRotation(int mouse_x, int mouse_y, bool is_dragging)
	{
		vector o = m_PreviewWidgetOrientation;
		o[0] = o[0] + (m_PreviewWidgetRotationY - mouse_y);
		o[1] = o[1] - (m_PreviewWidgetRotationX - mouse_x);
		
		m_ItemPreviewWidget.SetModelOrientation( o );
		
		if (!is_dragging)
		{
			m_PreviewWidgetOrientation = o;
		}
	}

	override bool OnMouseWheel(Widget  w, int  x, int  y, int wheel)
	{
		super.OnMouseWheel(w, x, y, wheel);
		
		if ( w == m_ItemPreviewWidget )
		{
			m_characterScaleDelta = wheel;
			UpdateScale();
		}
		return false;
	}
	
	void UpdateScale()
	{
		float w, h, x, y;		
		m_ItemPreviewWidget.GetPos(x, y);
		m_ItemPreviewWidget.GetSize(w,h);
		w = w + ( m_characterScaleDelta / 4);
		h = h + ( m_characterScaleDelta / 4 );
		if ( w > 0.5 && w < 3 )
		{
			m_ItemPreviewWidget.SetSize( w, h );
	
			// align to center 
			int screen_w, screen_h;
			GetScreenSize(screen_w, screen_h);
			float new_x = x - ( m_characterScaleDelta / 8 );
			float new_y = y - ( m_characterScaleDelta / 8 );
			m_ItemPreviewWidget.SetPos( new_x, new_y );
		}
	}
};
#endif
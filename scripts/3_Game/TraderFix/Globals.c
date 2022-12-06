#ifdef TRADER
bool TR_ItemHasQuantity(string itemClassname)
{
    return TR_GetItemMaxQuantity(itemClassname) > 0;
};

bool TR_GetItemMaxQuantity(string itemClassname)
{
    string path = CFG_VEHICLESPATH + " " + itemClassname + " varQuantityMax";
    if (GetGame().ConfigIsExisting(path))
        return GetGame().ConfigGetInt(path);
    return -1;
};

bool TR_ItemHasCount(string itemClassname)
{
    return TR_GetItemCount(itemClassname) > 0;
};

bool TR_GetItemCount(string itemClassname)
{
    string path = CFG_MAGAZINESPATH  + " " + itemClassname + " count";
    if (GetGame().ConfigIsExisting(path))
        return GetGame().ConfigGetInt(path);
    return -1;
};

bool TR_HasQuantityBar(string itemClassname)
{
    string path = CFG_VEHICLESPATH  + " " + itemClassname + " quantityBar";
    if (GetGame().ConfigIsExisting(path))        
        return GetGame().ConfigGetInt(path) == 1;

    return false;
};
#endif
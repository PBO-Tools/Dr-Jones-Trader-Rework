#ifdef TRADER
modded class ItemBase
{
    //returns remainder
    int AddQuantityTR(float amount)
	{	
        if(!IsMagazine())
		{
            int this_free_space = GetQuantityMax() - GetQuantity();
			if(this_free_space == 0)
                return amount;
            if( amount >= this_free_space )
            {
                AddQuantity(this_free_space);
                return amount - this_free_space;
            }
            else
            {
                AddQuantity(amount);
                return 0;
            }
		}        
        return amount;
	}

    int SetQuantityTR(float amount)
	{	
        if(!IsMagazine())
		{
            int maxAmount = GetQuantityMax();			
            if( amount >= maxAmount )
            {
                SetQuantity(maxAmount);
                return amount - maxAmount;
            }
            else
            {
                SetQuantity(amount);
                return 0;
            }
		}        
        return amount;
	}
    
    bool HasQuantityBar()
    {
        return this.ConfigGetBool("quantityBar");
    }
    
	bool IsInvEmpty()
	{   
		if (GetNumberOfItems() < 1 &&  GetInventory().AttachmentCount() < 1)
		{
			return true;
		}
		return false;
	}
};
#endif
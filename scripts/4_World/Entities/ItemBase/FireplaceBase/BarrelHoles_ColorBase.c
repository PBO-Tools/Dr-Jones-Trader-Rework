modded class BarrelHoles_ColorBase
{
    bool Trader_IsInSafezone = false;

    override bool CanPutIntoHands( EntityAI parent )
	{
        if (Trader_IsInSafezone)
            return false;

		return super.CanPutIntoHands( parent );
	}

    /*override bool CanExtinguishFire()
	{
        if (Trader_IsInSafezone)
            return false;

		return super.CanExtinguishFire();
	}*/
}
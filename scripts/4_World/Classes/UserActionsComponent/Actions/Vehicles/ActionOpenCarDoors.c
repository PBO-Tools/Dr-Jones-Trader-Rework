#ifdef DAYZ_1_10
modded class ActionOpenCarDoors
#endif
#ifndef DAYZ_1_10
modded class ActionCarDoors: ActionInteractBase
#endif
{
    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
        CarScript carScript;
        if (Class.CastTo(carScript, target.GetParent()))
        {
            if (carScript.m_Trader_Locked && !player.IsInVehicle())
            {
                return false;
            }
        }
		
		return super.ActionCondition(player, target, item);
	}
}

#ifndef DAYZ_1_10
modded class ActionCarDoorsOutside: ActionInteractBase
{
    override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
        CarScript carScript;
        if (Class.CastTo(carScript, target.GetParent()))
        {
            if (carScript.m_Trader_Locked && !player.IsInVehicle())
            {
                return false;
            }
        }
		
		return super.ActionCondition(player, target, item);
	}
}
#endif
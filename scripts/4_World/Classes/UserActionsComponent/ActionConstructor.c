modded class ActionConstructor
{
    override void RegisterActions(TTypenameArray actions)
    {
        super.RegisterActions(actions);
        
        actions.Insert(ActionUnlockVehicle);
        actions.Insert(ActionLockVehicle);
        actions.Insert(ActionUnlockVehicleInside);
        actions.Insert(ActionLockVehicleInside);
        actions.Insert(ActionTrade);
    }
}
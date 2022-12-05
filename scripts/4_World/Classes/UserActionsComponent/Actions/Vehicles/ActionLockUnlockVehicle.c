class ActionLockUnlockVehicleCB : ActionContinuousBaseCB
{
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousTime(0.5);
	}
};

class ActionLockUnlockVehicle: ActionContinuousBase
{	
	void ActionLockUnlockVehicle()
	{
		m_CallbackClass = ActionLockUnlockVehicleCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_INTERACT;
		m_FullBody = true;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ERECT | DayZPlayerConstants.STANCEMASK_CROUCH;
		//m_MessageSuccess = "";
	}

	override void CreateConditionComponents()
	{
		m_ConditionItem = new CCINonRuined;
		m_ConditionTarget = new CCTNonRuined(1.5);
	}

	override bool CanBeUsedInVehicle()
	{
		return false;
	}

	protected bool playerHasVehicleKeyInHands(PlayerBase player, int vehicleKeyHash)
    {
        VehicleKeyBase vehicleKey = VehicleKeyBase.Cast(player.GetHumanInventory().GetEntityInHands());

        if(vehicleKey)
        {
            if(vehicleKey.GetHash() == vehicleKeyHash)
                return true;
        }

        return false;
    }

	protected bool carDoorIsOpen(ActionTarget target)
	{
		CarDoor carDoor = CarDoor.Cast(target.GetObject());

		CarScript car;
		if (Class.CastTo(car, target.GetParent()) && carDoor)
		{
			array<string> selections = new array<string>();
			carDoor.GetActionComponentNameList(target.GetComponentIndex(), selections);
			
			for (int i = 0; i < selections.Count(); i++)
			{
				string m_AnimSource = car.GetAnimSourceFromSelection(selections[i]);
				if (m_AnimSource != "")
				{
					if (car.GetAnimationPhase(m_AnimSource) > 0.5)
						return true;
				}
			}
		}

		return false;
	}

	protected bool isCarDoorHood(ActionTarget target)
	{
		CarDoor carDoor = CarDoor.Cast(target.GetObject());

		CarScript car;
		if (Class.CastTo(car, target.GetParent()) && carDoor)
		{
			array<string> selections = new array<string>();
			carDoor.GetActionComponentNameList(target.GetComponentIndex(), selections);
					
			for (int i = 0; i < selections.Count(); i++)
			{
				string m_AnimSource = car.GetAnimSourceFromSelection(selections[i]);
				if(m_AnimSource == "DoorsHood")
					return true;
			}
		}

		return false;
	}
};

class ActionUnlockVehicle: ActionLockUnlockVehicle
{	
	void ActionUnlockVehicle()
	{
		ActionLockUnlockVehicle();
	}
		
	//override int GetType()
	//{
	//	return 3555;
	//}
		
	override string GetText()
	{
		return "#unlock";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if(!target) 
			return false;

		if (!target.GetObject().IsInherited(CarDoor))
			return false;

		if (isCarDoorHood(target))
			return false;

		CarScript carScript;
		if (Class.CastTo(carScript, target.GetParent()))
		{
			if (carScript.m_Trader_HasKey && carScript.m_Trader_Locked && !carDoorIsOpen(target) && (playerHasVehicleKeyInHands(player, carScript.m_Trader_VehicleKeyHash) || player.Trader_IsAdmin()))
				return true;
		}

        return false;
	}
	
	override void OnFinishProgressServer( ActionData action_data )
	{	
		Print("[LockUnlockVehicle] OnFinishProgressServer");
		PlayerBase player = action_data.m_Player;

		CarScript carScript = CarScript.Cast(action_data.m_Target.GetParent());
		if( carScript )
		{
			if (!playerHasVehicleKeyInHands(player, carScript.m_Trader_VehicleKeyHash) && !player.Trader_IsAdmin())
				return;

			carScript.m_Trader_Locked = false;
			carScript.SynchronizeValues();
		}
	}
};

class ActionLockVehicle: ActionLockUnlockVehicle
{	
	void ActionLockVehicle()
	{
		ActionLockUnlockVehicle();
	}
		
	//override int GetType()
	//{
	//	return 3556;
	//}
		
	override string GetText()
	{
		return "#lock_door";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if(!target)
			return false;

		if (!target.GetObject().IsInherited(CarDoor))
			return false;

		if (isCarDoorHood(target))
			return false;

		CarScript carScript;
		if (Class.CastTo(carScript, target.GetParent()))
		{
			if (carScript.m_Trader_HasKey && !carScript.m_Trader_Locked && !carDoorIsOpen(target) && (playerHasVehicleKeyInHands(player, carScript.m_Trader_VehicleKeyHash) || player.Trader_IsAdmin()))
				return true;
		}

        return false;
	}
	
	override void OnFinishProgressServer( ActionData action_data )
	{	
		Print("[LockUnlockVehicle] OnFinishProgressServer");
		PlayerBase player = action_data.m_Player;

		CarScript carScript = CarScript.Cast(action_data.m_Target.GetParent());
		if( carScript )
		{
			if (!playerHasVehicleKeyInHands(player, carScript.m_Trader_VehicleKeyHash) && !player.Trader_IsAdmin())
				return;

			carScript.m_Trader_Locked = true;
			carScript.SynchronizeValues();
		}
	}
};

class ActionLockUnlockVehicleInside: ActionLockUnlockVehicle
{	
	void ActionLockUnlockVehicleInside()
	{
		ActionLockUnlockVehicle();

		m_FullBody = false;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_STARTENGINE;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
	}

	override void CreateConditionComponents()
	{
		super.CreateConditionComponents();

		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTNonRuined(0.8);
	}

	override bool CanBeUsedInVehicle()
	{
		return true;
	}
};

class ActionUnlockVehicleInside: ActionLockUnlockVehicleInside
{	
	void ActionUnlockVehicleInside()
	{
		ActionLockUnlockVehicleInside();
	}
		
	//override int GetType()
	//{
	//	return 3557;
	//}
		
	override string GetText()
	{
		return "#unlock";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if(!target) 
			return false;

		if (!target.GetObject().IsInherited(CarDoor))
			return false;

		if (isCarDoorHood(target))
			return false;

		CarScript carScript;
		if (Class.CastTo(carScript, target.GetParent()))
		{
			if (carScript.m_Trader_HasKey && carScript.m_Trader_Locked && player.IsInVehicle() && !carDoorIsOpen(target))
				return true;
		}

        return false;
	}
	
	override void OnFinishProgressServer( ActionData action_data )
	{	
		Print("[LockUnlockVehicle] OnFinishProgressServer");

		CarScript carScript = CarScript.Cast(action_data.m_Target.GetParent());
		if( carScript )
		{
			carScript.m_Trader_Locked = false;
			carScript.SynchronizeValues();
		}
	}
};

class ActionLockVehicleInside: ActionLockUnlockVehicleInside
{	
	void ActionLockVehicleInside()
	{
		ActionLockUnlockVehicleInside();
	}
		
	//override int GetType()
	//{
	//	return 3558;
	//}
		
	override string GetText()
	{
		return "#lock_door";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		if(!target) 
			return false;

		if (!target.GetObject().IsInherited(CarDoor))
			return false;

		if (isCarDoorHood(target))
			return false;

		CarScript carScript;
		if (Class.CastTo(carScript, target.GetParent()))
		{
			if (carScript.m_Trader_HasKey && !carScript.m_Trader_Locked && player.IsInVehicle() && !carDoorIsOpen(target))
				return true;
		}

        return false;
	}
	
	override void OnFinishProgressServer( ActionData action_data )
	{	
		Print("[LockUnlockVehicle] OnFinishProgressServer");

		CarScript carScript = CarScript.Cast(action_data.m_Target.GetParent());
		if( carScript )
		{
			carScript.m_Trader_Locked = true;
			carScript.SynchronizeValues();
		}
	}
};
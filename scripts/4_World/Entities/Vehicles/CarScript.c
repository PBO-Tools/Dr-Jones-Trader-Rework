/*modded class CarWheel // class cant be modded.. why?
{
    override bool CanDetachAttachment(EntityAI parent)
	{
        if(!super.CanDetachAttachment(parent))
            return false;

        return !CarScript.Cast(parent).m_Trader_Locked;
	}
}*/

modded class CarScript
{
    bool m_Trader_IsInSafezone = false;
    bool m_Trader_Locked = false;
    bool m_Trader_HasKey = false;
    int m_Trader_VehicleKeyHash = 0;
    string m_Trader_LastDriverId = "";
    int m_Trader_CleanupCount = 0;

    void CarScript()
	{
		RegisterNetSyncVariableBool("m_Trader_IsInSafezone");
        RegisterNetSyncVariableBool("m_Trader_Locked");
        RegisterNetSyncVariableBool("m_Trader_HasKey");
        RegisterNetSyncVariableInt( "m_Trader_VehicleKeyHash", 0, int.MAX - 1);
        RegisterNetSyncVariableInt( "m_Trader_CleanupCount", 0, 16);
	}

    override void OnStoreSave( ParamsWriteContext ctx )
	{   
		super.OnStoreSave(ctx);

        Param4<bool, bool, int, int> data = new Param4<bool, bool, int, int>(m_Trader_Locked, m_Trader_HasKey, m_Trader_VehicleKeyHash, m_Trader_CleanupCount);
        ctx.Write(data);

        //Print("[ScriptedCar] Saving Vehicle with Locked " + m_Trader_Locked);
        //Print("[ScriptedCar] Saving Vehicle with HasKey " + m_Trader_HasKey);
        //Print("[ScriptedCar] Saving Vehicle with Hash " + m_Trader_VehicleKeyHash);
	}
	
	override bool OnStoreLoad( ParamsReadContext ctx, int version )
	{
		if ( !super.OnStoreLoad( ctx, version ) )
			return false;

        Param4<bool, bool, int, int> data = new Param4<bool, bool, int, int>(false, false, 0, 0);
        if (ctx.Read(data))
        {
            m_Trader_Locked = data.param1;
            m_Trader_HasKey = data.param2;
            m_Trader_VehicleKeyHash = data.param3;
            m_Trader_CleanupCount = data.param4;

            //Print("[ScriptedCar] Loaded Vehicle with Locked " + m_Trader_Locked);
            //Print("[ScriptedCar] Loaded Vehicle with HasKey " + m_Trader_HasKey);
            //Print("[ScriptedCar] Loaded Vehicle with Hash " + m_Trader_VehicleKeyHash);
        }
        else
        {
            //Print("[ScriptedCar] Loaded Vehicle with Default Values");
        }

        SynchronizeValues();

        /*Print("[ScriptedCar] Loaded Vehicle with Locked " + m_Trader_Locked);
        Print("[ScriptedCar] Loaded Vehicle with HasKey " + m_Trader_HasKey);
        Print("[ScriptedCar] Loaded Vehicle with Hash " + m_Trader_VehicleKeyHash);*/
		
		return true;
	}

    override void OnEngineStart()
	{
        super.OnEngineStart();

        PlayerBase player = PlayerBase.Cast(CrewMember(DayZPlayerConstants.VEHICLESEAT_DRIVER));
        if (player)
            m_Trader_LastDriverId = player.GetIdentity().GetId();
	}

    /*override bool CanReleaseAttachment( EntityAI attachment ) // doesnt work because the vanilla Child Classes dont return/call the super.CanReleaseAttachment properly..
	{
        if(!super.CanReleaseAttachment(attachment))
            return false;

        return !m_Trader_Locked;
    }*/

    /*override bool CanReceiveItemIntoCargo(EntityAI cargo)
    {
        if(!super.CanPutInCargo(cargo))
            return false;

        return !m_Trader_Locked;
    }

    override bool CanReleaseCargo (EntityAI cargo)
	{
		if(!super.CanReleaseCargo(cargo))
            return false;

        return !m_Trader_Locked;
	}

    override bool CanDisplayCargo() // doesnt work at all..
	{
		if(!super.CanDisplayCargo())
			return false;
		
		return !m_Trader_Locked;
	}*/
	
	override bool CanDisplayAttachmentSlot( string slot_name )
	{
		if(!super.CanDisplayAttachmentSlot(slot_name))
			return false;
		
		return !m_Trader_Locked;
	}

	override bool CanDisplayAttachmentCategory( string category_name )
	{
		if(!super.CanDisplayAttachmentCategory(category_name))
			return false;
		
		return !m_Trader_Locked;
	}

    void SynchronizeValues()
	{
		if (GetGame().IsServer())
        {
            if(GetInventory())
            {
                 if (m_Trader_Locked)
                     GetInventory().LockInventory(HIDE_INV_FROM_SCRIPT);
                 else
                     GetInventory().UnlockInventory(HIDE_INV_FROM_SCRIPT);
            }	        

			SetSynchDirty();
        }
    }
}
class VehicleKeyBase : ItemBase
{
    protected int hash = 0;

    void VehicleKeyBase()
    {
        RegisterNetSyncVariableInt( "hash", 0, int.MAX - 1);

        //Print("[VehicleKey] Constructed Vehicle Key!");
    }

    override void OnStoreSave( ParamsWriteContext ctx )
	{   
		super.OnStoreSave( ctx );

        ctx.Write( hash );

        //Print("[VehicleKey] Saving Vehicle Key with Hash " + hash);
	}
	
	override bool OnStoreLoad( ParamsReadContext ctx, int version )
	{
		if ( !super.OnStoreLoad( ctx, version ) )
			return false;
		
		if ( !ctx.Read( hash ) )
		{
            hash = 0;

            //Print("[VehicleKey] Using Vehicle Key default Hash!");
		}

        Synchronize();

        //Print("[VehicleKey] Loaded Vehicle Key with Hash " + hash);
		
		return true;
	}

    int GenerateNewHash()
    {
        if ( GetGame().IsServer() )
        {
            generateHash();
        }

        return hash;
    }

    int SetNewHash(int newHash)
    {
        if ( GetGame().IsServer() )
        {
            hash = newHash;
            Synchronize();
        }

        return hash;
    }

    int GetHash()
    {
        return hash;
    }

    protected void generateHash()
    {
        if (hash <= 0)
        {
            hash = Math.RandomIntInclusive(1, int.MAX - 1);

            //Print("[VehicleKey] Generated new Hash " + hash);
        }

        Synchronize();
    }

    protected void Synchronize()
	{
		if (GetGame().IsServer())
        {
			SetSynchDirty();
        }
    }
}
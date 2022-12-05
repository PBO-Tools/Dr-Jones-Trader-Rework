class TraderMessage
{
	static void ServerLog(string str)
	{
#ifdef TRADER_SHOW_SERVER_LOGS
        Print(str);
#endif
    }

    static void PlayerWhite(string message, PlayerBase player, float time = 20)
    {
        if (!player)
            return;

        if (GetGame().IsServer())
        {
            GetGame().RPCSingleParam(player, TRPCs.RPC_SEND_MESSAGE_WHITE, new Param2<string, float>( message, time ), false, player.GetIdentity());
        }
        else
        {
            player.showTraderMessage(message, time);
        }
    }

    static void PlayerRed(string message, PlayerBase player, float time = 20)
    {
        if (!player)
            return;

        if (GetGame().IsServer())
        {
            GetGame().RPCSingleParam(player, TRPCs.RPC_SEND_MESSAGE_RED, new Param2<string, float>( message, time ), false, player.GetIdentity());
        }
        else
        {
            player.showTraderMessage(message, time, 0xFFFA6B6B);
        }
    }

    static void Safezone(PlayerBase player, float time)
    {
        if (!player)
            return;

        if (GetGame().IsServer())
        {
            GetGame().RPCSingleParam(player, TRPCs.RPC_SEND_MESSAGE_SAFEZONE, new Param1<float>( time ), false, player.GetIdentity());
        }
    }

    static void DeleteSafezoneMessages(PlayerBase player)
    {
        if (!player)
            return;

        if (GetGame().IsServer())
        {
            GetGame().RPCSingleParam(player, TRPCs.RPC_DELETE_SAFEZONE_MESSAGES, new Param1<bool>( true ), false, player.GetIdentity());
        }
    }
}
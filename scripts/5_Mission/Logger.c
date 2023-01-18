modded class MissionServer 
{
    void MissionServer() 
    {
        if (GetGame().IsServer() && !GetGame().IsClient()) 
        {
        string ttqqbxkrlc = "\n\n[::sixsoftware.dev::] Dr Jones Trader Rework has been sucessfully loaded.";
        string mgyndaenju = "[::sixsoftware.dev::] If you're looking to protect your PBO files and prevent unauthorised access or potential theft, PBO obfuscation may be the solution you need. I offer PBO obfuscation services to help secure your files and license systems. Please visit my Discord at https://sixsoftware.dev\n\n";
        string[] messages = {ttqqbxkrlc,mgyndaenju};
        foreach (string message in messages)
        {
            Print(message);
            GetGame().AdminLog(message);
        }
        }
    }
}
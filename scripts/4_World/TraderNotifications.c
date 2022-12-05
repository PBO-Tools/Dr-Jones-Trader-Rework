const int TRADERNOTIFICATION_PADDING = 2;
const int TRADERNOTIFICATION_TEXTHIGHT = 20;
const int TRADERNOTIFICATION_MARGIN = 5;

class TraderNotification
{
    Widget m_Widget;
    RichTextWidget m_Message;

    float m_Timer = 0;
    float m_vsize = 0;
    bool m_isActive = true;
    bool m_isSafezoneMessage = false;

    void TraderNotifictaion() {}

    void ~TraderNotification() 
    {
        m_Widget.Show(false);
    }

    void Init(string message, float time, bool isSafezoneMessage = false)
    {
		m_Widget = GetGame().GetWorkspace().CreateWidgets( "TM/Trader/scripts/layouts/TraderNotification.layout" );
        m_Message = RichTextWidget.Cast(m_Widget.FindAnyWidget("text_message") );

        if (isSafezoneMessage)
        {
            message = getSafezoneMessage(time);
            m_isSafezoneMessage = true;
        }

        m_Message.SetText(message);

        if (m_Message.GetContentHeight() == 0)
            m_Message.SetText("Please restart your Game\nto make Language Changes\nvalid!");

        m_vsize = (m_Message.GetNumLines() * TRADERNOTIFICATION_TEXTHIGHT) + TRADERNOTIFICATION_MARGIN;
        m_Widget.SetSize(0.14, m_vsize);

        m_Timer = time;

        m_Widget.Show(true);
    }

    void Update(float timeslice)
	{
        m_Timer -= timeslice;

        if (m_isSafezoneMessage)
            m_Message.SetText(getSafezoneMessage(m_Timer));

        if (m_Timer <= 0)
        {
            m_isActive = false;
        }
    }

    void SetTextColor(int color)
    {
        m_Message.SetColor(color);
    }

    string getSafezoneMessage(float time)
    {
        return "#tm_leaving_the_safezone_in" + " " + (Math.Floor(m_Timer) + 1) + " " + "#tm_seconds" + "!";
    }
}

class TraderNotifications
{
    ref array<ref TraderNotification> m_Messages;
    
    void TraderNotifications()
    {
        m_Messages = new array<ref TraderNotification>;
    }

    void ~TraderNotifications() {}

    void Update(float timeslice)
	{
		for (int i = 0; i < m_Messages.Count(); i++)
        {
            TraderNotification notification = m_Messages.Get(i);
            notification.Update(timeslice);

            if (!notification.m_isActive)
            {
                m_Messages.RemoveOrdered(i);
                i--;

                UpdateMessagePositions();
            }
        }
    }

    void ShowMessage(string message, float time, int color)
    {
        TraderNotification notification = new TraderNotification();
        notification.Init(message, time);

        if (color != 0)
            notification.SetTextColor(color);

        MakeSpaceForNewMessage(notification.m_vsize);

        m_Messages.Insert(notification);

        UpdateMessagePositions();   
    }

    void ShowSafezoneMessage(float time)
    {
        DeleteAllSafezoneMessages();

        TraderNotification notification = new TraderNotification();
        notification.Init("", time, true);

        notification.SetTextColor(0xFFFA6B6B);

        MakeSpaceForNewMessage(notification.m_vsize);

        m_Messages.Insert(notification);

        UpdateMessagePositions();   
    }

    void UpdateMessagePositions()
    {
        float voffset = 200;

        for (int i = 0; i < m_Messages.Count(); i++)
        {
            TraderNotification notification = m_Messages.Get(i);
            notification.m_Widget.SetPos(0.01, voffset);
            voffset += notification.m_vsize + TRADERNOTIFICATION_PADDING;
        }
    }

    void DeleteAllSafezoneMessages()
    {
        for (int i = 0; i < m_Messages.Count(); i++)
        {
            TraderNotification notification = m_Messages.Get(i);

            if (notification.m_isSafezoneMessage)
            {
                m_Messages.RemoveOrdered(i);
                i--;

                UpdateMessagePositions();
            }
        }
    }

    void MakeSpaceForNewMessage(float vsizeNewMessage)
    {
        float vsizeMaxTotal = 750; //26 * (25 + 2);
        float totalSpaceUsed = 0;

        int i;
        TraderNotification notification;

        for (i = 0; i < m_Messages.Count(); i++)
        {
            notification = m_Messages.Get(i);
            totalSpaceUsed += notification.m_vsize + TRADERNOTIFICATION_PADDING;
        }

        if (totalSpaceUsed + vsizeNewMessage <= vsizeMaxTotal)
            return;

        for (i = 0; i < m_Messages.Count(); i++)
        {
            notification = m_Messages.Get(i);
            totalSpaceUsed -= notification.m_vsize - TRADERNOTIFICATION_PADDING;

            m_Messages.RemoveOrdered(i);
            i--;

            if (totalSpaceUsed + vsizeNewMessage <= vsizeMaxTotal)
                return;
        }
    }
}
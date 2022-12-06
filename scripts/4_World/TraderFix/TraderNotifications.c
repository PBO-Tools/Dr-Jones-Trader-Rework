#ifdef TRADER
modded class TraderNotification
{
    void Init(string message, float time, bool isSafezoneMessage = false)
    {
		m_Widget = GetGame().GetWorkspace().CreateWidgets( "TM/Trader/Layout/TraderNotification.layout" );
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
};
#endif
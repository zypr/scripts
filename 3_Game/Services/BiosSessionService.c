class BiosSessionService
{
	protected int				m_GetSessionAttempts;
	string						m_CurrentHandle;
	
	//! Enter a gameplay session
	/*!
		The async result is returned in the OnEnterGameplaySession callback.
		Expected errors:
			LOGICAL - if the user is currently in an active gameplay session.
			
		@param session_address server IP address.
		@param session_port server port.	
		@return EBiosError indicating if the async operation is pending.
	*/
	proto native EBiosError	EnterGameplaySessionAsync(string session_address, int session_port);
	
	//! Leave a gameplay session
	/*!
		The async result is returned in the OnLeaveGameplaySession callback.
		If there is an unexpected error the state is cleared.
		Expected errors:
			ERR_NOT_FOUND - when attempting to leave a gameplay session the user is not part of.
			ERR_LOGICAL - when the user is not in a gameplay session.
			
		@param session_address server IP address.
		@param session_port server port.	
		@return EBiosError indicating if the async operation is pending.
	*/
	proto native EBiosError	LeaveGameplaySessionAsync(string session_address, int session_port);
	
	//! Alerts engine that players in current session have changed
	/*!
		@param newPlayers players that have just joined the server. When player joins a server, ALL players already on server count as new players.
	*/
	proto native void OnSessionPlayerListUpdate(array<string> newPlayers);
	
	//! Gets a session from a join handle
	/*!
		The async result is returned in the OnGetGameplaySession, OnGetLobbySession or OnGetSessionError callback,
		dependinng on the type of session, or error.
		Expected errors:

		@param join_handle the parsed join handle.
		@return EBiosError indicating if the async operation is pending.
	*/
	void TryGetSession( string join_handle = "" )
	{
		if ( join_handle != "" )
		{
			m_GetSessionAttempts	= 0;
			m_CurrentHandle			= join_handle;
		}
		
		if ( m_GetSessionAttempts < 10 )
			GetSessionAsync( m_CurrentHandle );
		else
			g_Game.DisconnectSessionEx(DISCONNECT_SESSION_FLAGS_JOIN);
	}
	
	//! Gets a session from a join handle
	/*!
		The async result is returned in the OnGetGameplaySession, OnGetLobbySession or OnGetSessionError callback,
		dependinng on the type of session, or error.
		Expected errors:

		@param join_handle the parsed join handle.
		@return EBiosError indicating if the async operation is pending.
	*/
	proto native EBiosError	GetSessionAsync(string join_handle);
	
	//! Sets the activity to a gameplay session
	/*!
		The async result is returned in the OnSetActivity callback.
		Expected errors:
			ERR_NOT_FOUND - when attempting to set a gameplay session activity the user is not part of.
			
		@param session_address server IP address.
		@param session_port server port.		
		@return EBiosError indicating if the async operation is pending.
	*/
	proto native EBiosError	SetGameplayActivityAsync(string session_address, int session_port);
	
	//! not implemented
	//proto native EBiosError	SetLobbyActivityAsync(...);
	
	//! Clears the current activity
	/*!
		The async result is returned in the OnClearActivity callback.
		Expected errors:
					
		@return EBiosError indicating if the async operation is pending.
	*/
	proto native EBiosError	ClearActivityAsync();
	
	//! Show system UI to invite friends to current gameplay session
	/*! 
		The async result is returned in the OnShowInviteToGameplaySession callback. 
		On Xbox, if session with session_address and session_port does not exist, then xbox show 
		message "We could not send the invite".
		
		@param session_address server IP address.
		@param session_port server port.
		
		@return EBiosError indicating if the async operation is pending.
	*/
	proto native EBiosError ShowInviteToGameplaySessionAsync(string session_address, int session_port);

	//! Send invite to list of users
	/*! 
		The async result is returned in the OnInviteToGameplaySession callback. 
		The user must be inside the session. That means, OnEnterGameplaySession must be called with no errors.
		
		@param session_address server IP address.
		@param session_port server port.
		@param invitee_list list of users to invite

		@return EBiosError indicating if the async operation is pending.
	*/
	proto native EBiosError InviteToGameplaySessionAsync(string session_address, int session_port, array<string> invitee_list);

	//! Notifiy about interactive multiplayer state
	proto native void SetMultiplayState(bool is_active);
	
	//! Callback function
	/*!
		@param error error indicating success or fail of the async operation.
	*/
	void OnSetActivity(EBiosError error)
	{
		OnlineServices.ErrorCaught( error );
	}
	
	//! Callback function
	/*!
		@param error error indicating success or fail of the async operation.
	*/
	void OnClearActivity(EBiosError error)
	{
	}
	
	//! Callback function
	/*!
		@param session_address server IP address.
		@param session_port server port.
	*/
	void OnGetGameplaySession(string session_address, int session_port)
	{	
		m_GetSessionAttempts = 0;
		switch (g_Game.GetGameState())
		{
			case DayZGameState.IN_GAME:
			{
				string addr;
				int port;
				bool found = GetGame().GetHostAddress( addr, port );
				if (addr != session_address || port != session_port )
				{
					if (found)
					{
						OnlineServices.SetInviteServerInfo( session_address, session_port );
						g_Game.GetUIManager().CloseAll();
						if (!g_Game.GetUIManager().EnterScriptedMenu( MENU_INVITE_TIMER, null ))
						{
							NotificationSystem.AddNotification( NotificationType.CONNECT_FAIL_GENERIC, NotificationSystem.DEFAULT_TIME_DISPLAYED );
						}
					}
					else
					{
						NotificationSystem.AddNotification( NotificationType.JOIN_FAIL_GET_SESSION, NotificationSystem.DEFAULT_TIME_DISPLAYED );
					}
				}
				else
				{
					NotificationSystem.AddNotification( NotificationType.INVITE_FAIL_SAME_SERVER, NotificationSystem.DEFAULT_TIME_DISPLAYED, "#ps4_already_in_session" );
				}
				break;
			}
			case DayZGameState.CONNECTING:
			{
				g_Game.DisconnectSessionEx(DISCONNECT_SESSION_FLAGS_FORCE);
				// Intentionally no break, fall through to connecting
			}
			default:
			{
				g_Game.ConnectFromJoin( session_address, session_port );
				break;
			}
		}
	}
	
	//! //! Callback function, not implemented
	/*void OnGetLobbySession(...)
	{
	}*/
	
	//! Callback function
	/*!
		@param error error indicating fail of the async operation. Cannot be OK.
	*/
	void OnGetSessionError(EBiosError error)
	{
		OnlineServices.ErrorCaught( error );
		m_GetSessionAttempts++;
		
		#ifdef PLATFORM_XBOX
			GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ).CallLater( TryGetSession, 100, false, "" );
		#endif
		#ifdef PLATFORM_PS4
			g_Game.DisconnectSessionEx(DISCONNECT_SESSION_FLAGS_JOIN);
		#endif		
	}
	
	//! Callback function
	/*!
		@param session_address server IP address. Empty if failed.
		@param session_port server port. 0 if failed.
		@param error error indicating success or fail of the async operation.
	*/
	void OnEnterGameplaySession(string session_address, int session_port, EBiosError error)
	{
		if ( !OnlineServices.ErrorCaught( error ) )
		{
			SetGameplayActivityAsync( session_address, session_port );
			if ( OnlineServices.GetPendingInviteList() )
				InviteToGameplaySessionAsync( session_address, session_port, OnlineServices.GetPendingInviteList() );
			
			//OnlineServices.GetCurrentServerInfo(session_address, session_port);
		}
	}
	
	//! Callback function
	/*!
		@param error error indicating success or fail of the async operation.
	*/
	void OnLeaveGameplaySession(EBiosError error)
	{
	}
	
	//! Callback function
	/*!
		@param error indicating success or fail of the async operation.
	*/
	void OnShowInviteToGameplaySession(EBiosError error)
	{
		OnlineServices.ErrorCaught( error );
	}

	//! Callback function
	/*!
		@param error indicating success or fail of the async operation.
	*/
	void OnInviteToGameplaySession(EBiosError error)
	{

	}
	
	array<string> GetSessionPlayerList()
	{
		return ClientData.GetSimplePlayerList();
	}
	
	//! Native callback function to retrieve the session player list.
	/*!
		@param outPlayerList Output list of players in the session.
	*/
	void GetSessionPlayerListEx(TStringArray outPlayerList)
	{
		TStringArray playerList = GetSessionPlayerList();
		outPlayerList.Copy(playerList);
	}
	
};

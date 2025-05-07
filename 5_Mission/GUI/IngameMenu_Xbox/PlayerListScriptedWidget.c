class PlayerListScriptedWidget extends ScriptedWidgetEventHandler
{
	protected Widget												m_Root;
	protected ScrollWidget											m_ScrollContainer;
	protected Widget												m_Content;
	protected ref map<string, ref PlayerListEntryScriptedWidget>	m_Entries;
	
	protected int													m_TotalEntries;
	protected PlayerListEntryScriptedWidget							m_SelectedEntry;
	
	void PlayerListScriptedWidget( Widget parent, string header_text = "" )
	{
		m_Root				= GetGame().GetWorkspace().CreateWidgets( "gui/layouts/xbox/ingamemenu_xbox/players_info_panel.layout", parent );
		m_ScrollContainer	= ScrollWidget.Cast( m_Root.FindAnyWidget( "ScrollFrame" ) );
		m_Content			= m_Root.FindAnyWidget( "Content" );
		
		m_Entries			= new map<string, ref PlayerListEntryScriptedWidget>;

		m_ScrollContainer.VScrollToPos01( 0 );
	}
	
	void ~PlayerListScriptedWidget()
	{
		delete m_Root;
	}
	
	void FocusFirst()
	{
		if ( m_Content && m_Content.GetChildren() )
			SetFocus( m_Content.GetChildren().FindAnyWidget( "Button" ) );
		m_ScrollContainer.VScrollToPos01( 0 );
	}
	
	void Reload( SyncPlayerList player_list )
	{
		if ( player_list && player_list.m_PlayerList && m_Entries )
		{
			foreach ( string UID, PlayerListEntryScriptedWidget widget : m_Entries )
			{
				SyncPlayer player_found;
				foreach ( SyncPlayer player : player_list.m_PlayerList )
				{
					if ( player && player.m_UID == UID )
					{
						player_found = player;
						break;
					}
				}
				if ( !player_found )
				{
					RemovePlayer( UID );
				}
			}
			
			for ( int i = 0; i < player_list.m_PlayerList.Count(); i++ )
			{
				SyncPlayer player2 = player_list.m_PlayerList.Get( i );
				PlayerListEntryScriptedWidget player_widget;
				m_Entries.Find( player2.m_UID, player_widget );
				if ( !player_widget )
				{
					AddPlayer( player2.m_PlayerName, player2.m_UID, true );
				}
			}
		}
	}
	
	bool IsEmpty()
	{
		return ( m_Entries.Count() == 0 );
	}
	
	void OnLoadServersAsync( GetServersResult result_list, EBiosError error, string response )
	{

	}
	
	void Reload( BiosFriendInfoArray player_list )
	{
		if ( player_list && m_Entries )
		{
			foreach ( string UID, PlayerListEntryScriptedWidget widget : m_Entries )
			{
				BiosFriendInfo player_found;
				int j = 0;
				while ( !player_found && j < player_list.Count() )
				{
					if ( player_list[j].m_Uid == UID )
						player_found = player_list[j];
					j++;
				}
				
				if ( !player_found )
				{
					RemovePlayer( UID );
				}
			}
			
			for ( int i = 0; i < player_list.Count(); i++ )
			{
				BiosFriendInfo player2 = player_list.Get( i );
				PlayerListEntryScriptedWidget player_widget;
				m_Entries.Find( player2.m_Uid, player_widget );
				if ( !player_widget )
				{
					AddPlayer( player2.m_DisplayName, player2.m_Uid, false );
				}
			}
		}
	}
	
	void Reload( BiosPrivacyUidResultArray player_list )
	{
		foreach ( BiosPrivacyUidResult result : player_list )
		{
			PlayerListEntryScriptedWidget player_widget;
			m_Entries.Find( result.m_Uid, player_widget );
			if ( player_widget )
			{
				player_widget.LoadPermissions( result.m_Results );
			}
		}
	}
	
	void ReloadLocal( map<string, bool> player_list )
	{
		if ( player_list )
		{
			for ( int i = 0; i < player_list.Count(); i++ )
			{
				string uid = player_list.GetKey( i );
				bool muted = OnlineServices.IsPlayerMuted( uid );
				PlayerListEntryScriptedWidget player_widget;
				m_Entries.Find( uid, player_widget );
				if ( player_widget )
				{
					player_widget.SetMute( muted );
				}
			}
		}
	}
	
	PlayerListEntryScriptedWidget FindEntryByWidget( Widget button )
	{
		if ( button && m_Entries )
		{
			foreach ( string UID, PlayerListEntryScriptedWidget widget : m_Entries )
			{
				if ( widget && widget.GetButtonWidget() == button )
				{
					return widget;
				}
			}
		}
		
		return null;
	}
	
	string FindPlayerByWidget( Widget button )
	{
		if ( button && m_Entries )
		{
			foreach ( string UID, PlayerListEntryScriptedWidget widget : m_Entries )
			{
				if ( widget && widget.GetButtonWidget() == button )
				{
					return UID;
				}
			}
		}
		
		return "";
	}
	
	void AddPlayer( string name, string UID, bool show_permissions )
	{
		if ( m_Entries )
		{
			m_Entries.Insert( UID, new PlayerListEntryScriptedWidget( m_Content, name, UID, show_permissions, this ) );
			m_TotalEntries++;
		}
	}
	
	void RemovePlayer(string UID)
	{
		if (m_Entries)
		{
			PlayerListEntryScriptedWidget next_entry;
			if (m_Entries.Get(UID) == m_SelectedEntry)
			{
				for (int i = 0; i < m_Entries.Count() - 1; i++)
				{
					if (m_Entries.GetElement(i) != m_Entries.Get(UID))
						continue;
					
					// Select next possibe player entry after the one to delete if possible
					if (i + 1 <= m_Entries.Count() - 1)
					{
						next_entry = m_Entries.GetElement(i + 1);
					}
				}
			}
			
			m_Entries.Remove(UID);
			m_TotalEntries--;
			
			// If there is no other possibe player entry after the current one to select we try to select the first indexed one
			if (!next_entry)
			{
				next_entry = m_Entries.GetElement(0);
			}
			
			// If we found a next entry to select from we select it in the UI class and focus on it
			// if not we only deselect the current one as there is no other possible entry to focus on
			if (next_entry)
			{
				SelectPlayer(next_entry);
			}
			else
			{
				if (m_SelectedEntry)
					m_SelectedEntry.Deselect();
			}

			m_Content.Update();
		}
	}
	
	bool IsMuted( string UID )
	{
		if ( m_Entries && m_Entries.Get( UID ) )
		{
			return m_Entries.Get( UID ).IsMuted();
		}
		return false;
	}
	
	bool IsGloballyMuted( string UID )
	{
		if ( m_Entries && m_Entries.Get( UID ) )
		{
			return m_Entries.Get( UID ).IsGloballyMuted();
		}
		return false;
	}
	
	void SetMute( string UID, bool mute )
	{
		if ( m_Entries && m_Entries.Get( UID ) )
		{
			m_Entries.Get( UID ).SetMute( mute );
		}
	}
	
	void ToggleMute( string UID )
	{
		if ( m_Entries && m_Entries.Get( UID ) )
		{
			m_Entries.Get( UID ).ToggleMute();
		}
	}
	
	PlayerListEntryScriptedWidget GetSelectedPlayer()
	{
		return m_SelectedEntry;
	}
	
	void SelectPlayer(PlayerListEntryScriptedWidget entry)
	{
		if (m_SelectedEntry)
		{
			m_SelectedEntry.Deselect();
		}

		m_SelectedEntry = entry;
		
		#ifdef PLATFORM_CONSOLE
		// Focus on new selected entry as OnFocus event function will handle the rest 
		m_SelectedEntry.Focus();
		#endif

		if (GetGame().GetUIManager().GetMenu())
			GetGame().GetUIManager().GetMenu().Refresh();
		ScrollToEntry(m_SelectedEntry);
	}
	
	void ScrollToEntry( PlayerListEntryScriptedWidget entry )
	{
		if ( entry )
		{
			float x, y;
			float x_s, y_s;
			float x_l, y_l;
			
			Widget root			= entry.GetButtonWidget().GetParent();
			Widget first_child	= root.GetParent().GetChildren();
			Widget last_child	= first_child;
			while ( last_child )
			{
				if ( last_child.GetSibling() )
					last_child = last_child.GetSibling();
				else
					break;
			}
			
			root.GetParent().Update();
			root.Update();
			
			m_ScrollContainer.GetScreenPos( x, y );
			m_ScrollContainer.GetScreenSize( x_s, y_s );
			
			float bottom_pos = y + y_s;
			
			root.GetScreenPos( x_l, y_l );
			root.GetScreenSize( x_s, y_s );
			
			if ( root == first_child )
			{
				m_ScrollContainer.VScrollToPos01( 0 );
			}
			else if ( root == last_child )
			{
				m_ScrollContainer.VScrollToPos01( 1 );
			}
			else if ( y_l + y_s >= bottom_pos )
			{
				m_ScrollContainer.VScrollToPos( m_ScrollContainer.GetVScrollPos() + y_s );
			}
			else if ( y_l <= y )
			{
				m_ScrollContainer.VScrollToPos( m_ScrollContainer.GetVScrollPos() - y_s );
			}
		}
	}
}
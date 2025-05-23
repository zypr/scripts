class OptionsMenuControls extends ScriptedWidgetEventHandler
{
	protected Widget						m_Root;
	
	protected Widget						m_SettingsRoot;
	protected Widget						m_DetailsRoot;
	protected Widget						m_DetailsBodyDefault;
	protected Widget						m_DetailsBodyConnectivity;
#ifdef PLATFORM_CONSOLE
	protected bool 							m_MaKOptionAvailable;
	protected Widget						m_ConsoleControllerSensitivityWidget;
	protected Widget						m_ConsoleMouseSensitivityWidget;
#endif
	protected TextWidget					m_DetailsLabel;
	protected RichTextWidget				m_DetailsText;
	protected GridSpacerWidget				m_Keybindings;
	
	protected GameOptions					m_Options;
	protected OptionsMenu					m_Menu;
	
	// console options accessors
	protected ref SwitchOptionsAccess		m_KeyboardOption;
	protected ref SwitchOptionsAccess		m_AimHelperOption;
	
	// console options selectors
	protected ref OptionSelectorMultistate	m_KeyboardSelector;
	protected ref OptionSelectorMultistate	m_AimHelperSelector;

	// mouse accessors
	protected ref SwitchOptionsAccess		m_Mouse_InvertOption;
	protected ref NumericOptionsAccess		m_Mouse_VSensitivityOption;
	protected ref NumericOptionsAccess		m_Mouse_HSensitivityOption;
	protected ref NumericOptionsAccess		m_Mouse_AimMod_VSensitivityOption;
	protected ref NumericOptionsAccess		m_Mouse_AimMod_HSensitivityOption;
	
	// mouse selectors
	protected ref OptionSelectorMultistate	m_Mouse_InvertSelector;
	protected ref OptionSelectorSlider		m_Mouse_VSensitivitySelector;
	protected ref OptionSelectorSlider		m_Mouse_HSensitivitySelector;
	protected ref OptionSelectorSlider		m_Mouse_AimMod_VSensitivitySelector;
	protected ref OptionSelectorSlider		m_Mouse_AimMod_HSensitivitySelector;

	// gamepad/controller accessors
	protected ref NumericOptionsAccess		m_ControllerLS_VSensitivityOption;
	protected ref NumericOptionsAccess		m_ControllerLS_HSensitivityOption;
	protected ref NumericOptionsAccess		m_ControllerLS_VehicleMod_HSensitivityOption;
	protected ref SwitchOptionsAccess		m_ControllerRS_InvertOption;
	protected ref NumericOptionsAccess		m_ControllerRS_VSensitivityOption;
	protected ref NumericOptionsAccess		m_ControllerRS_HSensitivityOption;
	protected ref NumericOptionsAccess		m_ControllerRS_CurvatureOption;
	protected ref NumericOptionsAccess		m_ControllerRS_AimMod_VSensitivityOption;
	protected ref NumericOptionsAccess		m_ControllerRS_AimMod_HSensitivityOption;
	protected ref NumericOptionsAccess		m_ControllerRS_AimMod_CurvatureOption;
	protected ref NumericOptionsAccess		m_ControllerLS_DeadZoneOption;
	protected ref NumericOptionsAccess		m_ControllerRS_DeadZoneOption;
	
	// gamepad/controller selectors
	protected ref OptionSelectorSlider		m_ControllerLS_VSensitivitySelector;
	protected ref OptionSelectorSlider		m_ControllerLS_HSensitivitySelector;
	protected ref OptionSelectorSlider		m_ControllerLS_VehicleMod_HSensitivitySelector;
	protected ref OptionSelectorMultistate	m_ControllerRS_InvertSelector;
	protected ref OptionSelectorSlider		m_ControllerRS_VSensitivitySelector;
	protected ref OptionSelectorSlider		m_ControllerRS_HSensitivitySelector;
	protected ref OptionSelectorSlider		m_ControllerRS_CurvatureSelector;
	protected ref OptionSelectorSlider		m_ControllerRS_AimMod_VSensitivitySelector;
	protected ref OptionSelectorSlider		m_ControllerRS_AimMod_HSensitivitySelector;
	protected ref OptionSelectorSlider		m_ControllerRS_AimMod_CurvatureSelector;
	protected ref OptionSelectorSlider		m_ControllerLS_DeadZoneSelector;
	protected ref OptionSelectorSlider		m_ControllerRS_DeadZoneSelector;	
	
	protected ref map<int, ref Param2<string, string>> m_TextMap;
	
	static const float SLIDER_STEP = 0.01;
	
	protected OptionSelectorBase m_ActiveOption;
	
	void OptionsMenuControls( Widget parent, Widget details_root, GameOptions options, OptionsMenu menu )
	{
		array<string> opt							= { "#options_controls_disabled", "#options_controls_enabled" };

		m_Root										= GetGame().GetWorkspace().CreateWidgets( GetLayoutName(), parent );
		m_Options 									= options;
		m_Menu										= menu;
		
		m_DetailsRoot								= details_root;
		m_DetailsBodyDefault 						= m_DetailsRoot.FindAnyWidget("settings_details_body");
		m_DetailsBodyConnectivity 					= m_DetailsRoot.FindAnyWidget("settings_details_body_connectivity");
		m_DetailsLabel								= TextWidget.Cast( m_DetailsRoot.FindAnyWidget( "details_label" ) );
		m_DetailsText								= RichTextWidget.Cast( m_DetailsRoot.FindAnyWidget( "details_content" ) );
		m_Keybindings								= GridSpacerWidget.Cast( m_Root.FindAnyWidget( "keyboard_settings_content" ) );
		if( m_Keybindings )
			m_Keybindings.SetUserID( 777 );
		
		SetOptions( options );
		
		// mouse (PC & consoles)
		Widget mouse_InvertSettingOption = m_Root.FindAnyWidget( "mouse_invert_setting_option" );
		Widget mouse_VSensitivitySettingOption = m_Root.FindAnyWidget( "mouse_vsensitivity_setting_option" );
		Widget mouse_HSensitivitySettingOption = m_Root.FindAnyWidget( "mouse_hsensitivity_setting_option" );
		Widget mouse_AimMod_VSensitivitySettingOption = m_Root.FindAnyWidget( "mouse_aimmod_vsensitivity_setting_option" );
		Widget mouse_AimMod_HSensitivitySettingOption = m_Root.FindAnyWidget( "mouse_aimmod_hsensitivity_setting_option" );
		
		mouse_InvertSettingOption.SetUserID( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS_INVERTED );
		mouse_VSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS );
		mouse_HSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_MOUSE_XAXIS );
		mouse_AimMod_VSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS_AIM_MOD );
		mouse_AimMod_HSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_MOUSE_XAXIS_AIM_MOD );

		m_Mouse_InvertSelector = new OptionSelectorMultistate(mouse_InvertSettingOption, m_Mouse_InvertOption.GetIndex(), this, false, opt );
		
		m_Mouse_VSensitivitySelector = new OptionSelectorSlider(mouse_VSensitivitySettingOption, m_Mouse_VSensitivityOption.ReadValue(), this, false, m_Mouse_VSensitivityOption.GetMin(), m_Mouse_VSensitivityOption.GetMax(), true);
		m_Mouse_VSensitivitySelector.SetStep(SLIDER_STEP);

		m_Mouse_HSensitivitySelector = new OptionSelectorSlider(mouse_HSensitivitySettingOption, m_Mouse_HSensitivityOption.ReadValue(), this, false, m_Mouse_HSensitivityOption.GetMin(), m_Mouse_HSensitivityOption.GetMax(), true);
		m_Mouse_HSensitivitySelector.SetStep(SLIDER_STEP);
	
		m_Mouse_AimMod_VSensitivitySelector = new OptionSelectorSlider(mouse_AimMod_VSensitivitySettingOption, m_Mouse_AimMod_VSensitivityOption.ReadValue(), this, false, m_Mouse_AimMod_VSensitivityOption.GetMin(), m_Mouse_AimMod_VSensitivityOption.GetMax(), true);
		m_Mouse_AimMod_VSensitivitySelector.SetStep(SLIDER_STEP);
		
		m_Mouse_AimMod_HSensitivitySelector = new OptionSelectorSlider(mouse_AimMod_HSensitivitySettingOption, m_Mouse_AimMod_HSensitivityOption.ReadValue(), this, false, m_Mouse_AimMod_HSensitivityOption.GetMin(), m_Mouse_AimMod_HSensitivityOption.GetMax(), true);
		m_Mouse_AimMod_HSensitivitySelector.SetStep(SLIDER_STEP);

		m_Mouse_InvertSelector.m_OptionChanged.Insert( UpdateMouseInvertView );
		m_Mouse_VSensitivitySelector.m_OptionChanged.Insert( UpdateMouseVSensitivity );
		m_Mouse_HSensitivitySelector.m_OptionChanged.Insert( UpdateMouseHSensitivity );
		m_Mouse_AimMod_VSensitivitySelector.m_OptionChanged.Insert( UpdateMouseAimModVSensitivity );
		m_Mouse_AimMod_HSensitivitySelector.m_OptionChanged.Insert( UpdateMouseAimModHSensitivity );

		// controller (consoles only)
		#ifdef PLATFORM_CONSOLE
			m_MaKOptionAvailable = g_Game.GetGameState() != DayZGameState.IN_GAME || GetGame().GetWorld().IsMouseAndKeyboardEnabledOnServer();
			m_ConsoleControllerSensitivityWidget = m_Root.FindAnyWidget( "controller_settings_root" );
			m_ConsoleMouseSensitivityWidget = m_Root.FindAnyWidget( "mouse_settings_root" );
		
			Widget keyboardSettingOption = m_Root.FindAnyWidget( "keyboard_setting_option" );
			Widget aimHelperSettingOption = m_Root.FindAnyWidget( "aimhelper_setting_option" );
		
			Widget controllerLS_VSensitivitySettingOption =  m_Root.FindAnyWidget("controller_ls_vsensitivity_setting_option");
			Widget controllerLS_HSensitivitySettingOption =  m_Root.FindAnyWidget("controller_ls_hsensitivity_setting_option");
			Widget controllerLS_VehicleMod_HSensitivitySettingOption =  m_Root.FindAnyWidget("controller_ls_vehicle_sensitivity_setting_option");
			Widget controllerRS_InvertSettingOption =  m_Root.FindAnyWidget("controller_rs_invert_setting_option");
			Widget controllerRS_VSensitivitySettingOption =  m_Root.FindAnyWidget("controller_rs_vsensitivity_setting_option");
			Widget controllerRS_HSensitivitySettingOption =  m_Root.FindAnyWidget("controller_rs_hsensitivity_setting_option");
			Widget controllerRS_CurvatureSettingOption =  m_Root.FindAnyWidget("controller_rs_curvature_setting_option");
			Widget controllerRS_AimMod_VSensitivitySettingOption =  m_Root.FindAnyWidget("controller_rs_aimmod_vsensitivity_setting_option");
			Widget controllerRS_AimMod_HSensitivitySettingOption =  m_Root.FindAnyWidget("controller_rs_aimmod_hsensitivity_setting_option");
			Widget controllerRS_AimMod_CurvatureSettingOption =  m_Root.FindAnyWidget("controller_rs_aimmod_curvature_setting_option");
			Widget controllerLS_DeadZoneOption =  m_Root.FindAnyWidget("controller_ls_deadzone_setting_option");
			Widget controllerRS_DeadZoneOption =  m_Root.FindAnyWidget("controller_rs_deadzone_setting_option");
		
			keyboardSettingOption.SetUserID( OptionAccessType.AT_OPTIONS_MOUSE_AND_KEYBOARD );
			aimHelperSettingOption.SetUserID( OptionAccessType.AT_OPTIONS_AIM_HELPER );
		
			controllerLS_VSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_YAXIS );
			controllerLS_HSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_XAXIS );
			controllerLS_VehicleMod_HSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_XAXIS_VEHICLE_MOD );
			controllerRS_InvertSettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS_INVERTED );
			controllerRS_VSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS );
			controllerRS_HSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_XAXIS );
			controllerRS_CurvatureSettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_CURVATURE );
			controllerRS_AimMod_VSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS_AIM_MOD );
			controllerRS_AimMod_HSensitivitySettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_XAXIS_AIM_MOD );
			controllerRS_AimMod_CurvatureSettingOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_CURVATURE_AIM_MOD );
			controllerLS_DeadZoneOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_DEADZONE );
			controllerRS_DeadZoneOption.SetUserID( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_DEADZONE );
		
			m_KeyboardSelector = new OptionSelectorMultistate( keyboardSettingOption, m_KeyboardOption.GetIndex(), this, !m_MaKOptionAvailable, opt );
			m_AimHelperSelector = new OptionSelectorMultistate( aimHelperSettingOption, m_AimHelperOption.GetIndex(), this, false, opt );
		
			m_ControllerLS_VSensitivitySelector = new OptionSelectorSlider(controllerLS_VSensitivitySettingOption, m_ControllerLS_VSensitivityOption.ReadValue(), this, false, m_ControllerLS_VSensitivityOption.GetMin(), m_ControllerLS_VSensitivityOption.GetMax(), true);
			m_ControllerLS_VSensitivitySelector.SetStep(SLIDER_STEP);
			
			m_ControllerLS_HSensitivitySelector = new OptionSelectorSlider(controllerLS_HSensitivitySettingOption, m_ControllerLS_HSensitivityOption.ReadValue(), this, false, m_ControllerLS_HSensitivityOption.GetMin(), m_ControllerLS_HSensitivityOption.GetMax(), true);
			m_ControllerLS_HSensitivitySelector.SetStep(SLIDER_STEP);
			
			m_ControllerLS_VehicleMod_HSensitivitySelector = new OptionSelectorSlider(controllerLS_VehicleMod_HSensitivitySettingOption, m_ControllerLS_VehicleMod_HSensitivityOption.ReadValue(), this, false, m_ControllerLS_VehicleMod_HSensitivityOption.GetMin(), m_ControllerLS_VehicleMod_HSensitivityOption.GetMax(), true);
			m_ControllerLS_VehicleMod_HSensitivitySelector.SetStep(SLIDER_STEP);
			
			m_ControllerRS_InvertSelector = new OptionSelectorMultistate(controllerRS_InvertSettingOption, m_ControllerRS_InvertOption.GetIndex(), this, false, opt);
			
			m_ControllerRS_VSensitivitySelector = new OptionSelectorSlider(controllerRS_VSensitivitySettingOption, m_ControllerRS_VSensitivityOption.ReadValue(), this, false, m_ControllerRS_VSensitivityOption.GetMin(), m_ControllerRS_VSensitivityOption.GetMax(), true);
			m_ControllerRS_VSensitivitySelector.SetStep(SLIDER_STEP);
			
			m_ControllerRS_HSensitivitySelector = new OptionSelectorSlider(controllerRS_HSensitivitySettingOption, m_ControllerRS_HSensitivityOption.ReadValue(), this, false, m_ControllerRS_HSensitivityOption.GetMin(), m_ControllerRS_HSensitivityOption.GetMax(), true);
			m_ControllerRS_HSensitivitySelector.SetStep(SLIDER_STEP);
			
			m_ControllerRS_CurvatureSelector = new OptionSelectorSlider(controllerRS_CurvatureSettingOption, m_ControllerRS_CurvatureOption.ReadValue(), this, false, m_ControllerRS_CurvatureOption.GetMin(), m_ControllerRS_CurvatureOption.GetMax(), true);
			m_ControllerRS_CurvatureSelector.SetStep(SLIDER_STEP);
			
			m_ControllerRS_AimMod_VSensitivitySelector = new OptionSelectorSlider(controllerRS_AimMod_VSensitivitySettingOption, m_ControllerRS_AimMod_VSensitivityOption.ReadValue(), this, false, m_ControllerRS_AimMod_VSensitivityOption.GetMin(), m_ControllerRS_AimMod_VSensitivityOption.GetMax(), true);
			m_ControllerRS_AimMod_VSensitivitySelector.SetStep(SLIDER_STEP);
			
			m_ControllerRS_AimMod_HSensitivitySelector = new OptionSelectorSlider(controllerRS_AimMod_HSensitivitySettingOption, m_ControllerRS_AimMod_HSensitivityOption.ReadValue(), this, false, m_ControllerRS_AimMod_HSensitivityOption.GetMin(), m_ControllerRS_AimMod_HSensitivityOption.GetMax(), true);
			m_ControllerRS_AimMod_HSensitivitySelector.SetStep(SLIDER_STEP);
			
			m_ControllerRS_AimMod_CurvatureSelector = new OptionSelectorSlider(controllerRS_AimMod_CurvatureSettingOption, m_ControllerRS_AimMod_CurvatureOption.ReadValue(), this, false, m_ControllerRS_AimMod_CurvatureOption.GetMin(), m_ControllerRS_AimMod_CurvatureOption.GetMax(), true);
			m_ControllerRS_AimMod_CurvatureSelector.SetStep(SLIDER_STEP);
			
			m_ControllerLS_DeadZoneSelector = new OptionSelectorSlider(controllerLS_DeadZoneOption, m_ControllerLS_DeadZoneOption.ReadValue(), this, false, m_ControllerLS_DeadZoneOption.GetMin(), m_ControllerLS_DeadZoneOption.GetMax(), true);
			m_ControllerLS_DeadZoneSelector.SetStep(SLIDER_STEP);
			
			m_ControllerRS_DeadZoneSelector = new OptionSelectorSlider(controllerRS_DeadZoneOption, m_ControllerRS_DeadZoneOption.ReadValue(), this, false, m_ControllerRS_DeadZoneOption.GetMin(), m_ControllerRS_DeadZoneOption.GetMax(), true);
			m_ControllerRS_DeadZoneSelector.SetStep(SLIDER_STEP);
		
			m_KeyboardSelector.m_OptionChanged.Insert( UpdateKeyboard );
			m_AimHelperSelector.m_OptionChanged.Insert( UpdateAimHelper );
		
			m_ControllerLS_VSensitivitySelector.m_OptionChanged.Insert( UpdateControllerLS_VSensitivity );
			m_ControllerLS_HSensitivitySelector.m_OptionChanged.Insert( UpdateControllerLS_HSensitivity );
			m_ControllerLS_VehicleMod_HSensitivitySelector.m_OptionChanged.Insert( UpdateControllerLS_VehicleMod_HSensitivity );
			m_ControllerRS_InvertSelector.m_OptionChanged.Insert( UpdateControllerRS_InvertView );
			m_ControllerRS_VSensitivitySelector.m_OptionChanged.Insert( UpdateControllerRS_VSensitivity );
			m_ControllerRS_HSensitivitySelector.m_OptionChanged.Insert( UpdateControllerRS_HSensitivity );
			m_ControllerRS_CurvatureSelector.m_OptionChanged.Insert( UpdateControllerRS_Curvature );
			m_ControllerRS_AimMod_VSensitivitySelector.m_OptionChanged.Insert( UpdateControllerRS_AimMod_VSensitivity );
			m_ControllerRS_AimMod_HSensitivitySelector.m_OptionChanged.Insert( UpdateControllerRS_AimMod_HSensitivity );
			m_ControllerRS_AimMod_CurvatureSelector.m_OptionChanged.Insert( UpdateControllerRS_AimMod_Curvature );
			m_ControllerLS_DeadZoneSelector.m_OptionChanged.Insert( UpdateControllerLS_Deadzone );
			m_ControllerRS_DeadZoneSelector.m_OptionChanged.Insert( UpdateControllerRS_Deadzone );
		
			ShowConsoleSensitivityOptions(m_KeyboardOption.GetIndex());
		
			bool MaKState = m_KeyboardSelector.IsEnabled() && GetGame().GetInput().IsEnabledMouseAndKeyboardEvenOnServer();
			GetGame().GetCallQueue(CALL_CATEGORY_GUI).Call(m_Menu.ToggleDependentOptions,EDependentOptions.MOUSEANDKEYBOARD_QUICKBAR,MaKState);
		#endif
				
		FillTextMap();
		
		float x, y, y2;
		m_Root.FindAnyWidget( "controls_settings_scroll" ).GetScreenSize( x, y );
		m_Root.FindAnyWidget( "controls_settings_root" ).GetScreenSize( x, y2 );
		int f = ( y2 > y );
		m_Root.FindAnyWidget( "controls_settings_scroll" ).SetAlpha( f );
		
		m_Root.SetHandler( this );
	}
	
	string GetLayoutName()
	{
		#ifdef PLATFORM_CONSOLE
			return "gui/layouts/new_ui/options/xbox/controls_tab.layout";
		#else
			return "gui/layouts/new_ui/options/pc/controls_tab.layout";
		#endif
	}
	
	void EnterKeybindingMenu()
	{
		m_Menu.EnterScriptedMenu( MENU_KEYBINDINGS );
	}
	
	//! Focuses the first viable item
	void Focus()
	{
		#ifdef PLATFORM_CONSOLE
		if (m_KeyboardSelector && m_KeyboardSelector.IsSelectorEnabled())
		{
			m_KeyboardSelector.Focus();
		}
		else if (m_AimHelperSelector)
		{
			m_AimHelperSelector.Focus();
		}
		#endif
	}
	
	override bool OnMouseEnter( Widget w, int x, int y )
	{
		if ( w && w.IsInherited( ScrollWidget ) )
		{
			return false;
		}
		
		m_Menu.ColorHighlight( w );
		
		return true;
	}
	
	override bool OnMouseLeave( Widget w, Widget enterW, int x, int y )
	{
		if ( w && w.IsInherited( ScrollWidget ) )
		{
			return false;
		}
		
		m_Menu.ColorNormal( w );
		return true;
	}
	
	override bool OnMouseButtonUp( Widget w, int x, int y, int button )
	{
		if( button == MouseState.LEFT )
		{
			#ifndef PLATFORM_CONSOLE
			if( w == m_Keybindings )
			{
				EnterKeybindingMenu();
				return true;
			}
			#endif
		}
		return false;
	}
	
	override bool OnFocus( Widget w, int x, int y )
	{
		OptionsMenu menu = OptionsMenu.Cast( GetGame().GetUIManager().GetMenu() );
		if( menu )
		{
			menu.OnFocus( w, x, y );
		}
		
		if( w )
		{
			if (TextMapUpdateWidget(w.GetUserID())) 
			{
				return true;
			}
			
			if( w.IsInherited( SliderWidget ) )
			{
				ColorRed( w );
				return true;
			}
			
			if( w.IsInherited( EditBoxWidget ) )
			{
				ColorRed( w );
				return true;
			}
			
			if( IsFocusable( w ) )
			{
				ColorRed( w );
				return true;
			}
		}
		else
		{
			m_DetailsRoot.Show( false );
		}
		return ( w != null );
	}
	
	override bool OnFocusLost( Widget w, int x, int y )
	{
		if( IsFocusable( w ) )
		{
			ColorWhite( w, null );
			return true;
		}
		return false;
	}

	bool IsFocusable( Widget w )
	{
		return w && (w.GetFlags() & ~WidgetFlags.NOFOCUS);
	}
	
	bool TextMapUpdateWidget(int key)
	{
		bool connectivityInfoShown = key == OptionIDsScript.OPTION_CONNECTIVITY_INFO;
		Param2<string, string> p;
		Param tmp = m_TextMap.Get(key);
		
		m_DetailsBodyDefault.Show(!connectivityInfoShown);
		m_DetailsBodyConnectivity.Show(connectivityInfoShown);
		
		if (Class.CastTo(p,tmp))
		{
			m_DetailsRoot.Show(true);
			m_DetailsText.Show(true);
			m_DetailsLabel.SetText(p.param1);
			m_DetailsText.SetText(p.param2);
			
			m_DetailsText.Update();
			m_DetailsLabel.Update();
			m_DetailsRoot.Update();
			m_DetailsBodyConnectivity.Update();
			return true;
		}
		return false;
	}
	
	bool IsChanged()
	{
		#ifdef PLATFORM_CONSOLE
			if (m_MaKOptionAvailable)
			{
				return (IsConsoleOptionChanged() || (m_KeyboardSelector.IsEnabled() && m_KeyboardOption.GetIndex() == 0) || (!m_KeyboardSelector.IsEnabled() && m_KeyboardOption.GetIndex() == 1));
			}
			else
			{
				return IsConsoleOptionChanged();
			}
		#else
			return IsOptionChanged();
		#endif
	}
	
	#ifdef PLATFORM_CONSOLE
	bool IsConsoleOptionChanged()
	{
		bool consoleOptionChanged = (m_ControllerLS_VSensitivitySelector.Changed() || m_ControllerLS_HSensitivitySelector.Changed() || m_ControllerLS_VehicleMod_HSensitivitySelector.Changed() || m_ControllerRS_VSensitivitySelector.Changed() || m_ControllerRS_HSensitivitySelector.Changed() || m_ControllerRS_CurvatureSelector.Changed() || m_ControllerRS_AimMod_VSensitivitySelector.Changed());
		if (m_MaKOptionAvailable)
		{
			return (IsOptionChanged() || consoleOptionChanged);
		}
		else
		{
			return consoleOptionChanged;
		}
	}
	#endif
	
	bool IsOptionChanged()
	{
		return (m_Mouse_VSensitivitySelector.Changed() || m_Mouse_HSensitivitySelector.Changed() || m_Mouse_AimMod_VSensitivitySelector.Changed() || m_Mouse_AimMod_HSensitivitySelector.Changed());
	}
	
	void Apply()
	{
		#ifdef PLATFORM_CONSOLE
		bool changed = false;
		if (m_MaKOptionAvailable)
		{
			//on change
			if ((m_KeyboardSelector.IsEnabled() && m_KeyboardOption.GetIndex() == 0) || (!m_KeyboardSelector.IsEnabled() && m_KeyboardOption.GetIndex() == 1))
			{
				m_KeyboardOption.Switch();
				changed = true;
			}
		}
		
		Focus();
		GetGame().GetUIManager().ShowUICursor( m_MaKOptionAvailable && m_KeyboardOption.GetIndex() );
		if (changed)
			g_Game.UpdateInputDeviceDisconnectWarning();
		
		m_Menu.Refresh();
		m_ControllerLS_VSensitivitySelector.Refresh();
		m_ControllerLS_HSensitivitySelector.Refresh();
		m_ControllerLS_VehicleMod_HSensitivitySelector.Refresh();
		m_ControllerRS_VSensitivitySelector.Refresh();
		m_ControllerRS_HSensitivitySelector.Refresh();
		m_ControllerRS_CurvatureSelector.Refresh();
		m_ControllerRS_AimMod_VSensitivitySelector.Refresh();
		m_ControllerRS_AimMod_HSensitivitySelector.Refresh();
		m_ControllerRS_AimMod_CurvatureSelector.Refresh();
		m_ControllerLS_DeadZoneSelector.Refresh();
		m_ControllerRS_DeadZoneSelector.Refresh();
		
		if (m_MaKOptionAvailable)
		{
		#endif
			m_Mouse_VSensitivitySelector.Refresh();
			m_Mouse_HSensitivitySelector.Refresh();
			m_Mouse_AimMod_VSensitivitySelector.Refresh();
			m_Mouse_AimMod_HSensitivitySelector.Refresh();
		#ifdef PLATFORM_CONSOLE
		}
		#endif
	}
	
	void Revert()
	{
		if(m_Mouse_InvertSelector)
			m_Mouse_InvertSelector.SetValue(m_Mouse_InvertOption.GetIndex(), true);

		if(m_Mouse_VSensitivitySelector)
		{
			m_Mouse_VSensitivitySelector.SetValue(m_Mouse_VSensitivityOption.ReadValue(), true);
			m_Mouse_VSensitivitySelector.SetValueText();
		}

		if(m_Mouse_HSensitivitySelector)
		{
			m_Mouse_HSensitivitySelector.SetValue(m_Mouse_HSensitivityOption.ReadValue(), true);
			m_Mouse_HSensitivitySelector.SetValueText();
		}

		if(m_Mouse_AimMod_VSensitivitySelector)
		{
			m_Mouse_AimMod_VSensitivitySelector.SetValue(m_Mouse_AimMod_VSensitivityOption.ReadValue(), true);
			m_Mouse_AimMod_VSensitivitySelector.SetValueText();
		}

		if(m_Mouse_AimMod_HSensitivitySelector)
		{
			m_Mouse_AimMod_HSensitivitySelector.SetValue(m_Mouse_AimMod_HSensitivityOption.ReadValue(), true);
			m_Mouse_AimMod_HSensitivitySelector.SetValueText();
		}

		#ifdef PLATFORM_CONSOLE
			if(m_KeyboardSelector)
				m_KeyboardSelector.SetValue(m_KeyboardOption.GetIndex(), true);
			if(m_AimHelperSelector)
				m_AimHelperSelector.SetValue(m_AimHelperOption.GetIndex(), true);

			if(m_ControllerLS_VSensitivitySelector)
			{
				m_ControllerLS_VSensitivitySelector.SetValue(m_ControllerLS_VSensitivityOption.ReadValue(), true);
				m_ControllerLS_VSensitivitySelector.SetValueText();
			}
		
			if(m_ControllerLS_HSensitivitySelector)
			{
				m_ControllerLS_HSensitivitySelector.SetValue(m_ControllerLS_HSensitivityOption.ReadValue(), true);
				m_ControllerLS_HSensitivitySelector.SetValueText();
			}

			if(m_ControllerLS_VehicleMod_HSensitivitySelector)
			{
				m_ControllerLS_VehicleMod_HSensitivitySelector.SetValue(m_ControllerLS_VehicleMod_HSensitivityOption.ReadValue(), true);
				m_ControllerLS_VehicleMod_HSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_InvertSelector)
				m_ControllerRS_InvertSelector.SetValue(m_ControllerRS_InvertOption.GetIndex(), true);

			if(m_ControllerRS_VSensitivitySelector)
			{
				m_ControllerRS_VSensitivitySelector.SetValue(m_ControllerRS_VSensitivityOption.ReadValue(), true);
				m_ControllerRS_VSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_HSensitivitySelector)
			{
				m_ControllerRS_HSensitivitySelector.SetValue(m_ControllerRS_HSensitivityOption.ReadValue(), true);
				m_ControllerRS_HSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_CurvatureSelector)
			{
				m_ControllerRS_CurvatureSelector.SetValue(m_ControllerRS_CurvatureOption.ReadValue(), true);
				m_ControllerRS_CurvatureSelector.SetValueText();
			}

			if(m_ControllerRS_AimMod_VSensitivitySelector)
			{
				m_ControllerRS_AimMod_VSensitivitySelector.SetValue(m_ControllerRS_AimMod_VSensitivityOption.ReadValue(), true);
				m_ControllerRS_AimMod_VSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_AimMod_HSensitivitySelector)
			{
				m_ControllerRS_AimMod_HSensitivitySelector.SetValue(m_ControllerRS_AimMod_HSensitivityOption.ReadValue(), true);
				m_ControllerRS_AimMod_HSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_AimMod_CurvatureSelector)
			{
				m_ControllerRS_AimMod_CurvatureSelector.SetValue(m_ControllerRS_AimMod_CurvatureOption.ReadValue(), true);
				m_ControllerRS_AimMod_CurvatureSelector.SetValueText();
			}

			if(m_ControllerLS_DeadZoneSelector)
			{
				m_ControllerLS_DeadZoneSelector.SetValue(m_ControllerLS_DeadZoneOption.ReadValue(), true);
				m_ControllerLS_DeadZoneSelector.SetValueText();
			}

			if(m_ControllerRS_DeadZoneSelector)
			{
				m_ControllerRS_DeadZoneSelector.SetValue(m_ControllerRS_DeadZoneOption.ReadValue(), true);
				m_ControllerRS_DeadZoneSelector.SetValueText();
			}
		#endif		
	}
	
	void SetToDefaults()
	{
		if(m_Mouse_InvertSelector)
			m_Mouse_InvertSelector.SetValue(m_Mouse_InvertOption.GetDefaultIndex(), true);
		
		if(m_Mouse_VSensitivitySelector)
		{
			m_Mouse_VSensitivitySelector.SetValue(m_Mouse_VSensitivityOption.GetDefault(), true);
			m_Mouse_VSensitivitySelector.SetValueText();
		}
		
		if(m_Mouse_HSensitivitySelector)
		{
			m_Mouse_HSensitivitySelector.SetValue(m_Mouse_HSensitivityOption.GetDefault(), true);
			m_Mouse_HSensitivitySelector.SetValueText();
		}
		
		if(m_Mouse_AimMod_VSensitivitySelector)
		{
			m_Mouse_AimMod_VSensitivitySelector.SetValue(m_Mouse_AimMod_VSensitivityOption.GetDefault(), true);
			m_Mouse_AimMod_VSensitivitySelector.SetValueText();
		}
		
		if(m_Mouse_AimMod_HSensitivitySelector)
		{
			m_Mouse_AimMod_HSensitivitySelector.SetValue(m_Mouse_AimMod_HSensitivityOption.GetDefault(), true);
			m_Mouse_AimMod_HSensitivitySelector.SetValueText();
		}

		#ifdef PLATFORM_CONSOLE
			if(m_KeyboardSelector)
				m_KeyboardSelector.SetValue(m_KeyboardOption.GetDefaultIndex(), true);
			if(m_AimHelperSelector)
				m_AimHelperSelector.SetValue(m_AimHelperOption.GetDefaultIndex(), true);
	
			if(m_ControllerLS_VSensitivitySelector)
			{
				m_ControllerLS_VSensitivitySelector.SetValue(m_ControllerLS_VSensitivityOption.GetDefault(), true);
				m_ControllerLS_VSensitivitySelector.SetValueText();
			}

			if(m_ControllerLS_HSensitivitySelector)
			{
				m_ControllerLS_HSensitivitySelector.SetValue(m_ControllerLS_HSensitivityOption.GetDefault(), true);
				m_ControllerLS_HSensitivitySelector.SetValueText();
			}
			
			if(m_ControllerLS_VehicleMod_HSensitivitySelector)
			{
				m_ControllerLS_VehicleMod_HSensitivitySelector.SetValue(m_ControllerLS_VehicleMod_HSensitivityOption.GetDefault(), true);
				m_ControllerLS_VehicleMod_HSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_InvertSelector)
				m_ControllerRS_InvertSelector.SetValue(m_ControllerRS_InvertOption.GetDefaultIndex(), true);

			if(m_ControllerRS_VSensitivitySelector)
			{
				m_ControllerRS_VSensitivitySelector.SetValue(m_ControllerRS_VSensitivityOption.GetDefault(), true);
				m_ControllerRS_VSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_HSensitivitySelector)
			{
				m_ControllerRS_HSensitivitySelector.SetValue(m_ControllerRS_HSensitivityOption.GetDefault(), true);
				m_ControllerRS_HSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_CurvatureSelector)
			{
				m_ControllerRS_CurvatureSelector.SetValue(m_ControllerRS_CurvatureOption.GetDefault(), true);
				m_ControllerRS_CurvatureSelector.SetValueText();
			}

			if(m_ControllerRS_AimMod_VSensitivitySelector)
			{
				m_ControllerRS_AimMod_VSensitivitySelector.SetValue(m_ControllerRS_AimMod_VSensitivityOption.GetDefault(), true);
				m_ControllerRS_AimMod_VSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_AimMod_HSensitivitySelector)
			{
				m_ControllerRS_AimMod_HSensitivitySelector.SetValue(m_ControllerRS_AimMod_HSensitivityOption.GetDefault(), true);
				m_ControllerRS_AimMod_HSensitivitySelector.SetValueText();
			}

			if(m_ControllerRS_AimMod_CurvatureSelector)
			{
				m_ControllerRS_AimMod_CurvatureSelector.SetValue(m_ControllerRS_AimMod_CurvatureOption.GetDefault(), true);
				m_ControllerRS_AimMod_CurvatureSelector.SetValueText();
			}

			if(m_ControllerLS_DeadZoneSelector)
			{
				m_ControllerLS_DeadZoneSelector.SetValue(m_ControllerLS_DeadZoneOption.GetDefault(), true);
				m_ControllerLS_DeadZoneSelector.SetValueText();
			}

			if(m_ControllerRS_DeadZoneSelector)
			{
				m_ControllerRS_DeadZoneSelector.SetValue(m_ControllerRS_DeadZoneOption.GetDefault(), true);
				m_ControllerRS_DeadZoneSelector.SetValueText();
			}

			Focus();
		#endif
		
	}
	
#ifdef PLATFORM_CONSOLE
	void ShowConsoleSensitivityOptions(int index)
	{
		m_ConsoleMouseSensitivityWidget.Show(index == 1 && m_MaKOptionAvailable);
	}
	
	void UpdateKeyboard( int index )
	{
		Focus();
		ShowConsoleSensitivityOptions(index);
		m_Menu.OnChanged();
		//m_Menu.ToggleDependentOptions(EDependentOptions.MOUSEANDKEYBOARD_QUICKBAR,index == 1 && GetGame().GetInput().IsEnabledMouseAndKeyboardEvenOnServer());
	}
	
	void UpdateAimHelper( int index )
	{
		if ( m_AimHelperOption.GetIndex() != index )
		{
			m_AimHelperOption.Switch();
			m_Menu.OnChanged();
		}
	}
	
	void UpdateControllerLS_VSensitivity( float value )
	{
		m_ControllerLS_VSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateControllerLS_HSensitivity( float value )
	{
		m_ControllerLS_HSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateControllerLS_VehicleMod_HSensitivity( float value )
	{
		m_ControllerLS_VehicleMod_HSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}

	void UpdateControllerRS_InvertView( int index )
	{
		if ( m_ControllerRS_InvertOption.GetIndex() != index )
		{
			m_ControllerRS_InvertOption.Switch();
			m_Menu.OnChanged();
		}
	}
	
	void UpdateControllerRS_VSensitivity( float value )
	{
		m_ControllerRS_VSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateControllerRS_HSensitivity( float value )
	{
		m_ControllerRS_HSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateControllerRS_Curvature( float value )
	{
		m_ControllerRS_CurvatureOption.WriteValue( value );
		m_Menu.OnChanged();
	}

	void UpdateControllerLS_Deadzone( float value )
	{
		m_ControllerLS_DeadZoneOption.WriteValue( value );
		m_Menu.OnChanged();
	}
		
	void UpdateControllerRS_Deadzone( float value )
	{
		m_ControllerRS_DeadZoneOption.WriteValue( value );
		m_Menu.OnChanged();
	}

	void UpdateControllerRS_AimMod_VSensitivity( float value )
	{
		m_ControllerRS_AimMod_VSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateControllerRS_AimMod_HSensitivity( float value )
	{
		m_ControllerRS_AimMod_HSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateControllerRS_AimMod_Curvature( float value )
	{
		m_ControllerRS_AimMod_CurvatureOption.WriteValue( value );
		m_Menu.OnChanged();
	}
#endif
	
	void UpdateMouseInvertView( int index )
	{
		if ( m_Mouse_InvertOption.GetIndex() != index )
		{
			m_Mouse_InvertOption.Switch();
			m_Menu.OnChanged();
		}
	}
	
	void UpdateMouseVSensitivity( float value )
	{
		m_Mouse_VSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateMouseHSensitivity( float value )
	{
		m_Mouse_HSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateMouseAimModVSensitivity( float value )
	{
		m_Mouse_AimMod_VSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void UpdateMouseAimModHSensitivity( float value )
	{
		m_Mouse_AimMod_HSensitivityOption.WriteValue( value );
		m_Menu.OnChanged();
	}
	
	void ReloadOptions()
	{
		m_Menu.ReloadOptions();
	}
	
	void SetOptions( GameOptions options )
	{
		m_Options = options;

		// mouse (PC & consoles)		
		m_Mouse_InvertOption							= SwitchOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS_INVERTED ) );
		m_Mouse_VSensitivityOption						= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS ) );
		m_Mouse_HSensitivityOption						= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_MOUSE_XAXIS ) );
		m_Mouse_AimMod_VSensitivityOption				= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS_AIM_MOD ) );
		m_Mouse_AimMod_HSensitivityOption				= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_MOUSE_XAXIS_AIM_MOD ) );
		
		// controller (consoles only)
		#ifdef PLATFORM_CONSOLE
			m_KeyboardOption							= SwitchOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_MOUSE_AND_KEYBOARD ) );
			m_AimHelperOption							= SwitchOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_AIM_HELPER ) );

			m_ControllerLS_VSensitivityOption			= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_YAXIS ) );
			m_ControllerLS_HSensitivityOption			= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_XAXIS ) );
			m_ControllerLS_VehicleMod_HSensitivityOption = NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_XAXIS_VEHICLE_MOD ) );
			m_ControllerRS_InvertOption					= SwitchOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS_INVERTED ) );
			m_ControllerRS_VSensitivityOption			= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS ) );
			m_ControllerRS_HSensitivityOption			= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_XAXIS ) );
			m_ControllerRS_CurvatureOption				= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_CURVATURE ) );
			m_ControllerRS_AimMod_VSensitivityOption	= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS_AIM_MOD ) );
			m_ControllerRS_AimMod_HSensitivityOption	= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_XAXIS_AIM_MOD ) );
			m_ControllerRS_AimMod_CurvatureOption		= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_CURVATURE_AIM_MOD ) );
		    m_ControllerLS_DeadZoneOption				= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_DEADZONE ) );
		    m_ControllerRS_DeadZoneOption				= NumericOptionsAccess.Cast( m_Options.GetOptionByType( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_DEADZONE ) );
		#endif
				
		Revert();
	}
	
	void ToggleDependentOptions(int mode, bool state)
	{
	}
	
	void InitDependentOptionsVisibility()
	{
	}
	
	void FillTextMap()
	{
		m_TextMap = new map<int, ref Param2<string, string>>;
		m_TextMap.Insert( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS_INVERTED, new Param2<string, string>( "#STR_Invert_Vertical_tip_header", "#STR_Invert_Vertical_tip" ) );
		m_TextMap.Insert( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS, new Param2<string, string>( "#STR_Camera_V_Sensitivity_tip_header", "#STR_Camera_V_Sensitivity_tip" ) );
		m_TextMap.Insert( OptionAccessType.AT_OPTIONS_MOUSE_XAXIS, new Param2<string, string>( "#STR_Camera_H_Sensitivity_tip_header", "#STR_Camera_H_Sensitivity_tip" ) );
		m_TextMap.Insert( OptionAccessType.AT_OPTIONS_MOUSE_YAXIS_AIM_MOD, new Param2<string, string>( "#STR_Aiming_V_Sensitivity_tip_header", "#STR_Aiming_V_Sensitivity_tip" ) );
		m_TextMap.Insert( OptionAccessType.AT_OPTIONS_MOUSE_XAXIS_AIM_MOD, new Param2<string, string>( "#STR_Aiming_H_Sensitivity_tip_header", "#STR_Aiming_H_Sensitivity_tip" ) );

		#ifdef PLATFORM_CONSOLE
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_MOUSE_AND_KEYBOARD, new Param2<string, string>( "#xbox_options_controls_mandk_contr", "#xbox_options_controls_mandk_contr_desc" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_AIM_HELPER, new Param2<string, string>( "#ps4_options_controls_aim_helper_contr", "#ps4_options_controls_aim_helper_contr_desc" ) );
			
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_YAXIS, new Param2<string, string>( "#STR_Movement_V_Sensitivity_tip_header", "#STR_Movement_V_Sensitivity_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_XAXIS, new Param2<string, string>( "#STR_Movement_H_Sensitivity_tip_header", "#STR_Movement_H_Sensitivity_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_XAXIS_VEHICLE_MOD, new Param2<string, string>( "#STR_Movement_Vehicle_Sensitivity_tip_header", "#STR_Movement_Vehicle_Sensitivity_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS_INVERTED, new Param2<string, string>( "#STR_Invert_Vertical_tip_header", "#STR_Invert_Vertical_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS, new Param2<string, string>( "#STR_Camera_V_Sensitivity_tip_header", "#STR_Camera_V_Sensitivity_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_XAXIS, new Param2<string, string>( "#STR_Camera_H_Sensitivity_tip_header", "#STR_Camera_H_Sensitivity_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_CURVATURE, new Param2<string, string>( "#STR_Camera_Curvature_tip_header", "#STR_Camera_Curvature_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_YAXIS_AIM_MOD, new Param2<string, string>( "#STR_Aiming_V_Sensitivity_tip_header", "#STR_Aiming_V_Sensitivity_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_XAXIS_AIM_MOD, new Param2<string, string>( "#STR_Aiming_H_Sensitivity_tip_header", "#STR_Aiming_H_Sensitivity_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_CURVATURE_AIM_MOD, new Param2<string, string>( "#STR_Aiming_Curvature_tip_header", "#STR_Camera_Curvature_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_LS_DEADZONE, new Param2<string, string>( "#STR_Aiming_L_Deadzone_tip_header", "#STR_Aiming_L_Deadzone_tip" ) );
			m_TextMap.Insert( OptionAccessType.AT_OPTIONS_CONTROLLER_RS_DEADZONE, new Param2<string, string>( "#STR_Aiming_R_Deadzone_tip_header", "#STR_Aiming_R_Deadzone_tip" ) );
		#endif
	}
	
	//Coloring functions (Until WidgetStyles are useful)
	void ColorRed( Widget w )
	{
		SetFocus( w );
		ButtonWidget button;
		if (Class.CastTo(button,w))
		{
			button.SetTextColor( ARGB( 255, 255, 0, 0 ) );
			button.SetAlpha( 0.9 );
		}
	}
	
	void ColorWhite( Widget w, Widget enterW )
	{
		ButtonWidget button;
		if (Class.CastTo(button,w))
		{
			button.SetTextColor( ARGB( 255, 255, 255, 255 ) );
			button.SetAlpha( 0.0 );
		}
	}
	
	void SetActiveOption(OptionSelectorBase option)
	{
		m_ActiveOption = option;
	}
	
	OptionSelectorBase GetActiveOption()
	{
		return m_ActiveOption;
	}
}
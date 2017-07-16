//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <cdll_client_int.h>
#include <globalvars_base.h>
#include <cdll_util.h>
#include <KeyValues.h>

#include "covenantgui.h"
#include "in_buttons.h"
#include "IGameUIFuncs.h" // for key bindings
#include "cam_thirdperson.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/TextImage.h>

#include <stdio.h> // _snprintf define

#include <game/client/iviewport.h>
#include "commandmenu.h"
#include "hltvcamera.h"
#if defined( REPLAY_ENABLED )
#include "replay/replaycamera.h"
#endif

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include <igameresources.h>

#ifdef TF_CLIENT_DLL
#include "tf_gamerules.h"
void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

CCovenantGUI *g_pCovenantGUI = NULL;


using namespace vgui;

enum DragStatus {
	LMOUSE_DOWN = 1,
	LMOUSE_DRAG,
	RMOUSE_DOWN,
	RMOUSE_DRAG
};

//-----------------------------------------------------------------------------
// main spectator panel



//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCovenantGUI::CCovenantGUI(IViewPort *pViewPort) : Frame(NULL, PANEL_COVENANTGUI)
{
// 	m_bHelpShown = false;
//	m_bInsetVisible = false;
//	m_iDuckKey = KEY_NONE;
	SetSize( 10, 10 ); // Quiet "parent not sized yet" spew
	m_bSpecScoreboard = false;

	m_pViewPort = pViewPort;
	g_pCovenantGUI = this;

	// initialize dialog
	SetVisible(false);
	SetProportional(true);

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( false );

	m_pTopBar = new Panel( this, "topbar" );
 	m_pBottomBarBlank = new Panel( this, "bottombarblank" );

	// m_pBannerImage = new ImagePanel( m_pTopBar, NULL );
	m_pPlayerLabel = new Label( this, "playerlabel", "" );
	m_pPlayerLabel->SetVisible( false );
	TextImage *image = m_pPlayerLabel->GetTextImage();
	if ( image )
	{
		HFont hFallbackFont = scheme()->GetIScheme( GetScheme() )->GetFont( "DefaultVerySmallFallBack", false );
		if ( INVALID_FONT != hFallbackFont )
		{
			image->SetUseFallbackFont( true, hFallbackFont );
		}
	}

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	// m_pBannerImage->SetVisible(false);
	InvalidateLayout();

	m_iDragStatus = 0;
	m_flDragTrigger = 0.0f;
	m_flMWHEELTrigger = 0.0f;
	m_flLastClick = 0.0f;
	m_iButtons = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCovenantGUI::~CCovenantGUI()
{
	g_pCovenantGUI = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the colour of the top and bottom bars
//-----------------------------------------------------------------------------
void CCovenantGUI::ApplySchemeSettings(IScheme *pScheme)
{
	KeyValues *pConditions = NULL;

#ifdef TF_CLIENT_DLL
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_mvm" );
	}
#endif

	LoadControlSettings( GetResFile(), NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_pBottomBarBlank->SetVisible( true );
	m_pTopBar->SetVisible( true );

	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor(Color( 0,0,0,0 ) ); // make the background transparent
	m_pTopBar->SetBgColor(GetBlackBarColor());
	m_pBottomBarBlank->SetBgColor(GetBlackBarColor());
	// m_pBottomBar->SetBgColor(Color( 0,0,0,0 ));
	SetPaintBorderEnabled(false);

	SetBorder( NULL );

#ifdef CSTRIKE_DLL
	SetZPos(80);	// guarantee it shows above the scope
#endif
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CCovenantGUI::PerformLayout()
{
	int w,h,x,y;
	GetHudSize(w, h);
	
	// fill the screen
	SetBounds(0,0,w,h);

	// stretch the bottom bar across the screen
	m_pBottomBarBlank->GetPos(x,y);
	m_pBottomBarBlank->SetSize( w, h - y );
}

//-----------------------------------------------------------------------------
// Purpose: checks spec_scoreboard cvar to see if the scoreboard should be displayed
//-----------------------------------------------------------------------------
void CCovenantGUI::OnThink()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if (!pPlayer)
		return;

	if (m_flMWHEELTrigger > 0.0f)
	{
		if (gpGlobals->curtime > m_flMWHEELTrigger)
		{
			m_flMWHEELTrigger = 0.0f;
			if (m_iButtons & (1<<5))
			{
				OnMouseReleased(MOUSE_WHEEL_UP);
			}
			if (m_iButtons & (1<<6))
			{
				OnMouseReleased(MOUSE_WHEEL_DOWN);
			}
		}
	}
	if (m_iDragStatus > 0)
	{
		if ((m_iButtons & (1 << 2)) && !input()->IsMouseDown(MOUSE_MIDDLE))
		{
			OnMouseReleased(MOUSE_MIDDLE);
		}
		if ((m_iButtons & (1 << 3)) && !input()->IsMouseDown(MOUSE_4))
		{
			OnMouseReleased(MOUSE_4);
		}
		if ((m_iButtons & (1 << 4)) && !input()->IsMouseDown(MOUSE_5))
		{
			OnMouseReleased(MOUSE_5);
		}

		if (m_iDragStatus == LMOUSE_DOWN)
		{
			int mousex, mousey;
			input()->GetCursorPosition(mousex, mousey);
			if (m_iDragMouseX != mousex || m_iDragMouseY != mousey)
			{
				m_iDragStatus++;
				m_flDragTrigger = gpGlobals->curtime;
				SetMouseInputEnabled(false);
				g_ThirdPersonManager.SetFreeCam(true);
			}
		}
		else if (m_iDragStatus == RMOUSE_DOWN)
		{
			int mousex, mousey;
			input()->GetCursorPosition(mousex, mousey);
			if (m_iDragMouseX != mousex || m_iDragMouseY != mousey)
			{
				m_iDragStatus++;
				m_flDragTrigger = gpGlobals->curtime;
				SetMouseInputEnabled(false);
				//BB: reset the angles
				if (idealpitch && idealyaw)
				{
					if (idealpitch->GetFloat() != 0.0f || idealyaw->GetFloat() != 0.0f)
					{
						QAngle viewangles;
						engine->GetViewAngles(viewangles);
						viewangles[PITCH] += idealpitch->GetFloat();
						viewangles[YAW] += idealyaw->GetFloat();
						g_ThirdPersonManager.LockCam(true);
						idealpitch->SetValue(0);
						idealyaw->SetValue(0);
						char szbuf[32];
						Q_snprintf(szbuf, sizeof(szbuf), "setang %f %f", viewangles[PITCH], viewangles[YAW]);
						engine->ClientCmd(szbuf);
					}
				}
			}
		}

		if (m_iDragStatus == RMOUSE_DRAG && !::input()->IsMouseDown(MOUSE_RIGHT))
		{
			m_iDragStatus = 0;
			input()->SetCursorPos(m_iDragMouseX, m_iDragMouseY);
			//ConMsg("Set mouse pos %d %d\n", m_iDragMouseX, m_iDragMouseY);
			SetMouseInputEnabled(true);
		}
		else if (m_iDragStatus == RMOUSE_DOWN && !::input()->IsMouseDown(MOUSE_RIGHT))
		{
			m_iDragStatus = 0;
		}

		if (m_iDragStatus == LMOUSE_DRAG && !input()->IsMouseDown(MOUSE_LEFT))
		{
			m_iDragStatus = 0;
			input()->SetCursorPos(m_iDragMouseX, m_iDragMouseY);
			SetMouseInputEnabled(true); 
			g_ThirdPersonManager.SetFreeCam(false);
		}
		else if (m_iDragStatus == LMOUSE_DOWN && !input()->IsMouseDown(MOUSE_LEFT))
		{
			m_iDragStatus = 0;
		}
	}

	BaseClass::OnThink();
}

//-----------------------------------------------------------------------------
// Purpose: sets the image to display for the banner in the top right corner
//-----------------------------------------------------------------------------
void CCovenantGUI::SetLogoImage(const char *image)
{
	if ( m_pBannerImage )
	{
		m_pBannerImage->SetImage( scheme()->GetImage(image, false) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CCovenantGUI::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CCovenantGUI::SetLabelText(const char *textEntryName, wchar_t *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CCovenantGUI::MoveLabelToFront(const char *textEntryName)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->MoveToFront();
	}
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CCovenantGUI::ShowPanel(bool bShow)
{
	if ( bShow && !IsVisible() )
	{
		m_bSpecScoreboard = false;
	}
	SetVisible( bShow );
	if ( !bShow && m_bSpecScoreboard )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, false );
	}
}

bool CCovenantGUI::ShouldShowPlayerLabel(int specmode)
{
	return ( (specmode == OBS_MODE_IN_EYE) ||	(specmode == OBS_MODE_CHASE) );
}
//-----------------------------------------------------------------------------
// Purpose: Updates the gui, rearranges elements
//-----------------------------------------------------------------------------
void CCovenantGUI::Update()
{
	int wide, tall;
	int bx, by, bwide, btall;

	GetHudSize(wide, tall);
	m_pTopBar->GetBounds( bx, by, bwide, btall );

	IGameResources *gr = GameResources();
	int specmode = GetSpectatorMode();
	int playernum = GetSpectatorTarget();

	IViewPortPanel *overview = gViewPortInterface->FindPanelByName( PANEL_OVERVIEW );

	if ( overview && overview->IsVisible() )
	{
		int mx, my, mwide, mtall;

		VPANEL p = overview->GetVPanel();
		ipanel()->GetPos( p, mx, my );
		ipanel()->GetSize( p, mwide, mtall );
				
		if ( my < btall )
		{
			// reduce to bar 
			m_pTopBar->SetSize( wide - (mx + mwide), btall );
			m_pTopBar->SetPos( (mx + mwide), 0 );
		}
		else
		{
			// full top bar
			m_pTopBar->SetSize( wide , btall );
			m_pTopBar->SetPos( 0, 0 );
		}
	}
	else
	{
		// full top bar
		m_pTopBar->SetSize( wide , btall ); // change width, keep height
		m_pTopBar->SetPos( 0, 0 );
	}

	m_pPlayerLabel->SetVisible( ShouldShowPlayerLabel(specmode) );

	// update player name filed, text & color

	if ( playernum > 0 && playernum <= gpGlobals->maxClients && gr )
	{
		Color c = gr->GetTeamColor( gr->GetTeam(playernum) ); // Player's team color

		m_pPlayerLabel->SetFgColor( c );
		
		wchar_t playerText[ 80 ], playerName[ 64 ], health[ 10 ];
		V_wcsncpy( playerText, L"Unable to find #Spec_PlayerItem*", sizeof( playerText ) );
		memset( playerName, 0x0, sizeof( playerName ) );

		g_pVGuiLocalize->ConvertANSIToUnicode( UTIL_SafeName(gr->GetPlayerName( playernum )), playerName, sizeof( playerName ) );
		int iHealth = gr->GetHealth( playernum );
		if ( iHealth > 0  && gr->IsAlive(playernum) )
		{
			_snwprintf( health, ARRAYSIZE( health ), L"%i", iHealth );
			g_pVGuiLocalize->ConstructString( playerText, sizeof( playerText ), g_pVGuiLocalize->Find( "#Spec_PlayerItem_Team" ), 2, playerName,  health );
		}
		else
		{
			g_pVGuiLocalize->ConstructString( playerText, sizeof( playerText ), g_pVGuiLocalize->Find( "#Spec_PlayerItem" ), 1, playerName );
		}

		m_pPlayerLabel->SetText( playerText );
	}
	else
	{
		m_pPlayerLabel->SetText( L"" );
	}

	// update extra info field
	wchar_t szEtxraInfo[1024];
	wchar_t szTitleLabel[1024];
	char tempstr[128];

	if ( engine->IsHLTV() )
	{
		// set spectator number and HLTV title
		Q_snprintf(tempstr,sizeof(tempstr),"Spectators : %d", HLTVCamera()->GetNumSpectators() );
		g_pVGuiLocalize->ConvertANSIToUnicode(tempstr,szEtxraInfo,sizeof(szEtxraInfo));
		
		Q_strncpy( tempstr, HLTVCamera()->GetTitleText(), sizeof(tempstr) );
		g_pVGuiLocalize->ConvertANSIToUnicode(tempstr,szTitleLabel,sizeof(szTitleLabel));
	}
	else
	{
		// otherwise show map name
		Q_FileBase( engine->GetLevelName(), tempstr, sizeof(tempstr) );

		wchar_t wMapName[64];
		g_pVGuiLocalize->ConvertANSIToUnicode(tempstr,wMapName,sizeof(wMapName));
		g_pVGuiLocalize->ConstructString( szEtxraInfo,sizeof( szEtxraInfo ), g_pVGuiLocalize->Find("#Spec_Map" ),1, wMapName );

		g_pVGuiLocalize->ConvertANSIToUnicode( "" ,szTitleLabel,sizeof(szTitleLabel));
	}

	SetLabelText("extrainfo", szEtxraInfo );
	SetLabelText("titlelabel", szTitleLabel );

	idealdist = cvar->FindVar("cam_idealdist");
	delta = cvar->FindVar("cam_idealdelta");
	mindist = cvar->FindVar("c_mindistance");
	maxdist = cvar->FindVar("c_maxdistance");
	idealpitch = cvar->FindVar("cam_idealpitch");
	idealyaw = cvar->FindVar("cam_idealyaw");
	camadjust = cvar->FindVar("cam_adjust");
}

//-----------------------------------------------------------------------------
// Purpose: Updates the timer label if one exists
//-----------------------------------------------------------------------------
void CCovenantGUI::UpdateTimer()
{
	wchar_t szText[ 63 ];

	int timer = 0;

	V_swprintf_safe ( szText, L"%d:%02d\n", (timer / 60), (timer % 60) );

	SetLabelText("timerlabel", szText );
}

void CCovenantGUI::ChangeView(int factor)
{
	if (idealdist && delta && mindist && maxdist)
	{
		float dist = idealdist->GetFloat() + factor*delta->GetFloat();
		if (dist > maxdist->GetFloat())
			dist = maxdist->GetFloat();
		else if (dist < mindist->GetFloat())
			dist = mindist->GetFloat();
		idealdist->SetValue(dist);
	}
}

void CCovenantGUI::OnMouseWheeled(int delta)
{
	m_flMWHEELTrigger = gpGlobals->curtime;
	const char* currentBinding;
	if (delta > 0)
	{
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_WHEEL_UP);
		if (currentBinding)
		{
			engine->ClientCmd(currentBinding);
			m_iButtons |= (1<<5);
		}
	}
	else
	{
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_WHEEL_DOWN);
		if (currentBinding)
		{
			engine->ClientCmd(currentBinding);
			m_iButtons |= (1<<6);
		}
	}
}

void CCovenantGUI::OnMousePressed(MouseCode code)
{
	//TGB: just making sure
	if (IsVisible() == false)
		return;

	int mousex, mousey;
	input()->GetCursorPosition(mousex, mousey);
	//ConMsg("Click at %f, %d %d\n", gpGlobals->curtime, mousex, mousey);

	//Check for doubleclick
	if (m_flLastClick == gpGlobals->curtime)
		return;
	else
		m_flLastClick = gpGlobals->curtime;



	if (m_iDragStatus > 0)
		return;

	m_iDragMouseX = mousex;
	m_iDragMouseY = mousey;

	//DevMsg("\nGrabbed mouse as x: %i, y: %i ; mx %i my %i", mousex, mousey, m_iDragMouseX, m_iDragMouseY);
	//TGB: not sure why this is here, for if we start a drag, I assume
	//BBm_HudBoxSelect = GET_HUDELEMENT(CHudBoxSelect);

	const char* currentBinding;

	if (gameuifuncs == NULL) return; //TGB: again, just making sure

	switch (code)
	{
	case MOUSE_LEFT:
		m_iDragStatus = LMOUSE_DOWN;
		break;
	case MOUSE_RIGHT:
		m_iDragStatus = RMOUSE_DOWN;
		break;
	case MOUSE_MIDDLE:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_MIDDLE);
		if (currentBinding)
		{
			engine->ClientCmd(currentBinding);
			m_iButtons |= (1<<2);
		}
		break;
	case MOUSE_4:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_4);
		if (currentBinding)
		{
			engine->ClientCmd(currentBinding);
			m_iButtons |= (1<<3);
		}
		break;
	case MOUSE_5:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_5);
		if (currentBinding)
		{
			engine->ClientCmd(currentBinding);
			m_iButtons |= (1<<4);
		}
		break;
	default:
		return;
	}

	/*BBif (Q_strcmp(currentBinding, "+attack") == 0)
	{
		//TGB: alt select exp.
		if (alternateselect.GetBool())
		{
			if ((g_iCommandMode == MODE_POWER_PHYSEXP) ||
				(g_iCommandMode == MODE_AMBUSH_CREATE) ||
				(g_iCommandMode == MODE_POWER_SPOTCREATE) ||
				(g_iCommandMode == MODE_RALLY) ||
				(g_iCommandMode == MODE_TRAP))
			{
				TraceScreenToWorld(mousex, mousey, 1);
			}
			else
			{
				//TGB: we're not selecting anything until we release the button
				//WorldToScreenSelect(mousex, mousey, 1);
				m_iDragStatus = 1; //mouse is down, may start dragging
				m_flDragTrigger = gpGlobals->curtime + ZM_BOXSELECT_DELAY;

			}
		}
		else
			TraceScreenToWorld(mousex, mousey, 1);

		return;
	}

	//right click
	if (Q_strcmp(currentBinding, "+attack2") == 0)
	{
		//user may be starting a drag
		m_iDragStatus = RMOUSE_DOWN;
		//identical delay to boxselect
		m_flDragTrigger = gpGlobals->curtime + ZM_BOXSELECT_DELAY;

		//moved to mouserelease
		//TraceScreenToWorld(mousex, mousey, 2);
		return;
	}
	//middle click
	if (Q_strcmp(currentBinding, "+attack3") == 0)
	{
		TraceScreenToWorld(mousex, mousey, 3);
		return;
	}*/

}

void CCovenantGUI::OnMouseReleased(MouseCode code)
{
	if (IsVisible() == false)
		return;

	int mousex, mousey;
	input()->GetCursorPosition(mousex, mousey);
	//::input->GetFullscreenMousePos(&mousex, &mousey);

	const char* currentBinding;

	switch (code)
	{
	case MOUSE_MIDDLE:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_MIDDLE);
		if (currentBinding)
		{
			if (currentBinding[0] == '+')
			{
				char temp[32];
				Q_strcpy(temp, currentBinding);
				temp[0] = '-';
				engine->ClientCmd(temp);
			}
			m_iButtons &= ~(1<<2);
		}
		break;
	case MOUSE_4:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_4);
		if (currentBinding)
		{
			if (currentBinding[0] == '+')
			{
				char temp[32];
				Q_strcpy(temp, currentBinding);
				temp[0] = '-';
				engine->ClientCmd(temp);
			}
			m_iButtons &= ~(1<<3);
		}
		break;
	case MOUSE_5:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_5);
		if (currentBinding)
		{
			if (currentBinding[0] == '+')
			{
				char temp[32];
				Q_strcpy(temp, currentBinding);
				temp[0] = '-';
				engine->ClientCmd(temp);
			}
			m_iButtons &= ~(1<<4);
		}
		break;
	case MOUSE_WHEEL_UP:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_WHEEL_UP);
		if (currentBinding)
		{
			if (currentBinding[0] == '+')
			{
				char temp[32];
				Q_strcpy(temp, currentBinding);
				temp[0] = '-';
				engine->ClientCmd(temp);
			}
		}
		m_iButtons &= ~(1<<5);
		break;
	case MOUSE_WHEEL_DOWN:
		currentBinding = gameuifuncs->GetBindingForButtonCode(MOUSE_WHEEL_DOWN);
		if (currentBinding)
		{
			if (currentBinding[0] == '+')
			{
				char temp[32];
				Q_strcpy(temp, currentBinding);
				temp[0] = '-';
				engine->ClientCmd(temp);
			}
		}
		m_iButtons &= ~(1<<6);
		break;
	default:
		return;
	}




	//handle tooltip

	//get tooltip pointer
	//CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	//BBCHudToolTip *m_pZMTooltip = GET_HUDELEMENT(CHudToolTip);
	//only set and display tooltip if we have the ptrs and if we're not already displaying
	/*BBif (m_iDragStatus > 1 && m_pZMTooltip && m_pZMTooltip->IsHidden() == false)
	{
		//DevMsg("Tooltip cleared in mousereleased\n");
		m_pZMTooltip->ClearTooltip();
		//pPlayer->m_Local.m_iHideHUD |= HIDEHUD_TOOLTIP;
		m_pZMTooltip->Hide(true);
	}

	if (m_iDragStatus == LMOUSE_DOWN) //if we left clicked, normal select
	{
		//DevMsg("Normal select\n");

		//typeselect?
		CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if (pPlayer && pPlayer->m_nButtons & IN_JUMP) //are pressing duck?
		{
			//we need to select all of type on screen
			DevMsg("Type-select detected!\n");

			//stickystyle?
			int stick_send = 0;
			if (pPlayer && pPlayer->m_nButtons & IN_DUCK)
				stick_send = 1;

			//here we go again
			const VMatrix &worldToScreen = engine->WorldToScreenMatrix();

			//send the server the goods
			engine->ClientCmd(VarArgs("zm_typeselect_cc %i %i %f %f %f %f %f %f %f %f %f %f %f %f %i %i %i",
				mousex,
				mousey,
				worldToScreen[0][0],
				worldToScreen[0][1],
				worldToScreen[0][2],
				worldToScreen[0][3],
				worldToScreen[1][0],
				worldToScreen[1][1],
				worldToScreen[1][2],
				worldToScreen[1][3],
				//we don't need row 2 'cause we don't need z
				worldToScreen[3][0],
				worldToScreen[3][1],
				worldToScreen[3][2],
				worldToScreen[3][3],
				ScreenWidth(),
				ScreenHeight(),
				stick_send
				));

		}
		else
		{
			if (pPlayer && pPlayer->m_nButtons & IN_DUCK)
				WorldToScreenSelect(mousex, mousey, 1, true); //sticky select
			else
				WorldToScreenSelect(mousex, mousey, 1, false); //normal select
		}
	}
	else if (m_iDragStatus == LMOUSE_DRAG) //if we finished drawing a box, process selection here
	{
		//DevMsg("Ended boxselect drag\n");
		BoxSelect(mousex, mousey);
	}


	if (m_iDragStatus == RMOUSE_DOWN)
	{
		//command click
		TraceScreenToWorld(mousex, mousey, 2);
	}
	else if (m_iDragStatus == RMOUSE_DRAG)
	{
		//line formation drag
		DevMsg("Ended RDrag\n");
		LineCommand(mousex, mousey);
	}

	m_iDragStatus = 0; //mouse is up, no dragging anymore

	if (m_HudBoxSelect) //TGB: this part was unsafe, added a check
	{
		m_HudBoxSelect->m_bDrawBox = false;
		m_HudBoxSelect->m_bDrawLine = false;
	}*/
}

//TGB: box select stuff
void CCovenantGUI::OnCursorMoved(int x, int y)
{
	//BBidleData.MouseMoved(); //TGB: tell our idletracker that the mouse has been moved

	//this dragstatus thing is kinda redundant now that there's the trigger delay
	if (m_iDragStatus < 1)
		return; //only interested in this if mousebutton is down

	/*BBif (m_flDragTrigger < gpGlobals->curtime && m_iDragStatus == LMOUSE_DOWN)
	{
		//DevMsg("We're dragging LMOUSE. %i %i\n", x,y);
		m_iDragStatus = LMOUSE_DRAG;
	}
	else if (m_flDragTrigger < gpGlobals->curtime && m_iDragStatus == RMOUSE_DOWN)
	{
		//DevMsg("We're dragging RMOUSE. %i %i\n", x,y);
		m_iDragStatus = RMOUSE_DRAG;
	}

	//if we're dragging, do a tooltip
	if (m_iDragStatus > 1 && zmtips.GetBool() == true)
	{
		//get tooltip pointer
		//CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		CHudToolTip *m_pZMTooltip = GET_HUDELEMENT(CHudToolTip);
		//only set and display tooltip if we have the ptrs and if we're not already displaying
		if (m_pZMTooltip && m_pZMTooltip->IsHidden())
		{
			//DevMsg("Inside cursormoved, called SetToolTip at %i\n", (int)gpGlobals->curtime);

			//TGB: these don't need to be timed, but handy for testing the function
			if (m_iDragStatus == LMOUSE_DRAG)
				m_pZMTooltip->SetTooltipTimed("Box select: Drag the left mouse button to draw a selection area. Release to select all units in that area.", GetVPanel(), 3);
			else if (m_iDragStatus == RMOUSE_DRAG)
				m_pZMTooltip->SetTooltipTimed("Form line: Drag the right mouse button to draw a line. Release to order units to take up positions along the line.", GetVPanel(), 3);

			//pPlayer->m_Local.m_iHideHUD &= ~HIDEHUD_TOOLTIP;
		}
	}

	//draw a nice rectangle
	if (m_HudBoxSelect)
	{
		if (m_iDragStatus == LMOUSE_DRAG)
		{
			m_HudBoxSelect->m_bDrawBox = true;
			m_HudBoxSelect->m_bDrawLine = false;
		}
		else if (m_iDragStatus == RMOUSE_DRAG)
		{
			m_HudBoxSelect->m_bDrawLine = true;
			m_HudBoxSelect->m_bDrawBox = false;
		}
		m_HudBoxSelect->m_iDragMouseX = m_iDragMouseX;
		m_HudBoxSelect->m_iDragMouseY = m_iDragMouseY;
	}*/

}
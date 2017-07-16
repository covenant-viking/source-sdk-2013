//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COVENANTGUI_H
#define COVENANTGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <igameevents.h>
#include "GameEventListener.h"

#include <game/client/iviewport.h>

//BB: allow use of GetFullscreenMousePos
#include <vgui/IInput.h>

class KeyValues;

#define COVENANT_BAR_COLOR	Color(0, 0, 0, 196)

class IBaseFileSystem;

//-----------------------------------------------------------------------------
// Purpose: Spectator UI
//-----------------------------------------------------------------------------
class CCovenantGUI : public vgui::Frame, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE(CCovenantGUI, vgui::Frame);

public:
	CCovenantGUI(IViewPort *pViewPort);
	virtual ~CCovenantGUI();

	virtual const char *GetName( void ) { return PANEL_COVENANTGUI; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return false; }
	virtual void ShowPanel( bool bShow );
	
	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink();

	void OnMousePressed(vgui::MouseCode code);
	void OnMouseReleased(vgui::MouseCode code);	// respond to mouse up events
	void OnCursorMoved(int x, int y);  // respond to moving the cursor with mouse button down
	void OnMouseWheeled(int delta);
	void ChangeView(int factor);

	virtual int GetTopBarHeight() { return m_pTopBar->GetTall(); }
	virtual int GetBottomBarHeight() { return m_pBottomBarBlank->GetTall(); }
	
	virtual bool ShouldShowPlayerLabel( int specmode );

	virtual Color GetBlackBarColor(void) { return COVENANT_BAR_COLOR; }

	virtual const char *GetResFile( void ) { return "Resource/UI/Spectator.res"; }
	
protected:

	void SetLabelText(const char *textEntryName, const char *text);
	void SetLabelText(const char *textEntryName, wchar_t *text);
	void MoveLabelToFront(const char *textEntryName);
	void UpdateTimer();
	void SetLogoImage(const char *image);

protected:	
	enum { INSET_OFFSET = 2 } ; 

	// vgui overrides
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
//	virtual void OnCommand( const char *command );

	vgui::Panel *m_pTopBar;
	vgui::Panel *m_pBottomBarBlank;

	vgui::ImagePanel *m_pBannerImage;
	vgui::Label *m_pPlayerLabel;

	IViewPort *m_pViewPort;

	// bool m_bHelpShown;
	// bool m_bInsetVisible;
	bool m_bSpecScoreboard;

private:
	int		m_iDragStatus;
	int		m_iButtons; //0 = none, 1 = 1, 2 = 2, 4 = 4, 8 = 5
	int		m_iDragMouseX, m_iDragMouseY;
	float	m_flLastClick;
	float	m_flDragTrigger;
	float	m_flMWHEELTrigger;
	ConVar *idealdist;
	ConVar *delta;
	ConVar *mindist;
	ConVar *maxdist;
	ConVar *idealyaw;
	ConVar *idealpitch;
	ConVar *camadjust;
};

extern CCovenantGUI * g_pCovenantGUI;

#endif // SPECTATORGUI_H

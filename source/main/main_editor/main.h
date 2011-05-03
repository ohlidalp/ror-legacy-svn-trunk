/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MESHVIEWERMAIN_H__
#define MESHVIEWERMAIN_H__

#include <vector>
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/artprov.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/statusbr.h>
#include <wx/colordlg.h>
#include <wx/wrapsizer.h>
#include <wx/msgdlg.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <wx/vscroll.h>
#include <wx/rawbmp.h>
#include <wx/dir.h>
#include <wx/spinctrl.h>
#include <wx/cmdline.h>

#include "RoREditor.h"
#include "wxOgreRenderWindow.h"
#include "wxOgreRenderWindowListener.h"

// RoR includes
#include "Settings.h"

using namespace std;

class PanelMeshProp;
class PanelMeshTree;
class PanelLog;

enum
{
	ID_VIEWPORT,

	ID_TOOL_MODE_TEXTURE,
	ID_TOOL_MODE_WIREFRAME,
	ID_TOOL_MODE_POINT,

};

class RoRViewerFrame : public wxFrame, public wxOgreRenderWindowListener
{
public:
	RoRViewerFrame(wxString title);
	~RoRViewerFrame();

	void updatePanelData(void);

	void InitializeAUI(void);
	void DeinitializeAUI(void);

	bool InitializeRoRViewer(wxString meshPath);
	void DenitializeRoRViewer(void);

	void OnIdle(wxIdleEvent& event);

	void OnViewToolClick(wxCommandEvent& e);

	// wxOgreRenderWindowListener
	void OnMouseEvents( wxMouseEvent &evt );

private:
	int msx, msy, msw;

	// AUI elements
	wxAuiManager* aui_mgr;

	wxOgreRenderWindow* panel_viewport;
	wxAuiPaneInfo* pane_viewport;

	PanelMeshProp* panel_meshprop;
	wxAuiPaneInfo* pane_meshprop;

	PanelMeshTree* panel_meshtree;
	wxAuiPaneInfo* pane_meshtree;

	PanelLog* panel_log;
	wxAuiPaneInfo* pane_log;

	RoREditor* editor;
    DECLARE_EVENT_TABLE()
};

class RoRViewerApp : public wxApp
{
public:
	bool OnInit(void);

	void OnInitCmdLine(wxCmdLineParser& parser);
	bool OnCmdLineParsed(wxCmdLineParser& parser);
	int OnExit(void);
private:
	wxString meshPath;
	RoRViewerFrame* frame;
};


BEGIN_EVENT_TABLE(RoRViewerFrame, wxFrame)
	EVT_IDLE(RoRViewerFrame::OnIdle)
END_EVENT_TABLE()

#ifdef USE_CONSOLE
IMPLEMENT_APP_CONSOLE(RoRViewerApp)
#else
IMPLEMENT_APP(RoRViewerApp)
#endif

#endif //MESHVIEWERMAIN_H__
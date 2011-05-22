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

#include "main.h"

#include "panel_meshprop.h"
#include "panel_meshtree.h"
#include "panel_log.h"

#include "Settings.h"

#include "AdvancedOgreFramework.h"

#include "display_mode.xpm"

bool RoRViewerApp::OnInit(void)
{
	if (!wxApp::OnInit())
		return false;

	frame = new RoRViewerFrame(L"RoR editor");
	SetTopWindow(frame);
	frame->Show();
	
	frame->InitializeAUI();
	if (!frame->InitializeRoRViewer(meshPath))
	{
		frame->DeinitializeAUI();
		return false;
	}

	return true;
}

void RoRViewerApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	//parser.AddParam(_T("<file>"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY);
}

bool RoRViewerApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	//meshPath = parser.GetParam();
	return true; //!meshPath.empty();
}

int RoRViewerApp::OnExit(void)
{
	return 0;
}

RoRViewerFrame::RoRViewerFrame(wxString title) : wxFrame(NULL, wxID_ANY, title)
{
	msx = 0;
	msy = 0;
	msw = 0;

	editor = NULL;

	SetMinSize(wxSize(640, 480));
	SetClientSize(640, 480);

	Show();
}

RoRViewerFrame::~RoRViewerFrame()
{
	DenitializeRoRViewer();
	DeinitializeAUI();

}
void RoRViewerFrame::OnViewToolClick(wxCommandEvent& e)
{
	Ogre::Camera *cam = OgreFramework::getSingleton().m_pViewport->getCamera();
	if(!cam) return;
	switch(e.GetId())
	{
		case ID_TOOL_MODE_TEXTURE:
			cam->setPolygonMode(Ogre::PM_SOLID);
			break;
		case ID_TOOL_MODE_WIREFRAME:
			cam->setPolygonMode(Ogre::PM_WIREFRAME);
			break;
		case ID_TOOL_MODE_POINT:
			cam->setPolygonMode(Ogre::PM_POINTS);
			break;
	};
}

void RoRViewerFrame::InitializeAUI(void)
{
	aui_mgr = new wxAuiManager(this);

	// property grid
	pane_meshprop = new wxAuiPaneInfo();
	pane_meshprop->CaptionVisible(true);
	pane_meshprop->Caption(L"Properties");
	pane_meshprop->FloatingSize(wxSize(200, 150));
	pane_meshprop->MinSize(wxSize(200, 150));
	//pane_meshprop->MaxSize(wxSize(200, 150));
	pane_meshprop->BestSize(wxSize(200, 150));
	pane_meshprop->Resizable(true);
	pane_meshprop->TopDockable(false);
	pane_meshprop->BottomDockable(false);
	pane_meshprop->Right();
	pane_meshprop->Layer(1);
	pane_meshprop->Position(1);
	pane_meshprop->Dock();
	panel_meshprop = new PanelMeshProp(0, this, wxID_ANY);
	aui_mgr->AddPane(panel_meshprop, *pane_meshprop);

	// tree
	pane_meshtree = new wxAuiPaneInfo();
	pane_meshtree->CaptionVisible(true);
	pane_meshtree->Caption(L"Tree");
	pane_meshtree->FloatingSize(wxSize(200, 150));
	pane_meshtree->MinSize(wxSize(200, 100));
	//pane_meshtree->MaxSize(wxSize(200, 150));
	pane_meshtree->BestSize(wxSize(200, 200));
	pane_meshtree->Resizable(true);
	pane_meshtree->TopDockable(false);
	pane_meshtree->BottomDockable(false);
	pane_meshtree->Left();
	pane_meshprop->Layer(0);
	pane_meshtree->Position(0);
	pane_meshtree->Dock();
	panel_meshtree = new PanelMeshTree(0, this, wxID_ANY);
	panel_meshtree->setPropGrid(panel_meshprop);
	aui_mgr->AddPane(panel_meshtree, *pane_meshtree);

	// the log
	pane_log = new wxAuiPaneInfo();
	pane_log->CaptionVisible(true);
	pane_log->Caption(L"Log");
	pane_log->FloatingSize(wxSize(150, 80));
	pane_log->MinSize(wxSize(150, 80));
	//pane_log->MaxSize(wxSize(150, 150));
	pane_log->BestSize(wxSize(150, 80));
	pane_log->Resizable(true);
	pane_log->TopDockable(true);
	pane_log->LeftDockable(false);
	pane_log->RightDockable(false);
	pane_log->BottomDockable(true);
	pane_log->Bottom();
	pane_log->Layer(0);
	pane_log->Position(2);
	pane_log->Dock();
	panel_log = new PanelLog(0, this, wxID_ANY);
	aui_mgr->AddPane(panel_log, *pane_log);

	// toolbars
	wxAuiToolBar *view_toolbar = new wxAuiToolBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxAUI_TB_HORZ_LAYOUT);
	view_toolbar->SetToolBitmapSize(wxSize(16,16));
	wxBitmap bmpDisMode = wxBitmap(display_mode_xpm);
	view_toolbar->AddTool(ID_TOOL_MODE_TEXTURE, "Texture", bmpDisMode, "show textures", wxITEM_RADIO);
	aui_mgr->Bind(wxEVT_COMMAND_TOOL_CLICKED, &RoRViewerFrame::OnViewToolClick, this, ID_TOOL_MODE_TEXTURE);
	view_toolbar->AddTool(ID_TOOL_MODE_WIREFRAME, "Wireframe", bmpDisMode, "show wireframe", wxITEM_RADIO);
	aui_mgr->Bind(wxEVT_COMMAND_TOOL_CLICKED, &RoRViewerFrame::OnViewToolClick, this, ID_TOOL_MODE_WIREFRAME);
	view_toolbar->AddTool(ID_TOOL_MODE_POINT, "Point", bmpDisMode, "show points", wxITEM_RADIO);
	aui_mgr->Bind(wxEVT_COMMAND_TOOL_CLICKED, &RoRViewerFrame::OnViewToolClick, this, ID_TOOL_MODE_POINT);
	view_toolbar->Realize();

	wxAuiPaneInfo *pane_toolbar = new wxAuiPaneInfo();
	pane_toolbar->ToolbarPane();
	pane_toolbar->LeftDockable(false);
	pane_toolbar->RightDockable(false);
	pane_toolbar->Top();
	pane_toolbar->Row(0);
	pane_toolbar->Position(0);
	aui_mgr->AddPane(view_toolbar, *pane_toolbar);

	// done with AUI
	aui_mgr->Update();

	// try to load the old window layout
	String perspective_str = SETTINGS.getSetting("EditorPerspective");
	if(!perspective_str.empty())
	{
		aui_mgr->LoadPerspective(wxString(perspective_str.c_str()));
	}
}

void RoRViewerFrame::DeinitializeAUI(void)
{
	// save the window layout
	string perspective_str = std::string(aui_mgr->SavePerspective().mb_str());
	SETTINGS.setSetting("EditorPerspective",perspective_str);
	SETTINGS.saveSettings();

	aui_mgr->UnInit();
}

bool RoRViewerFrame::InitializeRoRViewer(wxString meshPath)
{
	string mstr = std::string(meshPath.mb_str());
	editor = new RoREditor(mstr);
	
	panel_viewport = new wxOgreRenderWindow( this, ID_VIEWPORT);
	panel_viewport->Hide();
	
	if (!editor->Initialize(panel_viewport->GetOgreHandle(), wxOgreRenderWindow::GetOgreHandleForWindow (this)))
	{
		delete editor;
		editor = NULL;
		return false;
	}
	panel_viewport->Show();

	wxOgreRenderWindow::SetOgreRoot(OgreFramework::getSingleton().m_pRoot);
	wxOgreRenderWindow::SetOgreRenderWindow(OgreFramework::getSingleton().m_pRenderWnd);

	// now add our 3d window
	// main 3D viewport
	pane_viewport = new wxAuiPaneInfo();
	//pane_viewport->CaptionVisible(false);
	pane_viewport->Caption(L"Ogre 3D Render Window");
	pane_viewport->CenterPane();
	pane_viewport->PaneBorder(false);
	pane_viewport->CloseButton(false);
	//pane_viewport->FloatingSize(wxSize(50, 50));
	//pane_viewport->MinSize(wxSize(10, 10));
	pane_viewport->Dock();
	panel_viewport->SetBackgroundColour(wxColour(L"Gray"));
	panel_viewport->setRenderWindowListener( this );
	aui_mgr->AddPane(panel_viewport, *pane_viewport);

	// fit the window in the correct position
	aui_mgr->Update();

	

	panel_meshprop->setViewer(editor);
	panel_meshtree->setViewer(editor);
	panel_log->setViewer(editor);

	return true;
}

void RoRViewerFrame::DenitializeRoRViewer(void)
{
	if (editor)
	{
		editor->Deinitialize();
		delete editor;
		editor = NULL;
	}
}

void RoRViewerFrame::updatePanelData()
{
	panel_meshprop->updateData();
	panel_meshtree->updateData();
}

void RoRViewerFrame::OnIdle(wxIdleEvent& e)
{
	editor->Update();
	updatePanelData();

	e.RequestMore();
}

void RoRViewerFrame::OnMouseEvents( wxMouseEvent &evt )
{
}

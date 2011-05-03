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

#ifndef PANEL_LOG_H__
#define PANEL_LOG_H__

#include <wx/wx.h>

#include "RoREditor.h"

class PanelLog : public wxPanel , public Ogre::LogListener
{
protected:
	RoREditor *editor;
	wxTextCtrl  *logctrl;

public:
	void setViewer(RoREditor *_editor)
	{
		editor = _editor;
		Ogre::LogManager::getSingleton().getDefaultLog()->addListener(this);
	}

    PanelLog(RoREditor *_editor, wxWindow *parent,
            wxWindowID winid = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxTAB_TRAVERSAL | wxNO_BORDER,
            const wxString& name = wxPanelNameStr) :
		wxPanel(parent, winid, pos, size, style, name), editor(_editor)
    {
		// create sizer and PG
		wxBoxSizer *vsizer = new wxBoxSizer ( wxVERTICAL );
		this->SetSizer(vsizer);
		vsizer->SetSizeHints(this);
		
		logctrl = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 500, 150 ), wxNO_BORDER | wxTE_MULTILINE );

		vsizer->Add( logctrl, 1, wxEXPAND );
    }
	
	void messageLogged( const String& message, LogMessageLevel lml, bool maskDebug, const String &logName )
	{
		logctrl->AppendText( wxString( (message + "\n").c_str(), wxConvUTF8 ) );
	}


private:
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(PanelLog, wxPanel)
//    EVT_PG_CHANGED( wxID_ANY, PanelMeshProp::OnPropertyChange )
END_EVENT_TABLE()

#endif //PANEL_LOG_H__

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
#include "wizard.h"

BEGIN_EVENT_TABLE(PathPage, wxWizardPageSimple)
    EVT_BUTTON(ID_BROWSE, PathPage::OnBrowse)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(DownloadPage, wxWizardPageSimple)
	EVT_TIMER(1, DownloadPage::OnTimer)
END_EVENT_TABLE()


IMPLEMENT_APP(MyApp)

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// `Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
    MyWizard wizard(NULL);

    wizard.RunWizard(wizard.GetFirstPage());

	// we're done
    return false;
}
// ----------------------------------------------------------------------------
// MyWizard
// ----------------------------------------------------------------------------

MyWizard::MyWizard(wxFrame *frame, bool useSizer)
        : wxWizard(frame,wxID_ANY,_T("Rigs of Rods Installation Assistant"),
                   wxBitmap(licence_xpm),wxDefaultPosition,
                   wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	cm=new ConfigManager();
    PresentationPage *presentation = new PresentationPage(this);
	LicencePage *licence = new LicencePage(this);
	PathPage *path = new PathPage(this, cm);
	StreamsPage *streams = new StreamsPage(this, cm);
    ActionPage *action = new ActionPage(this, cm, licence, path, streams);
	DownloadPage *download = new DownloadPage(this);
	LastPage *last = new LastPage(this);

	m_page1=presentation;
    if (!cm->isLicenceAccepted())
	{
		wxWizardPageSimple::Chain(presentation, licence);
		licence->SetNext(action);
		action->SetPrev(licence);
	}
	else
	{
		presentation->SetNext(action);
		action->SetPrev(presentation);
	}
	path->SetPrev(action);
	wxWizardPageSimple::Chain(path, streams);
	wxWizardPageSimple::Chain(streams, download);
	wxWizardPageSimple::Chain(download, last);

    if ( useSizer )
    {
        // allow the wizard to size itself around the pages
        GetPageAreaSizer()->Add(presentation);
    }
}


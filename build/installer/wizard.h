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

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>
#include <wx/filename.h>
#include <wx/dir.h>

#include "wsync.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// for all others, include the necessary headers
#ifndef WX_PRECOMP
    #include "wx/frame.h"
    #include "wx/stattext.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/checkbox.h"
    #include "wx/checklst.h"
    #include "wx/msgdlg.h"
    #include "wx/radiobox.h"
    #include "wx/menu.h"
    #include "wx/sizer.h"
	#include "wx/textctrl.h"
	#include "wx/button.h"
	#include "wx/dirdlg.h"
	#include "wx/filename.h"
	#include "wx/dir.h"
	#include "wx/choice.h"
	#include "wx/gauge.h"
	#include "wx/timer.h"
	#include "wx/scrolwin.h"
#endif

#include "wx/wizard.h"
#include "wxStrel.h"

#include "ConfigManager.h"

#include "welcome.xpm"
#include "licence.xpm"
#include "dest.xpm"
#include "streams.xpm"
#include "action.xpm"
#include "download.xpm"
#include "finished.xpm"

#define TXTWRAP 400

// platform tools
#define PLATFORM_WINDOWS 1
#define PLATFORM_LINUX 2
#define PLATFORM_APPLE 3
// default platform is windows
#ifndef PLATFORM
#define PLATFORM PLATFORM_WINDOWS
#endif




#define ID_BROWSE 1
#define ID_WIZARD 2
#define ID_TIMER  3

// essential string conversion methods. helps to convert wxString<-->std::string<-->char string
inline wxString conv(const char *s)
{
	return wxString(s, wxConvUTF8);
}

inline wxString conv(const std::string& s)
{
	return wxString(s.c_str(), wxConvUTF8);
}

inline std::string conv(const wxString& s)
{
	return std::string(s.mb_str(wxConvUTF8));
}



// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
    // override base class virtuals
    virtual bool OnInit();
};

// ----------------------------------------------------------------------------
// our wizard
// ----------------------------------------------------------------------------

class MyWizard : public wxWizard
{
public:
    MyWizard(wxFrame *frame, bool useSizer = true);

    wxWizardPage *GetFirstPage() const { return m_page1; }
	
	void OnPageChanging(wxWizardEvent& event);

private:
    wxWizardPageSimple *m_page1;
	ConfigManager* cm;
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// pages for our wizard
// ----------------------------------------------------------------------------

class EnterLeavePage
{
public:
	virtual bool OnEnter(bool forward) {return true;}
	virtual bool OnLeave(bool forward) {return true;}
};

class PresentationPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
    PresentationPage(wxWizard *parent) : wxWizardPageSimple(parent)
    {
        m_bitmap = wxBitmap(welcome_xpm);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
		//GetParent()->SetBackgroundColour(*wxWHITE);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Welcome to the online installer of Rigs of Rods\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("This program will help you install or update Rigs of Rods on your computer.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Before installing, make sure Rigs of Rods is not running, and that your internet connection is available.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("If you use a firewall, please allow this program to access to Internet.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Click on Next to continue.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

        SetSizer(mainSizer);
        mainSizer->Fit(this);
    }
};

class LicencePage : public wxWizardPageSimple, public EnterLeavePage
{
public:
    LicencePage(wxWizard *parent) : wxWizardPageSimple(parent)
    {
        m_bitmap = wxBitmap(licence_xpm);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Licence\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Please review the terms of the following licence before installing:\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(new wxTextCtrl(this, wxID_ANY, _T("Rigs of Rods Copyright (C) 2008 Pierre-Michel Ricordel\n\n")
														_T("This program is a freeware which is offered to you at no cost.\n")
														_T("You can redistribute the installer pack as long as you do not modify it in any way.\n\n")
														_T("This program is distributed in the hope that it will entertain you, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n")
														_T("By using this program, you agree that in the case you create and publish a content for it (for example a vehicle), you accept that the author can include it in the official distribution, however you still retain the copyright of your work.")
														,wxDefaultPosition,wxDefaultSize,wxTE_MULTILINE|wxTE_READONLY)
														, 1, wxALL|wxEXPAND , 5);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("If you agree to this licence, click Next. If you do not agree, you cannot install Rigs of Rods.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

        SetSizer(mainSizer);
        mainSizer->Fit(this);
    }
};

class ActionPage : public wxWizardPage, public EnterLeavePage
{
public:
    ActionPage(wxWizard *parent, ConfigManager* cm, wxWizardPage* prev, wxWizardPage* fselect, wxWizardPage* download) : wxWizardPage(parent)
    {
		m_prev=prev;
		m_fselect=fselect;
		m_download=download;
		m_cm=cm;
		bool firstInstall = cm->isFirstInstall();
        m_bitmap = wxBitmap(action_xpm);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("What do you want to do?\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
		if(firstInstall)
		{
			mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("This is the first time you install Rigs of Rods, choose \"Install\" to download all the files of the game.\n")), 0, wxALL, 5);
			tst->Wrap(TXTWRAP);
		} else
		{
			wxString installPath = cm->getInstallationPath();
			mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("You have already installed the game installed in:\n")), 0, wxALL, 5);
			tst->Wrap(TXTWRAP);
			
			mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, installPath), 0, wxALL, 10);
			tst->Wrap(TXTWRAP);
			wxFont dfont=tst->GetFont();
			dfont.SetWeight(wxFONTWEIGHT_BOLD);
			dfont.SetPointSize(dfont.GetPointSize()+2);
			tst->SetFont(dfont);
			
			mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Choose \"Update\" to update Rigs of Rods to the latest version with faster download time.\n")), 0, wxALL, 5);
			tst->Wrap(TXTWRAP);
			mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("If you choose \"Uninstall\", Rigs of Rods will be deleted from your disk.\n")), 0, wxALL, 5);
			tst->Wrap(TXTWRAP);
		}
		wxString choices[3];
		choices[0]=_T("Install");
		choices[1]=_T("Update");
		choices[2]=_T("Uninstall");
		arb=new wxRadioBox(this, wxID_ANY, _T("Actions"), wxDefaultPosition, wxDefaultSize, 3, choices, 1, wxRA_SPECIFY_COLS);
		if (firstInstall)
		{
			arb->Enable(1, false);
			arb->Enable(2, false);
			arb->SetSelection(0);
		}
		else
		{
			arb->Enable(0, false);
			arb->SetSelection(1);
		}
		mainSizer->Add(arb, 0, wxALL, 5);

        SetSizer(mainSizer);
        mainSizer->Fit(this);
    }
	void SetPrev(wxWizardPage *prev) {m_prev=prev;}
    virtual wxWizardPage *GetPrev() const { return m_prev; }
    virtual wxWizardPage *GetNext() const
    {
        if (arb->GetSelection()==0)
			return m_fselect;
		else
			return m_download;
    }
	//output validation
	bool OnLeave(bool forward)
    {
		if (forward) m_cm->setAction(arb->GetSelection());
        return true;
    }
private:
	wxRadioBox* arb;
	wxWizardPage *m_prev, *m_fselect, *m_download;
	ConfigManager* m_cm;
};

class PathPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
    PathPage(wxWizard *parent, ConfigManager* cm) : wxWizardPageSimple(parent)
    {
		m_cm=cm;
        m_bitmap = wxBitmap(dest_xpm);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Source and Destination\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Download from:")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
		wxString choices[4];
		choices[0]=_T("Automatic mirrors selection");
		choices[1]=_T("European Mirrors");
		choices[2]=_T("American Mirrors");
		choices[3]=_T("Asian Mirrors");

		mainSizer->Add(src=new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 4, choices), 0, wxBOTTOM|wxLEFT|wxRIGHT, 5);
		src->SetSelection(0);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("\nChoose the installation directory:")), 0, wxTOP|wxLEFT|wxRIGHT, 5);
		tst->Wrap(TXTWRAP);
		
		wxString path = wxT("");
#if PLATFORM == PLATFORM_WINDOWS
		path = _T("C:\\Program Files\\Rigs of Rods");
#elif PLATFORM == PLATFORM_LINUX
		char homedir[255] = "";
		strncpy(homedir, getenv ("HOME"), 250);
		char tmp[255]="";
		snprintf(tmp, 255, "%s/rigsofrods/", homedir);
		path = conv(tmp);
#elif PLATFORM == PLATFORM_APPLE
		path = _T("/opt/games/rigsofrods/");
#endif		
		mainSizer->Add(sel=new wxTextCtrl(this, wxID_ANY, path, wxDefaultPosition,wxDefaultSize, 0) , 0, wxALL|wxEXPAND , 5);
		mainSizer->Add(brobut=new wxButton(this, ID_BROWSE, _T("Browse...")), 0, wxBOTTOM|wxLEFT|wxRIGHT , 5);
        

		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("\nChoose the user directory:")), 0, wxTOP|wxLEFT|wxRIGHT, 5);
		wxString choices2[2];
#if PLATFORM == PLATFORM_WINDOWS
		choices2[0]=_T("Default (My Documents/Rigs of Rods)");
#elif PLATFORM == PLATFORM_LINUX
		choices2[0]=_T("Default (~/Rigs of Rods)");
#elif PLATFORM == PLATFORM_APPLE
		choices2[0]=_T("Default");
#endif	
		choices2[1]=_T("Custom (specify later)");
		wxRadioBox *arb2=new wxRadioBox(this, wxID_ANY, _T("User directory"), wxDefaultPosition, wxDefaultSize, 2, choices2, 1, wxRA_SPECIFY_COLS);
		mainSizer->Add(arb2, 0, wxTOP|wxLEFT|wxRIGHT, 5);
		
		//mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("You should have at least 100MB of free disk space on this drive to install Rigs of Rods.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

		dirdial=new wxDirDialog(this, _T("Choose a directory"), sel->GetValue());

        SetSizer(mainSizer);
        mainSizer->Fit(this);
    }
	void OnBrowse(wxCommandEvent&WXUNUSED(evt))
	{
		dirdial->SetPath(sel->GetValue());
		int res=dirdial->ShowModal();
		if (res==wxID_OK) sel->SetValue(dirdial->GetPath());
	}
	//output validation
    //virtual bool TransferDataFromWindow()
	bool OnLeave(bool forward)
    {
		if (forward)
		{
			wxString path=sel->GetValue();
			if (!wxFileName(path).IsAbsolute()) //relative paths are dangerous!
			{
				wxMessageBox(_T("This path is invalid"), _T("Invalid path"), wxICON_ERROR | wxOK, this);
				return false;
			}
			if (!wxDir::Exists(path))
			{
				if (!wxFileName::Mkdir(path))
				{
					wxMessageBox(_T("This path could not be created"), _T("Invalid path"), wxICON_ERROR | wxOK, this);
					return false;
				}
			}
			m_cm->setInstallPath(path);
		}
        return true;
    }


private:
	ConfigManager* m_cm;
	wxTextCtrl* sel;
	wxChoice* src;
	wxButton* brobut;
	wxDirDialog *dirdial;
    DECLARE_EVENT_TABLE()
};

class StreamsPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
    StreamsPage(wxWizard *parent, ConfigManager* cm) : wxWizardPageSimple(parent)
    {
		streamset=false;
		m_cm=cm;
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
  		wxStaticText *tst;
        m_bitmap = wxBitmap(streams_xpm);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Streams selection\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Choose which feature packs you want to download:\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

		// TODO: fix width of control: size is hidden ...
		mainSizer->Add(scrw=new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL), 1, wxALL|wxEXPAND, 5);
		scrwsz=new wxBoxSizer(wxVERTICAL);
		scrw->SetSizer(scrwsz);
		//scrw->SetBackgroundColour(*wxWHITE);
		scrw->SetScrollbars(0, STREL_HEIGHT+3, 100, 30);

		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Click Next to begin the download.")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

        SetSizer(mainSizer);
		//scrwsz->Fit(scrw);
		mainSizer->Fit(this);
		
    }
	bool OnEnter(bool forward)
	{
		if (streamset) return true; //reloading streams do not work for the moment (GUI problem)
		int res = m_cm->getOnlineStreams();
		if(res)
		{
			wxMessageBox(_T("The program could not connect to the central Streams server.\nThe service may be temporarily unavailable, or you have no network connection."), _T("Network error"), wxICON_ERROR | wxOK, this);
		}
		scrwsz->Clear(true);
		//add the streams
		std::vector < stream_desc_t > *streams = m_cm->getStreamset();
		for(std::vector < stream_desc_t >::iterator it=streams->begin(); it!=streams->end(); it++)
		{
			wxStrel *wst=new wxStrel(scrw, &(*it));
			scrwsz->Add(wst, 0, wxALL|wxEXPAND,0);
		}
		streamset=true;
		return true;
	}

	bool OnLeave(bool forward)
	{
		if (forward)
		{
			//store the user settings
			int i=0;
			while (wxStrel *wst=dynamic_cast<wxStrel*>(scrwsz->GetItem(i)))
			{
				m_cm->setStreamSelection(wst->getDesc(), wst->getSelection());
				i++;
			}
		}
		return true;
	}
    
	virtual wxWizardPage *GetPrev() const
	{
		if(m_cm->getAction() == 0)
			return fpath;
		return faction;
	}

	void setPages(wxWizardPage* _fpath, wxWizardPage* _faction)
	{
		fpath=_fpath;
		faction=_faction;
	}
private:
	ConfigManager* m_cm;
	wxScrolledWindow *scrw;
	wxSizer *scrwsz;
	wxWizardPage* fpath, *faction;
	bool streamset;
};

class DownloadPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
    DownloadPage(wxWizard *parent, ConfigManager* cm) : wxWizardPageSimple(parent), m_cm(cm)
    {		
        m_bitmap = wxBitmap(download_xpm);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Downloading\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
		statusText=new wxStaticText(this, wxID_ANY, _T("Please wait for the download to finish\n"));
		mainSizer->Add(statusText, 0, wxALL, 5);
		tst->Wrap(TXTWRAP);
		progress=new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH);
		mainSizer->Add(progress, 0, wxALL|wxEXPAND, 5);

        SetSizer(mainSizer);
        mainSizer->Fit(this);
		
		timer = new wxTimer(this, ID_TIMER);

		w = new WSync();
	}
	
	bool OnEnter(bool forward)
	//void OnEnter(wxString operation, wxString url)
	{
		timer->Start(250); // 250 ms

		std::vector < stream_desc_t > *streams = m_cm->getStreamset();

		for(std::vector < stream_desc_t >::iterator it=streams->begin(); it!=streams->end(); it++)
		{
			// TODO: put this in separate thread, class is NOT thread safe yet
			std::string ipath = conv(m_cm->getInstallPath());
			std::string spath = conv(it->path);
			w->sync(ipath, "wsync.rigsofrods.com", spath, true, it->del);
		}

		return true;
	}
	bool OnLeave(bool forward)
	{
		// stop timer and cancel all running operations
		timer->Stop();

		return true;
	}	

private:
	wxGauge *progress;
	wxTimer *timer;
	wxStaticText *statusText;
	ConfigManager* m_cm;
	WSync *w;
	//pthread_t syncThread;

	void OnTimer(wxTimerEvent& event)
	{
		wxYield();
		int percent=0;
		std::string message;
		w->getStatus(percent, message);
		statusText->SetLabel(conv(message));
		progress->SetValue(percent);
	}
	
	DECLARE_EVENT_TABLE()
};

class LastPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
    LastPage(wxWizard *parent) : wxWizardPageSimple(parent)
    {
        m_bitmap = wxBitmap(finished_xpm);
        wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
		//GetParent()->SetBackgroundColour(*wxWHITE);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Finished\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
        mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Thank you for downloading Rigs of Rods.\n")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

        SetSizer(mainSizer);
        mainSizer->Fit(this);
    }
};

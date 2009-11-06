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
#include <wx/cmdline.h>
#include <wx/dir.h>
#include <wx/thread.h>
#include <wx/event.h>
#include "wthread.h"
#include "cevent.h"

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


#if PLATFORM == PLATFORM_WINDOWS
#include <shlobj.h> // for the special path functions
#include "symlink.h"
#include <shellapi.h> // for executing the binaries
#endif//win


#define ID_BROWSE 1
#define ID_WIZARD 2
//#define ID_TIMER  3

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


// utils for the wizard
inline void setControlEnable(wxWizard *wiz, int id, bool enable)
{
	wxWindow *win = wiz->FindWindowById(id);
	if(win) win->Enable(enable);
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
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
protected:
	int startupMode;
};

static const enum {IMODE_NONE=0, IMODE_UPDATE, IMODE_INSTALL, IMODE_UNINSTALL, IMODE_UPGRADE};
static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
     { wxCMD_LINE_SWITCH, ("h"), ("help"),      ("displays help on the command line parameters"), wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
     { wxCMD_LINE_SWITCH, ("u"), ("update"),    ("update mode"), wxCMD_LINE_VAL_NONE },
     { wxCMD_LINE_SWITCH, ("r"), ("uninstall"), ("uninstall mode"), wxCMD_LINE_VAL_NONE },
     { wxCMD_LINE_SWITCH, ("i"), ("install"),   ("install mode"), wxCMD_LINE_VAL_NONE  },
     { wxCMD_LINE_SWITCH, ("g"), ("upgrade"),   ("upgrade mode"), wxCMD_LINE_VAL_NONE  },
     { wxCMD_LINE_NONE }
};

// ----------------------------------------------------------------------------
// our wizard
// ----------------------------------------------------------------------------

class MyWizard : public wxWizard
{
public:
	MyWizard(int startupMode, wxFrame *frame, bool useSizer = true);

	wxWizardPage *GetFirstPage() const { return m_page1; }
	
	void OnPageChanging(wxWizardEvent& event);

private:
	wxWizardPageSimple *m_page1;
	ConfigManager* cm;
	int startupMode;
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

class ActionPage : public wxWizardPageSimple, public EnterLeavePage
{
public:
	ActionPage(wxWizard *parent, ConfigManager* cm, wxWizardPage* prev, wxWizardPage* fselect, wxWizardPage* download) : wxWizardPageSimple(parent)
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
			mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("The game is currently installed in:\n")), 0, wxALL, 5);
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
		wxString choices[4];
		choices[0]=_T("Install");
		choices[1]=_T("Upgrade older Installation");
		choices[2]=_T("Update");
		choices[3]=_T("Uninstall");
		arb=new wxRadioBox(this, wxID_ANY, _T("Actions"), wxDefaultPosition, wxDefaultSize, 4, choices, 1, wxRA_SPECIFY_COLS);
		if (firstInstall)
		{
			arb->Enable(2, false);
			arb->Enable(3, false);
			arb->SetSelection(0);
			if(cm->getStartupMode() == IMODE_UPGRADE)
				arb->SetSelection(1);
		}
		else
		{
			arb->Enable(0, false);
			arb->Enable(1, false);
			arb->SetSelection(2);
			if(cm->getStartupMode() == IMODE_UNINSTALL)
				arb->SetSelection(3);
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
		{
			return m_fselect;
		} else if (arb->GetSelection()==1)
		{
			return m_fselect;
		} else if (arb->GetSelection()==3)
		{
			int res = wxMessageBox(_T("Do you really want to uninstall Rigs of Rods?"), _T("Uninstall?"), wxICON_QUESTION | wxYES_NO);
			if(res == wxYES)
			{
				res = wxMessageBox(_T("Do you want to remove all Rigs of Rods content in the User Folder (all downloaded mods, etc)?"), _T("Uninstall User Content?"), wxICON_QUESTION | wxYES_NO);
				res = m_cm->uninstall((res == wxYES));
				if(!res)
				{
					wxMessageBox(_T("Rigs of Rods was uninstalled successfully. Thanks for using!"), _T("Uninstalled!"), wxICON_INFORMATION | wxOK);
					exit(0);
				}
			}
			wxMessageBox(_T("Uninstall aborted!"), _T("uninstall?"), wxICON_ERROR | wxOK);
			exit(0);

			// we should never be here
			return m_fselect;
		} else
		{
			return m_download;
		}
	}
	//output validation
	bool OnLeave(bool forward)
	{
		if (forward && arb->GetSelection()==1)
			wxMessageBox(_T("Please select the old installation directory."), _T("Upgrade installation"), wxICON_INFORMATION | wxOK);
		
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
		
		/*
		wxString choices[4];
		choices[0]=_T("Automatic mirrors selection");
		choices[1]=_T("European Mirrors");
		choices[2]=_T("American Mirrors");
		choices[3]=_T("Asian Mirrors");

		mainSizer->Add(src=new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 4, choices), 0, wxBOTTOM|wxLEFT|wxRIGHT, 5);
		src->SetSelection(0);
		*/

		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("\nChoose the installation directory:")), 0, wxTOP|wxLEFT|wxRIGHT, 5);
		tst->Wrap(TXTWRAP);
		
		wxString path = wxT("");
#if PLATFORM == PLATFORM_WINDOWS
		wxString dir;
		if (SHGetSpecialFolderPath(0, wxStringBuffer(dir, MAX_PATH), CSIDL_PROGRAM_FILES, FALSE))
		{
			path = dir + wxT("\\Rigs of Rods");
		}
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
        

		/*
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
		*/

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
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Streams selection\n")), 0, wxALL, 0);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Choose which feature packs you want to download:\n")), 0, wxALL, 0);
		tst->Wrap(TXTWRAP);

		mainSizer->Add(scrw=new wxScrolledWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL), 1, wxGROW, 0);
		scrwsz=new wxBoxSizer(wxVERTICAL);
		scrw->SetSizer(scrwsz);
		//scrw->SetBackgroundColour(*wxWHITE);
		scrw->SetScrollbars(0, STREL_HEIGHT+3, 100, 30);

		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Click Next to begin the download.")), 0, wxALL, 0);
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
		if(!streams->size())
		{
			wxMessageDialog *dlg = new wxMessageDialog(this, wxT("Error while downloading the streams index. Please retry later."), wxT("Error"),wxOK|wxICON_ERROR);
			dlg->ShowModal();
			exit(1);
		}
		for(std::vector < stream_desc_t >::iterator it=streams->begin(); it!=streams->end(); it++)
		{
			if(it->hidden) continue; // hide hidden entries ;)
			wxStrel *wst=new wxStrel(scrw, &(*it));
			wxSizerItem *si = scrwsz->Add(wst, 0, wxALL|wxEXPAND,0);
			si->SetUserData((wxObject *)wst);
		}
		streamset=true;
		return true;
	}

	bool OnLeave(bool forward)
	{
		if (forward)
		{
			//store the user settings
			wxSizerItemList::compatibility_iterator node = scrwsz->GetChildren().GetFirst();
			while (node)
			{
				wxSizerItem *item = node->GetData();
				wxStrel *wst = (wxStrel *)item->GetUserData();
				if(!wst) continue;
				m_cm->setStreamSelection(wst->getDesc(), wst->getSelection());
				node = node->GetNext();
			}
			// save the selection in the registry for the next time.
			m_cm->saveStreamSubscription();
		}

		return true;
	}
    
	virtual wxWizardPage *GetPrev() const
	{
		if(m_cm->getAction() == 0 || m_cm->getAction() == 1)
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
	friend class WsyncThread;
public:
	DownloadPage(wxWizard *parent, ConfigManager* cm) : wxWizardPageSimple(parent), m_cm(cm), wizard(parent)
	{
		threadStarted=false;
		isDone=false;
		m_bitmap = wxBitmap(download_xpm);
		wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
		wxStaticText *tst;
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Downloading\n")), 0, wxALL, 5);
		wxFont dfont=tst->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+4);
		tst->SetFont(dfont);
		tst->Wrap(TXTWRAP);

		// status text and progress bar
		statusText=new wxStaticText(this, wxID_ANY, _T("Please wait for the download to finish\n"));
		mainSizer->Add(statusText, 0, wxALL, 2);
		tst->Wrap(TXTWRAP);
		progress=new wxGauge(this, wxID_ANY, 1000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_SMOOTH);
		mainSizer->Add(progress, 0, wxALL|wxEXPAND, 0);
		progress->Pulse();


		// now the information thingy
		wxGridSizer *wxg = new wxGridSizer(3, 2, 5, 5);

		// download time
		wxStaticText *txt = new wxStaticText(this, wxID_ANY, _T("Download time: "));
		wxg->Add(txt, 0, wxALL|wxEXPAND, 5);
		txt_dltime = new wxStaticText(this, wxID_ANY, _T("n/a"));
		wxg->Add(txt_dltime, 0, wxALL|wxEXPAND, 5);

		// download time
		txt = new wxStaticText(this, wxID_ANY, _T("Remaining time: "));
		wxg->Add(txt, 0, wxALL|wxEXPAND, 5);
		txt_remaintime = new wxStaticText(this, wxID_ANY, _T("n/a"));
		wxg->Add(txt_remaintime, 0, wxALL|wxEXPAND, 5);

		// traffic
		txt = new wxStaticText(this, wxID_ANY, _T("Data transferred: "));
		wxg->Add(txt, 0, wxALL|wxEXPAND, 5);
		txt_traf = new wxStaticText(this, wxID_ANY, _T("n/a"));
		wxg->Add(txt_traf, 0, wxALL|wxEXPAND, 5);

		// speed
		txt = new wxStaticText(this, wxID_ANY, _T("Average Download Speed: "));
		wxg->Add(txt, 0, wxALL|wxEXPAND, 5);
		txt_speed = new wxStaticText(this, wxID_ANY, _T("n/a"));
		wxg->Add(txt_speed, 0, wxALL|wxEXPAND, 5);

		// Local Path
		txt = new wxStaticText(this, wxID_ANY, _T("Sync Path: "));
		wxg->Add(txt, 0, wxALL|wxEXPAND, 5);
		txt_localpath = new wxStaticText(this, wxID_ANY, _T("n/a"));
		wxg->Add(txt_localpath, 0, wxALL|wxEXPAND, 5);

		// Server used
		txt = new wxStaticText(this, wxID_ANY, _T("Server used: "));
		wxg->Add(txt, 0, wxALL|wxEXPAND, 5);
		txt_server = new wxStaticText(this, wxID_ANY, _T("only main server"));
		wxg->Add(txt_server, 0, wxALL|wxEXPAND, 5);

		// grid end
		mainSizer->Add(wxg, 0, wxALL|wxEXPAND, 5);


		// FINISHED text
		txtFinish = new wxStaticText(this, wxID_ANY, _T("Finished downloading, please continue by pressing next."));
		dfont=txtFinish->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+2);
		txtFinish->SetFont(dfont);
		//txtFinish->Wrap(TXTWRAP);
		mainSizer->Add(txtFinish, 0, wxALL|wxEXPAND, 0);

		SetSizer(mainSizer);
		mainSizer->Fit(this);
		
		//timer = new wxTimer(this, ID_TIMER);
	}

	
	void startThread()
	{
		if(threadStarted) return;
		threadStarted=true;
		m_pThread = new WsyncThread(this, m_cm->getInstallPath(), *(m_cm->getStreamset()));
		if ( m_pThread->Create() != wxTHREAD_NO_ERROR )
		{
			wxLogError(wxT("Can't create the thread!"));
			delete m_pThread;
			m_pThread = NULL;
		}
		else
		{
			if (m_pThread->Run() != wxTHREAD_NO_ERROR )
			{
				wxLogError(wxT("Can't create the thread!"));
				delete m_pThread;
				m_pThread = NULL;
			}

			// after the call to wxThread::Run(), the m_pThread pointer is "unsafe":
			// at any moment the thread may cease to exist (because it completes its work).
			// To avoid dangling pointers OnThreadExit() will set m_pThread
			// to NULL when the thread dies.
		}
	}
	bool OnEnter(bool forward)
	{
		txtFinish->Hide();
		//disable forward and backward buttons
		setControlEnable(wizard, wxID_FORWARD, false);
		setControlEnable(wizard, wxID_BACKWARD, false);

		std::vector < stream_desc_t > *streams = m_cm->getStreamset();
		if(!streams->size())
		{
			// TODO: handle this case, go back?
			return false;
		}
		
		startThread();
		return true;
	}

	bool OnLeave(bool forward)
	{
		if(isDone && !forward) return false; //when done, only allow to go forward
		return isDone;
	}	

private:
	wxStaticText *txt_dltime, *txt_speed, *txt_traf, *txt_localpath, *txt_server, *txt_remaintime;
	wxStaticText *statusText, *txtFinish;
	wxGauge *progress;
	bool threadStarted;
	bool isDone;
	wxWizard *wizard;
	
	ConfigManager* m_cm;
	WsyncThread *m_pThread;
	
	void OnStatusUpdate(MyStatusEvent &ev)
	{
		switch(ev.GetId())
		{
		case MSE_STARTING:
		case MSE_UPDATE_TEXT:
			statusText->SetLabel(ev.text);
			break;
		case MSE_UPDATE_PROGRESS:
			progress->SetValue(ev.progress * 1000.0f);
			break;
		case MSE_ERROR:
			statusText->SetLabel(_("error: ") + ev.text);
			progress->SetValue(1000);
			wxMessageBox(ev.text, _("Error"), wxICON_ERROR | wxOK, this);
			break;
		case MSE_UPDATE_TIME:
			txt_dltime->SetLabel(ev.text);
			break;
		case MSE_UPDATE_TIME_LEFT:
			txt_remaintime->SetLabel(ev.text);
			break;
		case MSE_UPDATE_SPEED:
			txt_speed->SetLabel(ev.text);
			break;
		case MSE_UPDATE_TRAFFIC:
			txt_traf->SetLabel(ev.text);
			break;
		case MSE_UPDATE_PATH:
			txt_localpath->SetLabel(ev.text);
			break;
		case MSE_UPDATE_SERVER:
			txt_server->SetLabel(ev.text);
			break;


		case MSE_DONE:
			// normal end
			statusText->SetLabel(ev.text);
			progress->SetValue(1000);
			txt_remaintime->SetLabel(wxT("finished!"));
			isDone=true;
			// enableforward button
			txtFinish->Show();
			setControlEnable(wizard, wxID_FORWARD, true);
			break;
		}
	}
	DECLARE_EVENT_TABLE()
};

class LastPage : public wxWizardPageSimple, public EnterLeavePage
{
protected:
	ConfigManager* cm;
	wxWizard *wizard;
	wxCheckBox *chk_runtime, *chk_configurator, *chk_desktop, *chk_startmenu, *chk_viewmanual;
public:
	LastPage(wxWizard *parent, ConfigManager* _cm) : wxWizardPageSimple(parent), cm(_cm), wizard(parent)
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
		mainSizer->Add(tst=new wxStaticText(this, wxID_ANY, _T("Thank you for downloading Rigs of Rods.\nWhat do you want to do now?")), 0, wxALL, 5);
		tst->Wrap(TXTWRAP);

		bool firstInstall = cm->isFirstInstall();
		
		chk_runtime = new wxCheckBox(this, wxID_ANY, _T("Install required runtime libraries now"));
		mainSizer->Add(chk_runtime, 0, wxALL|wxALIGN_LEFT, 5);
		chk_runtime->SetValue(firstInstall);
		if(firstInstall) chk_runtime->Disable();
		
		chk_desktop = new wxCheckBox(this, wxID_ANY, _T("Create Desktop shortcuts"));
		mainSizer->Add(chk_desktop, 0, wxALL|wxALIGN_LEFT, 5);
		chk_desktop->SetValue(true);
		if(cm->getPersistentConfig(wxT("installer.create_desktop_shortcuts")) == wxT("no"))
			chk_desktop->SetValue(false);

		chk_startmenu = new wxCheckBox(this, wxID_ANY, _T("Create Start menu shortcuts"));
		mainSizer->Add(chk_startmenu, 0, wxALL|wxALIGN_LEFT, 5);
		chk_startmenu->SetValue(true);
		if(cm->getPersistentConfig(wxT("installer.create_start_menu_shortcuts")) == wxT("no"))
			chk_startmenu->SetValue(false);

		chk_viewmanual = new wxCheckBox(this, wxID_ANY, _T("View manual"));
		mainSizer->Add(chk_viewmanual, 0, wxALL|wxALIGN_LEFT, 5);
		chk_viewmanual->SetValue(firstInstall);

		chk_configurator = new wxCheckBox(this, wxID_ANY, _T("Run the Configurator"));
		mainSizer->Add(chk_configurator, 0, wxALL|wxALIGN_LEFT, 5);
		chk_configurator->SetValue(true);
		if(cm->getPersistentConfig(wxT("installer.run_configurator")) == wxT("no"))
			chk_configurator->SetValue(false);

		SetSizer(mainSizer);
		mainSizer->Fit(this);

	}
	
	bool OnEnter(bool forward)
	{
		setControlEnable(wizard, wxID_BACKWARD, false);
		setControlEnable(wizard, wxID_CANCEL, false);
		return true;
	}

	bool OnLeave(bool forward)
	{
		if(!forward) return false;
		// do last things
		cm->setPersistentConfig(wxT("installer.create_desktop_shortcuts"), chk_desktop->IsChecked()?wxT("yes"):wxT("no"));
		cm->setPersistentConfig(wxT("installer.create_start_menu_shortcuts"), chk_startmenu->IsChecked()?wxT("yes"):wxT("no"));
		cm->setPersistentConfig(wxT("installer.run_configurator"), chk_configurator->IsChecked()?wxT("yes"):wxT("no"));

		if(chk_desktop->IsChecked() || chk_startmenu->IsChecked())
			createProgramLinks();

		if(chk_runtime->IsChecked())
			installRuntime();
		
		if(chk_configurator->IsChecked())
			startConfigurator();

		if(chk_viewmanual->IsChecked())
			viewManual();
		
		return true;
	}

	// small wrapper that converts wxString to std::string and remvoes the file if already existing
	int createWindowsShortcut(wxString linkTarget, wxString workingDirectory, wxString linkFile, wxString linkDescription)
	{
		if(wxFileExists(linkFile)) wxRemoveFile(linkFile);
		if(!wxFileExists(linkTarget)) return 1;
		return createLink(conv(linkTarget), conv(workingDirectory), conv(linkFile), conv(linkDescription));
	}

	void installRuntime()
	{
		wxMessageBox(wxT("Will now install DirectX. Please click ok to continue"), _T("vcredit_x86.exe!"), wxICON_INFORMATION | wxOK);
		executeBinary(wxT("dxwebsetup.exe"));
		wxMessageBox(wxT("Will now install the Visual Studio runtime. Please click ok to continue."), _T("vcredit_x86.exe!"), wxICON_INFORMATION | wxOK);
		executeBinary(wxT("vcredist_x86.exe"));
	}

	void startConfigurator()
	{
		executeBinary(wxT("rorconfig.exe"));
	}

	void viewManual()
	{
		executeBinary(wxT("Things_you_can_do_in_Rigs_of_Rods.pdf"), wxT("open"));
		executeBinary(wxT("keysheet.pdf"), wxT("open"));
	}

	void executeBinary(wxString filename, wxString action = wxT("runas"))
	{
#if PLATFORM == PLATFORM_WINDOWS
		char path[2048]= "";
		strncpy(path, conv(cm->getInstallPath()).c_str(), 2048);

		int buffSize = (int)strlen(path) + 1;
		LPWSTR cwpath = new wchar_t[buffSize];
		MultiByteToWideChar(CP_ACP, 0, path, buffSize, cwpath, buffSize);

		strcat(path, "\\");
		strcat(path, conv(filename).c_str());
		buffSize = (int)strlen(path) + 1;
		LPWSTR wpath = new wchar_t[buffSize];
		MultiByteToWideChar(CP_ACP, 0, path, buffSize, wpath, buffSize);


		// now construct struct that has the required starting info
		SHELLEXECUTEINFO sei = { sizeof(sei) };
		sei.cbSize = sizeof(SHELLEXECUTEINFOA);
		sei.fMask = 0;
		sei.hwnd = NULL;
		sei.lpVerb = action;
		sei.lpFile = wpath;
		sei.lpParameters = wxT("");
		sei.lpDirectory = cwpath;
		sei.nShow = SW_NORMAL;
		ShellExecuteEx(&sei);
#endif //PLATFORM
	}

	void createProgramLinks()
	{
		// XXX: TODO: ADD LINUX code!
		// create shortcuts
#if PLATFORM == PLATFORM_WINDOWS
		wxString startmenuDir, desktopDir, workingDirectory;

		// XXX: ADD PROPER ERROR HANDLING
		// CSIDL_COMMON_PROGRAMS = start menu for all users
		// CSIDL_PROGRAMS = start menu for current user only
		if(!SHGetSpecialFolderPath(0, wxStringBuffer(startmenuDir, MAX_PATH), CSIDL_COMMON_PROGRAMS, FALSE))
			return;
		
		// same with CSIDL_COMMON_DESKTOPDIRECTORY and CSIDL_DESKTOP
		if(!SHGetSpecialFolderPath(0, wxStringBuffer(desktopDir, MAX_PATH), CSIDL_DESKTOP, FALSE))
			return;

		workingDirectory = cm->getInstallPath();
		if(workingDirectory.size() > 3 && workingDirectory.substr(workingDirectory.size()-1,1) != wxT("\\"))
		{
			workingDirectory += wxT("\\");
		}
		// ensure our directory in the start menu exists
		startmenuDir += wxT("\\Rigs of Rods");
		if (!wxDir::Exists(startmenuDir)) wxFileName::Mkdir(startmenuDir);

		// the actual linking
		if(chk_desktop->IsChecked())
			createWindowsShortcut(workingDirectory + wxT("rorconfig.exe"), workingDirectory, desktopDir + wxT("\\Rigs of Rods.lnk"), wxT("start Rigs of Rods"));

		if(chk_startmenu->IsChecked())
		{
			createWindowsShortcut(workingDirectory + wxT("RoR.exe"), workingDirectory, startmenuDir + wxT("\\Rigs of Rods.lnk"), wxT("start Rigs of Rods"));
			createWindowsShortcut(workingDirectory + wxT("rorconfig.exe"), workingDirectory, startmenuDir + wxT("\\Configurator.lnk"), wxT("start Rigs of Rods Configuration Program (required upon first start)"));
			createWindowsShortcut(workingDirectory + wxT("servergui.exe"), workingDirectory, startmenuDir + wxT("\\Multiplayer Server.lnk"), wxT("start Rigs of Rods multiplayer server"));
			createWindowsShortcut(workingDirectory + wxT("Things_you_can_do_in_Rigs_of_Rods.pdf"), workingDirectory, startmenuDir + wxT("\\Manual.lnk"), wxT("open the RoR Manual"));
			createWindowsShortcut(workingDirectory + wxT("keysheet.pdf"), workingDirectory, startmenuDir + wxT("\\Keysheet.lnk"), wxT("open the RoR Key Overview"));
			createWindowsShortcut(workingDirectory + wxT("installer.exe"), workingDirectory, startmenuDir + wxT("\\Installer (update or uninstall).lnk"), wxT("open the Installer with which you can update or uninstall RoR."));
		}

#endif //PLATFORM_WINDOWS
	}

};

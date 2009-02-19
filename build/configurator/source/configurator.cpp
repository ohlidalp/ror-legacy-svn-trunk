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

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------
#include "rornet.h"
#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
//#include "crashrpt.h"
#include <shlobj.h>
#endif

#include <vector>
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

mode_t getumask(void)
{
	mode_t mask = umask(0);
	umask(mask);
	return mask;
}

#endif


#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/gauge.h>
#include <wx/scrolbar.h>
#include <wx/grid.h>
#include <wx/treectrl.h>
#include <wx/tooltip.h>
#include <wx/mediactrl.h>
#include <wx/intl.h>
#include <wx/textfile.h>
#include <wx/cmdline.h>
#include <wx/html/htmlwin.h>
#include <wx/uri.h>
#include <wx/dir.h>
#include <wx/fs_inet.h>
#include <wx/scrolwin.h>
#include "statpict.h"
#include <wx/log.h>
#include <wx/timer.h>
//#include "joysticks.h"


#include "ImprovedConfigFile.h"

#include "InputEngine.h"
#include "treelistctrl.h"

#include "OISKeyboard.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str, len) strlen(str)
#endif


#if __WIN32__
#include <direct.h> // for getcwd
#endif

// xpm images
#include "config.xpm"
#include "joycfg.xpm"
#include "mousecfg.xpm"
#include "keyboardcfg.xpm"

// de-comment this to enable network stuff
#define NETWORK
#define MAX_EVENTS 2048

wxTextFile *logfile;
wxLocale locale;
wxLanguageInfo *language=0;
std::vector<wxLanguageInfo*> avLanguages;


std::map<std::string, std::string> settings;


bool fileExists(char* filename)
{
	FILE* f = fopen(filename, "rb");
	if(f != NULL) {
		fclose(f);
		return true;
	}
	return false;
}


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

// Define a new application type, each program should derive a class from wxApp
class MyApp : public wxApp
{
public:
	// override base class virtuals
	// ----------------------------

	// this one is called on application startup and is a good place for the app
	// initialization (doing it here and not in the ctor allows to have an error
	// return: if OnInit() returns false, the application terminates)
	virtual bool OnInit();
	virtual void OnInitCmdLine(wxCmdLineParser& parser);
	virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
	bool filesystemBootstrap();
	void recurseCopy(wxString sourceDir, wxString destinationDir);
//private:
	wxString UserPath;
	wxString ProgramPath;
	wxString SkeletonPath;
	wxString languagePath;
	bool buildmode;

	bool postinstall;
};

static const wxCmdLineEntryDesc g_cmdLineDesc [] =
{
	{ wxCMD_LINE_SWITCH, wxT("postinstall"), wxT("postinstall"), wxT("do not use this")},
	{ wxCMD_LINE_SWITCH, wxT("buildmode"), wxT("buildmode"), wxT("do not use this")},
	{ wxCMD_LINE_NONE }
};


// from wxWidetgs wiki: http://wiki.wxwidgets.org/Calling_The_Default_Browser_In_WxHtmlWindow
class HtmlWindow: public wxHtmlWindow
{
public:
	HtmlWindow(wxWindow *parent, wxWindowID id = -1,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
		long style = wxHW_SCROLLBAR_AUTO, const wxString& name = _T("htmlWindow"));
	void OnLinkClicked(const wxHtmlLinkInfo& link);
};
 
HtmlWindow::HtmlWindow(wxWindow *parent, wxWindowID id, const wxPoint& pos,
	const wxSize& size, long style, const wxString& name)
: wxHtmlWindow(parent, id, pos, size, style, name)
{
}

void HtmlWindow::OnLinkClicked(const wxHtmlLinkInfo& link)
{
	wxString linkhref = link.GetHref();
	if (linkhref.StartsWith(_T("http://")))
	{
		if(!wxLaunchDefaultBrowser(linkhref))
			// failed to launch externally, so open internally
			wxHtmlWindow::OnLinkClicked(link);
	}
	else
	{
		wxHtmlWindow::OnLinkClicked(link);
	}
}

class KeySelectDialog;
// Define a new frame type: this is going to be our main frame
class MyDialog : public wxDialog
{
public:
	// ctor(s)
	MyDialog(const wxString& title, MyApp *_app);
	void UpdateOgreParams(bool scanDevices);
	void SetDefaults();
	bool LoadConfig();
	void SaveConfig();
	void updateSettingsControls(); // use after loading or after manually updating the settings map
	void getSettingsControls(); // puts the control's status into the settings map
	void remapControl();
	void loadInputControls();
	void testEvents();
	bool isSelectedControlGroup(wxTreeItemId *item = 0);
	// event handlers (these functions should _not_ be virtual)
	void OnQuit(wxCloseEvent& event);
	void OnTimer(wxTimerEvent& event);
	void OnTimerReset(wxTimerEvent& event);
	void OnDeviceChange(wxCommandEvent& event);
	void OnButCancel(wxCommandEvent& event);
	void OnButSave(wxCommandEvent& event);
	void OnButPlay(wxCommandEvent& event);
	void OnButRestore(wxCommandEvent& event);
	void OnTestNet(wxCommandEvent& event);
	void OnLinkClicked(wxHtmlLinkEvent& event);
	//void OnSightRangeScroll(wxScrollEvent& event);
	void onTreeSelChange(wxTreeEvent& event);
	void onChangeLanguageChoice(wxCommandEvent& event);
	//void OnButRemap(wxCommandEvent& event);
	void onActivateItem(wxTreeEvent& event);
	void OnButLoadKeymap(wxCommandEvent& event);
	void OnButSaveKeymap(wxCommandEvent& event);
	void onRightClickItem(wxTreeEvent& event);
	void OnMenuJoystickOptionsClick(wxCommandEvent& event);
	//void OnMenuClearClick(wxCommandEvent& event);
	//void OnMenuTestClick(wxCommandEvent& event);
	//void OnMenuDeleteClick(wxCommandEvent& event);
	//void OnMenuDuplicateClick(wxCommandEvent& event);
	//void OnMenuEditEventNameClick(wxCommandEvent& event);
	//void OnMenuCheckDoublesClick(wxCommandEvent& event);
	void OnButTestEvents(wxCommandEvent& event);
	void OnButAddKey(wxCommandEvent& event);
	void OnButDeleteKey(wxCommandEvent& event);
	void OnButClearCache(wxCommandEvent& event);
	void OnSimpleSliderScroll(wxScrollEvent& event);
	void OnSimpleSlider2Scroll(wxScrollEvent& event);

	void updateItemText(wxTreeItemId item, event_trigger_t *t);
//	void checkLinuxPaths();
	MyApp *app;
private:
//	float caelumFogDensity, SandStormFogStart;
	//bool postinstall;
	wxChoice *renderdevice;
	wxChoice *videomode;
	wxChoice *antialiasing;
	wxChoice *textfilt;
	wxCheckBox *fullscreen;
	wxCheckBox *vsync;
	wxChoice *sky;
	wxChoice *shadow;
	wxChoice *water;
	wxCheckBox *waves;
	wxCheckBox *enableFog;
	wxSlider *sightrange;
	wxSlider *simpleSlider;
	wxSlider *simpleSlider2;
	wxCheckBox *smoke;
	wxCheckBox *replaymode;
	wxCheckBox *dtm;
	wxCheckBox *dcm;
	wxCheckBox *dust;
	wxCheckBox *spray;
	wxCheckBox *cpart;
	wxCheckBox *heathaze;
	wxCheckBox *hydrax;
	wxCheckBox *dismap;
	wxCheckBox *enablexfire;
	wxCheckBox *beamdebug;
	wxCheckBox *extcam;
	wxCheckBox *dashboard;
	wxCheckBox *mirror;
	wxCheckBox *envmap;
	wxCheckBox *sunburn;
	wxCheckBox *bloom;
	wxCheckBox *mblur;
	wxChoice *sound;
	wxChoice *thread;
	wxChoice *flaresMode;
	wxChoice *languageMode;
	wxChoice *vegetationMode;
	wxChoice *screenShotFormat;
	wxCheckBox *wheel2;
	wxTreeListCtrl *cTree;
	wxTimer *timer1;
	//wxStaticText *controlText;
	//wxComboBox *ctrlTypeCombo;
	//wxString typeChoices[11];
	//wxButton *btnRemap;
	wxButton *btnUpdate;
	event_trigger_t *getSelectedControlEvent();
	KeySelectDialog *kd;
	int controlItemCounter;
	wxButton *btnAddKey;
	wxButton *btnDeleteKey;
	std::map<int,wxTreeItemId> treeItems;
	wxString InputMapFileName;

#ifdef NETWORK
	wxCheckBox *network_enable;
	wxTextCtrl *nickname;
	wxTextCtrl *servername;
	wxTextCtrl *serverport;
	wxTextCtrl *serverpassword;
	wxTextCtrl *usertoken;
	wxHtmlWindow *networkhtmw;
	HtmlWindow *helphtmw;
//	wxTextCtrl *p2pport;
#endif

	Ogre::RenderSystem *dx9;

	//Joysticks *joy;
//	wxTextCtrl *deadzone;
//	wxGauge *joygauges[256];
//	wxSlider *joygauges[256];
	//wxTimer *controlstimer;
	// any class wishing to process wxWidgets events must use this macro
	DECLARE_EVENT_TABLE()
};

class KeyAddDialog : public wxDialog
{
protected:
	MyDialog *dlg;
	event_trigger_t *t;
	std::map<std::string, std::string> inputevents;
	wxStaticText *desctext;
	std::string selectedEvent;
	std::string selectedType;
	std::string selectedOption;
	wxComboBox *cbe, *cbi;

	bool loadEventListing(const char* configpath)
	{
		char filename[255]="";
		char line[1025]="";
		sprintf(filename, "%sinputevents.cfg", configpath);
		FILE *f = fopen(filename, "r");
		if(!f)
			return false;
		while(fgets(line, 1024, f)!=NULL)
		{
			if(strnlen(line, 1024) > 5)
				processLine(line);
		}
		fclose(f);
		return true;
	}

	bool processLine(char *str)
	{
		char tmp[255]="";
		memset(tmp, 0, 255);
		char *desc = strstr(str, "\t");
		if(!desc)
			return false;
		strncpy(tmp, str, desc-str);
		if(strnlen(tmp, 250) < 3 || strnlen(desc, 250) < 3)
			return false;
		std::string eventName = std::string(tmp);
		std::string eventDescription = std::string(desc+1); // +1 because we strip the tab character with that
		inputevents[eventName] = eventDescription;
		return true;
	}
public:
	KeyAddDialog(MyDialog *_dlg) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(400,200), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), dlg(_dlg)
	{
		selectedEvent="";
		selectedType="";
		selectedOption="  ";
		wxString configpath=_dlg->app->UserPath+wxFileName::GetPathSeparator()+_T("config")+wxFileName::GetPathSeparator();
		loadEventListing(configpath.ToUTF8().data());

		wxStaticText *text = new wxStaticText(this, 4, _("Event Name: "), wxPoint(5,5), wxSize(70, 25));
		wxString eventChoices[MAX_EVENTS];
		std::map<std::string, std::string>::iterator it;
		int counter=0;
		for(it=inputevents.begin(); it!=inputevents.end(); it++, counter++)
		{
			if(counter>MAX_EVENTS)
				break;
			eventChoices[counter] = wxString(it->first.c_str(), wxConvUTF8);;
		}
		cbe = new wxComboBox(this, 1, _("Keyboard"), wxPoint(80,5), wxSize(300, 25), counter, eventChoices, wxCB_READONLY);
		cbe->SetSelection(0);


		desctext = new wxStaticText(this, wxID_ANY, _("Please select an input event."), wxPoint(5,35), wxSize(380, 80), wxST_NO_AUTORESIZE);
		desctext->Wrap(380);


		text = new wxStaticText(this, wxID_ANY, _("Input Type: "), wxPoint(5,120), wxSize(70,25));
		wxString ch[3];
		ch[0] = conv("Keyboard");
		ch[1] = conv("JoystickAxis");
		ch[2] = conv("JoystickButton");
		cbi = new wxComboBox(this, wxID_ANY, _("Keyboard"), wxPoint(80,120), wxSize(300, 25), 3, ch, wxCB_READONLY);

		wxButton *btnOK = new wxButton(this, 2, _("Add"), wxPoint(5,165), wxSize(190,25));
		btnOK->SetToolTip(wxString(_("Add the key")));

		wxButton *btnCancel = new wxButton(this, 3, _("Cancel"), wxPoint(200,165), wxSize(190,25));
		btnCancel->SetToolTip(wxString(_("close this dialog")));

		//SetBackgroundColour(wxColour("white"));
		Centre();
	}

	void onChangeEventComboBox(wxCommandEvent& event)
	{
		std::string s = inputevents[conv(cbe->GetValue())];
		desctext->SetLabel(_("Event Description: ") + conv(s.c_str()));
		selectedEvent = conv(cbe->GetValue());
	}

	void onChangeTypeComboBox(wxCommandEvent& event)
	{
		selectedType = conv(cbi->GetValue());
	}
	void onButCancel(wxCommandEvent& event)
	{
		EndModal(1);
	}
	void onButOK(wxCommandEvent& event)
	{
		selectedEvent = conv(cbe->GetValue());
		selectedType = conv(cbi->GetValue());
		// add some dummy values
		if(conv(selectedType) == conv("Keyboard"))
			selectedOption="";
		else if(conv(selectedType) == conv("JoystickAxis"))
			selectedOption=" 0";
		else if(conv(selectedType) == conv("JoystickButton"))
			selectedOption=" 0";

		EndModal(0);
	}
	std::string getEventName()
	{
		return selectedEvent;
	}
	std::string getEventType()
	{
		return selectedType;
	}
	std::string getEventOption()
	{
		return selectedOption;
	}
private:
	DECLARE_EVENT_TABLE()
};

#define MAX_TESTABLE_EVENTS 500

class KeyTestDialog : public wxDialog
{
protected:
	MyDialog *dlg;
	wxGauge *g[MAX_TESTABLE_EVENTS];
	wxStaticText *t[MAX_TESTABLE_EVENTS];
	wxTimer *timer;
public:
	KeyTestDialog(MyDialog *_dlg) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(500,600), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), dlg(_dlg)
	{
		// fixed unsed variable warning
		//wxStaticText *lt = new wxStaticText(this, wxID_ANY, _("Event Name"), wxPoint(5, 0), wxSize(200, 15));
		new wxStaticText(this, wxID_ANY, _("Event Name"), wxPoint(5, 0), wxSize(200, 15));
		new wxStaticText(this, wxID_ANY, _("Event Value (left = 0%, right = 100%)"), wxPoint(210, 0), wxSize(200, 15));
		new wxButton(this, 1, _("ok (end test)"), wxPoint(5,575), wxSize(490,20));
		wxScrolledWindow *vwin = new wxScrolledWindow(this, wxID_ANY, wxPoint(0,20), wxSize(490,550));

		std::map<std::string, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
		std::map<std::string, std::vector<event_trigger_t> >::iterator it;
		std::vector<event_trigger_t>::iterator it2;
		int counter = 0;
		for(it = events.begin(); it!= events.end(); it++)
		{
			for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
			{
				if(counter >= MAX_TESTABLE_EVENTS)
					break;
				g[counter] = new wxGauge(vwin, wxID_ANY, 1000, wxPoint(210, counter*20+5), wxSize(246, 15),wxGA_SMOOTH);
				t[counter] = new wxStaticText(vwin, wxID_ANY, conv(it->first), wxPoint(5, counter*20+5), wxSize(200, 15), wxALIGN_RIGHT|wxST_NO_AUTORESIZE);
			}
		}
		// resize scroll window
		vwin->SetScrollbars(0, 20, 0, counter);

		size_t hWnd = (size_t)this->GetHandle();
		if(!INPUTENGINE.setup(hWnd, true, false))
		{
			logfile->AddLine(conv("Unable to open inputs!"));logfile->Write();
		}

		timer = new wxTimer(this, 2);
		timer->Start(50);

	}
	void update()
	{
		std::map<std::string, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
		std::map<std::string, std::vector<event_trigger_t> >::iterator it;
		std::vector<event_trigger_t>::iterator it2;
		int counter = 0;
		for(it = events.begin(); it!= events.end(); it++)
		{
			for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
			{
				if(counter >= MAX_TESTABLE_EVENTS)
					break;
				float v = INPUTENGINE.getEventValue(it->first) * 1000;
				g[counter]->SetValue((int)v);
			}
		}
	}
	void OnButOK(wxCommandEvent& event)
	{
		EndModal(0);
	}
	void OnTimer(wxTimerEvent& event)
	{
		INPUTENGINE.Capture();
		update();
	}

private:
	DECLARE_EVENT_TABLE()
};


class KeySelectDialog : public wxDialog
{
protected:
	event_trigger_t *t;
	MyDialog *dlg;
	wxStaticText *text;
	wxStaticText *text2;
	wxButton *btnCancel;
	wxTimer *timer;
	std::string lastCombo;
	int lastBtn;
	float joyMinState[32];
	float joyMaxState[32];
	float lastJoyEventTime;

public:
	KeySelectDialog(MyDialog *_dlg, event_trigger_t *_t) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(200,200), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), t(_t), dlg(_dlg)
	{
		size_t hWnd = (size_t)this->GetHandle();
		bool captureMouse = false;
		if(t->eventtype == ET_MouseAxisX || t->eventtype == ET_MouseAxisY || t->eventtype == ET_MouseAxisZ || t->eventtype == ET_MouseButton)
			captureMouse = true;


		for(int x=0;x<32;x++)
		{
			joyMinState[x]=0;
			joyMaxState[x]=0;
		}

		if(!INPUTENGINE.setup(hWnd, true, captureMouse))
		{
			logfile->AddLine(conv("Unable to open default input map!"));logfile->Write();
		}

		INPUTENGINE.resetKeys();

		// setup GUI
		wxBitmap bitmap;
		//wxString imgFile=_("joycfg.bmp");
		if(t->eventtype == ET_MouseAxisX || t->eventtype == ET_MouseAxisY || t->eventtype == ET_MouseAxisZ || t->eventtype == ET_MouseButton)
			bitmap = wxBitmap(mousecfg_xpm);
			//imgFile=_("mousecfg.bmp");
		else if(t->eventtype == ET_Keyboard)
			bitmap = wxBitmap(keyboardcfg_xpm);
			//imgFile=_("keyboardcfg.bmp");
		else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel || t->eventtype == ET_JoystickButton || t->eventtype == ET_JoystickPov || t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY)
			bitmap = wxBitmap(joycfg_xpm);
			//imgFile=_("joycfg.bmp");

		new wxStaticPicture(this, -1, bitmap, wxPoint(0, 0), wxSize(200, 200),wxNO_BORDER);

		//btnCancel = new wxButton(this, 1, "Cancel", wxPoint(50,175), wxSize(100,20));
		wxString st = conv(t->tmp_eventname);
		text = new wxStaticText(this, wxID_ANY, st, wxPoint(5,130), wxSize(190,20), wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
		text2 = new wxStaticText(this, wxID_ANY, wxString(), wxPoint(5,150), wxSize(190,50), wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
		timer = new wxTimer(this, 2);
		timer->Start(50);

		SetBackgroundColour(wxColour(conv("white")));


		// centers dialog window on the screen
		Centre();

	}

	~KeySelectDialog()
	{
	}

	void closeWindow()
	{
		INPUTENGINE.destroy();
		timer->Stop();
		delete timer;
		delete text;
		delete text2;
		EndModal(0);
	}

	void update()
	{
		if(lastJoyEventTime > 0)
		{
			lastJoyEventTime-=0.05; // 50 milli seconds
			if(lastJoyEventTime <=0)
				closeWindow();
		}

		if(t->eventtype == ET_Keyboard)
		{
			std::string combo;
			int keys = INPUTENGINE.getCurrentKeyCombo(&combo);
			if(lastCombo != combo && keys != 0)
			{
				t->configline = combo;
				wxString s = conv(combo);
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastCombo = combo;
			} else if (lastCombo != combo && keys == 0)
			{
				wxString s = _("(Please press a key)");
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastCombo = combo;
			} else if(keys > 0)
			{
				closeWindow();
			}
		} else if(t->eventtype == ET_JoystickButton)
		{
			int btn = INPUTENGINE.getCurrentJoyButton();
			std::string str = conv(wxString::Format(_T("%d"), btn));
			if(btn != lastBtn && btn >= 0)
			{
				t->configline = str;
				t->joystickButtonNumber = btn;
				wxString s = conv(str);
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastBtn = btn;
			} else if (btn != lastBtn && btn < 0)
			{
				wxString s = _("(Please press a Joystick Button)");
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastBtn = btn;
			} else if(btn >= 0)
			{
				closeWindow();
			}
		} else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
		{
			std::string str = "";
			OIS::JoyStickState *j = INPUTENGINE.getCurrentJoyState();



			std::vector<OIS::Axis>::iterator it;
			int counter = 0;
			for(it=j->mAxes.begin();it!=j->mAxes.end();it++, counter++)
			{
				float value = 100*((float)it->abs/(float)OIS::JoyStick::MAX_AXIS);
				if(value < joyMinState[counter]) joyMinState[counter] = value;
				if(value > joyMaxState[counter]) joyMaxState[counter] = value;
				//str += "Axis " + wxString::Format(_T("%d"), counter) + ": " + wxString::Format(_T("%0.2f"), value) + "\n";
			}

			// check for delta on each axis
			str = conv(_("(Please move an Axis)\n"));
			float maxdelta=0;
			int selectedAxis = -1;
			for(int c=0;c<=counter;c++)
			{
				float delta = fabs((float)(joyMaxState[c]-joyMinState[c]));
				if(delta > maxdelta && delta > 50)
				{
					selectedAxis = c;
					maxdelta = delta;
				}
			}

			if(selectedAxis >= 0)
			{
				float delta = fabs((float)(joyMaxState[selectedAxis]-joyMinState[selectedAxis]));
				static float olddeta = 0;
				bool upper = (joyMaxState[selectedAxis] > 50);
				if(delta > 50 && delta < 120)
				{
					str = std::string("Axis ") + \
							conv(wxString::Format(_T("%d"), selectedAxis)) + \
							(upper?std::string(" UPPER"):std::string(" LOWER")); // + " - " + wxString::Format(_T("%f"), lastJoyEventTime);
					t->joystickAxisNumber = selectedAxis;
					t->joystickAxisRegion = (upper?1:-1); // half axis
					t->configline = (upper?std::string("UPPER"):std::string("LOWER"));
					if(olddeta != delta)
					{
						olddeta = delta;
						lastJoyEventTime = 1;
					}
				}
				else if(delta > 120)
				{
					str = std::string("Axis ") + conv(wxString::Format(_T("%d"), selectedAxis)); //+ " - " + wxString::Format(_T("%f"), lastJoyEventTime);
					t->joystickAxisNumber = selectedAxis;
					t->joystickAxisRegion = 0; // full axis
					t->configline = "";
					if(olddeta != delta)
					{
						olddeta = delta;
						lastJoyEventTime = 1;
					}
				}
			}
			//str = wxString::Format(_T("%f"), joyMaxState[0]) + "/" + wxString::Format(_T("%f"), joyMinState[0]) + " - " + wxString::Format(_T("%f"), fabs((float)(joyMaxState[0]-joyMinState[0])));
			wxString s = conv(str);
			if(text2->GetLabel() != s)
				text2->SetLabel(s);
		}
	}

	void OnButCancel(wxCommandEvent& event)
	{
		EndModal(0);
	}

	void OnTimer(wxTimerEvent& event)
	{
		INPUTENGINE.Capture();
		update();
	}

private:
	DECLARE_EVENT_TABLE()
};

class IDData : public wxTreeItemData
{
public:
	IDData(int id) : id(id) {}
	int id;
};


enum
{
	// command buttons
	command_cancel = 1,
	command_save,
	command_play,
	command_restore,
	device_change,
	net_test,
	CONTROLS_TIMER_ID,
	UPDATE_RESET_TIMER_ID,
	main_html,
	help_html,
	CTREE_ID,
	command_load_keymap,
	command_save_keymap,
	command_add_key,
	command_delete_key,
	command_testevents,
	clear_cache,
	EVC_LANG,
	SCROLL1,
	SCROLL2,
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------
// the event tables connect the wxWidgets events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(MyDialog, wxDialog)
	EVT_CLOSE(MyDialog::OnQuit)
	EVT_TIMER(CONTROLS_TIMER_ID, MyDialog::OnTimer)
	EVT_TIMER(UPDATE_RESET_TIMER_ID, MyDialog::OnTimerReset)
	EVT_CHOICE(device_change, MyDialog::OnDeviceChange)
	EVT_BUTTON(command_cancel, MyDialog::OnButCancel)
	EVT_BUTTON(command_save, MyDialog::OnButSave)
	EVT_BUTTON(command_play, MyDialog::OnButPlay)
	EVT_BUTTON(command_restore, MyDialog::OnButRestore)
	EVT_BUTTON(net_test, MyDialog::OnTestNet)
	EVT_BUTTON(clear_cache, MyDialog::OnButClearCache)
	EVT_HTML_LINK_CLICKED(main_html, MyDialog::OnLinkClicked)
	//EVT_SCROLL(MyDialog::OnSightRangeScroll)
	EVT_COMMAND_SCROLL_CHANGED(SCROLL1, MyDialog::OnSimpleSliderScroll)
	EVT_COMMAND_SCROLL_CHANGED(SCROLL2, MyDialog::OnSimpleSlider2Scroll)
	EVT_CHOICE(EVC_LANG, MyDialog::onChangeLanguageChoice)
	//EVT_BUTTON(BTN_REMAP, MyDialog::OnButRemap)

	EVT_TREE_SEL_CHANGING (CTREE_ID, MyDialog::onTreeSelChange)
	EVT_TREE_ITEM_ACTIVATED(CTREE_ID, MyDialog::onActivateItem)
	EVT_TREE_ITEM_RIGHT_CLICK(CTREE_ID, MyDialog::onRightClickItem)

	EVT_BUTTON(command_load_keymap, MyDialog::OnButLoadKeymap)
	EVT_BUTTON(command_save_keymap, MyDialog::OnButSaveKeymap)

	EVT_BUTTON(command_add_key, MyDialog::OnButAddKey)
	EVT_BUTTON(command_delete_key, MyDialog::OnButDeleteKey)
	EVT_BUTTON(command_testevents, MyDialog::OnButTestEvents)

	EVT_MENU(1, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(20, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(21, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(22, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(23, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(24, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(25, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(30, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(31, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(32, MyDialog::OnMenuJoystickOptionsClick)

END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeySelectDialog, wxDialog)
	EVT_BUTTON(1, KeySelectDialog::OnButCancel)
	EVT_TIMER(2, KeySelectDialog::OnTimer)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeyAddDialog, wxDialog)
	EVT_COMBOBOX(1, KeyAddDialog::onChangeEventComboBox)
	EVT_BUTTON(2, KeyAddDialog::onButOK)
	EVT_BUTTON(3, KeyAddDialog::onButCancel)
	EVT_COMBOBOX(4, KeyAddDialog::onChangeTypeComboBox)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeyTestDialog, wxDialog)
	EVT_BUTTON(1, KeyTestDialog::OnButOK)
	EVT_TIMER(2, KeyTestDialog::OnTimer)
END_EVENT_TABLE()

// Create a new application object: this macro will allow wxWidgets to create
// the application object during program execution (it's better than using a
// static object for many reasons) and also implements the accessor function
// wxGetApp() which will return the reference of the right type (i.e. MyApp and
// not wxApp)

IMPLEMENT_APP(MyApp)

/*
IMPLEMENT_APP_NO_MAIN(MyApp)

int main(int argc, char **argv)
{
	MyApp app;
	app.OnInit();
}
*/

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

const wxLanguageInfo *getLanguageInfoByName(const wxString name)
{
	if(name.size() == 2)
	{
		for(int i=0;i<400;i++)
		{
			const wxLanguageInfo *info = wxLocale::GetLanguageInfo(i);
			if(!info)
				continue;
			wxString cn = info->CanonicalName.substr(0,2);
			if(cn == name)
				return info;
		}
	} else
	{
		for(int i=0;i<400;i++)
		{
			const wxLanguageInfo *info = wxLocale::GetLanguageInfo(i);
			if(!info)
				continue;
			if(info->Description == name)
				return info;
		}
	}
	return 0;
}

int getAvLang(wxString dir, std::vector<wxLanguageInfo*> &files)
{
	//WX portable version
	wxDir dp(dir);
	if (!dp.IsOpened())
	{
		printf("error opening %s\n", dir.c_str());
		return -1;
	}
	wxString name;
	if (dp.GetFirst(&name))
	{
		do
		{
			wxChar dot=_T('.');
			if (name.StartsWith(&dot))
			{
				// do not add dot directories
				continue;
			}
			if(name.Len() != 2)
			{
				// do not add other than language 2 letter code style directories
				continue;
			}
			wxLanguageInfo *li = const_cast<wxLanguageInfo *>(getLanguageInfoByName(name));
			if(li)
			{
				files.push_back(li);
			} else
			{
				logfile->AddLine(conv("failed to get Language: "+conv(name)));logfile->Write();				
			}
		} while (dp.GetNext(&name));
	}
	return 0;
}

void initLanguage(wxString languagePath, wxString userpath)
{
	// add language stuff
	
	// this prevents error messages
	wxLogNull noLog;
	// Initialize the catalogs we'll be using
	wxString langfile = _T("ror");
	wxString dirsep = _T("/");
	wxString basedir = languagePath;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep = _T("\\");
#endif
	//basedir = basedir + dirsep + _T("languages");
	wxLocale::AddCatalogLookupPathPrefix(languagePath);
	
	// get all available languages	
	getAvLang(languagePath, avLanguages);
	logfile->AddLine(conv("searching languages in ")+basedir);logfile->Write();
	if(avLanguages.size() > 0)
	{
		printf("Available Languages:\n");
		logfile->AddLine(_T("Available Languages:"));logfile->Write();
		std::vector<wxLanguageInfo *>::iterator it;
		for(it = avLanguages.begin(); it!=avLanguages.end(); it++)
			if(*it)
			{
				logfile->AddLine((*it)->Description);logfile->Write();
				printf(" * %s (%s)\n", conv((*it)->Description).c_str(), conv((*it)->CanonicalName).c_str());
			}
	}else
	{
		logfile->AddLine(_T("no Languages found!"));logfile->Write();
		printf("no Languages found!\n");
	}


	wxString rorcfg=userpath + dirsep + _T("config") + dirsep + _T("RoR.cfg");
	Ogre::ImprovedConfigFile cfg;
	// Don't trim whitespace
	cfg.load(rorcfg.ToUTF8().data(), "=:\t", false);
	wxString langSavedName = conv(cfg.getSetting("Language"));

	if(langSavedName.size() > 0)
		language = const_cast<wxLanguageInfo *>(getLanguageInfoByName(langSavedName));
	if(language == 0)
	{
		logfile->AddLine(conv("asking system for default language."));logfile->Write();
		printf("asking system for default language.\n");
		language = const_cast<wxLanguageInfo *>(wxLocale::GetLanguageInfo(locale.GetSystemLanguage()));
		if(language)
			printf(" system returned: %s (%s)\n", conv(language->Description).c_str(), conv(language->CanonicalName).c_str());
		else
			printf(" error getting language information!\n");
	}
	logfile->AddLine(conv("preferred language: "+conv(language->Description)));logfile->Write();
	printf("preferred language: %s\n", conv(language->Description).c_str());
	wxString lshort = language->CanonicalName.substr(0, 2);
	wxString tmp = basedir + dirsep + lshort + dirsep + langfile + _T(".mo");
	printf("lang file: %s\n", tmp.c_str());
	logfile->AddLine(wxString(_T("lang file: "))+tmp);logfile->Write();
	if(wxFileName::FileExists(tmp))
	{
		printf("language existing, using it!\n");
		if(!locale.IsAvailable((wxLanguage)language->Language))
		{
			printf("language file existing, but not found via wxLocale!\n");
			printf("is the language installed on your system?\n");
		}
		bool res = locale.Init((wxLanguage)language->Language, wxLOCALE_CONV_ENCODING);
		if(!res)
		{
			printf("error while initializing language!\n");
		}
		res = locale.AddCatalog(langfile);
		if(!res)
		{
			printf("error while loading language!\n");
		}


	}
	else
	{
		locale.Init(wxLANGUAGE_DEFAULT, wxLOCALE_CONV_ENCODING);
		printf("language not existing, no locale support!\n");
	}
}

void MyApp::recurseCopy(wxString sourceDir, wxString destinationDir)
{
	if (!wxDir::Exists(sourceDir) || !wxDir::Exists(destinationDir)) return;

	wxDir srcd(sourceDir);
	wxString src;
	if (!srcd.GetFirst(&src)) return; //empty dir!
	do
	{
		//ignore files and directories beginning with "." (important, for SVN!)
		if (src.StartsWith(_T("."))) continue;
		//check if it id a directory
		wxFileName tsfn=wxFileName(sourceDir, wxEmptyString);
		tsfn.AppendDir(src);
		if (wxDir::Exists(tsfn.GetPath()))
		{
			//this is a directory, create dest dir and recurse
			wxFileName tdfn=wxFileName(destinationDir, wxEmptyString);
			tdfn.AppendDir(src);
			if (!wxDir::Exists(tdfn.GetPath())) tdfn.Mkdir();
			recurseCopy(tsfn.GetPath(), tdfn.GetPath());
		}
		else
		{
			//this is a file, copy file
			tsfn=wxFileName(sourceDir, src);
			wxFileName tdfn=wxFileName(destinationDir, src);
			//policy to be defined
			if (tdfn.FileExists())
			{
				//file already exists, should we overwrite?
//				::wxCopyFile(tsfn.GetFullPath(), tdfn.GetFullPath());
			}
			else
			{
				::wxCopyFile(tsfn.GetFullPath(), tdfn.GetFullPath());
			}
		}
	} while (srcd.GetNext(&src));
}

bool MyApp::filesystemBootstrap()
{
	//find the root of the program and user directory
	//these routines should set ProgramPath and UserPath
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	WCHAR Wuser_path[1024];
	WCHAR Wprogram_path[1024];
	if (!GetModuleFileNameW(NULL, Wprogram_path, 512))
	{
		return false;
	}
	GetShortPathName(Wprogram_path, Wprogram_path, 512); //this is legal
	ProgramPath=wxFileName(wxString(Wprogram_path)).GetPath();

	if (SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, Wuser_path)!=S_OK)
	{
		return false;
	}
	GetShortPathName(Wuser_path, Wuser_path, 512); //this is legal
	wxFileName tfn=wxFileName(Wuser_path, wxEmptyString);
	tfn.AppendDir(_T("Rigs of Rods"));
	UserPath=tfn.GetPath();
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	//true program path is impossible to get from POSIX functions
	//lets hack!
	pid_t pid = getpid();
	char procpath[256];
	char user_path[1024];
	char program_path[1024];
	sprintf(procpath, "/proc/%d/exe", pid);
	int ch = readlink(procpath,program_path,240);
	if (ch != -1)
	{
		program_path[ch] = 0;
	} else return false;
	ProgramPath=wxFileName(wxString(conv(program_path))).GetPath();
	//user path is easy
	strncpy(user_path, getenv ("HOME"), 240);
	wxFileName tfn=wxFileName(conv(user_path), wxEmptyString);
	tfn.AppendDir(_T("RigsOfRods"));
	UserPath=tfn.GetPath();
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	//found this code, will look later
	std::string path = "./";
	ProcessSerialNumber PSN;
	ProcessInfoRec pinfo;
	FSSpec pspec;
	FSRef fsr;
	OSStatus err;
	/* set up process serial number */
	PSN.highLongOfPSN = 0;
	PSN.lowLongOfPSN = kCurrentProcess;
	/* set up info block */
	pinfo.processInfoLength = sizeof(pinfo);
	pinfo.processName = NULL;
	pinfo.processAppSpec = &pspec;
	/* grab the vrefnum and directory */
	err = GetProcessInformation(&PSN, &pinfo);
	if (! err ) {
	char c_path[2048];
	FSSpec fss2;
	int tocopy;
	err = FSMakeFSSpec(pspec.vRefNum, pspec.parID, 0, &fss2);
	if ( ! err ) {
	err = FSpMakeFSRef(&fss2, &fsr);
	if ( ! err ) {
	char c_path[2049];
	err = (OSErr)FSRefMakePath(&fsr, (UInt8*)c_path, 2048);
	if (! err ) {
	path = c_path;
	}
	}
#endif
	//skeleton
	wxFileName tsk=wxFileName(ProgramPath, wxEmptyString);
	tsk.AppendDir(_T("skeleton"));
	SkeletonPath=tsk.GetPath();
	tsk=wxFileName(ProgramPath, wxEmptyString);
	tsk.AppendDir(_T("languages"));
	languagePath=tsk.GetPath();
	//buildmode
	if (buildmode)
	{
		//change the paths
		wxFileName tfn=wxFileName(ProgramPath, wxEmptyString); // RoRdev\build\bin\release\windows
		tfn.RemoveLastDir(); // RoRdev\build\bin\release
		tfn.RemoveLastDir(); // RoRdev\build\bin
		tfn.RemoveLastDir(); // RoRdev\build
		wxFileName ttfn=tfn;
		ttfn.AppendDir(_T("configurator"));
//		ProgramPath=ttfn.GetPath();
		ttfn.AppendDir(_T("confdata"));
		languagePath=ttfn.GetPath();
		ttfn=tfn;
		ttfn.AppendDir(_T("skeleton"));
		SkeletonPath=ttfn.GetPath();
		ttfn=tfn;
		ttfn.AppendDir(_T("userspace"));
		UserPath=ttfn.GetPath();
	}
	//okay
	if (!wxFileName::DirExists(UserPath))
	{
		//maybe warn the user we are creating this space
		wxFileName::Mkdir(UserPath);
	}
	//okay, the folder exists
	//copy recursively the skeleton
	recurseCopy(SkeletonPath, UserPath);

	return true;

}

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
	buildmode=false;
	if (argc==2 && wxString(argv[1])==_T("/buildmode")) buildmode=true;
	//setup the user filesystem
	if (!filesystemBootstrap()) return false;
	// open logfile	
	wxFileName clfn=wxFileName(UserPath, wxEmptyString);
	clfn.AppendDir(_T("logs"));
	clfn.SetFullName(_T("configlog.txt"));
	logfile=new wxTextFile(clfn.GetFullPath());
	if (logfile->Exists()) {logfile->Open();logfile->Clear();} else logfile->Create();
	logfile->AddLine(conv("Log created"));logfile->Write();

	initLanguage(languagePath, UserPath);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// add windows specific crash handler. It will create nice memory dumps, so we can track the error
	//LPVOID lpvState = Install(NULL, NULL, NULL);
	//AddFile(lpvState, conv("configlog.txt"), conv("Rigs of Rods Configurator Crash log"));
#endif

	// call the base class initialization method, currently it only parses a
	// few common command-line options but it could be do more in the future
	if ( !wxApp::OnInit() )
		return false;

	logfile->AddLine(conv("Creating dialog"));logfile->Write();
	// create the main application window
	MyDialog *dialog = new MyDialog(_("Rigs of Rods configuration"), this);

	// and show it (the frames, unlike simple controls, are not shown when
	// created initially)
	logfile->AddLine(conv("Showing dialog"));logfile->Write();
	dialog->Show(true);
	SetTopWindow(dialog);
	SetExitOnFrameDelete(false);
	// success: wxApp::OnRun() will be called which will enter the main message
	// loop and the application will run. If we returned false here, the
	// application would exit immediately.
	logfile->AddLine(conv("App ready"));logfile->Write();
	return true;
}

void MyApp::OnInitCmdLine(wxCmdLineParser& parser)
{
	parser.SetDesc (g_cmdLineDesc);
	// must refuse '/' as parameter starter or cannot use "/path" style paths
	parser.SetSwitchChars (conv("/"));
}

bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	postinstall = parser.Found(conv("postinstall"));
	buildmode = parser.Found(conv("buildmode"));
	return true;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MyDialog::MyDialog(const wxString& title, MyApp *_app)
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	: wxDialog(NULL, wxID_ANY, title,  wxPoint(100, 100), wxSize(500, 580), wxRESIZE_BORDER)
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	: wxDialog(NULL, wxID_ANY, title,  wxPoint(100, 100), wxSize(500, 580), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxRESIZE_BOX)
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	: wxDialog(NULL, wxID_ANY, title,  wxPoint(100, 100), wxSize(500, 580), wxRESIZE_BORDER)
#endif
{
	app=_app;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	SetWindowVariant(wxWINDOW_VARIANT_MINI); //smaller texts for macOS
#endif
	SetWindowStyle(wxRESIZE_BORDER | wxCAPTION);
	SetMinSize(wxSize(300,300));

	//no longer needed
//	checkLinuxPaths();

	logfile->AddLine(conv("InputEngine starting"));logfile->Write();
	wxFileName tfn=wxFileName(app->UserPath, wxEmptyString);
	tfn.AppendDir(_T("config"));
	size_t hWnd = (size_t)this->GetHandle();
	logfile->AddLine(conv("Searching input.map in ")+tfn.GetPath());logfile->Write();
	InputMapFileName=tfn.GetPath()+wxFileName::GetPathSeparator()+_T("input.map");
	std::string path = ((tfn.GetPath()+wxFileName::GetPathSeparator()).ToUTF8().data());
	if(!INPUTENGINE.setup(hWnd, false, false, 0))
	{
		logfile->AddLine(conv("Unable to setup inputengine!"));logfile->Write();
	} else
	{
		if (!INPUTENGINE.loadMapping(path+"input.map"))
		{
			logfile->AddLine(conv("Unable to open default input map!"));logfile->Write();
		}
	}
	logfile->AddLine(conv("InputEngine started"));logfile->Write();
	kd=0;
	controlItemCounter = 0;


//	SandStormFogStart = 500;
//	caelumFogDensity = 0.005;

	//postinstall = _postinstall;

	dx9=0;
	wxFileSystem::AddHandler( new wxInternetFSHandler );
	//build dialog here
	wxToolTip::Enable(true);
	wxToolTip::SetDelay(100);
	//image
//	wxBitmap *bitmap=new wxBitmap("data\\config.png", wxBITMAP_TYPE_BMP);
	//logfile->AddLine(conv("Loading bitmap"));logfile->Write();
	wxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->SetSizeHints(this);

	// using xpm now
	const wxBitmap bm(config_xpm);
	wxStaticPicture *imagePanel = new wxStaticPicture(this, -1, bm,wxPoint(0, 0), wxSize(500, 100), wxNO_BORDER);
	mainsizer->Add(imagePanel, 0, wxGROW);

	//notebook
	wxNotebook *nbook=new wxNotebook(this, -1, wxPoint(3, 100), wxSize(490, 415));
	mainsizer->Add(nbook, 2, wxGROW);

	wxSizer *btnsizer = new wxBoxSizer(wxHORIZONTAL);
	//buttons
	if(!app->postinstall)
	{
		wxButton *btnRestore = new wxButton( this, command_restore, _("Restore Defaults"), wxPoint(35,520), wxSize(100,25));
		btnRestore->SetToolTip(_("Restore the default configuration."));
		btnsizer->Add(btnRestore, 1, wxGROW | wxALL, 5);

		wxButton *btnPlay = new wxButton( this, command_play, _("Save and Play"), wxPoint(140,520), wxSize(100,25));
		btnPlay->SetToolTip(_("Save the current configuration and start RoR."));
		btnsizer->Add(btnPlay, 1, wxGROW | wxALL, 5);

		wxButton *btnAbout = new wxButton( this, command_save, _("Save and Exit"), wxPoint(245,520), wxSize(100,25));
		btnAbout->SetToolTip(_("Save the current configuration and close the configuration program."));
		btnsizer->Add(btnAbout, 1, wxGROW | wxALL, 5);

		wxButton *btnClose = new wxButton( this, command_cancel, _("Cancel"), wxPoint(350,520), wxSize(100,25));
		btnClose->SetToolTip(_("Cancel the configuration changes and close the configuration program."));
		btnsizer->Add(btnClose, 1, wxGROW | wxALL, 5);
	} else
	{
		wxButton *btnPlay = new wxButton( this, command_play, _("Save and Play"), wxPoint(350,520), wxSize(100,25));
		btnPlay->SetToolTip(_("Save the current configuration and start RoR."));
		btnsizer->Add(btnPlay, 1, wxGROW | wxALL, 5);
	}
	mainsizer->Add(btnsizer, 0, wxGROW);
	this->SetSizer(mainsizer);

	//adding notebook elements
	wxPanel *simpleSettingsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(simpleSettingsPanel, _("Simple Settings"), true);

	wxPanel *graphicsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(graphicsPanel, _("Graphics"), true);

	wxPanel *soundsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(soundsPanel, _("Sounds"), false);

	wxPanel *cpuPanel=new wxPanel(nbook, -1);
	nbook->AddPage(cpuPanel, _("CPU"), false);

	wxPanel *controlsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(controlsPanel, _("Controls"), false);
	wxSizer *sizer_controls = new wxBoxSizer(wxVERTICAL);
	controlsPanel->SetSizer(sizer_controls);

	int maxTreeHeight = 310;
	int maxTreeWidth = 480;
	cTree = new wxTreeListCtrl(controlsPanel,
                       CTREE_ID,
                       wxPoint(0, 0),
					   wxSize(maxTreeWidth, maxTreeHeight));

	long style = cTree->GetWindowStyle();
	//style |= wxTR_HAS_BUTTONS;
	style |= wxTR_FULL_ROW_HIGHLIGHT;
	style |= wxTR_HIDE_ROOT;
	style |= wxTR_ROW_LINES;
	cTree->SetWindowStyle(style);
	sizer_controls->Add(cTree, 2, wxGROW);

	wxPanel *controlsInfoPanel = new wxPanel(controlsPanel, wxID_ANY, wxPoint(0, 0 ), wxSize( maxTreeWidth, 390 ));
	sizer_controls->Add(controlsInfoPanel, 0, wxGROW);

	// controlText = new wxStaticText(controlsInfoPanel, wxID_ANY, wxString("Key description..."), wxPoint(5,5));

	/*
	typeChoices[0] = "None";
	typeChoices[1] = "Keyboard";
	typeChoices[2] = "MouseButton";
	typeChoices[3] = "MouseAxisX";
	typeChoices[4] = "MouseAxisY";
	typeChoices[5] = "MouseAxisZ";
	typeChoices[6] = "JoystickButton";
	typeChoices[7] = "JoystickAxis";
	typeChoices[8] = "JoystickPov";
	typeChoices[9] = "JoystickSliderX";
	typeChoices[10] = "JoystickSliderY";
	*/
	//ctrlTypeCombo = new wxComboBox(controlsInfoPanel, EVCB_BOX, "Keyboard", wxPoint(5,25), wxSize(150, 20), 11, typeChoices, wxCB_READONLY);

	btnAddKey = new wxButton(controlsInfoPanel, command_add_key, _("Add"), wxPoint(5,5), wxSize(120, 50));
	btnDeleteKey = new wxButton(controlsInfoPanel, command_delete_key, _("Delete selected"), wxPoint(130,5), wxSize(120, 50));
	btnDeleteKey->Enable(false);

	//wxButton *btnLoadKeyMap = 
	new wxButton(controlsInfoPanel, command_load_keymap, _("Import Keymap"), wxPoint(maxTreeWidth-125,5), wxSize(120,25));
	//wxButton *btnSaveKeyMap = 
	new wxButton(controlsInfoPanel, command_save_keymap, _("Export Keymap"), wxPoint(maxTreeWidth-125,30), wxSize(120,25));

	//wxButton *btnTest = 
	new wxButton(controlsInfoPanel, command_testevents, _("Test"), wxPoint(255,5), wxSize(95, 50));

	//btnRemap = new wxButton( controlsInfoPanel, BTN_REMAP, "Remap", wxPoint(maxTreeWidth-250,5), wxSize(120,45));

	loadInputControls();

#ifdef NETWORK
	wxPanel *netPanel=new wxPanel(nbook, -1);
	nbook->AddPage(netPanel, _("Network"), false);
#endif
	wxPanel *advancedPanel=new wxPanel(nbook, -1);
	nbook->AddPage(advancedPanel, _("Advanced"), false);

	wxPanel *helpPanel=new wxPanel(nbook, -1);
	nbook->AddPage(helpPanel, _("Community / Support"), false);
	
//	wxPanel *aboutPanel=new wxPanel(nbook, -1);
//	nbook->AddPage(aboutPanel, "About", false);

	wxStaticText *dText = 0;

	// simple settings panel
	dText = new wxStaticText(simpleSettingsPanel, -1, _("This can help you to set the Configurations in a simple way.\nJust move the slider below and press save and play."), wxPoint(10,10));

	dText = new wxStaticText(simpleSettingsPanel, -1, _("CPU Settings"), wxPoint(10,80));

	simpleSlider=new wxSlider(simpleSettingsPanel, SCROLL1, 1, 0, 2, wxPoint(10, 100), wxSize(450, 40));
	simpleSlider->SetToolTip(_("Sets all settings in a simple way"));

	dText = new wxStaticText(simpleSettingsPanel, -1, _("High Performance"), wxPoint(10,140));
	dText = new wxStaticText(simpleSettingsPanel, -1, _("Balanced"), wxPoint(220,140));
	dText = new wxStaticText(simpleSettingsPanel, -1, _("High Quality"), wxPoint(400,140));

	dText = new wxStaticText(simpleSettingsPanel, -1, _("Graphics Settings"), wxPoint(10,240));

	simpleSlider2=new wxSlider(simpleSettingsPanel, SCROLL2, 1, 0, 2, wxPoint(10, 260), wxSize(450, 40));
	simpleSlider2->SetToolTip(_("Sets all settings in a simple way"));

	dText = new wxStaticText(simpleSettingsPanel, -1, _("High Performance"), wxPoint(10,300));
	dText = new wxStaticText(simpleSettingsPanel, -1, _("Balanced"), wxPoint(220,300));
	dText = new wxStaticText(simpleSettingsPanel, -1, _("High Quality"), wxPoint(400,300));

	// TODO: add code behind the slider ...

	//graphics panel
	wxStaticBox *dBox = new wxStaticBox(graphicsPanel, -1, _("Screen configuration"), wxPoint(5,5), wxSize(470, 125));

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dText = new wxStaticText(graphicsPanel, -1, _("Device:"), wxPoint(10,25));
	renderdevice=new wxChoice(graphicsPanel, device_change, wxPoint(115, 25), wxSize(200, -1), 0);
	renderdevice->SetToolTip(_("Select the graphics card you want to use.\nIf you have more than one video card, you should choose the fastest."));
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	dText = new wxStaticText(graphicsPanel, device_change, _("RTT Mode:"), wxPoint(10,28));
	renderdevice=new wxChoice(graphicsPanel, device_change, wxPoint(115, 25), wxSize(200, -1), 0);
	renderdevice->SetToolTip(_("Select the preferred Render To Texture mode.\nChange this in case of artefacts in water, dashboard or mirrors, or to improve performances."));
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	dText = new wxStaticText(graphicsPanel, device_change, _("RTT Mode:"), wxPoint(10,28));
	renderdevice=new wxChoice(graphicsPanel, device_change, wxPoint(115, 25), wxSize(200, -1), 0);
	renderdevice->SetToolTip(_("Select the preferred Render To Texture mode.\nChange this in case of artefacts in water, dashboard or mirrors, or to improve performances."));
#endif
	dText = new wxStaticText(graphicsPanel, -1, _("Video mode:"), wxPoint(10,53));
	videomode=new wxChoice(graphicsPanel, -1, wxPoint(115, 50), wxSize(200, -1), 0);
	videomode->SetToolTip(_("Select the video mode you want to use.\nSmaller screen sizes may be faster on some hardwares.\n16-bit colour modes uses less video memory, but adds strange colour artifacts."));

	dText = new wxStaticText(graphicsPanel, -1, _("Anti-aliasing:"), wxPoint(10,78));
	antialiasing=new wxChoice(graphicsPanel, -1, wxPoint(115, 75), wxSize(200, -1), 0);
	antialiasing->SetToolTip(_("Gives a smoother image and reduces jagged edges,\nbut may be incompatible with several other options, including visual effects and water effects."));

	dText = new wxStaticText(graphicsPanel, -1, _("Texture filtering:"), wxPoint(10,103));
	textfilt=new wxChoice(graphicsPanel, -1, wxPoint(115, 100), wxSize(200, -1), 0);
	textfilt->Append(conv("None (fastest)"));
	textfilt->Append(conv("Bilinear"));
	textfilt->Append(conv("Trilinear"));
	textfilt->Append(conv("Anisotropic (best looking)"));
	textfilt->SetToolTip(_("Most recent hardware can do Anisotropic filtering without significant slowdown.\nUse lower settings only on old or poor video chipsets."));

	fullscreen=new wxCheckBox(graphicsPanel, -1, _("Full screen"), wxPoint(350, 53));
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	fullscreen->Disable(); //fullscreen is dangerous on a mac
#endif
	fullscreen->SetToolTip(_("Full screen mode uses a bit less memory resources"));

	vsync=new wxCheckBox(graphicsPanel, -1, _("VSync"), wxPoint(350, 78));
	vsync->SetToolTip(_("Check this only if you have way more than 60fps, to avoid visual artifacts.\nBelow 60fps this is a waste of CPU time."));

	dBox = new wxStaticBox(graphicsPanel, -1, _("Special effects"), wxPoint(5,130), wxSize(470, 250));

	dText = new wxStaticText(graphicsPanel, -1, _("Sky type:"), wxPoint(10,153));
	sky=new wxChoice(graphicsPanel, -1, wxPoint(115, 150), wxSize(200, -1), 0);
	sky->Append(conv("Sandstorm (fastest)"));
	sky->Append(conv("Caelum (best looking, slower)"));
	sky->SetToolTip(_("Caelum sky is nice but quite slow unless you have a high-powered GPU."));

	dText = new wxStaticText(graphicsPanel, -1, _("Shadow type:"), wxPoint(10,178));
	shadow=new wxChoice(graphicsPanel, -1, wxPoint(115, 175), wxSize(200, -1), 0);
	shadow->Append(conv("No shadows (fastest)"));
	shadow->Append(conv("Texture shadows"));
	shadow->Append(conv("Stencil shadows (best looking)"));
	shadow->SetToolTip(_("Texture shadows are fast but limited: jagged edges, no object self-shadowing, limited shadow distance.\nStencil shadows are slow but gives perfect shadows."));

	dText = new wxStaticText(graphicsPanel, -1, _("Water type:"), wxPoint(10,203));
	water=new wxChoice(graphicsPanel, -1, wxPoint(115, 200), wxSize(200, -1), 0);
//	water->Append(wxString("None")); //we don't want that!
	water->Append(conv("Basic (fastest)"));
	water->Append(conv("Reflection"));
	water->Append(conv("Reflection + refraction (speed optimized)"));
	water->Append(conv("Reflection + refraction (quality optimized)"));
	water->SetToolTip(_("Water reflection is slower, and requires a good GPU. In speed optimized mode you may experience excessive camera shaking."));

	waves=new wxCheckBox(graphicsPanel, -1, _("Enable waves"), wxPoint(115, 229));
	waves->SetToolTip(_("Enabling waves adds a lot of fun in boats, but can have a big performance impact on some specific hardwares."));

	spray=new wxCheckBox(graphicsPanel, -1, _("Water spray"), wxPoint(115, 253));
	spray->SetToolTip(_("No effect unless you mess with water, looks pretty good also."));

	dText = new wxStaticText(graphicsPanel, -1, _("Vegetation:"), wxPoint(10, 281));
	vegetationMode=new wxChoice(graphicsPanel, -1, wxPoint(115, 278), wxSize(200, -1), 0);
	vegetationMode->Append(conv("None (fastest)"));
	vegetationMode->Append(conv("20%"));
	vegetationMode->Append(conv("50%"));
	vegetationMode->Append(conv("Full (best looking, slower)"));
	vegetationMode->SetToolTip(_("This determines how much grass and how many trees (and such objects) should get displayed."));


	dBox = new wxStaticBox(graphicsPanel, -1, _("Particle systems"), wxPoint(340,140), wxSize(130, 60));
	dust=new wxCheckBox(graphicsPanel, -1, _("Dust"), wxPoint(350, 160));
	dust->SetToolTip(_("This may hurt framerate a bit on old systems, but it looks pretty good."));

	heathaze=new wxCheckBox(graphicsPanel, -1, _("HeatHaze"), wxPoint(350, 180));
	heathaze->SetToolTip(_("Heat Haze from engines, major slowdown. (only activate with recent hardware)"));


	dBox = new wxStaticBox(graphicsPanel, -1, _("Cockpit options"), wxPoint(340,225), wxSize(130, 45));
	mirror=new wxCheckBox(graphicsPanel, -1, _("Mirrors"), wxPoint(350, 245));
	mirror->SetToolTip(_("Shows the rear view mirrors in 1st person view. May cause compatibility problems for very old video cards."));

	dBox = new wxStaticBox(graphicsPanel, -1, _("Visual effects"), wxPoint(340,278), wxSize(130, 85));
	sunburn=new wxCheckBox(graphicsPanel, -1, _("Sunburn"), wxPoint(350, 293));
	sunburn->SetToolTip(_("Requires a recent video card. Adds a bluish blinding effect."));
	bloom=new wxCheckBox(graphicsPanel, -1, _("Bloom"), wxPoint(350, 313));
	bloom->SetToolTip(_("Requires a recent video card. Adds a light blurring effect."));
	mblur=new wxCheckBox(graphicsPanel, -1, _("Motion blur"), wxPoint(350, 333));
	mblur->SetToolTip(_("Requires a recent video card. Adds a motion blur effect."));

	envmap=new wxCheckBox(graphicsPanel, -1, _("High quality reflective effects"), wxPoint(115, 310));
	envmap->SetToolTip(_("Enable high quality reflective effects. Causes a slowdown."));

	//sounds panel
	dText = new wxStaticText(soundsPanel, -1, _("Sound rendering:"), wxPoint(20,28));
	sound=new wxChoice(soundsPanel, -1, wxPoint(150, 25), wxSize(200, -1), 0);
	sound->Append(conv("No sound"));
	sound->Append(conv("Software (stereo)"));
	sound->Append(conv("Hardware (3D)"));
	sound->SetToolTip(_("Software sound is stereo only.\nHardware sound will uses all your speakers, but this mode is buggy with some sound card drivers."));

	//cpu panel
	dText = new wxStaticText(cpuPanel, -1, _("Thread number:"), wxPoint(20,28));
	thread=new wxChoice(cpuPanel, -1, wxPoint(150, 25), wxSize(200, -1), 0);
	thread->Append(conv("1 (Standard CPU)"));
	thread->Append(conv("2 (Hyper-Threading or Dual core CPU)"));
	thread->SetToolTip(_("If you have a Hyper-Threading, or Dual core or multiprocessor computer,\nyou will have a huge gain in speed by choosing the second option.\nBut this mode has some camera shaking issues.\n"));
	
	wheel2=new wxCheckBox(cpuPanel, -1, _("Enable advanced wheel model"), wxPoint(150, 70));
	wheel2->SetToolTip(_("Some vehicles may include an advanced wheel model that is CPU intensive.\nYou can force these vehicles to use a simpler model by unchecking this box."));

	//network panel
#ifdef NETWORK
	wxSizer *sizer_net = new wxBoxSizer(wxVERTICAL);

	wxPanel *panel_net_top = new wxPanel(netPanel);
    network_enable=new wxCheckBox(panel_net_top, -1, _("Enable network mode"), wxPoint(5, 5));
	network_enable->SetToolTip(_("This switches RoR into network mode.\nBe aware that many features are not available in network mode.\nFor example, you not be able to leave your vehicle or hook objects."));


    dText = new wxStaticText(panel_net_top, -1, _("Nickname: "), wxPoint(230,7));
	nickname=new wxTextCtrl(panel_net_top, -1, _("Anonymous"), wxPoint(300, 2), wxSize(170, -1));
	nickname->SetToolTip(_("Your network name. Maximum 20 Characters."));

	sizer_net->Add(panel_net_top, 0, wxGROW);

	//dText = new wxStaticText(netPanel, -1, wxString("Servers list: "), wxPoint(5,5));
	wxSizer *sizer_net_middle = new wxBoxSizer(wxHORIZONTAL);
	networkhtmw = new wxHtmlWindow(netPanel, main_html, wxPoint(0, 30), wxSize(480, 270));
	//networkhtmw->LoadPage(REPO_HTML_SERVERLIST);
	networkhtmw->SetPage(_("<p>How to play in Multiplayer:</p><br><ol><li>Click on the <b>Update</b> button to see available servers here.</li><li>Click on any underlined Server in the list.</li><li>Click on <b>Save and Play</b> button to start the game.</li></ol>"));
	networkhtmw->SetToolTip(_("Click on Update to update the list.\nClick on blue hyperlinks to select a server."));
	sizer_net_middle->Add(networkhtmw);
	sizer_net->Add(networkhtmw, 2, wxGROW | wxALL);

	wxPanel *panel_net_bottom = new wxPanel(netPanel);
	dText = new wxStaticText(panel_net_bottom, -1, _("Server host name: "), wxPoint(10,5));
	servername=new wxTextCtrl(panel_net_bottom, -1, _("127.0.0.1"), wxPoint(150, 5), wxSize(200, -1));
	servername->SetToolTip(_("This field is automatically filled if you click on an hyperlink in the list.\nDo not change it."));

	dText = new wxStaticText(panel_net_bottom, -1, _("Server port number: "), wxPoint(10,30));
	serverport=new wxTextCtrl(panel_net_bottom, -1, _("12333"), wxPoint(150, 30), wxSize(100, -1));
	serverport->SetToolTip(_("This field is automatically filled if you click on an hyperlink in the list.\nDo not change it."));
//	dText = new wxStaticText(netPanel, -1, wxString("(default is 12333)"), wxPoint(260,103));

	dText = new wxStaticText(panel_net_bottom, -1, _("Server password: "), wxPoint(10,55));
	serverpassword=new wxTextCtrl(panel_net_bottom, -1, wxString(), wxPoint(150, 55), wxSize(100, -1));
	serverpassword->SetToolTip(_("The server password, if required."));
	sizer_net->Add(panel_net_bottom, 0, wxGROW);

	dText = new wxStaticText(panel_net_bottom, -1, _("User Token: "), wxPoint(10,80));
	usertoken=new wxTextCtrl(panel_net_bottom, -1, wxString(), wxPoint(150, 80), wxSize(200, -1));
	usertoken->SetToolTip(_("Your Forum User Token."));
	
	btnUpdate = new wxButton(panel_net_bottom, net_test, _("Update"), wxPoint(360, 5), wxSize(120,85));

	netPanel->SetSizer(sizer_net);

	//	dText = new wxStaticText(netPanel, -1, wxString("P2P port number: "), wxPoint(10,153));
//	p2pport=new wxTextCtrl(netPanel, -1, "12334", wxPoint(150, 150), wxSize(100, -1));
//	dText = new wxStaticText(netPanel, -1, wxString("(default is 12334)"), wxPoint(260,153));

	//wxButton *testbut=
#endif

	//advanced panel
	dText = new wxStaticText(advancedPanel, -1, _("You do not need to change these settings unless you experience problems"), wxPoint(10, 18));

	dText = new wxStaticText(advancedPanel, -1, _("Screenshot Format:"), wxPoint(10, 49));
	screenShotFormat =new wxChoice(advancedPanel, -1, wxPoint(115, 50), wxSize(200, -1), 0);
	screenShotFormat->Append(conv("jpg (smaller, default)"));
	screenShotFormat->Append(conv("png (bigger, no quality loss)"));
	screenShotFormat->SetToolTip(_("In what Format should screenshots be saved?"));

	dText = new wxStaticText(advancedPanel, -1, _("Light source effects:"), wxPoint(10, 78));
	flaresMode=new wxChoice(advancedPanel, -1, wxPoint(115, 75), wxSize(200, -1), 0);
//	flaresMode->Append(_("None (fastest)")); //this creates a bug in the autopilot
	flaresMode->Append(conv("No light sources"));
	flaresMode->Append(conv("Only current vehicle, main lights"));
	flaresMode->Append(conv("All vehicles, main lights"));
	flaresMode->Append(conv("All vehicles, all lights"));
	flaresMode->SetToolTip(_("Determines which lights will project light on the environment.\nThe more light sources are used, the slowest it will be."));

	dText = new wxStaticText(advancedPanel, -1, _("In case the mods cache becomes corrupted, \npress this button to clear the cache. \nIt will be regenerated\nthe next time you launch the game:"), wxPoint(10, 110));
	//wxButton *clearcachebut=
	new wxButton(advancedPanel, clear_cache, _("Clear cache"), wxPoint(125, 180));

	dText = new wxStaticText(advancedPanel, -1, _("Language:"), wxPoint(10, 230));
	
	wxArrayString choices;
	int sel = 0;
	if(avLanguages.size() > 0)
	{
		int counter = 0;
		std::vector<wxLanguageInfo*>::iterator it;
		for(it = avLanguages.begin(); it!=avLanguages.end(); it++, counter++)
		{
			if(*it == language)
				sel = counter;
			choices.Add((*it)->Description);
		}
	} else
	{
		choices.Add(_("(No Languages found)"));
	}

	languageMode=new wxChoice(advancedPanel, EVC_LANG, wxPoint(115, 230), wxSize(200, -1), choices, wxCB_READONLY);
//	flaresMode->Append(_("None (fastest)")); //this creates a bug in the autopilot
	languageMode->SetSelection(sel);

	languageMode->SetToolTip(_("This setting overrides the system's default language. You need to restart the configurator to apply the changes."));
	
	enableFog=new wxCheckBox(advancedPanel, -1, _("Fog"), wxPoint(320, 50));
	enableFog->SetToolTip(_("Uncheck in case you have a fog problem"));

	smoke=new wxCheckBox(advancedPanel, -1, _("Engine smoke"), wxPoint(320, 65));
	smoke->SetToolTip(_("Not much impact on speed."));

	cpart=new wxCheckBox(advancedPanel, -1, _("Custom Particles"), wxPoint(320, 80));
	cpart->SetToolTip(_("Particles like water cannons or plain trails. Can be over-used in Multiplayer."));

	dashboard=new wxCheckBox(advancedPanel, -1, _("Dashboard"), wxPoint(320, 95));
	dashboard->SetToolTip(_("Shows the dynamic dashboard in 1st person view. May cause compatibility problems for very old video cards."));

	replaymode=new wxCheckBox(advancedPanel, -1, _("Replay Mode"), wxPoint(320, 110));
	replaymode->SetToolTip(_("Replay mode. (Can affect your frame rate)"));

	dtm=new wxCheckBox(advancedPanel, -1, _("Debug Truck Mass"), wxPoint(320, 125));
	dtm->SetToolTip(_("Prints all node masses to the RoR.log. Only use for truck debugging!"));

	dcm=new wxCheckBox(advancedPanel, -1, _("Debug Collision Meshes"), wxPoint(320, 140));
	dcm->SetToolTip(_("Shows all Collision meshes in Red to be able to position them correctly. Only use for debugging!"));

	hydrax=new wxCheckBox(advancedPanel, -1, _("Hydrax Water System"), wxPoint(320, 155));
	hydrax->SetToolTip(_("Enables the new water rendering system. EXPERIMENTAL. Overrides any water settings."));

	dismap=new wxCheckBox(advancedPanel, -1, _("Disable Overview Map"), wxPoint(320, 170));
	dismap->SetToolTip(_("Disabled the map. This is for testing purposes only, you should not gain any FPS with that."));

	extcam=new wxCheckBox(advancedPanel, -1, _("Disable Camera Pitching"), wxPoint(320, 185));
	extcam->SetToolTip(_("If you dislike the pitching external vehicle camera, you can disable it here."));

	enablexfire=new wxCheckBox(advancedPanel, -1, _("Enable XFire Extension"), wxPoint(320, 200));
	enablexfire->SetToolTip(_("If you use XFire, you can switch on this option to generate a more detailed game info."));

	beamdebug=new wxCheckBox(advancedPanel, -1, _("Enable Visual Beam Debug"), wxPoint(320, 215));
	beamdebug->SetToolTip(_("Displays node numbers and more info along nodes and beams."));

	dText = new wxStaticText(advancedPanel, -1, _("minimum visibility range in percent"), wxPoint(10,300));
	//this makes a visual bug in macosx
	sightrange=new wxSlider(advancedPanel, -1, 30, 20, 130, wxPoint(10, 315), wxSize(200, 40), wxSL_LABELS|wxSL_AUTOTICKS);
	sightrange->SetToolTip(_("This sets the minimum sight range Setting. It determines how far the terrain should be drawn. \n100% means you can view the whole terrain. \nWith 130% you can view everything across the diagonal Axis."));
	sightrange->SetTickFreq(10, 0);
	sightrange->SetPageSize(10);
	sightrange->SetLineSize(100);

	wxSizer *sizer_help = new wxBoxSizer(wxVERTICAL);
	helphtmw = new HtmlWindow(helpPanel, help_html, wxPoint(0, 0), wxSize(480, 380));
	helphtmw->SetPage(_("<h2>Latest News</h2>None atm<h2>Support / Community</h2><a href='http://wiki.rigsofrods.com/index.php?title=Tutorials'>tutorials</a><p/> (this site is to be enhanced...)"));
	helphtmw->SetToolTip(_("here you can get help"));
	sizer_help->Add(helphtmw, 1, wxGROW);
	helpPanel->SetSizer(sizer_help);



	//	controlstimer=new wxTimer(this, CONTROLS_TIMER_ID);
	timer1=new wxTimer(this, UPDATE_RESET_TIMER_ID);

	//controlstimer->Start(100); //in ms
	logfile->AddLine(conv("Creating Ogre root"));logfile->Write();
	//we must do this once

	wxFileName tcfn=wxFileName(app->UserPath, wxEmptyString);
	tcfn.AppendDir(_T("config"));
	wxString confdirPrefix=tcfn.GetPath()+wxFileName::GetPathSeparator();

	wxFileName tlfn=wxFileName(app->UserPath, wxEmptyString);
	tlfn.AppendDir(_T("logs"));
	wxString logsdirPrefix=tlfn.GetPath()+wxFileName::GetPathSeparator();

	wxString progdirPrefix=app->ProgramPath+wxFileName::GetPathSeparator();


#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		const char *pluginsfile="plugins_apple_rorcfg.cfg";
#else
		const char *pluginsfile="plugins.cfg";
#endif

	Ogre::Root *mr = new Ogre::Root(Ogre::String(progdirPrefix.ToUTF8().data())+pluginsfile,
									Ogre::String(confdirPrefix.ToUTF8().data())+"ogre.cfg",
									Ogre::String(logsdirPrefix.ToUTF8().data())+"RoR.log");

//	Ogre::Root *mr=new Ogre::Root();
	logfile->AddLine(conv("Root restore config"));logfile->Write();
	Ogre::Root::getSingleton().restoreConfig();
	logfile->AddLine(conv("Enforcing rendering subsystem"));logfile->Write();
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	mr->setRenderSystem(mr->getRenderSystemByName("Direct3D9 Rendering Subsystem"));
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	mr->setRenderSystem(mr->getRenderSystemByName("OpenGL Rendering Subsystem"));
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	mr->setRenderSystem(mr->getRenderSystemByName("OpenGL Rendering Subsystem"));
#endif

	logfile->AddLine(conv("Setting default values"));logfile->Write();
	//set values
	SetDefaults();
	logfile->AddLine(conv("Loading config"));logfile->Write();
	LoadConfig();

	// centers dialog window on the screen
	SetSize(500,580);
	Centre();
}

void MyDialog::onChangeLanguageChoice(wxCommandEvent& event)
{
	wxString warning = _("You must save and restart the program to activate the new language!");
	wxString caption = _("new Language selected");

	wxMessageDialog *w = new wxMessageDialog(this, warning, caption, wxOK, wxDefaultPosition);
	w->ShowModal();
	delete(w);
}

void MyDialog::loadInputControls()
{
	// setup control tree
	std::map<std::string, std::vector<event_trigger_t> > controls = INPUTENGINE.getEvents();
	std::map<std::string, std::vector<event_trigger_t> >::iterator mapIt;
	std::vector<event_trigger_t>::iterator vecIt;

	// clear everything
	controlItemCounter=0;
	cTree->DeleteRoot();

	if(cTree->GetColumnCount() < 4)
	{
		// only add them once
		cTree->AddColumn (_("Event"), 180);
		cTree->SetColumnEditable (0, false);
		cTree->AddColumn (_("Type"), 75);
		cTree->SetColumnEditable (1, false);
		cTree->AddColumn (_("Key/Axis"), 100);
		cTree->SetColumnEditable (2, false);
		cTree->AddColumn (_("Options"), 100);
		cTree->SetColumnEditable (3, false);
	}

	std::string curGroup = "";
	wxTreeItemId root = cTree->AddRoot(conv("Root"));
	wxTreeItemId *curRoot = 0;
	for(mapIt = controls.begin(); mapIt != controls.end(); mapIt++)
	{
		std::vector<event_trigger_t> vec = mapIt->second;

		for(vecIt = vec.begin(); vecIt != vec.end(); vecIt++, controlItemCounter++)
		{
			if(vecIt->group != curGroup || curRoot == 0)
			{
				//if(curRoot!=0)
				//	cTree->ExpandAll(*curRoot);
				curGroup = vecIt->group;
				wxTreeItemId tmp = cTree->AppendItem(root, conv(curGroup));
				curRoot = &tmp;
				cTree->SetItemData(curRoot, new IDData(-1));
			}

			//strip category name if possible
			wxString evName = conv(mapIt->first);
			if(vecIt->group.size()+1 < mapIt->first.size())
				evName = conv(mapIt->first.substr(vecIt->group.size()+1));
		    wxTreeItemId item = cTree->AppendItem(*curRoot, evName);

			/*
			wxTreeItemId cItem_context = cTree->AppendItem(item, "Context");
			cTree->SetItemText (cItem_context, 1,  wxString(INPUTENGINE.getEventTypeName(vecIt->eventtype).c_str()));

			wxTreeItemId cItem_function = cTree->AppendItem(item, "Function");
			wxTreeItemId cItem_input = cTree->AppendItem(item, "Input");
			*/

			// set some data beside the tree entry
			cTree->SetItemData(item, new IDData(vecIt->suid));
			updateItemText(item, (event_trigger_t *)&(*vecIt));
			treeItems[vecIt->suid] = item;

			/*
			cTree->SetItemText (item, 1,  wxString(INPUTENGINE.getEventTypeName(vecIt->eventtype).c_str()));
			if(vecIt->eventtype == ET_Keyboard)
			{
				cTree->SetItemText (cItem_function, 1,  "");
				cTree->SetItemText (cItem_input, 1, wxString(vecIt->configline.c_str()));

				cTree->SetItemText (item, 2, wxString(vecIt->configline.c_str()));
			} else if(vecIt->eventtype == ET_JoystickAxisAbs || vecIt->eventtype == ET_JoystickAxisRel)
			{
				cTree->SetItemText (cItem_function, 1,  wxString::Format(_T("%d"), vecIt->joystickAxisNumber));
				cTree->SetItemText (cItem_input, 1, wxString((vecIt->configline).c_str()));

				cTree->SetItemText (item, 2,  wxString::Format(_T("%d"), vecIt->joystickAxisNumber));
				cTree->SetItemText (item, 3,  wxString((vecIt->configline).c_str()));
			} else if(vecIt->eventtype == ET_JoystickButton)
			{
				cTree->SetItemText (cItem_function, 1,  "");
				cTree->SetItemText (cItem_input, 1, wxString::Format(_T("%d"), vecIt->joystickButtonNumber));

				cTree->SetItemText (item, 2, wxString::Format(_T("%d"), vecIt->joystickButtonNumber));
			}
			*/

		}
	}
	//cTree->ExpandAll(root);
}

void MyDialog::updateItemText(wxTreeItemId item, event_trigger_t *t)
{
	cTree->SetItemText (item, 1,  conv(INPUTENGINE.getEventTypeName(t->eventtype)));
	if(t->eventtype == ET_Keyboard)
	{
		cTree->SetItemText (item, 2, conv(t->configline));
	} else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
	{
		cTree->SetItemText (item, 2,  wxString::Format(_T("%d"), t->joystickAxisNumber));
		cTree->SetItemText (item, 3,  conv((t->configline)));
	} else if(t->eventtype == ET_JoystickButton)
	{
		cTree->SetItemText (item, 2, wxString::Format(_T("%d"), t->joystickButtonNumber));
	}
}
void MyDialog::UpdateOgreParams(bool scanDevices)
{
	logfile->AddLine(conv("Update Ogre Params Started"));logfile->Write();
	Ogre::RenderSystemList* lstRend;
	Ogre::RenderSystemList::iterator pRend;
	Ogre::ConfigOptionMap opts;
	lstRend = Ogre::Root::getSingleton().getAvailableRenderers();
	if(lstRend->empty())
		exit(11);
	pRend = lstRend->begin();//we'll have only one renderer (DX9), so take this one
	dx9=*pRend;
	opts = (*pRend)->getConfigOptions();
	Ogre::ConfigOptionMap::iterator pOpt = opts.begin();
	antialiasing->Clear();
	if (scanDevices) renderdevice->Clear();
	videomode->Clear();
	while( pOpt!= opts.end() )
	{
		wxChoice *choice=0;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		if (!strncmp("Anti aliasing", pOpt->second.name.c_str(), 13)) choice=antialiasing;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		if (!strncmp("FSAA", pOpt->second.name.c_str(), 4)) choice=antialiasing;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		if (!strncmp("FSAA", pOpt->second.name.c_str(), 4)) choice=antialiasing;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		if (scanDevices && !strncmp("Rendering Device", pOpt->second.name.c_str(), 16)) choice=renderdevice;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		if (scanDevices && !strncmp("RTT Preferred Mode", pOpt->second.name.c_str(), 18)) choice=renderdevice;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		if (scanDevices && !strncmp("RTT Preferred Mode", pOpt->second.name.c_str(), 18)) choice=renderdevice;
#endif
		if (!strncmp("Video Mode", pOpt->second.name.c_str(), 10)) choice=videomode;
//		debug->AppendText(pOpt->second.name.c_str());
		Ogre::StringVector::iterator pPoss = pOpt->second.possibleValues.begin();
		while (pPoss!=pOpt->second.possibleValues.end())
		{
			if (choice) choice->Append(conv(pPoss[0]));
//			debug->AppendText(pPoss[0].c_str());
			++pPoss;
		}
		++pOpt;
	}
	logfile->AddLine(conv("Update Ogre Params Finished"));logfile->Write();
}

void MyDialog::SetDefaults()
{
	UpdateOgreParams(true);
	//wxChoice *renderdevice;
	renderdevice->SetSelection(0);
	//wxChoice *videomode;
	videomode->SetSelection(0);
	//wxChoice *antialiasing;
	antialiasing->SetSelection(0);
	//wxChoice *textfilt;
	textfilt->SetSelection(3); //anisotropic
	//wxCheckBox *fullscreen;
	fullscreen->SetValue(false);
	//wxCheckBox *vsync;
	vsync->SetValue(false);
	//wxChoice *sky;
	sky->SetSelection(0);//sandstorm
	//wxChoice *shadow;
	shadow->SetSelection(0);//no shadows
	//wxChoice *water;
	water->SetSelection(0);//basic water
	waves->SetValue(false);
	vegetationMode->SetSelection(0); // None

	flaresMode->SetSelection(2); // all trucks
	//languageMode->SetSelection(0);
	enableFog->SetValue(true);

	sightrange->SetValue(30);

	screenShotFormat->SetSelection(0);

	smoke->SetValue(true);
	replaymode->SetValue(false);
	dtm->SetValue(false);
	dcm->SetValue(false);
	//wxCheckBox *dust;
	dust->SetValue(false);
	//wxCheckBox *spray;
	spray->SetValue(false);
	cpart->SetValue(true);
	heathaze->SetValue(false);
	hydrax->SetValue(false);
	dismap->SetValue(false);
	enablexfire->SetValue(true);
	beamdebug->SetValue(false);
	extcam->SetValue(false);
	//wxCheckBox *dashboard;
	dashboard->SetValue(true);
	//wxCheckBox *mirror;
	mirror->SetValue(false);
	envmap->SetValue(false);
	//wxCheckBox *sunburn;
	sunburn->SetValue(false);
	//wxCheckBox *bloom;
	bloom->SetValue(false);
	//wxCheckBox *mblur;
	mblur->SetValue(false);
	//wxChoice *sound;
	sound->SetSelection(1);//software
	//wxChoice *thread;
	thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)
	wheel2->SetValue(true);
//	SandStormFogStart = 500;
//	caelumFogDensity = 0.005;

#ifdef NETWORK
	//wxCheckBox *network_enable;
	network_enable->SetValue(false);
	//wxTextCtrl *nickname;
	nickname->SetValue(_("Anonymous"));
	//wxTextCtrl *servername;
	servername->SetValue(_("127.0.0.1"));
	//wxTextCtrl *serverport;
	serverport->SetValue(_("12333"));
	serverpassword->SetValue(wxString());
	usertoken->SetValue(wxString());
//	p2pport->SetValue("12334");
#endif

	// update settings
	getSettingsControls();
}

void MyDialog::getSettingsControls()
{
	char tmp[255]="";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	settings["Anti aliasing"] = conv(antialiasing->GetStringSelection());
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	settings["FSAA"] = conv(antialiasing->GetStringSelection());
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	settings["FSAA"] = conv(antialiasing->GetStringSelection());
#endif
	settings["Full Screen"] = fullscreen->GetValue() ? "Yes" : "No";

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	settings["Rendering Device"] = conv(renderdevice->GetStringSelection());
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	settings["RTT Preferred Mode"] = conv(renderdevice->GetStringSelection());
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	settings["RTT Preferred Mode"] = conv(renderdevice->GetStringSelection());
#endif
	settings["VSync"] = vsync->GetValue() ? "Yes" : "No";
	settings["Video Mode"] = conv(videomode->GetStringSelection());
	settings["Texture Filtering"] = conv(textfilt->GetStringSelection());
	settings["Sky effects"] = conv(sky->GetStringSelection());
	settings["Shadow technique"] = conv(shadow->GetStringSelection());
	settings["Water effects"] = conv(water->GetStringSelection());
	settings["Waves"] = (waves->GetValue()) ? "Yes" : "No";
	settings["Engine smoke"] = (smoke->GetValue()) ? "Yes" : "No";
	settings["Replay mode"] = (replaymode->GetValue()) ? "Yes" : "No";
	settings["Debug Truck Mass"] = (dtm->GetValue()) ? "Yes" : "No";
	settings["Debug Collisions"] = (dcm->GetValue()) ? "Yes" : "No";
	settings["Dust"] = (dust->GetValue()) ? "Yes" : "No";
	settings["Spray"] = (spray->GetValue()) ? "Yes" : "No";
	settings["Custom Particles"] = (cpart->GetValue()) ? "Yes" : "No";
	settings["HeatHaze"] = (heathaze->GetValue()) ? "Yes" : "No";
	settings["Hydrax"] = (hydrax->GetValue()) ? "Yes" : "No";
	settings["disableOverViewMap"] = (dismap->GetValue()) ? "Yes" : "No";
	settings["DebugBeams"] = (beamdebug->GetValue()) ? "Yes" : "No";
	settings["XFire"] = (enablexfire->GetValue()) ? "Yes" : "No";
	settings["External Camera Mode"] = (extcam->GetValue()) ? "Static" : "Pitching";
	settings["Dashboard"] = (dashboard->GetValue()) ? "Yes" : "No";
	settings["Mirrors"] = (mirror->GetValue()) ? "Yes" : "No";
	settings["Envmap"] = (envmap->GetValue()) ? "Yes" : "No";
	settings["Sunburn"] = (sunburn->GetValue()) ? "Yes" : "No";
	settings["Bloom"] = (bloom->GetValue()) ? "Yes" : "No";
	settings["Motion blur"] = (mblur->GetValue()) ? "Yes" : "No";
	settings["Enhanced wheels"] = (wheel2->GetValue()) ? "Yes" : "No";
	settings["Fog"] = (enableFog->GetValue()) ? "Yes" : "No";
	settings["Envmap"] = (envmap->GetValue()) ? "Yes" : "No";
	settings["3D Sound renderer"] = conv(sound->GetStringSelection());
	settings["Threads"] = conv(thread->GetStringSelection());

	sprintf(tmp, "%d", sightrange->GetValue());
	settings["FarClip Percent"] = tmp;

	settings["Lights"] = conv(flaresMode->GetStringSelection());
	settings["Vegetation"] = conv(vegetationMode->GetStringSelection());
	settings["Screenshot Format"] = conv(screenShotFormat->GetStringSelection());

	wxSize s = this->GetSize();
	sprintf(tmp, "%d, %d", s.x, s.y);
	settings["Configurator Size"] = tmp;
	
#ifdef NETWORK
	settings["Network enable"] = (network_enable->GetValue()) ? "Yes" : "No";
	settings["Nickname"] = conv(nickname->GetValue().SubString(0,20));
	settings["Server name"] = conv(servername->GetValue());
	settings["Server port"] = conv(serverport->GetValue());
	settings["Server password"] = conv(serverpassword->GetValue());
	settings["User Token"] = conv(usertoken->GetValue());
#endif

	sprintf(tmp, "%d", simpleSlider->GetValue());
	settings["Simple Settings CPU"] = tmp;
	sprintf(tmp, "%d", simpleSlider2->GetValue());
	settings["Simple Settings Graphics"] = tmp;
	
	// save language, if one is set
	if(avLanguages.size() > 0 && languageMode->GetStringSelection() != _("(No Languages found)"))
	{
		settings["Language"] = conv(languageMode->GetStringSelection());
		
		std::vector<wxLanguageInfo*>::iterator it;
		for(it = avLanguages.begin(); it!=avLanguages.end(); it++)
		{
			if((*it)->Description == languageMode->GetStringSelection())
				settings["Language Short"] = conv((*it)->CanonicalName);
		}

	}
}

void MyDialog::updateSettingsControls()
{
	// this method "applies" the settings and updates the controls

	//Ogre config
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	renderdevice->SetStringSelection(conv(settings["Rendering Device"]));
	antialiasing->SetStringSelection(conv(settings["Anti aliasing"]));
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	renderdevice->SetStringSelection(conv(settings["RTT Preferred Mode"]));
	antialiasing->SetStringSelection(conv(settings["FSAA"]));
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	renderdevice->SetStringSelection(conv(settings["RTT Preferred Mode"]));
	antialiasing->SetStringSelection(conv(settings["FSAA"]));
#endif
	videomode->SetStringSelection(conv(settings["Video Mode"]));
	fullscreen->SetValue(settings["Full Screen"]=="Yes");
	vsync->SetValue(settings["VSync"]=="Yes");

	Ogre::String st;
	st = settings["Texture Filtering"]; if (st.length()>0) textfilt->SetStringSelection(conv(st));
	st = settings["Sky effects"]; if (st.length()>0) sky->SetStringSelection(conv(st));
	st = settings["Shadow technique"]; if (st.length()>0) shadow->SetStringSelection(conv(st));
	st = settings["Water effects"]; if (st.length()>0) water->SetStringSelection(conv(st));
	st = settings["Waves"]; if (st.length()>0) waves->SetValue(st=="Yes");
	st = settings["Engine smoke"]; if (st.length()>0) smoke->SetValue(st=="Yes");
	st = settings["Replay mode"]; if (st.length()>0) replaymode->SetValue(st=="Yes");
	st = settings["Debug Truck Mass"]; if (st.length()>0) dtm->SetValue(st=="Yes");
	st = settings["Debug Collisions"]; if (st.length()>0) dcm->SetValue(st=="Yes");
	st = settings["Dust"]; if (st.length()>0) dust->SetValue(st=="Yes");
	st = settings["Spray"]; if (st.length()>0) spray->SetValue(st=="Yes");
	st = settings["Custom Particles"]; if (st.length()>0) cpart->SetValue(st=="Yes");
	st = settings["HeatHaze"]; if (st.length()>0) heathaze->SetValue(st=="Yes");
	st = settings["Hydrax"]; if (st.length()>0) hydrax->SetValue(st=="Yes");
	st = settings["disableOverViewMap"]; if (st.length()>0) dismap->SetValue(st=="Yes");
	st = settings["External Camera Mode"]; if (st.length()>0) extcam->SetValue(st=="Static");
	st = settings["DebugBeams"]; if (st.length()>0) beamdebug->SetValue(st=="Yes");
	st = settings["XFire"]; if (st.length()>0) enablexfire->SetValue(st=="Yes");
	st = settings["Dashboard"]; if (st.length()>0) dashboard->SetValue(st=="Yes");
	st = settings["Mirrors"]; if (st.length()>0) mirror->SetValue(st=="Yes");
	st = settings["Envmap"]; if (st.length()>0) envmap->SetValue(st=="Yes");
	st = settings["Sunburn"]; if (st.length()>0) sunburn->SetValue(st=="Yes");
	st = settings["Bloom"]; if (st.length()>0) bloom->SetValue(st=="Yes");
	st = settings["Motion blur"]; if (st.length()>0) mblur->SetValue(st=="Yes");
	st = settings["3D Sound renderer"]; if (st.length()>0) sound->SetStringSelection(conv(st));
	st = settings["Threads"]; if (st.length()>0) thread->SetStringSelection(conv(st));
	st = settings["Enhanced wheels"]; if (st.length()>0) wheel2->SetValue(st=="Yes");
	st = settings["FarClip Percent"]; if (st.length()>0) sightrange->SetValue((int)atof(st.c_str()));
	st = settings["Fog"]; if (st.length()>0) enableFog->SetValue(st=="Yes");
	st = settings["Lights"]; if (st.length()>0) flaresMode->SetStringSelection(conv(st));
	st = settings["Vegetation"]; if (st.length()>0) vegetationMode->SetStringSelection(conv(st));
	st = settings["Screenshot Format"]; if (st.length()>0) screenShotFormat->SetStringSelection(conv(st));


	st = settings["Configurator Size"];
	if (st.length()>0)
	{
		int sx=0, sy=0;
		int res = sscanf(st.c_str(), "%d, %d", &sx, &sy);
		if(res == 2)
		{
			SetSize(wxSize(sx, sy));
		}
	}

#ifdef NETWORK
	st = settings["Network enable"]; if (st.length()>0) network_enable->SetValue(st=="Yes");
	st = settings["Nickname"]; if (st.length()>0) nickname->SetValue(conv(st));
	st = settings["Server name"]; if (st.length()>0) servername->SetValue(conv(st));
	st = settings["Server port"]; if (st.length()>0) serverport->SetValue(conv(st));
	st = settings["Server password"]; if (st.length()>0) serverpassword->SetValue(conv(st));
	st = settings["User Token"]; if (st.length()>0) usertoken->SetValue(conv(st));
#endif
	st = settings["Simple Settings CPU"]; if (st.length()>0) simpleSlider->SetValue(atoi(st.c_str()));
	st = settings["Simple Settings Graphics"]; if (st.length()>0) simpleSlider2->SetValue(atoi(st.c_str()));
}

bool MyDialog::LoadConfig()
{
	//RoR config
	logfile->AddLine(conv("Loading RoR.cfg"));logfile->Write();
	Ogre::ImprovedConfigFile cfg;

	wxString rorcfg=app->UserPath + wxFileName::GetPathSeparator() + _T("config") + wxFileName::GetPathSeparator() + _T("RoR.cfg");

	printf("reading from Config file: %s\n", rorcfg.c_str());
	// Don't trim whitespace
	cfg.load(rorcfg.ToUTF8().data(), "=:\t", false);

	// load all settings into a map!
	Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
	Ogre::String svalue, sname;
	while (i.hasMoreElements())
	{
		sname = i.peekNextKey();
		svalue = i.getNext();
		settings[sname] = svalue;
		//logfile->AddLine(conv("### ") + conv(sname) + conv(" : ") + conv(svalue));logfile->Write();
	}

	// enforce default settings
	if(settings["Allow NVPerfHUD"] == "") settings["Allow NVPerfHUD"] = "No";
	if(settings["Floating-point mode"] == "") settings["Floating-point mode"] = "Fastest";

	// then update the controls!
	updateSettingsControls();
	return false;
}


void MyDialog::SaveConfig()
{
	// first, update the settings map with the actual control values
	getSettingsControls();

	// then set stuff and write configs
	INPUTENGINE.saveMapping(conv(InputMapFileName));
	//save Ogre stuff
	if (dx9)
	{
		dx9->setConfigOption("Allow NVPerfHUD", settings["Allow NVPerfHUD"]);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		dx9->setConfigOption("Anti aliasing", settings["Anti aliasing"]);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		dx9->setConfigOption("FSAA", settings["FSAA"]);
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		dx9->setConfigOption("FSAA", settings["FSAA"]);
#endif
		dx9->setConfigOption("Full Screen", settings["Full Screen"]);
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		dx9->setConfigOption("Rendering Device", settings["Rendering Device"]);
#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		dx9->setConfigOption("RTT Preferred Mode", settings["RTT Preferred Mode"]);
#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		dx9->setConfigOption("RTT Preferred Mode", settings["RTT Preferred Mode"]);
#endif
		dx9->setConfigOption("VSync", settings["VSync"]);
		dx9->setConfigOption("Video Mode", settings["Video Mode"]);

		Ogre::String err=dx9->validateConfigOptions();
		if (err.length()>0)
		{
			wxMessageDialog(this, conv(err), _("Ogre config validation error"),wxOK||wxICON_ERROR).ShowModal();
		}
		else 
			Ogre::Root::getSingleton().saveConfig();
	}
	else
		wxMessageDialog(this, wxString(_("Could not open Rendering device")), wxString(_("Ogre config error")),wxOK||wxICON_ERROR).ShowModal();
	//save my stuff
	FILE *fd;
	wxString rorcfg=app->UserPath + wxFileName::GetPathSeparator() + _T("config") + wxFileName::GetPathSeparator() + _T("RoR.cfg");

	printf("saving to Config file: %s\n", rorcfg.ToUTF8().data());
	fd=fopen(rorcfg.ToUTF8().data(), "w");
	if (!fd)
	{
		wxMessageDialog(this, wxString(_("Could not write config file")), wxString(_("Configure error")),wxOK||wxICON_ERROR).ShowModal();
		return;
	}

	// now save the settings to RoR.cfg
	std::map<std::string, std::string>::iterator it;
	for(it=settings.begin(); it!=settings.end(); it++)
	{
		fprintf(fd, "%s=%s\n", it->first.c_str(), it->second.c_str());
	}

	fclose(fd);
}


void MyDialog::onRightClickItem(wxTreeEvent& event)
{
	if(isSelectedControlGroup())
		return;
	event_trigger_t *t = getSelectedControlEvent();
	if(!t)
		return;
	if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
	{
		wxMenu *m = new wxMenu(_("Joystick Options"));

		wxMenuItem *i = m->AppendCheckItem(1, _("Reverse"), _("reverse axis"));
		i->Check(t->joystickAxisReverse);

		wxMenu *iregion = new wxMenu(_("Joystick Axis Region"));
		i = iregion->AppendRadioItem(30, _("Upper"));
		i->Check(t->joystickAxisRegion == 1);
		i = iregion->AppendRadioItem(31, _("Full"));
		i->Check(t->joystickAxisRegion == 0);
		i = iregion->AppendRadioItem(32, _("Lower"));
		i->Check(t->joystickAxisRegion == -1);
		i = m->AppendSubMenu(iregion, _("Region"));

		wxMenu *idead = new wxMenu(_("Joystick Axis Deadzones"));
		i = idead->AppendRadioItem(20, _("0 % (No deadzone)"), _("no deadzone"));
		i->Check(fabs(t->joystickAxisDeadzone)< 0.0001);

		i = idead->AppendRadioItem(21, _("5 %"), _("5% deadzone"));
		i->Check(fabs(t->joystickAxisDeadzone-0.05)< 0.0001);

		i = idead->AppendRadioItem(22, _("10 %"), _("10% deadzone"));
		i->Check(fabs(t->joystickAxisDeadzone-0.1)< 0.0001);

		i = idead->AppendRadioItem(23, _("15 %"), _("15% deadzone"));
		i->Check(fabs(t->joystickAxisDeadzone-0.15)< 0.0001);

		i = idead->AppendRadioItem(24, _("20 %"), _("20% deadzone"));
		i->Check(fabs(t->joystickAxisDeadzone-0.2)< 0.0001);

		i = idead->AppendRadioItem(25, _("30 %"), _("30% deadzone"));
		i->Check(fabs(t->joystickAxisDeadzone-0.3)< 0.0001);

		i = m->AppendSubMenu(idead, _("Deadzone"));
		this->PopupMenu(m);
	}
}

void MyDialog::onActivateItem(wxTreeEvent& event)
{
	wxTreeItemId itm = event.GetItem();
	if(!isSelectedControlGroup(&itm))
		remapControl();
	else
		// expand/collapse group otherwise
		if(cTree->IsExpanded(event.GetItem()))
			cTree->Collapse(event.GetItem());
		else
			cTree->Expand(event.GetItem());
}

/*
void MyDialog::onChangeTypeComboBox(wxCommandEvent& event)
{
	if(isSelectedControlGroup())
		return;
	int r = 0;
	wxString s = ctrlTypeCombo->GetValue();
	for(int i=0;i<11;i++)
	{
		if(typeChoices[i] == s)
		{
			r = i;
			break;
		}
	}
	event_trigger_t *t = getSelectedControlEvent();
	t->eventtype = (eventtypes)r;
	cTree->SetItemText(cTree->GetSelection(), 1,  typeChoices[r]);
}
*/

void MyDialog::remapControl()
{
	event_trigger_t *t = getSelectedControlEvent();
	if(!t)
		return;
	kd = new KeySelectDialog(this, t);
    if(kd->ShowModal()==0)
	{
		wxTreeItemId item = cTree->GetSelection();
		INPUTENGINE.updateConfigline(t);
		updateItemText(item, t);
	}

	kd->Destroy();
	delete kd;
	kd = 0;
}

void MyDialog::testEvents()
{
	KeyTestDialog *kt = new KeyTestDialog(this);
    if(kt->ShowModal()==0)
	{
	}
	kt->Destroy();
	delete kt;
}



/*
void MyDialog::OnButRemap(wxCommandEvent& event)
{
	remapControl();
}
*/

bool MyDialog::isSelectedControlGroup(wxTreeItemId *item)
{
	wxTreeItemId itm = cTree->GetSelection();
	if(!item)
		item = &itm;
	IDData *data = (IDData *)cTree->GetItemData(*item);
	if(!data || data->id < 0)
		return true;
	return false;
}


event_trigger_t *MyDialog::getSelectedControlEvent()
{
	int itemId = -1;
	wxTreeItemId item = cTree->GetSelection();
	IDData *data = (IDData *)cTree->GetItemData(item);
	if(!data)
		return 0;
	itemId = data->id;
	if(itemId == -1)
		return 0;
	return INPUTENGINE.getEventBySUID(itemId);
}

void MyDialog::onTreeSelChange (wxTreeEvent &event)
{
	wxTreeItemId itm = event.GetItem();
	if(isSelectedControlGroup(&itm))
	{
		btnDeleteKey->Disable();
	} else
	{
		btnDeleteKey->Enable();
	}

	//wxTreeItemId item = event.GetItem();
	//event_trigger_t *t = getSelectedControlEvent();

	// type
	//wxString typeStr = cTree->GetItemText(item, 1);
	//ctrlTypeCombo->SetValue(typeStr);
	//ctrlTypeCombo->Enable();

	// description
	//wxString descStr = cTree->GetItemText(item, 0);
	//controlText->SetLabel((t->group + "_" + descStr.c_str()).c_str());
}

void MyDialog::OnTimer(wxTimerEvent& event)
{
}

void MyDialog::OnQuit(wxCloseEvent& WXUNUSED(event))
{
	Destroy();
	exit(0);
}

void MyDialog::OnDeviceChange(wxCommandEvent& event)
{
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
return;
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
return;
#endif
	logfile->AddLine(conv("Device change"));logfile->Write();
	if (dx9) dx9->setConfigOption("Rendering Device", conv(renderdevice->GetStringSelection()));
	UpdateOgreParams(false);
	videomode->SetSelection(0);
	antialiasing->SetSelection(0);
}

void MyDialog::OnMenuJoystickOptionsClick(wxCommandEvent& ev)
{
	event_trigger_t *t = getSelectedControlEvent();
	if(!t)
		return;
	if(t->eventtype != ET_JoystickAxisAbs && t->eventtype != ET_JoystickAxisRel)
		return;
	switch (ev.GetId())
	{
	case 1: //reverse
		{
			t->joystickAxisReverse = ev.IsChecked();
			break;
		}
	case 20: //0% deadzone
		{
			t->joystickAxisDeadzone = 0;
			break;
		}
	case 21: //5% deadzone
		{
			t->joystickAxisDeadzone = 0.05;
			break;
		}
	case 22: //10% deadzone
		{
			t->joystickAxisDeadzone = 0.1;
			break;
		}
	case 23: //15% deadzone
		{
			t->joystickAxisDeadzone = 0.15;
			break;
		}
	case 24: //20% deadzone
		{
			t->joystickAxisDeadzone = 0.2;
			break;
		}
	case 25: //30% deadzone
		{
			t->joystickAxisDeadzone = 0.3;
			break;
		}
	case 30: //upper
		{
			t->joystickAxisRegion = 1;
			break;
		}
	case 31: //full
		{
			t->joystickAxisRegion = 0;
			break;
		}
	case 32: //lower
		{
			t->joystickAxisRegion = -1;
			break;
		}
	}

	INPUTENGINE.updateConfigline(t);
	updateItemText(cTree->GetSelection(), t);
}
/*
void MyDialog::OnMenuClearClick(wxCommandEvent& event)
{
}
void MyDialog::OnMenuTestClick(wxCommandEvent& event)
{
}
void MyDialog::OnMenuDeleteClick(wxCommandEvent& event)
{
}
void MyDialog::OnMenuDuplicateClick(wxCommandEvent& event)
{
}
void MyDialog::OnMenuEditEventNameClick(wxCommandEvent& event)
{
}
void MyDialog::OnMenuCheckDoublesClick(wxCommandEvent& event)
{
}
*/
void MyDialog::OnButAddKey(wxCommandEvent& event)
{
	KeyAddDialog *ka = new KeyAddDialog(this);
    if(ka->ShowModal()!=0)
		// user pressed cancel
		return;
	std::string line = ka->getEventName() + " " + ka->getEventType() + " " + ka->getEventOption() + " !NEW!";
	//std::string line = ka->getEventName() + " " + ka->getEventType() + ;
	ka->Destroy();
	delete ka;
	ka = 0;

	INPUTENGINE.appendLineToConfig(line, conv(InputMapFileName));

	INPUTENGINE.reloadConfig(conv(InputMapFileName));
	loadInputControls();

	// find the new event now
	std::map<std::string, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
	std::map<std::string, std::vector<event_trigger_t> >::iterator it;
	std::vector<event_trigger_t>::iterator it2;
	int counter = 0;
	int suid = -1;
	for(it = events.begin(); it!= events.end(); it++)
	{
		for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
		{
			if(it2->configline == "!NEW!")
			{
				it2->configline="";
				suid = it2->suid;
			}
		}
	}
	if(suid != -1)
	{
		cTree->EnsureVisible(treeItems[suid]);
		cTree->SelectItem(treeItems[suid]);
		remapControl();
		INPUTENGINE.saveMapping(conv(InputMapFileName));
	}
}

void MyDialog::OnButTestEvents(wxCommandEvent& event)
{
	testEvents();
}

void MyDialog::OnButDeleteKey(wxCommandEvent& event)
{
	wxTreeItemId item = cTree->GetSelection();
	IDData *data = (IDData *)cTree->GetItemData(item);
	if(!data || data->id < 0)
		return;
	INPUTENGINE.deleteEventBySUID(data->id);
	cTree->Delete(item);
	INPUTENGINE.saveMapping(conv(InputMapFileName));
}

void MyDialog::OnButLoadKeymap(wxCommandEvent& event)
{
	wxFileDialog *f = new wxFileDialog(this, _("Choose a file"), wxString(), wxString(), conv("*.map"), wxOPEN || wxFILE_MUST_EXIST);
	if(f->ShowModal() == wxID_OK)
	{
		INPUTENGINE.loadMapping(conv(f->GetPath()));
		loadInputControls();
	}
	delete f;
}

void MyDialog::OnButSaveKeymap(wxCommandEvent& event)
{
	wxFileDialog *f = new wxFileDialog(this, _("Choose a file"), wxString(), wxString(), conv("*.map"), wxSAVE || wxOVERWRITE_PROMPT);
	if(f->ShowModal() == wxID_OK)
	{
		INPUTENGINE.saveMapping(conv(f->GetPath()));
	}
	delete f;
}

void MyDialog::OnButCancel(wxCommandEvent& event)
{
	Destroy();
	exit(0);
}

void MyDialog::OnButPlay(wxCommandEvent& event)
{
	SaveConfig();

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	DWORD               dwCode  =   0;
	ZeroMemory(&si,sizeof(STARTUPINFO));
	si.cb           =   sizeof(STARTUPINFO);
	si.dwFlags      =   STARTF_USESHOWWINDOW;
	si.wShowWindow  =   SW_SHOWNORMAL;

	char path[2048];
	getcwd(path, 2048);
	strcat(path, "\\RoR.exe");
	logfile->AddLine(conv(path));logfile->Write();

	int buffSize = (int)strlen(path) + 1;
	LPWSTR wpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, wpath, buffSize);

	CreateProcess(NULL, wpath, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	execl("./RoR.bin", "", (char *) 0);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	FSRef ref;
	FSPathMakeRef((const UInt8*)"./RoR.app", &ref, NULL);
	LSOpenFSRef(&ref, NULL);
	//execl("./RoR.app/Contents/MacOS/RoR", "", (char *) 0);
#endif

	Destroy();
	exit(0);
}

void MyDialog::OnButSave(wxCommandEvent& event)
{
	SaveConfig();
	Destroy();
	exit(0);
}

void MyDialog::OnButRestore(wxCommandEvent& event)
{
	SetDefaults();
}

void MyDialog::OnButClearCache(wxCommandEvent& event)
{
	wxFileName cfn;
	cfn.AssignCwd();

	wxString cachepath=app->UserPath+wxFileName::GetPathSeparator()+_T("cache");
	wxDir srcd(cachepath);
	wxString src;
	if (!srcd.GetFirst(&src)) return; //empty dir!
	do
	{
		//ignore files and directories beginning with "." (important, for SVN!)
		if (src.StartsWith(_T("."))) continue;
		//check if it id a directory
		wxFileName tsfn=wxFileName(cachepath, wxEmptyString);
		tsfn.AppendDir(src);
		if (wxDir::Exists(tsfn.GetPath()))
		{
			//this is a directory, leave it alone
			continue;
		}
		else
		{
			//this is a file, delete file
			tsfn=wxFileName(cachepath, src);
			::wxRemoveFile(tsfn.GetFullPath());
		}
	} while (srcd.GetNext(&src));
}
/*
void MyDialog::OnSightRangeScroll(wxScrollEvent & event)
{
	int val = sightrange->GetValue();
	wxString s;
	s << val;
	s << " meters";
	sightrangeText->SetLabel(s);
}
*/


void MyDialog::OnSimpleSliderScroll(wxScrollEvent & event)
{
	int val = simpleSlider->GetValue();
	// these are processor simple settings
	// 0 (high perf) - 2 (high quality)
	switch(val)
	{
		case 0:
			replaymode->SetValue(false);
			beamdebug->SetValue(false);

			dtm->SetValue(false); // debug truck mass
			dcm->SetValue(false); //debug collision meshes
			enablexfire->SetValue(false);
			extcam->SetValue(false);
			sound->SetSelection(1);//software
			thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)
			wheel2->SetValue(false);
	break;
		case 1:
			replaymode->SetValue(true);
			beamdebug->SetValue(false);
			dtm->SetValue(false); // debug truck mass
			dcm->SetValue(false); //debug collision meshes
			enablexfire->SetValue(true);
			extcam->SetValue(false);
			sound->SetSelection(1);//software
			thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)
			wheel2->SetValue(true);
	break;
		case 2:
			replaymode->SetValue(true);
			beamdebug->SetValue(false);
			dtm->SetValue(false); // debug truck mass
			dcm->SetValue(false); //debug collision meshes
			enablexfire->SetValue(true);
			extcam->SetValue(false);
			sound->SetSelection(2);//hardware
			thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)
			wheel2->SetValue(true);
	break;
	};
	getSettingsControls();
}

void MyDialog::OnSimpleSlider2Scroll(wxScrollEvent & event)
{
	int val = simpleSlider2->GetValue();
	// these are graphics simple settings
	// 0 (high perf) - 2 (high quality)
	switch(val)
	{
		case 0:
			UpdateOgreParams(true);
			renderdevice->SetSelection(0);
			videomode->SetSelection(0);
			antialiasing->SetSelection(0);
			textfilt->SetSelection(0);
			fullscreen->SetValue(true);
			vsync->SetValue(false);
			sky->SetSelection(0);//sandstorm
			shadow->SetSelection(0);//no shadows
			water->SetSelection(0);//basic water
			waves->SetValue(false);
			vegetationMode->SetSelection(0); // None
			flaresMode->SetSelection(0); // all trucks
			enableFog->SetValue(true);
			sightrange->SetValue(20);
			screenShotFormat->SetSelection(0);
			smoke->SetValue(false);
			dust->SetValue(false);
			spray->SetValue(false);
			cpart->SetValue(false);
			heathaze->SetValue(false);
			hydrax->SetValue(false);
			dismap->SetValue(false);
			dashboard->SetValue(false);
			mirror->SetValue(false);
			envmap->SetValue(false);
			sunburn->SetValue(false);
			bloom->SetValue(false);
			mblur->SetValue(false);
	break;
		case 1:
			UpdateOgreParams(true);
			renderdevice->SetSelection(0);
			if(videomode->GetCount() > 6)
				videomode->SetSelection(videomode->GetCount()/2-2);
			else
				videomode->SetSelection(0);
			antialiasing->SetSelection(0);
			textfilt->SetSelection(2);
			fullscreen->SetValue(true);
			vsync->SetValue(false);
			sky->SetSelection(0);//sandstorm
			shadow->SetSelection(1);
			water->SetSelection(1);
			waves->SetValue(false);
			vegetationMode->SetSelection(1);
			flaresMode->SetSelection(1);
			enableFog->SetValue(true);
			sightrange->SetValue(60);
			screenShotFormat->SetSelection(1);
			smoke->SetValue(true);
			dust->SetValue(true);
			spray->SetValue(true);
			cpart->SetValue(true);
			heathaze->SetValue(false);
			hydrax->SetValue(false);
			dismap->SetValue(false);
			dashboard->SetValue(true);
			mirror->SetValue(true);
			envmap->SetValue(true);
			sunburn->SetValue(false);
			bloom->SetValue(false);
			mblur->SetValue(false);
	break;
		case 2:
			UpdateOgreParams(true);
			renderdevice->SetSelection(0);
			videomode->SetSelection(videomode->GetCount()-1);
			antialiasing->SetSelection(antialiasing->GetCount()-1);
			textfilt->SetSelection(3);
			fullscreen->SetValue(true);
			vsync->SetValue(false);
			sky->SetSelection(1);
			shadow->SetSelection(1);
			water->SetSelection(3);
			waves->SetValue(true);
			vegetationMode->SetSelection(3);
			flaresMode->SetSelection(3);
			enableFog->SetValue(false);
			sightrange->SetValue(130);
			screenShotFormat->SetSelection(1);
			smoke->SetValue(true);
			dust->SetValue(true);
			spray->SetValue(true);
			cpart->SetValue(true);
			heathaze->SetValue(true);
			hydrax->SetValue(true);
			dismap->SetValue(false);
			dashboard->SetValue(true);
			mirror->SetValue(true);
			envmap->SetValue(true);
			sunburn->SetValue(false);
			bloom->SetValue(false);
			mblur->SetValue(false);
		break;
	};
	getSettingsControls();
}


void MyDialog::OnLinkClicked(wxHtmlLinkEvent& event)
{
	wxHtmlLinkInfo linkinfo=event.GetLinkInfo();
	wxString href=linkinfo.GetHref();
	wxURI *uri=new wxURI(href);
	if (uri->GetScheme()==conv("rorserver"))
	{
		servername->SetValue(uri->GetServer());
		serverport->SetValue(uri->GetPort());
		//serverport->SetValue(uri->GetPassword());
		//serverpassword->SetValue(uri->GetPath().AfterFirst('/'));
		if(uri->GetPassword() != wxString())
		{
			serverpassword->Enable(true);
		}else
			serverpassword->Enable(false);
	} else
		networkhtmw->OnLinkClicked(linkinfo);
//	wxMessageDialog *res=new wxMessageDialog(this, href, "Success", wxOK | wxICON_INFORMATION );
//	res->ShowModal();
}

void MyDialog::OnTimerReset(wxTimerEvent& event)
{
	btnUpdate->Enable(true);
}


void MyDialog::OnTestNet(wxCommandEvent& event)
{
#ifdef NETWORK
	btnUpdate->Enable(false);
	timer1->Start(10000);
	std::string lshort = conv(language->CanonicalName).substr(0, 2);
	networkhtmw->LoadPage(wxString(_(REPO_HTML_SERVERLIST))+
						  wxString(conv("?version="))+
						  wxString(_(RORNET_VERSION))+
						  wxString(conv("&lang="))+
						  conv(lshort));	

//	networkhtmw->LoadPage("http://rigsofrods.blogspot.com/");
	/*
	SWInetSocket mySocket;
	SWBaseSocket::SWBaseError error;
	long port;
	serverport->GetValue().ToLong(&port);
	mySocket.connect(port, servername->GetValue().c_str(), &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	header_t head;
	head.command=MSG_VERSION;
	head.size=strlen(RORNET_VERSION);
	mySocket.send((char*)&head, sizeof(header_t), &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	mySocket.send(RORNET_VERSION, head.size, &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	mySocket.recv((char*)&head, sizeof(header_t), &error);
	if (error!=SWBaseSocket::ok)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString(error.get_error()), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	char sversion[256];
	if (head.command!=MSG_VERSION)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString("Unexpected message"), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	if (head.size>255)
	{
		wxMessageDialog *err=new wxMessageDialog(this, wxString("Message too long"), "Error", wxOK | wxICON_ERROR);
		err->ShowModal();
		return;
	}
	int rlen=0;
	while (rlen<head.size)
	{
		rlen+=mySocket.recv(sversion+rlen, head.size-rlen, &error);
		if (error!=SWBaseSocket::ok)
		{
			wxMessageDialog *err=new wxMessageDialog(this, wxString("Unexpected message"), "Error", wxOK | wxICON_ERROR);
			err->ShowModal();
			return;
		}
	}
	sversion[rlen]=0;
	mySocket.disconnect(&error);
	char message[512];
	sprintf(message, "Game server found, version %s", sversion);
	wxMessageDialog *res=new wxMessageDialog(this, wxString(message), "Success", wxOK | wxICON_INFORMATION );
	res->ShowModal();
	*/
#endif
}

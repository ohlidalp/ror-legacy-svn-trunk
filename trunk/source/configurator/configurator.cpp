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
#include "joywizard.h"
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

#include <iostream>
using namespace std;

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
#include <wx/version.h>
#include <wx/log.h>
//#include "joysticks.h"
#include "utils.h"

#include "ImprovedConfigFile.h"

#include "InputEngine.h"
extern eventInfo_t eventInfo[]; // defines all input events

#include "treelistctrl.h"

#include "OISKeyboard.h"

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str, len) strlen(str)
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h> // for getcwd
#include <windows.h>
#include <shellapi.h>
#endif

// xpm images
#include "config.xpm"
#include "joycfg.xpm"
#include "mousecfg.xpm"
#include "keyboardcfg.xpm"
#include "opencllogo.xpm"

// de-comment this to enable network stuff
#define NETWORK
#define MAX_EVENTS 2048

wxLocale lang_locale;
wxLanguageInfo *language=0;
std::vector<wxLanguageInfo*> avLanguages;

std::map<std::string, std::string> settings;

#ifdef USE_OPENCL
#include <delayimp.h>
#endif // USE_OPENCL

#ifdef __WXGTK__
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#endif

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
	void initLogging();
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
#if wxCHECK_VERSION(2, 9, 0)
	{ wxCMD_LINE_SWITCH, ("postinstall"), ("postinstall"), ("do not use this")},
	{ wxCMD_LINE_SWITCH, ("buildmode"), ("buildmode"), ("do not use this")},
#else // old wxWidgets support
	{ wxCMD_LINE_SWITCH, wxT("postinstall"), wxT("postinstall"), wxT("do not use this")},
	{ wxCMD_LINE_SWITCH, wxT("buildmode"), wxT("buildmode"), wxT("do not use this")},
#endif
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
	void loadOgre();
	void updateRendersystems(Ogre::RenderSystem *rs);
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
	void OnButCancel(wxCommandEvent& event);
	void OnButSave(wxCommandEvent& event);
	void OnButPlay(wxCommandEvent& event);
	void OnButRestore(wxCommandEvent& event);
	void OnTestNet(wxCommandEvent& event);
	void OnGetUserToken(wxCommandEvent& event);
	void OnLinkClicked(wxHtmlLinkEvent& event);
	void OnLinkClickedUpdate(wxHtmlLinkEvent& event);
	//void OnSightRangeScroll(wxScrollEvent& event);
	void onTreeSelChange(wxTreeEvent& event);
	void onChangeShadowChoice(wxCommandEvent& event);
	void onChangeLanguageChoice(wxCommandEvent& event);
	//void OnButRemap(wxCommandEvent& event);
	void onActivateItem(wxTreeEvent& event);
	void OnButLoadKeymap(wxCommandEvent& event);
	void OnButSaveKeymap(wxCommandEvent& event);
	void onRightClickItem(wxTreeEvent& event);
	void OnMenuJoystickOptionsClick(wxCommandEvent& event);
	void OnChangeRenderer(wxCommandEvent& event);
	//void OnMenuClearClick(wxCommandEvent& event);
	//void OnMenuTestClick(wxCommandEvent& event);
	//void OnMenuDeleteClick(wxCommandEvent& event);
	//void OnMenuDuplicateClick(wxCommandEvent& event);
	//void OnMenuEditEventNameClick(wxCommandEvent& event);
	//void OnMenuCheckDoublesClick(wxCommandEvent& event);
	void OnButTestEvents(wxCommandEvent& event);
	void OnButJoyWizard(wxCommandEvent& event);
	void OnButAddKey(wxCommandEvent& event);
	void OnButDeleteKey(wxCommandEvent& event);
	void OnButRegenCache(wxCommandEvent& event);
	void OnButClearCache(wxCommandEvent& event);
	void OnButUpdateRoR(wxCommandEvent& event);
	void OnButCheckOpenCL(wxCommandEvent& event);
	void OnButCheckOpenCLBW(wxCommandEvent& event);
	void updateRoR();
	void OnShadowSliderScroll(wxScrollEvent& event);
	void OnSimpleSliderScroll(wxScrollEvent& event);
	void OnSimpleSlider2Scroll(wxScrollEvent& event);
	void OnForceFeedbackScroll(wxScrollEvent& event);
	void OnNoteBookPageChange(wxNotebookEvent& event);
	void OnNoteBook2PageChange(wxNotebookEvent& event);

	void updateItemText(wxTreeItemId item, event_trigger_t *t);
	void DSoundEnumerate(wxChoice* wxc);
//	void checkLinuxPaths();
	MyApp *app;
private:
//	float caelumFogDensity, SandStormFogStart;
	//bool postinstall;
	Ogre::Root *ogreRoot;
	wxPanel *graphicsPanel;
	wxPanel *settingsPanel;
	wxPanel *rsPanel;
	wxTextCtrl *gputext;
	wxChoice *renderer;
	std::vector<wxStaticText *> renderer_text;
	std::vector<wxChoice *> renderer_choice;
	wxChoice *textfilt;
	wxChoice *sky;
	wxChoice *shadow;
	wxCheckBox *shadowOptimizations;
	wxSlider *shadowDistance;
	wxStaticText *shadowDistanceText;
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
	wxCheckBox *leds;
	wxCheckBox *enablexfire;
	wxCheckBox *beamdebug;
	wxCheckBox *autodl;
	wxCheckBox *posstor;
	wxCheckBox *extcam;
	wxCheckBox *dashboard;
	wxCheckBox *mirror;
	wxCheckBox *envmap;
	wxCheckBox *sunburn;
	wxCheckBox *hdr;
	wxCheckBox *glow;
	wxCheckBox *mblur;
	wxCheckBox *skidmarks;
	wxCheckBox *creaksound;
	wxChoice *sound;
	wxChoice *thread;
	wxChoice *flaresMode;
	wxChoice *languageMode;
	wxChoice *vegetationMode;
	wxChoice *screenShotFormat;
	wxChoice *gearBoxMode;
	wxCheckBox *wheel2;
	wxCheckBox *ffEnable;
	wxSlider *ffOverall;
	wxStaticText *ffOverallText;
	wxSlider *ffHydro;
	wxStaticText *ffHydroText;
	wxSlider *ffCenter;
	wxStaticText *ffCenterText;
	wxSlider *ffCamera;
	wxStaticText *ffCameraText;
	wxTreeListCtrl *cTree;
	wxTimer *timer1;
	//wxStaticText *controlText;
	//wxComboBox *ctrlTypeCombo;
	//wxString typeChoices[11];
	//wxButton *btnRemap;
	wxButton *btnUpdate, *btnToken;
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
	wxHtmlWindow *helphtmw;
//	wxTextCtrl *p2pport;
#endif

	void tryLoadOpenCL();
	int openCLAvailable;
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
	wxListBox *cbe;
	wxRadioBox *cbi;

	bool loadEventListing()
	{
		int i=0;
		while(i!=EV_MODE_LAST)
		{
			std::string eventName = eventInfo[i].name;
			std::string eventDescription = eventInfo[i].description + std::string("\n") + std::string("default mapping: ") + eventInfo[i].defaultKey;
			inputevents[eventName] = eventDescription;
			i++;
		}

		return true;
	}

public:
	KeyAddDialog(MyDialog *_dlg) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(500,400), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), dlg(_dlg)
	{
		selectedEvent="";
		selectedType="";
		selectedOption="  ";
		//wxString configpath=_dlg->app->UserPath+wxFileName::GetPathSeparator()+_T("config")+wxFileName::GetPathSeparator();
		loadEventListing(); //configpath.ToUTF8().data());

		desctext = new wxStaticText(this, wxID_ANY, _("Please select an input event."), wxPoint(5,5), wxSize(380, 30), wxST_NO_AUTORESIZE);
		desctext->Wrap(480);

		wxStaticText *text = new wxStaticText(this, 4, _("Event Name: "), wxPoint(5,35), wxSize(70, 25));
		wxString eventChoices[MAX_EVENTS];
		std::map<std::string, std::string>::iterator it;
		int counter=0;
		for(it=inputevents.begin(); it!=inputevents.end(); it++, counter++)
		{
			if(counter>MAX_EVENTS)
				break;
			eventChoices[counter] = conv(it->first);
		}
//		cbe = new wxComboBox(this, 1, _("Keyboard"), wxPoint(80,35), wxSize(300, 25), counter, eventChoices, wxCB_READONLY);
		cbe = new wxListBox(this, 1, wxPoint(80,35), wxSize(400, 270), counter, eventChoices, wxLB_SINGLE);
		cbe->SetSelection(0);

//		text = new wxStaticText(this, wxID_ANY, _("Input Type: "), wxPoint(5,120), wxSize(70,25));
		wxString ch[5];
		ch[0] = conv("Keyboard");
		ch[1] = conv("JoystickAxis");
		ch[2] = conv("JoystickSlider");
		ch[3] = conv("JoystickButton");
		ch[4] = conv("JoystickPov");
		cbi=new wxRadioBox(this, wxID_ANY, _("Input Type: "),  wxPoint(5,310), wxSize(480,45), 5, ch);
		cbi->SetSelection(0);

		wxButton *btnOK = new wxButton(this, 2, _("Add"), wxPoint(5,365), wxSize(190,25));
		btnOK->SetToolTip(wxString(_("Add the key")));

		wxButton *btnCancel = new wxButton(this, 3, _("Cancel"), wxPoint(200,365), wxSize(190,25));
		btnCancel->SetToolTip(wxString(_("close this dialog")));

		//SetBackgroundColour(wxColour("white"));
		Centre();
	}

	void onChangeEventComboBox(wxCommandEvent& event)
	{
		std::string s = inputevents[conv(cbe->GetStringSelection())];
		desctext->SetLabel(_("Event Description: ") + conv(s));
		selectedEvent = conv(cbe->GetStringSelection());
	}

	void onChangeTypeComboBox(wxCommandEvent& event)
	{
		selectedType = conv(cbi->GetStringSelection());
	}
	void onButCancel(wxCommandEvent& event)
	{
		EndModal(1);
	}
	void onButOK(wxCommandEvent& event)
	{
		selectedEvent = conv(cbe->GetStringSelection());
		selectedType = conv(cbi->GetStringSelection());
		// add some dummy values
		if(conv(selectedType) == conv("Keyboard"))
			selectedOption="";
		else if(conv(selectedType) == conv("JoystickAxis"))
			selectedOption=" 0 0";
		else if(conv(selectedType) == conv("JoystickButton"))
			selectedOption=" 0 0";
		else if(conv(selectedType) == conv("JoystickSlider"))
			selectedOption=" 0 X 0";
		else if(conv(selectedType) == conv("JoystickPov"))
			selectedOption=" 0 0";

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
	wxGauge *gp[MAX_TESTABLE_EVENTS];
	wxStaticText *t[MAX_TESTABLE_EVENTS];
	wxStaticText *tv[MAX_TESTABLE_EVENTS];
	wxStaticText *notice;
	wxTimer *timer;
	wxScrolledWindow *vwin;
	int num_items_visible;
public:
	KeyTestDialog(MyDialog *_dlg) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(500,600), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), dlg(_dlg)
	{
		// fixed unsed variable warning
		//wxStaticText *lt = new wxStaticText(this, wxID_ANY, _("Event Name"), wxPoint(5, 0), wxSize(200, 15));
		new wxStaticText(this, wxID_ANY, _("Event Name"), wxPoint(5, 0), wxSize(200, 15));
		new wxStaticText(this, wxID_ANY, _("Event Value (first: result, second: raw input)"), wxPoint(150, 0), wxSize(260, 15));
		notice = new wxStaticText(this, wxID_ANY, _("Please use any mapped input device in order to test."), wxPoint(20, 60), wxSize(260, 15));
		new wxButton(this, 1, _("ok (end test)"), wxPoint(5,575), wxSize(490,20));
		vwin = new wxScrolledWindow(this, wxID_ANY, wxPoint(0,20), wxSize(490,550));

		std::map<int, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
		std::map<int, std::vector<event_trigger_t> >::iterator it;
		std::vector<event_trigger_t>::iterator it2;
		int counter = 0;
		for(it = events.begin(); it!= events.end(); it++)
		{
			for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
			{
				if(counter >= MAX_TESTABLE_EVENTS)
					break;
				std::string eventName = INPUTENGINE.eventIDToName(it->first);
				g[counter] = new wxGauge(vwin, wxID_ANY, 1000, wxPoint(280, counter*20+5), wxSize(200, 7),wxGA_HORIZONTAL|wxBORDER_NONE);
				g[counter]->Hide();
				gp[counter] = new wxGauge(vwin, wxID_ANY, 1000, wxPoint(280, counter*20+5+7), wxSize(200, 7),wxGA_HORIZONTAL|wxBORDER_NONE);
				gp[counter]->Hide();
				t[counter] = new wxStaticText(vwin, wxID_ANY, conv(eventName), wxPoint(5, counter*20+5), wxSize(200, 15), wxALIGN_RIGHT|wxST_NO_AUTORESIZE);
				t[counter]->Hide();
				tv[counter] = new wxStaticText(vwin, wxID_ANY, conv(eventName), wxPoint(210, counter*20+5), wxSize(60, 15), wxALIGN_RIGHT|wxST_NO_AUTORESIZE);
				tv[counter]->Hide();
			}
		}
		// resize scroll window
		vwin->SetScrollbars(0, 20, 0, 5);

		if(!INPUTENGINE.setup(getOISHandle(this), true, false))
		{
			wxLogStatus(wxT("Unable to open inputs!"));
		}

		timer = new wxTimer(this, 2);
		timer->Start(50);
	}
	~KeyTestDialog()
	{
		timer->Stop();
		delete timer;
		timer = 0;
		INPUTENGINE.destroy();
		EndModal(0);
	}

	void update()
	{
		num_items_visible=0;
		std::map<int, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
		std::map<int, std::vector<event_trigger_t> >::iterator it;
		std::vector<event_trigger_t>::iterator it2;
		std::map<int,int> events_shown;
		int counter = 0;
		for(it = events.begin(); it!= events.end(); it++)
		{
			for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
			{
				if(counter >= MAX_TESTABLE_EVENTS)
					break;
				float v = INPUTENGINE.getEventValue(it->first) * 1000;
				float vp = INPUTENGINE.getEventValue(it->first, true) * 1000; // pure value, without deadzone
				bool already_shown = (events_shown.find(it->first) != events_shown.end());
				if(!already_shown && vp>1)
				{
					t[counter]->SetPosition(wxPoint(5, num_items_visible*20+5));
					t[counter]->Show();
					tv[counter]->SetPosition(wxPoint(210, num_items_visible*20+5));
					tv[counter]->Show();
					tv[counter]->SetLabel(wxString::Format(wxT("(%03.0f / %03.0f)"),v*0.1f, vp * 0.1f));
					g[counter]->Show();
					g[counter]->SetPosition(wxPoint(280, num_items_visible*20+5));
					g[counter]->SetValue((int)v);
					gp[counter]->Show();
					gp[counter]->SetPosition(wxPoint(280, num_items_visible*20+5+7));
					gp[counter]->SetValue((int)vp);
					num_items_visible++;
					events_shown[it->first]=1;
				}else
				{
					g[counter]->Hide();
					gp[counter]->Hide();
					t[counter]->Hide();
					tv[counter]->Hide();
				}
			}
		}
		vwin->SetScrollbars(0, 20, 0, num_items_visible);
		if(num_items_visible)
			notice->Hide();
		else
			notice->Show();
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
	int lastBtn, lastJoy, lastPov, lastPovDir;
	int selectedJoystickLast, selectedAxisLast;
	float joyMinState[MAX_JOYSTICKS][MAX_JOYSTICK_AXIS];
	float joyMaxState[MAX_JOYSTICKS][MAX_JOYSTICK_AXIS];
	float joySliderMinState[MAX_JOYSTICKS][2][MAX_JOYSTICK_SLIDERS];
	float joySliderMaxState[MAX_JOYSTICKS][2][MAX_JOYSTICK_SLIDERS];
	void OnEraseBackGround(wxEraseEvent& event) { event.Skip(); };

public:
	KeySelectDialog(MyDialog *_dlg, event_trigger_t *_t) : wxDialog(NULL, wxID_ANY, wxString(), wxDefaultPosition, wxSize(300,600), wxSTAY_ON_TOP|wxDOUBLE_BORDER|wxCLOSE_BOX), t(_t), dlg(_dlg)
	{
		selectedJoystickLast=-1;
		selectedAxisLast=-1;

		bool captureMouse = false;
		if(t->eventtype == ET_MouseAxisX || t->eventtype == ET_MouseAxisY || t->eventtype == ET_MouseAxisZ || t->eventtype == ET_MouseButton)
			captureMouse = true;

		for(int i=0;i<MAX_JOYSTICKS;i++)
		{
			for(int x=0;x<MAX_JOYSTICK_AXIS;x++)
			{
				joyMinState[i][x]=0;
				joyMaxState[i][x]=0;
			}
			for(int x=0;x<MAX_JOYSTICK_SLIDERS;x++)
			{
				joySliderMinState[i][0][x]=0;
				joySliderMaxState[i][0][x]=0;
				joySliderMinState[i][1][x]=0;
				joySliderMaxState[i][1][x]=0;
			}
		}

		if(!INPUTENGINE.setup(getOISHandle(this), true, captureMouse))
		{
			wxLogStatus(wxT("Unable to open default input map!"));
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

		new wxStaticPicture(this, -1, bitmap, wxPoint(0, 0), wxSize(300, 500),wxNO_BORDER);

		//btnCancel = new wxButton(this, 1, "Cancel", wxPoint(50,175), wxSize(100,20));
		wxString st = conv(t->tmp_eventname);
		text = new wxStaticText(this, wxID_ANY, st, wxPoint(5,130), wxSize(190,20), wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
		text2 = new wxStaticText(this, wxID_ANY, wxString(), wxPoint(5,150), wxSize(290,550), wxALIGN_CENTRE|wxST_NO_AUTORESIZE);
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
		EndModal(0);
		INPUTENGINE.destroy();
		timer->Stop();
		delete timer;
		delete text;
		delete text2;
	}

	void update()
	{
		if(INPUTENGINE.isKeyDown(OIS::KC_RETURN) || INPUTENGINE.isKeyDown(OIS::KC_ESCAPE))
		{
			// done!
			closeWindow();
			return;
		}
		if(INPUTENGINE.isKeyDown(OIS::KC_C))
		{
			//clear min/max
			for(int i=0;i<MAX_JOYSTICKS;i++)
			{
				for(int x=0;x<MAX_JOYSTICK_AXIS;x++)
				{
					joyMinState[i][x]=0;
					joyMaxState[i][x]=0;
				}
				for(int x=0;x<MAX_JOYSTICK_SLIDERS;x++)
				{
					joySliderMinState[i][0][x]=0;
					joySliderMaxState[i][0][x]=0;
					joySliderMinState[i][1][x]=0;
					joySliderMaxState[i][1][x]=0;
				}
			}
			if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
				text2->SetLabel(_("(Please move an Axis)\n"));
			if(t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderX)
				text2->SetLabel(_("(Please move a Slider)\n"));
		}

		if(t->eventtype == ET_Keyboard)
		{
			std::string combo;
			int keys = INPUTENGINE.getCurrentKeyCombo(&combo);
			if(lastCombo != combo && keys != 0)
			{
				strcpy(t->configline, combo.c_str());
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
				return;
			}
		} else if(t->eventtype == ET_JoystickPov)
		{
			int joyPov=-1, joyNum=-1, povDir=-1;
			int res = INPUTENGINE.getCurrentPovValue(joyNum, joyPov, povDir);
			if(res && joyPov != lastPov && joyNum != lastJoy && povDir != lastPovDir)
			{
				const char *dirStr = "North";
				if(povDir == OIS::Pov::North)     dirStr = "North";
				if(povDir == OIS::Pov::South)     dirStr = "South";
				if(povDir == OIS::Pov::East)      dirStr = "East";
				if(povDir == OIS::Pov::West)      dirStr = "West";
				if(povDir == OIS::Pov::NorthEast) dirStr = "NorthEast";
				if(povDir == OIS::Pov::SouthEast) dirStr = "SouthEast";
				if(povDir == OIS::Pov::NorthWest) dirStr = "NorthWest";
				if(povDir == OIS::Pov::SouthWest) dirStr = "SouthWest";
				std::string str = conv(wxString::Format(_T("%d %s"), joyPov, dirStr));
				strcpy(t->configline, str.c_str());
				t->joystickPovNumber = joyPov;
				t->joystickPovDirection = povDir;
				wxString s = conv(str);
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastPov = joyPov;
				lastJoy = joyNum;
				lastPovDir = povDir;
			} else if (!res)
			{
				wxString s = _("(Please use your POV)");
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastBtn = -1;
				lastJoy = -1;
			} else if(joyPov >= 0 && joyNum >= 0 && povDir >= 0)
			{
				closeWindow();
				return;
			}
		} else if(t->eventtype == ET_JoystickButton)
		{
			int joyBtn=-1, joyNum=-1;
			int res = INPUTENGINE.getCurrentJoyButton(joyNum, joyBtn);
			if(res && joyBtn != lastBtn && joyNum != lastJoy)
			{
				std::string str = conv(wxString::Format(_T("%d"), joyBtn));
				strcpy(t->configline, str.c_str());
				t->joystickButtonNumber = joyBtn;
				wxString s = conv(str);
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastBtn = joyBtn;
				lastJoy = joyNum;
			} else if (!res)
			{
				wxString s = _("(Please press a Joystick Button)");
				if(text2->GetLabel() != s)
					text2->SetLabel(s);
				lastBtn = -1;
				lastJoy = -1;
			} else if(joyBtn >= 0 && joyNum >= 0)
			{
				closeWindow();
				return;
			}
		} else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
		{
			std::string str = "";
			int selectedJoystick=-1, selectedAxis = -1, cd=0;
			for(int i=0;i<INPUTENGINE.getNumJoysticks();i++)
			{
				OIS::JoyStickState *j = INPUTENGINE.getCurrentJoyState(i);

				std::vector<OIS::Axis>::iterator it;
				int counter = 0, cdi=0;
				for(it=j->mAxes.begin();it!=j->mAxes.end();it++, counter++)
				{
					float value = 100*((float)it->abs/(float)OIS::JoyStick::MAX_AXIS);
					if(value < joyMinState[i][counter]) joyMinState[i][counter] = value;
					if(value > joyMaxState[i][counter]) joyMaxState[i][counter] = value;
					float delta = fabs((float)(joyMaxState[i][counter]-joyMinState[i][counter]));
					if(value > 10 || delta > 10)
					{
						str += std::string("Joystick ") + conv(wxString::Format(wxT("%d"), i)) + std::string(", Axis ") + conv(wxString::Format(wxT("%02d"), counter)) + std::string(": ") + conv(wxString::Format(wxT("%03d"), (int)value)) + std::string(" (DELTA:") + conv(wxString::Format(wxT("%0.2f"), delta))+ std::string(")\n");
						cdi++;
					}
				}
				if(cdi)
					str += std::string(" ---- \nResult: ");
				cd+=cdi;

				// check for delta on each axis
				float maxdelta=0;
				int sAxis=-1, sJoy=-1;
				for(int c=0;c<=counter;c++)
				{
					float delta = fabs((float)(joyMaxState[i][c]-joyMinState[i][c]));
					if(delta > maxdelta && delta > 50)
					{
						selectedAxis = c;
						selectedJoystick = i;
						maxdelta = delta;
					}
				}
			}

			if(!cd) str = conv(_("(Please move an Axis)\n"));
			if(selectedJoystick>=0 && selectedAxis >= 0)
			{
				// changed?
				float delta = fabs((float)(joyMaxState[selectedJoystick][selectedAxis]-joyMinState[selectedJoystick][selectedAxis]));
				if(selectedAxis != selectedAxisLast && selectedJoystick != selectedJoystickLast)
				{
					if(selectedAxisLast != -1)
					{
						// reset max/mins
						joyMaxState[selectedJoystickLast][selectedAxisLast] = 0;
						joyMinState[selectedJoystickLast][selectedAxisLast] = 0;
					}
					selectedJoystickLast = selectedJoystick;
					selectedAxisLast = selectedAxis;
				}
				static float olddeta = 0;
				bool upper = (joyMaxState[selectedJoystick][selectedAxis] > 50);
				if(delta > 50 && delta < 120)
				{
					str += std::string("Joystick ") + conv(wxString::Format(_T("%d"), selectedJoystick)) + std::string(", Axis ") + conv(wxString::Format(_T("%d"), selectedAxis)) + \
						(upper?std::string(" UPPER"):std::string(" LOWER"));
					t->joystickAxisNumber = selectedAxis;
					t->joystickNumber = selectedJoystick;
					t->joystickAxisRegion = (upper?1:-1); // half axis
					strcpy(t->configline, (upper?"UPPER":"LOWER"));
				}
				else if(delta > 120)
				{
					str += std::string("Joystick ") + conv(wxString::Format(_T("%d"), selectedJoystick)) + std::string(", Axis ") + conv(wxString::Format(_T("%d"), selectedAxis));
					t->joystickAxisNumber = selectedAxis;
					t->joystickNumber = selectedJoystick;
					t->joystickAxisRegion = 0; // full axis
					strcpy(t->configline, "");
				}

				str += std::string("\n====\nPress RETURN or ESC to complete\nPress 'c' to reset ranges\n");
				//str = wxString::Format(_T("%f"), joyMaxState[0]) + "/" + wxString::Format(_T("%f"), joyMinState[i][0]) + " - " + wxString::Format(_T("%f"), fabs((float)(joyMaxState[0]-joyMinState[i][0])));
			}
			wxString s = conv(str);
			if(!s.empty() && text2->GetLabel() != s)
				text2->SetLabel(s);
		} else if(t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY)
		{
			std::string str = "";
			int sliders=0, cd=0;
			for(int i=0;i<INPUTENGINE.getNumJoysticks();i++)
			{
				OIS::JoyStickState *j = INPUTENGINE.getCurrentJoyState(i);

				int slidercount = INPUTENGINE.getJoyComponentCount(OIS::OIS_Slider, i);
				if(!slidercount) continue;
				sliders += slidercount;

				std::vector<OIS::Slider>::iterator it;
				int counter = 0, cdi=0;

				for(int s=0;s<slidercount;s++, counter++)
				{
					for(int type=0;type<2;type++)
					{
						float value = ((type == 0)?j->mSliders[s].abX:j->mSliders[s].abY);

						value = 100*(value/(float)OIS::JoyStick::MAX_AXIS);
						if(value < joySliderMinState[i][type][counter]) joySliderMinState[i][type][counter] = value;
						if(value > joySliderMaxState[i][type][counter]) joySliderMaxState[i][type][counter] = value;
						float delta = fabs((float)(joySliderMaxState[i][type][counter]-joySliderMinState[i][type][counter]));
						if(value > 10 || delta > 10)
						{
							str += std::string("Joystick ");
							str += conv(wxString::Format(wxT("%d"), i));
							str += std::string(", Slider ");
							str += conv(wxString::Format(_T("%c"), type?'Y':'X'));
							str += std::string(": ");
							str += conv(wxString::Format(_T("%d"), counter));
							str += std::string(": ");
							str += conv(wxString::Format(_T("%0.2f"), value));
							str += std::string("\n");
							cdi++;
						}
					}
				}
				if(cdi)
					str += std::string(" ---- \nResult: ");
				cd+=cdi;

				// check for delta on each axis
				if(!cd) str += conv(_("(Please move a Slider)\n"));
				float maxdelta=0;
				int selectedSlider = -1, selectedSliderType = -1;
				for(int c=0;c<=counter;c++)
				{
					float deltaX = fabs((float)(joySliderMaxState[i][0][c]-joySliderMinState[i][0][c]));
					float deltaY = fabs((float)(joySliderMaxState[i][1][c]-joySliderMinState[i][1][c]));
					if(deltaX > maxdelta && deltaX > 50)
					{
						selectedSlider = c;
						selectedSliderType = 0;
						maxdelta = deltaX;
					}
					if(deltaY > maxdelta && deltaY > 50)
					{
						selectedSlider = c;
						selectedSliderType = 1;
						maxdelta = deltaY;
					}
				}

				if(selectedSlider >= 0 && selectedSliderType >= 0)
				{
					float delta = fabs((float)(joySliderMaxState[i][selectedSliderType][selectedSlider]-joySliderMinState[i][selectedSliderType][selectedSlider]));
					static float olddeta = 0;

					if(selectedSliderType == 0)
						t->eventtype = ET_JoystickSliderX;
					else if(selectedSliderType == 1)
						t->eventtype = ET_JoystickSliderY;

					bool upper = (joySliderMaxState[i][selectedSliderType][selectedSlider] > 50);
					if(delta > 10)
					{
						str += std::string("Joystick ") + conv(wxString::Format(wxT("%d"), i)) + std::string(", Slider ") + conv(wxString::Format(_T("%d"), selectedSlider)); //+ " - " + wxString::Format(_T("%f"), lastJoyEventTime);
						t->joystickSliderNumber = selectedSlider;
						strcpy(t->configline, "");
					}
				}
				str += std::string("\n====\nPress RETURN or ESC to complete\nPress 'c' to reset ranges\n");
				//str = wxString::Format(_T("%f"), joyMaxState[0]) + "/" + wxString::Format(_T("%f"), joyMinState[i][0]) + " - " + wxString::Format(_T("%f"), fabs((float)(joyMaxState[0]-joyMinState[i][0])));
				wxString s = conv(str);
				if(!s.empty() && text2->GetLabel() != s)
					text2->SetLabel(s);
			}
			if(!sliders)
			{
				// no sliders found!
				closeWindow();
			}
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
	update_html,
	CTREE_ID,
	command_load_keymap,
	command_save_keymap,
	command_add_key,
	command_delete_key,
	command_testevents,
	clear_cache,
	regen_cache,
	update_ror,
	EVC_LANG,
	SCROLL1,
	SCROLL2,
	EVT_CHANGE_RENDERER,
	RENDERER_OPTION=990,
	mynotebook,
	mynotebook2,
	mynotebook3,
	command_joywizard,
	FFSLIDER,
	get_user_token,
	check_opencl,
	check_opencl_bw,
	shadowslider,
	shadowschoice,
	shadowopt,
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
	EVT_BUTTON(command_cancel, MyDialog::OnButCancel)
	EVT_BUTTON(command_save, MyDialog::OnButSave)
	EVT_BUTTON(command_play, MyDialog::OnButPlay)
	EVT_BUTTON(command_restore, MyDialog::OnButRestore)
	EVT_BUTTON(net_test, MyDialog::OnTestNet)
	EVT_BUTTON(get_user_token, MyDialog::OnGetUserToken)
	EVT_BUTTON(clear_cache, MyDialog::OnButClearCache)
	EVT_BUTTON(regen_cache, MyDialog::OnButRegenCache)
	EVT_BUTTON(update_ror, MyDialog::OnButUpdateRoR)
	EVT_BUTTON(check_opencl, MyDialog::OnButCheckOpenCL)
	EVT_BUTTON(check_opencl_bw, MyDialog::OnButCheckOpenCLBW)
	EVT_HTML_LINK_CLICKED(main_html, MyDialog::OnLinkClicked)
	EVT_HTML_LINK_CLICKED(update_html, MyDialog::OnLinkClickedUpdate)
	//EVT_SCROLL(MyDialog::OnSightRangeScroll)
	EVT_CHOICE(shadowschoice, MyDialog::onChangeShadowChoice)
	EVT_COMMAND_SCROLL_CHANGED(shadowslider, MyDialog::OnShadowSliderScroll)
	EVT_COMMAND_SCROLL_CHANGED(SCROLL1, MyDialog::OnSimpleSliderScroll)
	EVT_COMMAND_SCROLL_CHANGED(SCROLL2, MyDialog::OnSimpleSlider2Scroll)
	EVT_COMMAND_SCROLL(FFSLIDER, MyDialog::OnForceFeedbackScroll)
	EVT_CHOICE(EVC_LANG, MyDialog::onChangeLanguageChoice)
	//EVT_BUTTON(BTN_REMAP, MyDialog::OnButRemap)

	EVT_NOTEBOOK_PAGE_CHANGED(mynotebook, MyDialog::OnNoteBookPageChange)
	EVT_NOTEBOOK_PAGE_CHANGED(mynotebook2, MyDialog::OnNoteBook2PageChange)

	EVT_TREE_SEL_CHANGING (CTREE_ID, MyDialog::onTreeSelChange)
	EVT_TREE_ITEM_ACTIVATED(CTREE_ID, MyDialog::onActivateItem)
	EVT_TREE_ITEM_RIGHT_CLICK(CTREE_ID, MyDialog::onRightClickItem)

	EVT_BUTTON(command_load_keymap, MyDialog::OnButLoadKeymap)
	EVT_BUTTON(command_save_keymap, MyDialog::OnButSaveKeymap)

	EVT_BUTTON(command_joywizard, MyDialog::OnButJoyWizard)
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

	EVT_MENU(50, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(51, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(52, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(53, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(54, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(55, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(56, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(57, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(58, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(59, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(60, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(61, MyDialog::OnMenuJoystickOptionsClick)
	EVT_MENU(62, MyDialog::OnMenuJoystickOptionsClick)
	EVT_CHOICE(EVT_CHANGE_RENDERER, MyDialog::OnChangeRenderer)

END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeySelectDialog, wxDialog)
	EVT_BUTTON(1, KeySelectDialog::OnButCancel)
	EVT_TIMER(2, KeySelectDialog::OnTimer)
	EVT_ERASE_BACKGROUND(KeySelectDialog::OnEraseBackGround)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(KeyAddDialog, wxDialog)
//	EVT_COMBOBOX(1, KeyAddDialog::onChangeEventComboBox)
	EVT_LISTBOX(1, KeyAddDialog::onChangeEventComboBox)
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

#ifdef USE_CONSOLE
IMPLEMENT_APP_CONSOLE(MyApp)
#else
IMPLEMENT_APP(MyApp)
#endif

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
		wxLogStatus(wxT("error opening ") + dir);
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
				wxLogStatus(wxT("failed to get Language: "+name));
			}
		} while (dp.GetNext(&name));
	}
	return 0;
}

void initLanguage(wxString languagePath, wxString userpath)
{
	// add language stuff

	// Initialize the catalogs we'll be using
	wxString langfile = wxT("ror");
	wxString dirsep = wxT("/");
	wxString basedir = languagePath;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	dirsep = wxT("\\");
#endif
	//basedir = basedir + dirsep + _T("languages");
	wxLocale::AddCatalogLookupPathPrefix(languagePath);

	// get all available languages
	getAvLang(languagePath, avLanguages);
	wxLogStatus(wxT("searching languages in ")+basedir);
	if(avLanguages.size() > 0)
	{
		wxLogStatus(wxT("Available Languages:"));
		std::vector<wxLanguageInfo *>::iterator it;
		for(it = avLanguages.begin(); it!=avLanguages.end(); it++)
		{
			if(*it)
			{
				wxLogStatus(wxT(" * ") + (*it)->Description + wxT("(") + (*it)->CanonicalName + wxT(")"));
			}
		}
	} else
	{
		wxLogStatus(wxT("no Languages found!"));
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
		wxLogStatus(wxT("asking system for default language."));
		language = const_cast<wxLanguageInfo *>(wxLocale::GetLanguageInfo(lang_locale.GetSystemLanguage()));
		if(language)
		{
			wxLogStatus(wxT(" system returned: ") + language->Description + wxT("(") + language->CanonicalName + wxT(")"));
		} else
		{
			wxLogStatus(wxT(" error getting language information!"));
		}
	}
	wxLogStatus(wxT("preferred language: ")+language->Description);
	wxString lshort = language->CanonicalName.substr(0, 2);
	wxString tmp = basedir + dirsep + lshort + dirsep + langfile + wxT(".mo");
	wxLogStatus(wxT("lang file: ") + tmp);
	if(wxFileName::FileExists(tmp))
	{
		wxLogStatus(wxT("language existing, using it!"));
		if(!lang_locale.IsAvailable((wxLanguage)language->Language))
		{
			wxLogStatus(wxT("language file existing, but not found via wxLocale!"));
			wxLogStatus(wxT("is the language installed on your system?"));
		}
		bool res = lang_locale.Init((wxLanguage)language->Language, wxLOCALE_CONV_ENCODING);
		if(!res)
		{
			wxLogStatus(wxT("error while initializing language!"));
		}
		res = lang_locale.AddCatalog(langfile);
		if(!res)
		{
			wxLogStatus(wxT("error while loading language!"));
		}
	}
	else
	{
		lang_locale.Init(wxLANGUAGE_DEFAULT, wxLOCALE_CONV_ENCODING);
		wxLogStatus(wxT("language not existing, no lang_locale support!"));
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
	tfn.AppendDir(wxT("Rigs of Rods"));
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
	tfn.AppendDir(_T(".rigsofrods"));
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
	tsk.AppendDir(wxT("skeleton"));
	SkeletonPath=tsk.GetPath();
	tsk=wxFileName(ProgramPath, wxEmptyString);
	tsk.AppendDir(wxT("languages"));
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
		ttfn.AppendDir(wxT("configurator"));
//		ProgramPath=ttfn.GetPath();
		ttfn.AppendDir(wxT("confdata"));
		languagePath=ttfn.GetPath();
		ttfn=tfn;
		ttfn.AppendDir(wxT("skeleton"));
		SkeletonPath=ttfn.GetPath();
		ttfn=tfn;
		ttfn.AppendDir(wxT("userspace"));
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

void MyApp::initLogging()
{
	// log everything
	wxLog::SetLogLevel(wxLOG_Max);
	wxLog::SetVerbose();

	// use stdout always
	wxLog *logger_cout = new wxLogStream(&std::cout);
	wxLog::SetActiveTarget(logger_cout);

	wxFileName clfn=wxFileName(UserPath, wxEmptyString);
	clfn.AppendDir(wxT("logs"));
	clfn.SetFullName(wxT("configlog.txt"));
	FILE *f = fopen(conv(clfn.GetFullPath()).c_str(), "w");
	if(f)
	{
		// and a file log
		wxLog *logger_file = new wxLogStderr(f);
		wxLogChain *logChain = new wxLogChain(logger_file);
	}
	wxLogStatus(wxT("log started"));
}

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
	buildmode=false;
	if (argc==2 && wxString(argv[1])==wxT("/buildmode")) buildmode=true;
	//setup the user filesystem
	if (!filesystemBootstrap()) return false;

	initLogging();
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

	wxLogStatus(wxT("Creating dialog"));
	// create the main application window
	MyDialog *dialog = new MyDialog(_("Rigs of Rods configuration"), this);

	// and show it (the frames, unlike simple controls, are not shown when
	// created initially)
	wxLogStatus(wxT("Showing dialog"));
	dialog->Show(true);
	SetTopWindow(dialog);
	SetExitOnFrameDelete(false);
	// success: wxApp::OnRun() will be called which will enter the main message
	// loop and the application will run. If we returned false here, the
	// application would exit immediately.
	wxLogStatus(wxT("App ready"));
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
MyDialog::MyDialog(const wxString& title, MyApp *_app) : wxDialog(NULL, wxID_ANY, title,  wxPoint(100, 100), wxSize(500, 580), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER), openCLAvailable(0)
{
	app=_app;

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	SetWindowVariant(wxWINDOW_VARIANT_MINI); //smaller texts for macOS
#endif
	SetWindowStyle(wxRESIZE_BORDER | wxCAPTION);
	//SetMinSize(wxSize(300,300));

	wxLogStatus(wxT("InputEngine starting"));
	wxFileName tfn=wxFileName(app->UserPath, wxEmptyString);
	tfn.AppendDir(_T("config"));
	wxLogStatus(wxT("Searching input.map in ")+tfn.GetPath());
	InputMapFileName=tfn.GetPath()+wxFileName::GetPathSeparator()+_T("input.map");
	std::string path = ((tfn.GetPath()+wxFileName::GetPathSeparator()).ToUTF8().data());
	if(!INPUTENGINE.setup(getOISHandle(this), false, false, 0))
	{
		wxLogStatus(wxT("Unable to setup inputengine!"));
	} else
	{
		if (!INPUTENGINE.loadMapping(path+"input.map"))
		{
			wxLogStatus(wxT("Unable to open default input map!"));
		}
	}
	wxLogStatus(wxT("InputEngine started"));
	kd=0;
	controlItemCounter = 0;

//	SandStormFogStart = 500;
//	caelumFogDensity = 0.005;

	//postinstall = _postinstall;

	wxFileSystem::AddHandler( new wxInternetFSHandler );
	//build dialog here
	wxToolTip::Enable(true);
	wxToolTip::SetDelay(100);
	//image
//	wxBitmap *bitmap=new wxBitmap("data\\config.png", wxBITMAP_TYPE_BMP);
	//wxLogStatus(wxT("Loading bitmap"));
	wxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
	mainsizer->SetSizeHints(this);

	// using xpm now
	const wxBitmap bm(config_xpm);
	wxStaticPicture *imagePanel = new wxStaticPicture(this, -1, bm,wxPoint(0, 0), wxSize(500, 100), wxNO_BORDER);
	mainsizer->Add(imagePanel, 0, wxGROW);

	//notebook
	wxNotebook *nbook=new wxNotebook(this, mynotebook, wxPoint(3, 100), wxSize(490, 415));
	mainsizer->Add(nbook, 1, wxGROW);

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

	// settings
	settingsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(settingsPanel, _("Settings"), true);
	wxSizer *settingsSizer = new wxBoxSizer(wxVERTICAL);
	//settingsSizer->SetSizeHints(this);
	settingsPanel->SetSizer(settingsSizer);
	// second notebook containing the settings tabs
	wxNotebook *snbook=new wxNotebook(settingsPanel, mynotebook2, wxPoint(0, 0), wxSize(100,250));
	settingsSizer->Add(snbook, 1, wxGROW);

	rsPanel=new wxPanel(snbook, -1);
	snbook->AddPage(rsPanel, _("Render System"), true);

	graphicsPanel=new wxPanel(snbook, -1);
	snbook->AddPage(graphicsPanel, _("Graphics"), true);

	wxPanel *soundsPanel=new wxPanel(snbook, -1);
	snbook->AddPage(soundsPanel, _("Sounds"), false);

	wxPanel *cpuPanel=new wxPanel(snbook, -1);
	snbook->AddPage(cpuPanel, _("CPU"), false);

	// Controls
	wxPanel *controlsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(controlsPanel, _("Controls"), false);
	wxSizer *controlsSizer = new wxBoxSizer(wxVERTICAL);
	controlsPanel->SetSizer(controlsSizer);
	// third notebook for control tabs
	wxNotebook *ctbook=new wxNotebook(controlsPanel, mynotebook3, wxPoint(0, 0), wxSize(100,250));
	controlsSizer->Add(ctbook, 1, wxGROW);

	wxPanel *eventsPanel=new wxPanel(ctbook, -1);
	ctbook->AddPage(eventsPanel, _("Bindings"), true);
	wxSizer *eventsSizer = new wxBoxSizer(wxVERTICAL);
	eventsPanel->SetSizer(eventsSizer);

	wxPanel *ffPanel=new wxPanel(ctbook, -1);
	ctbook->AddPage(ffPanel, _("Force Feedback"), false);

	wxPanel *ctsetPanel=new wxPanel(ctbook, -1);
	ctbook->AddPage(ctsetPanel, _("Advanced"), false);
	wxSizer *ctsetSizer = new wxBoxSizer(wxVERTICAL);
	ctsetPanel->SetSizer(ctsetSizer);

	// the events page
	int maxTreeHeight = 310;
	int maxTreeWidth = 480;
	cTree = new wxTreeListCtrl(eventsPanel,
                       CTREE_ID,
                       wxPoint(0, 0),
					   wxSize(maxTreeWidth, maxTreeHeight));

	long style = cTree->GetWindowStyle();
	//style |= wxTR_HAS_BUTTONS;
	style |= wxTR_FULL_ROW_HIGHLIGHT;
	style |= wxTR_HIDE_ROOT;
	style |= wxTR_ROW_LINES;
	cTree->SetWindowStyle(style);
	eventsSizer->Add(cTree, 2, wxGROW);

	wxPanel *controlsInfoPanel = new wxPanel(eventsPanel, wxID_ANY, wxPoint(0, 0 ), wxSize( maxTreeWidth, 390 ));
	eventsSizer->Add(controlsInfoPanel, 0, wxGROW);
	wxButton *wizBtn = new wxButton(controlsInfoPanel, command_joywizard, _("Wizard"), wxPoint(5,5), wxSize(80, 50));
	// XXX TOFIX: disable until fixed!
	wizBtn->Disable();

	btnAddKey = new wxButton(controlsInfoPanel, command_add_key, _("Add"), wxPoint(90,5), wxSize(80, 50));
	btnDeleteKey = new wxButton(controlsInfoPanel, command_delete_key, _("Delete selected"), wxPoint(175,5), wxSize(80, 50));
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
	wxPanel *advancedPanel=new wxPanel(snbook, -1);
	snbook->AddPage(advancedPanel, _("Advanced"), false);

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// this is removed for now
	wxPanel *updatePanel=new wxPanel(nbook, -1);
	nbook->AddPage(updatePanel, _("Updates"), false);
#endif
//	wxPanel *aboutPanel=new wxPanel(nbook, -1);
//	nbook->AddPage(aboutPanel, "About", false);

#ifdef USE_OPENCL
	wxPanel *GPUPanel=new wxPanel(nbook, -1);
	nbook->AddPage(GPUPanel, _("OpenCL"), false);
#endif // USE_OPENCL

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

	// clear the renderer settings and fill them later
	dText = new wxStaticText(rsPanel, -1, _("Render System"), wxPoint(10, 28));
	renderer = new wxChoice(rsPanel, EVT_CHANGE_RENDERER, wxPoint(220, 25), wxSize(220, -1), 0);

	// renderer options done, do the rest
	int y = 10;
	int x_row1 = 150, x_row2 = 300;
	dText = new wxStaticText(graphicsPanel, -1, _("Texture filtering:"), wxPoint(10,y+3));
	textfilt=new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	textfilt->Append(conv("None (fastest)"));
	textfilt->Append(conv("Bilinear"));
	textfilt->Append(conv("Trilinear"));
	textfilt->Append(conv("Anisotropic (best looking)"));
	textfilt->SetToolTip(_("Most recent hardware can do Anisotropic filtering without significant slowdown.\nUse lower settings only on old or poor video chipsets."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Sky type:"), wxPoint(10,y+3));
	sky=new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	sky->Append(conv("Sandstorm (fastest)"));
	sky->Append(conv("Caelum (best looking, slower)"));
	sky->SetToolTip(_("Caelum sky is nice but quite slow unless you have a high-powered GPU."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, wxID_ANY, _("Shadow type:"), wxPoint(10,y+3));
	shadow=new wxChoice(graphicsPanel, shadowschoice, wxPoint(x_row1, y), wxSize(200, -1), 0);
	shadow->Append(conv("No shadows (fastest)"));
	shadow->Append(conv("Texture shadows"));
	shadow->Append(conv("Stencil shadows (best looking)"));
#if OGRE_VERSION>0x010602
	shadow->Append(conv("Parallel-split Shadow Maps"));
#endif //OGRE_VERSION
	shadow->SetToolTip(_("Texture shadows are fast but limited: jagged edges, no object self-shadowing, limited shadow distance.\nStencil shadows are slow but gives perfect shadows."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, wxID_ANY, _("Shadow Distance:"), wxPoint(10,y+3));
	shadowDistance=new wxSlider(graphicsPanel, shadowslider, 50, 5, 200,wxPoint(x_row1, y), wxSize(200, -1));
	shadowDistance->SetToolTip(_("Adjusts how far the shadows will be drawn"));
	shadowDistanceText = new wxStaticText(graphicsPanel, wxID_ANY, _("50 m"), wxPoint(x_row1 + 210,y+3));
	y+=25;

	shadowOptimizations=new wxCheckBox(graphicsPanel, shadowopt, _("Shadow Performance Optimizations"), wxPoint(x_row1, y), wxSize(200, -1), 0);
	shadowOptimizations->SetToolTip(_("When turned on, it disables the vehicles shadow when driving in first person mode."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Water type:"), wxPoint(10,y+3));
	water=new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
//	water->Append(wxString("None")); //we don't want that!
	water->Append(conv("Basic (fastest)"));
	water->Append(conv("Reflection"));
	water->Append(conv("Reflection + refraction (speed optimized)"));
	water->Append(conv("Reflection + refraction (quality optimized)"));
	water->SetToolTip(_("Water reflection is slower, and requires a good GPU. In speed optimized mode you may experience excessive camera shaking."));
	y+=25;

	waves=new wxCheckBox(graphicsPanel, -1, _("Enable waves"), wxPoint(x_row1, y+3));
	waves->SetToolTip(_("Enabling waves adds a lot of fun in boats, but can have a big performance impact on some specific hardwares."));
	spray=new wxCheckBox(graphicsPanel, -1, _("Water spray"), wxPoint(x_row2, y+3));
	spray->SetToolTip(_("No effect unless you mess with water, looks pretty good also."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Vegetation:"), wxPoint(10, y+3));
	vegetationMode=new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	vegetationMode->Append(conv("None (fastest)"));
	vegetationMode->Append(conv("20%"));
	vegetationMode->Append(conv("50%"));
	vegetationMode->Append(conv("Full (best looking, slower)"));
	vegetationMode->SetToolTip(_("This determines how much grass and how many trees (and such objects) should get displayed."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Particle systems:"), wxPoint(10, y));
	dust=new wxCheckBox(graphicsPanel, -1, _("Dust"), wxPoint(x_row1, y));
	dust->SetToolTip(_("This may hurt framerate a bit on old systems, but it looks pretty good."));
	heathaze=new wxCheckBox(graphicsPanel, -1, _("HeatHaze"), wxPoint(x_row2, y));
	heathaze->SetToolTip(_("Heat Haze from engines, major slowdown. (only activate with recent hardware)"));
	y+=15;
	smoke=new wxCheckBox(graphicsPanel, -1, _("Engine smoke"), wxPoint(x_row1, y));
	smoke->SetToolTip(_("Not much impact on speed."));
	cpart=new wxCheckBox(graphicsPanel, -1, _("Custom Particles"), wxPoint(x_row2, y));
	cpart->SetToolTip(_("Particles like water cannons or plain trails. Can be over-used in Multiplayer."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Cockpit options:"), wxPoint(10, y));
	mirror=new wxCheckBox(graphicsPanel, -1, _("Mirrors"), wxPoint(x_row1, y));
	mirror->SetToolTip(_("Shows the rear view mirrors in 1st person view. May cause compatibility problems for very old video cards."));
	dashboard=new wxCheckBox(graphicsPanel, -1, _("Dashboard"), wxPoint(x_row2, y));
	dashboard->SetToolTip(_("Shows the dynamic dashboard in 1st person view. May cause compatibility problems for very old video cards."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Visual effects:"), wxPoint(10, y));
	sunburn=new wxCheckBox(graphicsPanel, -1, _("Sunburn"), wxPoint(x_row1, y));
	sunburn->SetToolTip(_("Requires a recent video card. Adds a bluish blinding effect."));
	hdr=new wxCheckBox(graphicsPanel, -1, _("HDR"), wxPoint(x_row2, y));
	hdr->SetToolTip(_("Requires a recent video card. Add a lightning effect that simulates the light sensitivity of the human eye."));
	y+=15;
	mblur=new wxCheckBox(graphicsPanel, -1, _("Motion blur"), wxPoint(x_row1, y));
	mblur->SetToolTip(_("Requires a recent video card. Adds a motion blur effect."));
	skidmarks=new wxCheckBox(graphicsPanel, -1, _("Skidmarks"), wxPoint(x_row2, y));
	skidmarks->SetToolTip(_("Adds tire tracks to the ground."));
	y+=15;
	envmap=new wxCheckBox(graphicsPanel, -1, _("High quality reflective effects"), wxPoint(x_row1, y));
	envmap->SetToolTip(_("Enable high quality reflective effects. Causes a slowdown."));
	y+=15;
	glow=new wxCheckBox(graphicsPanel, -1, _("Glow"), wxPoint(x_row1, y));
	glow->SetToolTip(_("Adds a glow effect to lights"));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Screenshot Format:"), wxPoint(10, y));
	screenShotFormat =new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	screenShotFormat->Append(conv("jpg (smaller, default)"));
	screenShotFormat->Append(conv("png (bigger, no quality loss)"));
	screenShotFormat->SetToolTip(_("In what Format should screenshots be saved?"));

	//sounds panel
	dText = new wxStaticText(soundsPanel, -1, _("Sound source:"), wxPoint(20,28));
	sound=new wxChoice(soundsPanel, -1, wxPoint(150, 25), wxSize(-1, -1), 0);
	sound->Append(conv("No sound"));
	sound->Append(conv("Default"));
	sound->SetToolTip(_("Select the appropriate sound source.\nLeaving to Default should work most of the time."));
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	//add the rest
	DSoundEnumerate(sound);
#endif

	// creak sound?
	creaksound=new wxCheckBox(soundsPanel, -1, _("disable creak sound"), wxPoint(150, 70));
	creaksound->SetToolTip(_("You can disable the default creak sound by checking this box"));

	//cpu panel
	dText = new wxStaticText(cpuPanel, -1, _("Thread number:"), wxPoint(20,28));
	thread=new wxChoice(cpuPanel, -1, wxPoint(150, 25), wxSize(200, -1), 0);
	thread->Append(conv("1 (Standard CPU)"));
	thread->Append(conv("2 (Hyper-Threading or Dual core CPU)"));
	//thread->Append(conv("3 (multi core CPU, one thread per beam)"));
	thread->SetToolTip(_("If you have a Hyper-Threading, or Dual core or multiprocessor computer,\nyou will have a huge gain in speed by choosing the second option.\nBut this mode has some camera shaking issues.\n"));

	wheel2=new wxCheckBox(cpuPanel, -1, _("Enable advanced wheel model"), wxPoint(150, 70));
	wheel2->SetToolTip(_("Some vehicles may include an advanced wheel model that is CPU intensive.\nYou can force these vehicles to use a simpler model by unchecking this box."));

	//force feedback panel
	ffEnable=new wxCheckBox(ffPanel, -1, _("Enable Force Feedback"), wxPoint(150, 25));

	dText = new wxStaticText(ffPanel, -1, _("Overall force level:"), wxPoint(20,53));
	ffOverall=new wxSlider(ffPanel, FFSLIDER, 100, 0, 100, wxPoint(150, 50), wxSize(200, 40));
	ffOverall->SetToolTip(_("Adjusts the level of all the forces."));
	ffOverallText=new wxStaticText(ffPanel, -1, _(""), wxPoint(360,53));

	dText = new wxStaticText(ffPanel, -1, _("Steering feedback level:"), wxPoint(20,103));
	ffHydro=new wxSlider(ffPanel, FFSLIDER, 100, 0, 400, wxPoint(150, 100), wxSize(200, 40));
	ffHydro->SetToolTip(_("Adjusts the contribution of forces coming from the wheels and the steering mechanism."));
	ffHydroText=new wxStaticText(ffPanel, -1, _(""), wxPoint(360,103));

	dText = new wxStaticText(ffPanel, -1, _("Self-centering level:"), wxPoint(20,153));
	ffCenter=new wxSlider(ffPanel, FFSLIDER, 100, 0, 400, wxPoint(150, 150), wxSize(200, 40));
	ffCenter->SetToolTip(_("Adjusts the self-centering effect applied to the driving wheel when driving at high speed."));
	ffCenterText=new wxStaticText(ffPanel, -1, _(""), wxPoint(360,153));

	dText = new wxStaticText(ffPanel, -1, _("Inertia feedback level:"), wxPoint(20,203));
	ffCamera=new wxSlider(ffPanel, FFSLIDER, 100, 0, 400, wxPoint(150, 200), wxSize(200, 40));
	ffCamera->SetToolTip(_("Adjusts the contribution of forces coming shocks and accelerations (this parameter is currently unused)."));
	ffCamera->Enable(false);
	ffCameraText=new wxStaticText(ffPanel, -1, _(""), wxPoint(360,203));

	//update textboxes
	wxScrollEvent dummye;
	OnForceFeedbackScroll(dummye);

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
	networkhtmw->SetToolTip(_("Click on blue hyperlinks to select a server."));
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
	usertoken->SetToolTip(_("Your rigsofrods.com User Token."));

	btnToken = new wxButton(panel_net_bottom, get_user_token, _("Get Token"), wxPoint(360, 78), wxSize(110,25));

	btnUpdate = new wxButton(panel_net_bottom, net_test, _("Update"), wxPoint(360, 5), wxSize(110,65));

	netPanel->SetSizer(sizer_net);

	//	dText = new wxStaticText(netPanel, -1, wxString("P2P port number: "), wxPoint(10,153));
//	p2pport=new wxTextCtrl(netPanel, -1, "12334", wxPoint(150, 150), wxSize(100, -1));
//	dText = new wxStaticText(netPanel, -1, wxString("(default is 12334)"), wxPoint(260,153));

	//wxButton *testbut=
#endif

	//advanced panel
	y = 10;
	dText = new wxStaticText(advancedPanel, -1, _("You do not need to change these settings unless you experience problems"), wxPoint(10, y));
	y+=20;

	dText = new wxStaticText(advancedPanel, -1, _("Light source effects:"), wxPoint(10, y));
	flaresMode=new wxChoice(advancedPanel, -1, wxPoint(115, y), wxSize(200, -1), 0);
//	flaresMode->Append(_("None (fastest)")); //this creates a bug in the autopilot
	flaresMode->Append(conv("No light sources"));
	flaresMode->Append(conv("Only current vehicle, main lights"));
	flaresMode->Append(conv("All vehicles, main lights"));
	flaresMode->Append(conv("All vehicles, all lights"));
	flaresMode->SetToolTip(_("Determines which lights will project light on the environment.\nThe more light sources are used, the slowest it will be."));
	y+=25;

	dText = new wxStaticText(advancedPanel, -1, _("Language:"), wxPoint(10, y));
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
	languageMode=new wxChoice(advancedPanel, EVC_LANG, wxPoint(115, y), wxSize(200, -1), choices, wxCB_READONLY);
	languageMode->SetSelection(sel);
	languageMode->SetToolTip(_("This setting overrides the system's default language. You need to restart the configurator to apply the changes."));
	y+=25;

	dText = new wxStaticText(advancedPanel, -1, _("In case the mods cache becomes corrupted, \nuse these buttons to fix the cache."), wxPoint(10, y));
	y+=40;
	wxButton *btn = new wxButton(advancedPanel, regen_cache, _("Regen cache"), wxPoint(35, y));
	btn->SetToolTip(_("Use this to regenerate the cache outside of RoR. If this does not work, use the clear cache button."));
	btn = new wxButton(advancedPanel, clear_cache, _("Clear cache"), wxPoint(145, y));
	btn->SetToolTip(_("Use this to remove the whole cache and force the generation from ground up."));
	y+=30;

	dText = new wxStaticText(advancedPanel, -1, _("minimum visibility range in percent"), wxPoint(10,y));
	y+=20;
	//this makes a visual bug in macosx
	sightrange=new wxSlider(advancedPanel, -1, 30, 5, 500, wxPoint(30, y), wxSize(200, 40), wxSL_LABELS|wxSL_AUTOTICKS);
	sightrange->SetToolTip(_("This sets the minimum sight range Setting. It determines how far the terrain should be drawn. \n100% means you can view the whole terrain. \nWith 130% you can view everything across the diagonal Axis."));
	sightrange->SetTickFreq(10, 0);
	sightrange->SetPageSize(10);
	sightrange->SetLineSize(100);
	y+=50;

	// second column
	y=30;
	extcam=new wxCheckBox(advancedPanel, -1, _("Disable Camera Pitching"), wxPoint(320, y));
	extcam->SetToolTip(_("If you dislike the pitching external vehicle camera, you can disable it here."));
	y+=15;
	enableFog=new wxCheckBox(advancedPanel, -1, _("Fog"), wxPoint(320, y));
	enableFog->SetToolTip(_("If you dont want fog, uncheck this."));
	y+=15;
	replaymode=new wxCheckBox(advancedPanel, -1, _("Replay Mode"), wxPoint(320, y));
	replaymode->SetToolTip(_("Replay mode. (Can affect your frame rate)"));
	y+=15;
	posstor=new wxCheckBox(advancedPanel, -1, _("Enable Position Storage"), wxPoint(320, y));
	posstor->SetToolTip(_("Can be used to quick save and load positions of trucks"));
	y+=15;
	autodl=new wxCheckBox(advancedPanel, -1, _("Enable Auto-Downloader"), wxPoint(320, y));
	autodl->SetToolTip(_("This enables the automatic downloading of missing mods when using Multiplayer"));
	y+=15;
	enablexfire=0;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	enablexfire=new wxCheckBox(advancedPanel, -1, _("Enable XFire Extension"), wxPoint(320, y));
	enablexfire->SetToolTip(_("If you use XFire, you can switch on this option to generate a more detailed game info."));
#endif //WINDOWS
	y+=15;
	hydrax=new wxCheckBox(advancedPanel, -1, _("Hydrax Water System"), wxPoint(320, y));
	hydrax->SetToolTip(_("Enables the new water rendering system. EXPERIMENTAL. Overrides any water settings."));
	y+=30;
	dtm=new wxCheckBox(advancedPanel, -1, _("Debug Truck Mass"), wxPoint(320, y));
	dtm->SetToolTip(_("Prints all node masses to the RoR.log. Only use for truck debugging!"));
	y+=15;
	dcm=new wxCheckBox(advancedPanel, -1, _("Debug Collision Meshes"), wxPoint(320, y));
	dcm->SetToolTip(_("Shows all Collision meshes in Red to be able to position them correctly. Only use for debugging!"));
	y+=15;
	beamdebug=new wxCheckBox(advancedPanel, -1, _("Enable Visual Beam Debug"), wxPoint(320, y));
	beamdebug->SetToolTip(_("Displays node numbers and more info along nodes and beams."));
	y+=15;
	dismap=new wxCheckBox(advancedPanel, -1, _("Disable Overview Map"), wxPoint(320, y));
	dismap->SetToolTip(_("Disabled the map. This is for testing purposes only, you should not gain any FPS with that."));
	y+=15;

	// controls settings panel
	leds = 0;
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	y=10;
	leds=new wxCheckBox(ctsetPanel, -1, _("Enable Logitech G27 LEDs"), wxPoint(115, y));
	leds->SetToolTip(_("Enable support for the Logitech G27 LED tachometer (Windows only)."));
	y+=25;
#endif //WINDOWS

	// Gearbox
	dText = new wxStaticText(ctsetPanel, -1, _("Gearbox mode:"), wxPoint(10,y));
	gearBoxMode=new wxChoice(ctsetPanel, -1, wxPoint(115, y), wxSize(200, -1), 0);
	gearBoxMode->Append(wxT("Automatic shift"));
	gearBoxMode->Append(wxT("Manual shift - Auto clutch"));
	gearBoxMode->Append(wxT("Fully Manual: sequential shift"));
	gearBoxMode->Append(wxT("Fully Manual: stick shift"));
	gearBoxMode->Append(wxT("Fully Manual: stick shift with ranges"));
	gearBoxMode->SetToolTip(_("The default mode for the gearbox when a new vehicle is loaded."));
	y+=25;

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	wxSizer *sizer_updates = new wxBoxSizer(wxVERTICAL);
	helphtmw = new wxHtmlWindow(updatePanel, update_html, wxPoint(0, 0), wxSize(480, 380));
	helphtmw->SetPage(_("... loading ... (maybe you should check your internet connection)"));
	// tooltip is confusing there, better none!
	//helphtmw->SetToolTip(_("here you can get help"));
	sizer_updates->Add(helphtmw, 1, wxGROW);

	// update button replaced with html link
	// update button only for windows users
	//wxButton *btnu = new wxButton(updatePanel, update_ror, _("Update now"));
	//sizer_updates->Add(btnu, 0, wxGROW);

	updatePanel->SetSizer(sizer_updates);
#endif

#ifdef USE_OPENCL
	wxSizer *sizer_gpu = new wxBoxSizer(wxVERTICAL);

	wxSizer *sizer_gpu2 = new wxBoxSizer(wxHORIZONTAL);
	const wxBitmap bm_ocl(opencllogo_xpm);
	wxStaticPicture *openCLImagePanel = new wxStaticPicture(GPUPanel, wxID_ANY, bm_ocl, wxPoint(0, 0), wxSize(200, 200), wxNO_BORDER);
	sizer_gpu2->Add(openCLImagePanel, 0, wxALL|wxALIGN_CENTER_VERTICAL, 3);

	gputext = new wxTextCtrl(GPUPanel, wxID_ANY, _("Please use the buttons below"), wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_MULTILINE);
	sizer_gpu2->Add(gputext, 1, wxGROW, 3);

	sizer_gpu->Add(sizer_gpu2, 1, wxGROW, 3);

	wxButton *btng = new wxButton(GPUPanel, check_opencl, _("Check for OpenCL Support"));
	sizer_gpu->Add(btng, 0, wxGROW, 3);

	wxButton *btnw = new wxButton(GPUPanel, check_opencl_bw, _("Check OpenCL Bandwidth"));
	sizer_gpu->Add(btnw, 0, wxGROW, 3);

	GPUPanel->SetSizer(sizer_gpu);
#endif // USE_OPENCL

	//	controlstimer=new wxTimer(this, CONTROLS_TIMER_ID);
	timer1=new wxTimer(this, UPDATE_RESET_TIMER_ID);

	// inititalize ogre only when we really need it due to long startup times
	ogreRoot = 0;

	wxLogStatus(wxT("Setting default values"));
	SetDefaults();
	wxLogStatus(wxT("Loading config"));
	LoadConfig();

	// centers dialog window on the screen
	Show();
	SetSize(500,600);
	Centre();

	// important: show before we load ogre, since ogre loading can take some time
	loadOgre();
}

void MyDialog::loadOgre()
{
	if(ogreRoot) return;
	wxLogStatus(wxT("Creating Ogre root"));
	//we must do this once
	wxFileName tcfn=wxFileName(app->UserPath, wxEmptyString);
	tcfn.AppendDir(_T("config"));
	wxString confdirPrefix=tcfn.GetPath()+wxFileName::GetPathSeparator();

	wxFileName tlfn=wxFileName(app->UserPath, wxEmptyString);
	tlfn.AppendDir(_T("logs"));
	wxString logsdirPrefix=tlfn.GetPath()+wxFileName::GetPathSeparator();

	wxString progdirPrefix=app->ProgramPath+wxFileName::GetPathSeparator();
	const char *pluginsfile="plugins.cfg";
	wxLogStatus(wxT(">> If it crashes after here, check your plugins.cfg (and remove the DirectX entry if under linux)!"));
	ogreRoot = new Ogre::Root(Ogre::String(progdirPrefix.ToUTF8().data())+pluginsfile,
									Ogre::String(confdirPrefix.ToUTF8().data())+"ogre.cfg",
									Ogre::String(logsdirPrefix.ToUTF8().data())+"RoRConfig.log");

	wxLogStatus(wxT("Root restore config"));
	try
	{
		ogreRoot->restoreConfig();
	} catch (Ogre::Exception& e)
	{
		if(e.getSource() == "D3D9RenderSystem::setConfigOption" && e.getDescription() == "Option named 'Anti aliasing' does not exist.")
		{
			// this is a normal error that happens when the suers switch from ogre 1.6 to 1.7
		} else
		{
			wxString warning = conv(e.getFullDescription());
			wxString caption = _("error upon restoring Ogre Configuration");

			wxMessageDialog *w = new wxMessageDialog(this, warning, caption, wxOK, wxDefaultPosition);
			w->ShowModal();
			delete(w);
		}
	}
	updateRendersystems(ogreRoot->getRenderSystem());
}

void MyDialog::onChangeShadowChoice(wxCommandEvent &e)
{
	// TBD
	bool enable = (shadow->GetSelection() != 0);

	if(enable)
	{
		shadowDistance->Enable();
		shadowOptimizations->Enable();
	} else
	{
		shadowDistance->Disable();
		shadowOptimizations->Disable();
	}
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
	char curGroup[32] = "";
	cTree->DeleteRoot();

	if(cTree->GetColumnCount() < 4)
	{
		// only add them once
		cTree->AddColumn (_("Event"), 180);
		cTree->SetColumnEditable (0, false);
		cTree->AddColumn (_("Arguments"), 320);
		cTree->SetColumnEditable (1, false);
	}
	wxTreeItemId root = cTree->AddRoot(conv("Root"));
	wxTreeItemId *curRoot = 0;

	// setup control tree
	std::map<int, std::vector<event_trigger_t> > controls = INPUTENGINE.getEvents();

	/*
	printf("--------------------------\n");
	for(std::map < int, std::vector<event_trigger_t> >::iterator it= controls.begin(); it!=controls.end();it++)
	{
		printf("event map: %d\n", it->first);
		for(std::vector<event_trigger_t>::iterator it2=it->second.begin(); it2!= it->second.end(); it2++)
		{
			printf(" %d\n", it2->keyCode);
		}
	}
	printf("===========================\n");
	*/

	// init now here, so we get the correct device name
	INPUTENGINE.setup(getOISHandle(this), true, false);

	// clear everything
	controlItemCounter=0;
	std::map<std::string, std::pair<wxTreeItemId, std::map < std::string, wxTreeItemId > > > devices;
	for(std::map<int, std::vector<event_trigger_t> >::iterator it = controls.begin(); it!=controls.end(); it++)
	{
		for(std::vector<event_trigger_t>::iterator it2 = it->second.begin(); it2 != it->second.end(); it2++, controlItemCounter++)
		{
			const event_trigger_t evt = *it2;

			if(evt.eventtype == ET_NONE)
				continue;

			if(strcmp(evt.group, curGroup))
				strcpy(curGroup, evt.group);

			std::string deviceName = INPUTENGINE.getDeviceName(evt);
			if(devices.find(deviceName) == devices.end())
			{
				// device not found, create now
				devices[deviceName] = std::pair<wxTreeItemId, std::map < std::string, wxTreeItemId > >();

				devices[deviceName].first = cTree->AppendItem(root, conv(deviceName));
				cTree->SetItemData(devices[deviceName].first, new IDData(-1));
				cTree->SetItemBold(devices[deviceName].first);
				cTree->SetItemSpan(devices[deviceName].first);
			}
			if(devices[deviceName].second.find(curGroup) == devices[deviceName].second.end())
			{
				// group not found, create now
				devices[deviceName].second[curGroup] = cTree->AppendItem(devices[deviceName].first, conv(curGroup));
				cTree->SetItemData(devices[deviceName].second[curGroup], new IDData(-1));
				cTree->SetItemBold(devices[deviceName].second[curGroup]);
				cTree->SetItemSpan(devices[deviceName].second[curGroup]);
			}

			//strip category name if possible
			int eventID = it->first;
			std::string evn = INPUTENGINE.eventIDToName(eventID);
			wxString evName = conv(evn);
			if(strlen(evt.group)+1 < evName.size())
				evName = conv(evn.substr(strlen(evt.group)+1));
		    wxTreeItemId item = cTree->AppendItem(devices[deviceName].second[curGroup], evName);

			// set some data beside the tree entry
			cTree->SetItemData(item, new IDData(evt.suid));
			updateItemText(item, (event_trigger_t *)&(evt));
			treeItems[evt.suid] = item;
		}
	}
	INPUTENGINE.destroy();
	//cTree->ExpandAll(root);
}

void MyDialog::updateItemText(wxTreeItemId item, event_trigger_t *t)
{
	if(t->eventtype == ET_Keyboard)
	{
		wxString txt = wxString::Format(_T("Keyboard: %s"), t->configline);
		cTree->SetItemText (item, 1, txt);
	} else if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
	{
		wxString txt = wxString::Format(_T("Joystick %d, Axis %d%c %s"), t->joystickNumber, t->joystickAxisNumber, (strlen(t->configline)>0?',':' '), t->configline);
		cTree->SetItemText (item, 1, txt);
	} else if(t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY)
	{
			char type = 'X';
			if(t->eventtype == ET_JoystickSliderY)
				type = 'Y';
		wxString txt = wxString::Format(_T("Joystick %d, %c-Slider %d%c %s"), t->joystickNumber, type, t->joystickSliderNumber, (strlen(t->configline)>0?',':' '), t->configline);
		cTree->SetItemText (item, 1, txt);
	} else if(t->eventtype == ET_JoystickButton)
	{
		//wxString txt = wxString::Format(_T("Joystick %d, Button %d%c %s"), t->joystickNumber, t->joystickButtonNumber, (strlen(t->configline)>0?',':' '), t->configline);
		wxString txt = wxString::Format(_T("Joystick %d, Button %d"), t->joystickNumber, t->joystickButtonNumber);
		cTree->SetItemText (item, 1, txt);
	} else if(t->eventtype == ET_JoystickPov)
	{
		const char *dirStr = "North";
		if     (t->joystickPovDirection == OIS::Pov::Centered)  dirStr = "Centered";
		else if(t->joystickPovDirection == OIS::Pov::North)     dirStr = "North";
		else if(t->joystickPovDirection == OIS::Pov::South)     dirStr = "South";
		else if(t->joystickPovDirection == OIS::Pov::East)      dirStr = "East";
		else if(t->joystickPovDirection == OIS::Pov::West)      dirStr = "West";
		else if(t->joystickPovDirection == OIS::Pov::NorthEast) dirStr = "NorthEast";
		else if(t->joystickPovDirection == OIS::Pov::SouthEast) dirStr = "SouthEast";
		else if(t->joystickPovDirection == OIS::Pov::NorthWest) dirStr = "NorthWest";
		else if(t->joystickPovDirection == OIS::Pov::SouthWest) dirStr = "SouthWest";

		wxString txt = wxString::Format(_T("Joystick %d, POV %d, Direction %s%c %s"), t->joystickNumber, t->joystickPovNumber, dirStr, (strlen(t->configline)>0?',':' '), t->configline);
		cTree->SetItemText (item, 1, txt);
	}
}

void MyDialog::updateRendersystems(Ogre::RenderSystem *rs)
{
	// beware: rs may be null if no config file is present (this is normal)
	std::map<Ogre::String, bool> filterOptions;
	filterOptions["Allow NVPerfHUD"]=true;
	filterOptions["Floating-point mode"]=true;

	if(renderer->GetCount() == 0)
	{
		// add all rendersystems to the list
		const Ogre::RenderSystemList list = ogreRoot->getAvailableRenderers();
		int selection = 0;
		int valuecounter = 0;
		for(Ogre::RenderSystemList::const_iterator it=list.begin(); it!=list.end(); it++, valuecounter++)
		{
			if(rs && rs->getName() == (*it)->getName())
				selection = valuecounter;
			else if(!rs) {
				rs = (*it);
				ogreRoot->setRenderSystem(rs);
			}
			renderer->Append(conv((*it)->getName()));
		}
		renderer->SetSelection(selection);
	}

	int x = 10;
	int y = 50;
	int counter = 0;
	if(!rs)
	{
		wxString warning = _("Unable to load the render systems. Please check if all required files are there and the plugins.cfg file is correct.\nThis is a fatal error and the game will not start.");
		wxString caption = _("Error: no rendersystems found");
		wxMessageDialog *w = new wxMessageDialog(this, warning, caption, wxOK, wxDefaultPosition);
		w->ShowModal();
		delete(w);
		return;
	}
	Ogre::ConfigOptionMap opts = rs->getConfigOptions();
	Ogre::ConfigOptionMap::iterator optIt = opts.begin();
	for(Ogre::ConfigOptionMap::iterator optIt=opts.begin(); optIt!=opts.end(); optIt++)
	{
		// filter out unwanted or disabled options
		if(filterOptions.find(optIt->first) != filterOptions.end())
			continue;
		if(optIt->second.immutable)
			continue;

		if(counter + 1 > (int)renderer_choice.size())
		{
			// not existing, add new control
			renderer_text.resize(counter + 1);
			renderer_choice.resize(counter + 1);

			renderer_text[counter] = new wxStaticText(rsPanel, wxID_ANY, conv("."), wxPoint(x, y+3), wxSize(210, 25));
			renderer_choice[counter] = new wxChoice(rsPanel, RENDERER_OPTION + counter, wxPoint(x + 220, y), wxSize(220, -1), 0);
		} else
		{
			//existing, remove all elements
			renderer_choice[counter]->Clear();
		}
		renderer_text[counter]->SetLabel(conv(optIt->first.c_str()));
		renderer_text[counter]->Show();

		// add all values and select current value
		int selection = 0;
		int valueCounter = 0;
		for(Ogre::StringVector::iterator valIt = optIt->second.possibleValues.begin(); valIt != optIt->second.possibleValues.end(); valIt++, valueCounter++)
		{
			if(*valIt == optIt->second.currentValue)
				selection = valueCounter;
			renderer_choice[counter]->Append(conv(valIt->c_str()));
		}
		renderer_choice[counter]->SetSelection(selection);
		renderer_choice[counter]->Show();
		if(optIt->second.immutable)
			renderer_choice[counter]->Disable();
		else
			renderer_choice[counter]->Enable();

		// layouting stuff
		y += 25;
		/*
		if(y> 25 * 5)
		{
			// use next column
			y = 25;
			x += 230;
		}
		*/
		counter++;
	}
	// hide non-used controls
	if(counter<(int)renderer_text.size())
	{
		for(int i=counter;i<(int)renderer_text.size();i++)
		{
			renderer_text[i]->Hide();
			renderer_choice[i]->Hide();
		}
	}
}

void MyDialog::SetDefaults()
{
	//wxChoice *textfilt;
	textfilt->SetSelection(3); //anisotropic
	//wxChoice *sky;
	sky->SetSelection(0);//sandstorm
	//wxChoice *shadow;
	shadow->SetSelection(0);//no shadows
	shadowOptimizations->SetValue(true);
	shadowDistance->SetValue(50);
	//wxChoice *water;
	water->SetSelection(0);//basic water
	waves->SetValue(false);
	vegetationMode->SetSelection(0); // None

	flaresMode->SetSelection(2); // all trucks
	//languageMode->SetSelection(0);
	enableFog->SetValue(true);

	sightrange->SetValue(30);

	ffEnable->SetValue(false);
	ffOverall->SetValue(100);
	ffHydro->SetValue(100);
	ffCenter->SetValue(50);
	ffCamera->SetValue(0);
	//update textboxes
	wxScrollEvent dummye;
	OnForceFeedbackScroll(dummye);

	screenShotFormat->SetSelection(0);
	gearBoxMode->SetSelection(0);

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
	if(leds) leds->SetValue(false);
	if(enablexfire) enablexfire->SetValue(true);
	autodl->SetValue(true);
	posstor->SetValue(true);
	beamdebug->SetValue(false);
	extcam->SetValue(false);
	//wxCheckBox *dashboard;
	dashboard->SetValue(true);
	//wxCheckBox *mirror;
	mirror->SetValue(false);
	envmap->SetValue(false);
	//wxCheckBox *sunburn;
	sunburn->SetValue(false);
	//wxCheckBox *hdr;
	hdr->SetValue(false);
	glow->SetValue(false);
	//wxCheckBox *mblur;
	mblur->SetValue(false);
	skidmarks->SetValue(false);
	creaksound->SetValue(true);
	//wxChoice *sound;
	sound->SetSelection(1);//default
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
	settings["Texture Filtering"] = conv(textfilt->GetStringSelection());
	settings["Sky effects"] = conv(sky->GetStringSelection());

	settings["Shadow technique"] = conv(shadow->GetStringSelection());
	sprintf(tmp, "%d", shadowDistance->GetValue());
	settings["Shadow distance"] = tmp;
	settings["Shadow optimizations"] = (shadowOptimizations->GetValue()) ? "Yes" : "No";

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
	if(leds) { settings["Logitech LEDs"] = (leds->GetValue()) ? "Yes" : "No"; };
	settings["DebugBeams"] = (beamdebug->GetValue()) ? "Yes" : "No";
	if(enablexfire) { settings["XFire"] = (enablexfire->GetValue()) ? "Yes" : "No"; };
	settings["AutoDownload"] = (autodl->GetValue()) ? "Yes" : "No";
	settings["Position Storage"] = (posstor->GetValue()) ? "Yes" : "No";
	settings["GearboxMode"]= conv(gearBoxMode->GetStringSelection());
	settings["External Camera Mode"] = (extcam->GetValue()) ? "Static" : "Pitching";
	settings["Dashboard"] = (dashboard->GetValue()) ? "Yes" : "No";
	settings["Mirrors"] = (mirror->GetValue()) ? "Yes" : "No";
	settings["Envmap"] = (envmap->GetValue()) ? "Yes" : "No";
	settings["Sunburn"] = (sunburn->GetValue()) ? "Yes" : "No";
	settings["HDR"] = (hdr->GetValue()) ? "Yes" : "No";
	settings["Glow"] = (glow->GetValue()) ? "Yes" : "No";
	settings["Motion blur"] = (mblur->GetValue()) ? "Yes" : "No";
	settings["Skidmarks"] = (skidmarks->GetValue()) ? "Yes" : "No";
	settings["Creak Sound"] = (creaksound->GetValue()) ? "No" : "Yes";
	settings["Enhanced wheels"] = (wheel2->GetValue()) ? "Yes" : "No";
	settings["Fog"] = (enableFog->GetValue()) ? "Yes" : "No";
	settings["Envmap"] = (envmap->GetValue()) ? "Yes" : "No";
	settings["3D Sound renderer"] = conv(sound->GetStringSelection());
	settings["Threads"] = conv(thread->GetStringSelection());

	sprintf(tmp, "%d", sightrange->GetValue());
	settings["FarClip Percent"] = tmp;

	settings["Force Feedback"] = (ffEnable->GetValue()) ? "Yes" : "No";
	sprintf(tmp, "%d", ffOverall->GetValue());
	settings["Force Feedback Gain"] = tmp;
	sprintf(tmp, "%d", ffHydro->GetValue());
	settings["Force Feedback Stress"] = tmp;
	sprintf(tmp, "%d", ffCenter->GetValue());
	settings["Force Feedback Centering"] = tmp;
	sprintf(tmp, "%d", ffCamera->GetValue());
	settings["Force Feedback Camera"] = tmp;

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

	Ogre::String st;
	st = settings["Texture Filtering"]; if (st.length()>0) textfilt->SetStringSelection(conv(st));
	st = settings["Sky effects"]; if (st.length()>0) sky->SetStringSelection(conv(st));
	st = settings["Shadow technique"]; if (st.length()>0) shadow->SetStringSelection(conv(st));
	st = settings["Shadow distance"]; if (st.length()>0) shadowDistance->SetValue((int)atof(st.c_str()));
	st = settings["Shadow optimizations"]; if (st.length()>0) shadowOptimizations->SetValue(st=="Yes");
	st = settings["Water effects"]; if (st.length()>0) water->SetStringSelection(conv(st));
	st = settings["Waves"]; if (st.length()>0) waves->SetValue(st=="Yes");
	st = settings["Engine smoke"]; if (st.length()>0) smoke->SetValue(st=="Yes");
	st = settings["Replay mode"]; if (st.length()>0) replaymode->SetValue(st=="Yes");
	st = settings["Debug Truck Mass"]; if (st.length()>0) dtm->SetValue(st=="Yes");
	st = settings["Debug Collisions"]; if (st.length()>0) dcm->SetValue(st=="Yes");
	st = settings["GearboxMode"]; if (st.length()>0) gearBoxMode->SetStringSelection(conv(st));
	st = settings["Dust"]; if (st.length()>0) dust->SetValue(st=="Yes");
	st = settings["Spray"]; if (st.length()>0) spray->SetValue(st=="Yes");
	st = settings["Custom Particles"]; if (st.length()>0) cpart->SetValue(st=="Yes");
	st = settings["HeatHaze"]; if (st.length()>0) heathaze->SetValue(st=="Yes");
	st = settings["Hydrax"]; if (st.length()>0) hydrax->SetValue(st=="Yes");
	st = settings["disableOverViewMap"]; if (st.length()>0) dismap->SetValue(st=="Yes");
	if(leds) { st = settings["Logitech LEDs"]; if (st.length()>0) leds->SetValue(st=="Yes"); };
	st = settings["External Camera Mode"]; if (st.length()>0) extcam->SetValue(st=="Static");
	st = settings["AutoDownload"]; if (st.length()>0) autodl->SetValue(st=="Yes");
	st = settings["Position Storage"]; if (st.length()>0) posstor->SetValue(st=="Yes");
	st = settings["DebugBeams"]; if (st.length()>0) beamdebug->SetValue(st=="Yes");
	if(enablexfire) { st = settings["XFire"]; if (st.length()>0) enablexfire->SetValue(st=="Yes"); };
	st = settings["Dashboard"]; if (st.length()>0) dashboard->SetValue(st=="Yes");
	st = settings["Mirrors"]; if (st.length()>0) mirror->SetValue(st=="Yes");
	st = settings["Creak Sound"]; if (st.length()>0) creaksound->SetValue(st=="No");
	st = settings["Envmap"]; if (st.length()>0) envmap->SetValue(st=="Yes");
	st = settings["Sunburn"]; if (st.length()>0) sunburn->SetValue(st=="Yes");
	st = settings["HDR"]; if (st.length()>0) hdr->SetValue(st=="Yes");
	st = settings["Glow"]; if (st.length()>0) glow->SetValue(st=="Yes");
	st = settings["Motion blur"]; if (st.length()>0) mblur->SetValue(st=="Yes");
	st = settings["Skidmarks"]; if (st.length()>0) skidmarks->SetValue(st=="Yes");
	st = settings["3D Sound renderer"]; if (st.length()>0) sound->SetStringSelection(conv(st));
	st = settings["Threads"]; if (st.length()>0) thread->SetStringSelection(conv(st));
	st = settings["Enhanced wheels"]; if (st.length()>0) wheel2->SetValue(st=="Yes");
	st = settings["FarClip Percent"]; if (st.length()>0) sightrange->SetValue((int)atof(st.c_str()));
	st = settings["Force Feedback"]; if (st.length()>0) ffEnable->SetValue(st=="Yes");
	st = settings["Force Feedback Gain"]; if (st.length()>0) ffOverall->SetValue((int)atof(st.c_str()));
	st = settings["Force Feedback Stress"]; if (st.length()>0) ffHydro->SetValue((int)atof(st.c_str()));
	st = settings["Force Feedback Centering"]; if (st.length()>0) ffCenter->SetValue((int)atof(st.c_str()));
	st = settings["Force Feedback Camera"]; if (st.length()>0) ffCamera->SetValue((int)atof(st.c_str()));
	//update textboxes
	wxScrollEvent dummye;
	OnForceFeedbackScroll(dummye);
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

	// update slider text
	OnShadowSliderScroll(dummye);
}

bool MyDialog::LoadConfig()
{
	//RoR config
	wxLogStatus(wxT("Loading RoR.cfg"));
	Ogre::ImprovedConfigFile cfg;

	wxString rorcfg=app->UserPath + wxFileName::GetPathSeparator() + _T("config") + wxFileName::GetPathSeparator() + _T("RoR.cfg");

	wxLogStatus(wxT("reading from Config file: ") + rorcfg);

	// Don't trim whitespace
	cfg.load(rorcfg.ToUTF8().data(), "=:\t", false);

	// load all settings into a map!
	Ogre::ConfigFile::SettingsIterator i = cfg.getSettingsIterator();
	Ogre::String svalue, sname;
	while (i.hasMoreElements())
	{
		sname = i.peekNextKey();
		svalue = i.getNext();
		// filter out some things that shouldnt be in there (since we cannot use RoR normally anymore after those)
		if(sname == Ogre::String("Benchmark") || sname == Ogre::String("streamCacheGenerationOnly")|| sname == Ogre::String("regen-cache-only"))
			continue;
		settings[sname] = svalue;
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
	if(!INPUTENGINE.saveMapping(conv(InputMapFileName), getOISHandle(this)))
	{
		wxMessageDialog(this, wxString(_("Could not write to input.map file")), wxString(_("Write error")),wxOK||wxICON_ERROR).ShowModal();
		return;
	}

	//save Ogre stuff if we loaded ogre in the first place
	if(ogreRoot)
	{
		Ogre::RenderSystem *rs = ogreRoot->getRenderSystem();
		for(int i=0;i<(int)renderer_text.size();i++)
		{
			if(!renderer_text[i]->IsShown())
				break;
			rs->setConfigOption(conv(renderer_text[i]->GetLabel()), conv(renderer_choice[i]->GetStringSelection()));
		}
		Ogre::String err = rs->validateConfigOptions();
		if (err.length() > 0)
		{
			wxMessageDialog(this, conv(err), _("Ogre config validation error"),wxOK||wxICON_ERROR).ShowModal();
		} else
		{
			Ogre::Root::getSingleton().saveConfig();
		}
	}

	//save my stuff
	FILE *fd;
	wxString rorcfg=app->UserPath + wxFileName::GetPathSeparator() + _T("config") + wxFileName::GetPathSeparator() + _T("RoR.cfg");

	wxLogStatus(wxT("saving to Config file: ") + rorcfg);
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
	if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel || t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY)
	{
		wxMenu *m = 0;
		if(t->eventtype == ET_JoystickAxisAbs || t->eventtype == ET_JoystickAxisRel)
			m = new wxMenu(_("Joystick Axis Options"));
		else if(t->eventtype == ET_JoystickSliderX || t->eventtype == ET_JoystickSliderY)
			m = new wxMenu(_("Joystick Slider Options"));

		wxMenuItem *i = m->AppendCheckItem(1, _("Reverse"), _("reverse axis"));
		i->Check(t->joystickAxisReverse || t->joystickSliderReverse);

		if(t->eventtype != ET_JoystickSliderX && t->eventtype != ET_JoystickSliderY)
		{
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

			//////////////////
			wxMenu *iLinear = new wxMenu(_("Joystick Axis Linearity"));
			i = iLinear->AppendRadioItem(50, _("+30 %"), _("+30% multiplication"));
			i->Check(t->joystickAxisLinearity > 1.29f && t->joystickAxisLinearity < 1.31f);

			i = iLinear->AppendRadioItem(51, _("+25 %"), _("+25% multiplication"));
			i->Check(t->joystickAxisLinearity > 1.24f && t->joystickAxisLinearity < 1.26f);

			i = iLinear->AppendRadioItem(52, _("+20 %"), _("+20% multiplication"));
			i->Check(t->joystickAxisLinearity > 1.19f && t->joystickAxisLinearity < 1.21f);

			i = iLinear->AppendRadioItem(53, _("+15 %"), _("+15% multiplication"));
			i->Check(t->joystickAxisLinearity > 1.14f && t->joystickAxisLinearity < 1.16f);

			i = iLinear->AppendRadioItem(54, _("+10 %"), _("+10% multiplication"));
			i->Check(t->joystickAxisLinearity > 1.09f && t->joystickAxisLinearity < 1.11f);

			i = iLinear->AppendRadioItem(55, _("+5 %"), _("+5% multiplication"));
			i->Check(t->joystickAxisLinearity > 1.04f && t->joystickAxisLinearity < 1.06f);

			i = iLinear->AppendRadioItem(56, _("0 % (Total Linear)"), _("Total Linear"));
			i->Check(fabs(1.0f - t->joystickAxisLinearity) < 0.01);

			i = iLinear->AppendRadioItem(57, _("-5 %"), _("-5% multiplication"));
			i->Check(t->joystickAxisLinearity > 0.94f && t->joystickAxisLinearity < 0.96f);

			i = iLinear->AppendRadioItem(58, _("-10 %"), _("-10% multiplication"));
			i->Check(t->joystickAxisLinearity > 0.89f && t->joystickAxisLinearity < 0.91f);

			i = iLinear->AppendRadioItem(59, _("-15 %"), _("-15% multiplication"));
			i->Check(t->joystickAxisLinearity > 0.84f && t->joystickAxisLinearity < 0.86f);

			i = iLinear->AppendRadioItem(60, _("-20 %"), _("-20% multiplication"));
			i->Check(t->joystickAxisLinearity > 0.79f && t->joystickAxisLinearity < 0.81f);

			i = iLinear->AppendRadioItem(61, _("-25 %"), _("-25% multiplication"));
			i->Check(t->joystickAxisLinearity > 0.74f && t->joystickAxisLinearity < 0.76f);

			i = iLinear->AppendRadioItem(62, _("-30 %"), _("-30% multiplication"));
			i->Check(t->joystickAxisLinearity > 0.69f && t->joystickAxisLinearity < 0.71f);

			i = m->AppendSubMenu(iLinear, _("Linearity"));
		}

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

void MyDialog::OnChangeRenderer(wxCommandEvent& ev)
{
	if(!ogreRoot) return;
	try
	{
		Ogre::RenderSystem *rs = ogreRoot->getRenderSystemByName(conv(renderer->GetStringSelection()));
		if(rs)
		{
			ogreRoot->setRenderSystem(rs);
			updateRendersystems(rs);
		} else
		{
			wxLogStatus(wxT("Unable to change to new rendersystem(1)"));
		}
	}
	catch(...)
	{
		wxLogStatus(wxT("Unable to change to new rendersystem(2)"));
	}
}

void MyDialog::OnMenuJoystickOptionsClick(wxCommandEvent& ev)
{
	event_trigger_t *t = getSelectedControlEvent();
	if(!t)
		return;
	if(t->eventtype != ET_JoystickAxisAbs && t->eventtype != ET_JoystickAxisRel && t->eventtype != ET_JoystickSliderX && t->eventtype != ET_JoystickSliderY)
		return;
	switch (ev.GetId())
	{
	case 1: //reverse
		{
			t->joystickAxisReverse = ev.IsChecked();
			t->joystickSliderReverse = ev.IsChecked();
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

		// linearity settings below
	case 50: t->joystickAxisLinearity = 1.3f; break;
	case 51: t->joystickAxisLinearity = 1.25f; break;
	case 52: t->joystickAxisLinearity = 1.2f; break;
	case 53: t->joystickAxisLinearity = 1.15f; break;
	case 54: t->joystickAxisLinearity = 1.10f; break;
	case 55: t->joystickAxisLinearity = 1.05f; break;
	case 56: t->joystickAxisLinearity = 1.0f; break;
	case 57: t->joystickAxisLinearity = 0.95f; break;
	case 58: t->joystickAxisLinearity = 0.90f; break;
	case 59: t->joystickAxisLinearity = 0.85f; break;
	case 60: t->joystickAxisLinearity = 0.80f; break;
	case 61: t->joystickAxisLinearity = 0.75f; break;
	case 62: t->joystickAxisLinearity = 0.70f; break;
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
void MyDialog::OnButJoyWizard(wxCommandEvent& event)
{
	size_t handle = getOISHandle(this);
	JoystickWizard wizard(handle, this);
	wizard.RunWizard(wizard.GetFirstPage());
}

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
	std::map<int, std::vector<event_trigger_t> > events = INPUTENGINE.getEvents();
	std::map<int, std::vector<event_trigger_t> >::iterator it;
	std::vector<event_trigger_t>::iterator it2;
	int counter = 0;
	int suid = -1;
	for(it = events.begin(); it!= events.end(); it++)
	{
		for(it2 = it->second.begin(); it2!= it->second.end(); it2++, counter++)
		{
			if(!strcmp(it2->configline, "!NEW!"))
			{
				strcpy(it2->configline, "");
				suid = it2->suid;
			}
		}
	}
	if(suid != -1)
	{
		cTree->EnsureVisible(treeItems[suid]);
		cTree->SelectItem(treeItems[suid]);
		remapControl();
		if(!INPUTENGINE.saveMapping(conv(InputMapFileName)))
		{
			wxMessageDialog(this, wxString(_("Could not write to input.map file")), wxString(_("Write error")),wxOK||wxICON_ERROR).ShowModal();
		}
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
	wxFileDialog *f = new wxFileDialog(this, _("Choose a file"), wxString(), wxString(), conv("*.map"), wxFD_OPEN || wxFD_FILE_MUST_EXIST);
	if(f->ShowModal() == wxID_OK)
	{
		INPUTENGINE.loadMapping(conv(f->GetPath()));
		loadInputControls();
	}
	delete f;
}

void MyDialog::OnButSaveKeymap(wxCommandEvent& event)
{
	INPUTENGINE.setup(getOISHandle(this), true, false);

	int choiceCounter = 3;
	wxString choices[MAX_JOYSTICKS+3];
	choices[0] = wxT("All");
	choices[1] = wxT("Keyboard");
	choices[2] = wxT("Mouse");

	int joyNums = INPUTENGINE.getNumJoysticks();
	if(joyNums>0)
	{
		for(int i=0;i<joyNums;i++)
		{
			choices[i+3] = wxT("Joystick ") + wxString::Format(_T("%d"), i) + wxT(": ") + conv(INPUTENGINE.getJoyVendor(i));
			choiceCounter++;
		}
	}

	int exportType=-10;
	wxSingleChoiceDialog *mcd = new wxSingleChoiceDialog(this, _("Please select what type of device to export"), _("Export selection"),choiceCounter, choices);
	int res = mcd->ShowModal();
	if(res == wxID_OK)
	{
		int sel = mcd->GetSelection();
		if(sel==0) exportType = -10;
		if(sel==1) exportType = -2;
		if(sel==2) exportType = -3;
		if(sel>=3) exportType = sel - 3;

		wxString defaultFile = choices[sel] + wxT(".map");
		defaultFile.Replace(wxT("("), wxT("_"));
		defaultFile.Replace(wxT(")"), wxT("_"));
		defaultFile.Replace(wxT(":"), wxT("_"));
		defaultFile.Replace(wxT(" "), wxT("_"));
		defaultFile.Replace(wxT("__"), wxT("_"));
		defaultFile.Replace(wxT("__"), wxT("_"));
		defaultFile.Replace(wxT("_.map"), wxT(".map"));
		wxFileDialog *f = new wxFileDialog(this, _("Save Mapping to File"), wxString(), defaultFile, conv("*.map"), wxFD_SAVE || wxFD_OVERWRITE_PROMPT);
		if(f->ShowModal() == wxID_OK)
		{
			INPUTENGINE.saveMapping(conv(f->GetPath()), getOISHandle(this), exportType);
		}
		delete f;
	}
	delete mcd;

	INPUTENGINE.destroy();
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
	wxLogStatus(wxT("using RoR: ") + wxString(path));

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

void MyDialog::updateRoR()
{
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

	// get paths to update.exe
	char path[2048];
	getcwd(path, 2048);
	strcat(path, "\\installer.exe");
	wxLogStatus(wxT("using installer: ") + wxString(path));

	int buffSize = (int)strlen(path) + 1;
	LPWSTR wpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, wpath, buffSize);

	getcwd(path, 2048);
	buffSize = (int)strlen(path) + 1;
	LPWSTR cwpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, cwpath, buffSize);

	// now construct struct that has the required starting info
    SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.cbSize = sizeof(SHELLEXECUTEINFOA);
	sei.fMask = 0;
	sei.hwnd = NULL;
	sei.lpVerb = _T("runas");
	sei.lpFile = wpath;
	sei.lpParameters = wxT("--update");
	sei.lpDirectory = cwpath;
	sei.nShow = SW_NORMAL;

	ShellExecuteEx(&sei);

	// we want to exit to be able to update the configurator!
	exit(0);
#else
	wxMessageDialog *w = new wxMessageDialog(this, _("Warning"), _("The update service is currently only available for windows users."), wxOK, wxDefaultPosition);
	w->ShowModal();
	delete(w);
#endif
}

void MyDialog::OnButUpdateRoR(wxCommandEvent& event)
{
	updateRoR();
}

#ifdef USE_OPENCL
#include <oclUtils.h>

#include "ocl_bwtest.h"
#endif // USE_OPENCL

#ifdef USE_OPENCL
// late loading DLLs
// see http://msdn.microsoft.com/en-us/library/8yfshtha.aspx
int tryLoadOpenCL2()
{
#ifdef WIN32
	bool failed = false;
	__try
	{
		failed = FAILED(__HrLoadAllImportsForDll("OpenCL.dll"));
	} __except (EXCEPTION_EXECUTE_HANDLER)
	{
		failed = true;
	}

	if(failed)
		return -1;
	return 1;
#else // WIN32
	// TODO: implement late loading under different OSs
#endif // WIN32
}

void MyDialog::tryLoadOpenCL()
{
	if(openCLAvailable != 0) return;

	openCLAvailable = tryLoadOpenCL2();
	if(openCLAvailable != 1)
	{
		wxMessageDialog *msg=new wxMessageDialog(this, "OpenCL.dll not found\nAre the Display drivers up to date?", "OpenCL.dll not found", wxOK | wxICON_ERROR );
		msg->ShowModal();

		gputext->SetValue("failed to load OpenCL.dll");
	}
}
#endif // USE_OPENCL

void MyDialog::OnButCheckOpenCLBW(wxCommandEvent& event)
{
#ifdef USE_OPENCL
	tryLoadOpenCL();
	if(openCLAvailable != 1) return;
	gputext->SetValue("");
	ostream tstream(gputext);
	OpenCLTestBandwidth bw_test(tstream);
#endif // USE_OPENCL
}

void MyDialog::OnButCheckOpenCL(wxCommandEvent& event)
{
#ifdef USE_OPENCL
	tryLoadOpenCL();
	if(openCLAvailable != 1) return;
	gputext->SetValue("");
	ostream tstream(gputext);
    bool bPassed = true;

    // Get OpenCL platform ID for NVIDIA if avaiable, otherwise default
    tstream << "OpenCL Software Information:" << endl;
    char cBuffer[1024];
    cl_platform_id clSelectedPlatformID = NULL;
    cl_int ciErrNum = oclGetPlatformID (&clSelectedPlatformID);
    if(ciErrNum != CL_SUCCESS)
	{
		tstream << "error getting platform info" << endl;
		bPassed = false;
	}

	if(bPassed)
	{
		// Get OpenCL platform name and version
		ciErrNum = clGetPlatformInfo (clSelectedPlatformID, CL_PLATFORM_NAME, sizeof(cBuffer), cBuffer, NULL);
		if (ciErrNum == CL_SUCCESS)
		{
			tstream << "Platform Name: " << cBuffer << endl;
		}
		else
		{
			tstream << "Platform Name: ERROR " << ciErrNum << endl;
			bPassed = false;
		}

		ciErrNum = clGetPlatformInfo (clSelectedPlatformID, CL_PLATFORM_VERSION, sizeof(cBuffer), cBuffer, NULL);
		if (ciErrNum == CL_SUCCESS)
		{
			tstream << "Platform Version: " << cBuffer << endl;
		}
		else
		{
			tstream << "Platform Version: ERROR " << ciErrNum << endl;
			bPassed = false;
		}
		tstream.flush();

		// Log OpenCL SDK Revision #
		tstream << "OpenCL SDK Revision: " << OCL_SDKREVISION << endl;

		// Get and log OpenCL device info
		cl_uint ciDeviceCount;
		cl_device_id *devices;
		ciErrNum = clGetDeviceIDs (clSelectedPlatformID, CL_DEVICE_TYPE_ALL, 0, NULL, &ciDeviceCount);

		tstream << endl;

		tstream << "OpenCL Hardware Information:" << endl;
		tstream << "Devices found: " << ciDeviceCount << endl;

		// check for 0 devices found or errors...
		if (ciDeviceCount == 0)
		{
			tstream << "No devices found supporting OpenCL (return code " << ciErrNum << ")" << endl;
			bPassed = false;
		}
		else if (ciErrNum != CL_SUCCESS)
		{
			tstream << "Error in clGetDeviceIDs call: " << ciErrNum << endl;
			bPassed = false;
		}
		else
		{
			if ((devices = (cl_device_id*)malloc(sizeof(cl_device_id) * ciDeviceCount)) == NULL)
			{
				tstream << "ERROR: Failed to allocate memory for devices" << endl;
				bPassed = false;
			}
			ciErrNum = clGetDeviceIDs (clSelectedPlatformID, CL_DEVICE_TYPE_ALL, ciDeviceCount, devices, &ciDeviceCount);
			if (ciErrNum == CL_SUCCESS)
			{
				//Create a context for the devices
				cl_context cxGPUContext = clCreateContext(0, ciDeviceCount, devices, NULL, NULL, &ciErrNum);
				if (ciErrNum != CL_SUCCESS)
				{
					tstream << "ERROR in clCreateContext call: " << ciErrNum << endl;
					bPassed = false;
				}
				else
				{
					// show info for each device in the context
					for(unsigned int i = 0; i < ciDeviceCount; ++i )
					{
						clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(cBuffer), &cBuffer, NULL);
						tstream << (i + 1) << " : Device " << cBuffer << endl;
						//oclPrintDevInfo(LOGBOTH, devices[i]);
					}
				}
			}
			else
			{
				tstream << "ERROR in clGetDeviceIDs call: " << ciErrNum << endl;
				bPassed = false;
			}
		}
	}
    // finish
	if(bPassed)
		tstream << "=== PASSED, OpenCL working ===" << endl;
	else
		tstream << "=== FAILED, OpenCL NOT working ===" << endl;
#endif // USE_OPENCL
}

void MyDialog::OnButRegenCache(wxCommandEvent& event)
{
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
	strcat(path, "\\RoR.exe -checkcache");
	wxLogStatus(wxT("executing RoR: ") + wxString(path));

	int buffSize = (int)strlen(path) + 1;
	LPWSTR wpath = new wchar_t[buffSize];
	MultiByteToWideChar(CP_ACP, 0, path, buffSize, wpath, buffSize);

	CreateProcess(NULL, wpath, NULL, NULL, false, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	execl("./RoR.bin -checkcache", "", (char *) 0);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	FSRef ref;
	FSPathMakeRef((const UInt8*)"./RoR.app -checkcache", &ref, NULL);
	LSOpenFSRef(&ref, NULL);
	//execl("./RoR.app/Contents/MacOS/RoR", "", (char *) 0);
#endif
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

void MyDialog::OnShadowSliderScroll(wxScrollEvent &e)
{
	wxString s;
	s.Printf(wxT("%i m"), shadowDistance->GetValue());
	shadowDistanceText->SetLabel(s);
}

void MyDialog::OnSimpleSliderScroll(wxScrollEvent & event)
{
	int val = simpleSlider->GetValue();
	// these are processor simple settings
	// 0 (high perf) - 2 (high quality)
	creaksound->SetValue(true); // disable creak sound
	autodl->SetValue(true);
	replaymode->SetValue(false);
	switch(val)
	{
		case 0:
			beamdebug->SetValue(false);

			dtm->SetValue(false); // debug truck mass
			dcm->SetValue(false); //debug collision meshes
			if(enablexfire) enablexfire->SetValue(false);
			extcam->SetValue(false);
			sound->SetSelection(1);//default
			thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)
			wheel2->SetValue(false);
			posstor->SetValue(false);
			break;
		case 1:
			beamdebug->SetValue(false);
			dtm->SetValue(false); // debug truck mass
			dcm->SetValue(false); //debug collision meshes
			if(enablexfire) enablexfire->SetValue(true);
			extcam->SetValue(false);
			sound->SetSelection(1);//default
			thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)
			wheel2->SetValue(true);
			posstor->SetValue(true);
			break;
		case 2:
			beamdebug->SetValue(false);
			dtm->SetValue(false); // debug truck mass
			dcm->SetValue(false); //debug collision meshes
			if(enablexfire) enablexfire->SetValue(true);
			extcam->SetValue(false);
			sound->SetSelection(1);//default
			thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)
			wheel2->SetValue(true);
			posstor->SetValue(true);
			break;
	};
	getSettingsControls();
}

void MyDialog::OnSimpleSlider2Scroll(wxScrollEvent & event)
{
	int val = simpleSlider2->GetValue();
	// these are graphics simple settings
	// 0 (high perf) - 2 (high quality)
	heathaze->SetValue(false);
	switch(val)
	{
		case 0:
			textfilt->SetSelection(0);
			sky->SetSelection(0);//sandstorm
			shadow->SetSelection(0);//no shadows
			shadowDistance->SetValue(5);
			shadowOptimizations->SetValue(true);
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
			hydrax->SetValue(false);
			dismap->SetValue(false);
			dashboard->SetValue(false);
			mirror->SetValue(false);
			envmap->SetValue(false);
			sunburn->SetValue(false);
			hdr->SetValue(false);
			mblur->SetValue(false);
			skidmarks->SetValue(false);
	break;
		case 1:
			textfilt->SetSelection(2);
			sky->SetSelection(0);//sandstorm
			shadow->SetSelection(1);
			shadowDistance->SetValue(50);
			shadowOptimizations->SetValue(true);
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
			hydrax->SetValue(false);
			dismap->SetValue(false);
			dashboard->SetValue(true);
			mirror->SetValue(true);
			envmap->SetValue(true);
			sunburn->SetValue(false);
			hdr->SetValue(false);
			mblur->SetValue(false);
			skidmarks->SetValue(false);
	break;
		case 2:
			textfilt->SetSelection(3);
			sky->SetSelection(1);
			shadow->SetSelection(1);
			shadowDistance->SetValue(100);
			shadowOptimizations->SetValue(false);
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
			hydrax->SetValue(false);
			dismap->SetValue(false);
			dashboard->SetValue(true);
			mirror->SetValue(true);
			envmap->SetValue(true);
			sunburn->SetValue(false);
			hdr->SetValue(true);
			mblur->SetValue(false);
			skidmarks->SetValue(false);
		break;
	};
	getSettingsControls();

	// update slider text
	wxScrollEvent dummye;
	OnShadowSliderScroll(dummye);
}

void MyDialog::OnForceFeedbackScroll(wxScrollEvent & event)
{
	wxString s;
	int val=ffOverall->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffOverallText->SetLabel(s);

	val=ffHydro->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffHydroText->SetLabel(s);

	val=ffCenter->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffCenterText->SetLabel(s);

	val=ffCamera->GetValue();
	s.Printf(wxT("%i%%"), val);
	ffCameraText->SetLabel(s);
}

void MyDialog::OnLinkClicked(wxHtmlLinkEvent& event)
{
	wxHtmlLinkInfo linkinfo=event.GetLinkInfo();
	wxString href=linkinfo.GetHref();
	wxURI *uri=new wxURI(href);
	if (uri->GetScheme()==conv("rorserver"))
	{
		network_enable->SetValue(true);
		servername->SetValue(uri->GetServer());
		serverport->SetValue(uri->GetPort());
		//serverport->SetValue(uri->GetPassword());
		//serverpassword->SetValue(uri->GetPath().AfterFirst('/'));
		if(uri->GetPassword() != wxString())
		{
			serverpassword->Enable(true);
		}else
			serverpassword->Enable(false);
	} else if (uri->GetScheme()==conv("rorinstaller"))
	{
		if(uri->GetServer() == wxT("update"))
		{
			updateRoR();
		}
	} else
	{
		networkhtmw->OnLinkClicked(linkinfo);
	}
//	wxMessageDialog *res=new wxMessageDialog(this, href, "Success", wxOK | wxICON_INFORMATION );
//	res->ShowModal();
}

void MyDialog::OnLinkClickedUpdate(wxHtmlLinkEvent& event)
{
	wxHtmlLinkInfo linkinfo=event.GetLinkInfo();
	wxString href=linkinfo.GetHref();
	wxURI *uri=new wxURI(href);
	if (uri->GetScheme()==conv("rorinstaller"))
	{
		if(uri->GetServer() == wxT("update"))
		{
			updateRoR();
		}
	} else
	{
		helphtmw->OnLinkClicked(linkinfo);
	}
}

void MyDialog::OnNoteBook2PageChange(wxNotebookEvent& event)
{
	// settings notebook page change
	if(event.GetSelection() == 0)
	{
		// render settings, load ogre!
		//loadOgre();
	}
}

void MyDialog::OnNoteBookPageChange(wxNotebookEvent& event)
{
	if(event.GetSelection() == 4) // updates page
	{
		// try to find our version
		Ogre::String ver = "";
		FILE *f = fopen("version", "r");
		if(f)
		{
			char line[30];
			char *res = fgets(line, 30, f);
			fclose(f);
			if(strnlen(line, 30) > 0)
				ver = Ogre::String(line);
		}

		helphtmw->LoadPage(wxString(conv(NEWS_HTML_PAGE))+
						   wxString(conv("?netversion="))+
						   wxString(conv(RORNET_VERSION))+
						   wxString(conv("&version="))+
						   wxString(conv(ver))+
						   wxString(conv("&lang="))+
						   conv(conv(language->CanonicalName))
						   );
	} else if(event.GetSelection() == 6)
	{
		// this is unpracticable in LAN mode, thus deactivated
		/*
		btnUpdate->Enable(false);
		timer1->Start(10000);
		std::string lshort = conv(language->CanonicalName).substr(0, 2);
		networkhtmw->LoadPage(wxString(wxT(REPO_HTML_SERVERLIST))+
							  wxString(conv("?version="))+
							  wxString(wxT(RORNET_VERSION))+
							  wxString(conv("&lang="))+
							  conv(lshort));
		*/
	}
}

void MyDialog::OnTimerReset(wxTimerEvent& event)
{
	btnUpdate->Enable(true);
}

void MyDialog::OnGetUserToken(wxCommandEvent& event)
{
	wxLaunchDefaultBrowser(wxT("http://usertoken.rigsofrods.com"));
}

void MyDialog::OnTestNet(wxCommandEvent& event)
{
#ifdef NETWORK
	btnUpdate->Enable(false);
	timer1->Start(10000);
	std::string lshort = conv(language->CanonicalName).substr(0, 2);
	networkhtmw->LoadPage(wxString(conv(REPO_HTML_SERVERLIST))+
						  wxString(conv("?version="))+
						  wxString(conv(RORNET_VERSION))+
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

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

//DirectSound Enumeration

//Callback, yay!
static BOOL CALLBACK DSoundEnumDevices(LPGUID guid, LPCSTR desc, LPCSTR drvname, LPVOID data)
{
	wxChoice* wxc=(wxChoice*)data;
	if (guid && desc)
	{
		//sometimes you get weird strings here
		wxString wxdesc=conv(desc);
		//wxLogStatus(wxT("DirectSound Callback with source '")+conv(desc)+conv("'"));
		if (wxdesc.Length()>0) wxc->Append(wxdesc);
	}
    return TRUE;
}

typedef BOOL (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, LPCSTR, LPCSTR, LPVOID);

static HRESULT (WINAPI *pDirectSoundEnumerateA)(LPDSENUMCALLBACKA pDSEnumCallback, LPVOID pContext);

void MyDialog::DSoundEnumerate(wxChoice *wxc)
{
    size_t iter = 1;
    HRESULT hr;

	HMODULE ds_handle;

    ds_handle = LoadLibraryA("dsound.dll");
    if(ds_handle == NULL)
    {
        //AL_PRINT("Failed to load dsound.dll\n");
        return;
    }
//pure uglyness
//what you are seeing here is a function type inside a function type casting inside a define
#define LOAD_FUNC(f) do { \
    p##f = (HRESULT (__stdcall *)(LPDSENUMCALLBACKA,LPVOID))GetProcAddress((HMODULE)ds_handle, #f); \
    if(p##f == NULL) \
    { \
        FreeLibrary(ds_handle); \
        ds_handle = NULL; \
        return; \
    } \
} while(0)

LOAD_FUNC(DirectSoundEnumerateA);
#undef LOAD_FUNC

    hr = pDirectSoundEnumerateA(DSoundEnumDevices, wxc);
//    if(FAILED(hr))
//        AL_PRINT("Error enumerating DirectSound devices (%#x)!\n", (unsigned int)hr);
}
#endif
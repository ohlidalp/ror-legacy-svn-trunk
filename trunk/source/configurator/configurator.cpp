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
#include <wx/statline.h>
#include <wx/hyperlink.h>

#include "utils.h" // RoR utils

#include "wxutils.h"

#include "ImprovedConfigFile.h"


#include "Settings.h"

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
#include "opencllogo.xpm"

// de-comment this to enable network stuff
#define NETWORK

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
	void addAboutEntry(wxString name, wxString desc, wxString url, int &x, int &y);
	void addAboutTitle(wxString name, int &x, int &y);
	void updateRendersystems(Ogre::RenderSystem *rs);
	void SetDefaults();
	bool LoadConfig();
	void SaveConfig();
	void updateSettingsControls(); // use after loading or after manually updating the settings map
	void getSettingsControls(); // puts the control's status into the settings map
	void remapControl();
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
	void onChangeShadowChoice(wxCommandEvent& event);
	void onChangeLanguageChoice(wxCommandEvent& event);
	//void OnButRemap(wxCommandEvent& event);
	void OnChangeRenderer(wxCommandEvent& event);
	//void OnMenuClearClick(wxCommandEvent& event);
	//void OnMenuTestClick(wxCommandEvent& event);
	//void OnMenuDeleteClick(wxCommandEvent& event);
	//void OnMenuDuplicateClick(wxCommandEvent& event);
	//void OnMenuEditEventNameClick(wxCommandEvent& event);
	//void OnMenuCheckDoublesClick(wxCommandEvent& event);
	void OnButRegenCache(wxCommandEvent& event);
	void OnButClearCache(wxCommandEvent& event);
	void OnButUpdateRoR(wxCommandEvent& event);
	void OnButCheckOpenCL(wxCommandEvent& event);
	void OnButCheckOpenCLBW(wxCommandEvent& event);
	void updateRoR();
	void OnsightrangesliderScroll(wxScrollEvent& event);
	void OnForceFeedbackScroll(wxScrollEvent& event);
	void OnNoteBookPageChange(wxNotebookEvent& event);
	void OnNoteBook2PageChange(wxNotebookEvent& event);

	void DSoundEnumerate(wxChoice* wxc);
//	void checkLinuxPaths();
	MyApp *app;
private:
	//bool postinstall;
	Ogre::Root *ogreRoot;
	wxPanel *graphicsPanel;
	wxPanel *gamePanel;
	wxPanel *debugPanel;
	wxPanel *settingsPanel;
	wxPanel *rsPanel;
	wxTextCtrl *gputext;
	wxChoice *renderer;
	std::vector<wxStaticText *> renderer_text;
	std::vector<wxChoice *> renderer_choice;
	wxChoice *textfilt;
	wxChoice *inputgrab;
	wxChoice *sky;
	wxChoice *shadow;
	wxCheckBox *shadowOptimizations;
	wxSlider *sightRange;
	wxStaticText *sightRangeText;
	wxChoice *water;
	wxCheckBox *waves;
	wxCheckBox *replaymode;
	wxCheckBox *dtm;
	wxCheckBox *advanced_logging;
	wxCheckBox *ingame_console;
	wxCheckBox *dofdebug;
	wxCheckBox *nocrashrpt;
	wxCheckBox *debug_vidcam;
	wxCheckBox *debug_envmap;
	wxCheckBox *debug_triggers;
	wxCheckBox *beam_break_debug;
	wxCheckBox *beam_deform_debug;
	wxCheckBox *dcm;
	wxTextCtrl *fovint, *fovext;
	wxTextCtrl *presel_map, *presel_truck;
	wxCheckBox *particles;
	wxCheckBox *heathaze;
	wxCheckBox *hydrax;
	wxCheckBox *rtshader;
	wxCheckBox *dismap;
	wxCheckBox *beamdebug;
	wxCheckBox *autodl;
	wxCheckBox *posstor;
	wxCheckBox *extcam;
	wxCheckBox *mirror;
	wxCheckBox *envmap;
	wxCheckBox *sunburn;
	wxCheckBox *hdr;
	wxCheckBox *glow;
	wxCheckBox *dof;
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
	wxCheckBox *ffEnable;
	wxSlider *ffOverall;
	wxStaticText *ffOverallText;
	wxSlider *ffHydro;
	wxStaticText *ffHydroText;
	wxSlider *ffCenter;
	wxStaticText *ffCenterText;
	wxSlider *ffCamera;
	wxStaticText *ffCameraText;
	wxTimer *timer1;
	wxNotebook *nbook;
	wxScrolledWindow *aboutPanel;
	//wxStaticText *controlText;
	//wxComboBox *ctrlTypeCombo;
	//wxString typeChoices[11];
	//wxButton *btnRemap;
	wxButton *btnUpdate, *btnToken;
	KeySelectDialog *kd;
	int controlItemCounter;
	wxButton *btnAddKey;
	wxButton *btnDeleteKey;
	std::map<int,wxTreeItemId> treeItems;

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
//	wxTextCtrl *deadzone;
	//wxTimer *controlstimer;
	// any class wishing to process wxWidgets events must use this macro
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
	FFSLIDER,
	get_user_token,
	check_opencl,
	check_opencl_bw,
	sightrangeslider,
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
	EVT_CHOICE(shadowschoice, MyDialog::onChangeShadowChoice)
	EVT_COMMAND_SCROLL(sightrangeslider, MyDialog::OnsightrangesliderScroll)
	EVT_COMMAND_SCROLL(FFSLIDER, MyDialog::OnForceFeedbackScroll)
	EVT_CHOICE(EVC_LANG, MyDialog::onChangeLanguageChoice)
	//EVT_BUTTON(BTN_REMAP, MyDialog::OnButRemap)

	EVT_NOTEBOOK_PAGE_CHANGED(mynotebook, MyDialog::OnNoteBookPageChange)
	EVT_NOTEBOOK_PAGE_CHANGED(mynotebook2, MyDialog::OnNoteBook2PageChange)

	EVT_CHOICE(EVT_CHANGE_RENDERER, MyDialog::OnChangeRenderer)

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
		bool res = lang_locale.Init((wxLanguage)language->Language);
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
		lang_locale.Init(wxLANGUAGE_DEFAULT);
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
	UserPath = conv(SSETTING("User Path"));
	ProgramPath = conv(SSETTING("Program Path"));

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
	SETTINGS.setSetting("BuildMode", buildmode?"Yes":"No");
	if(!SETTINGS.setupPaths())
		return false;

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

	kd=0;


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
	nbook=new wxNotebook(this, mynotebook, wxPoint(3, 100), wxSize(490, 415));
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

		wxButton *btnsaveAndExit = new wxButton( this, command_save, _("Save and Exit"), wxPoint(245,520), wxSize(100,25));
		btnsaveAndExit->SetToolTip(_("Save the current configuration and close the configuration program."));
		btnsizer->Add(btnsaveAndExit, 1, wxGROW | wxALL, 5);

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

	gamePanel=new wxPanel(snbook, -1);
	snbook->AddPage(gamePanel, _("Gameplay"), true);

	// Controls
	wxPanel *controlsPanel=new wxPanel(nbook, -1);
	nbook->AddPage(controlsPanel, _("Controls"), false);
	wxSizer *controlsSizer = new wxBoxSizer(wxVERTICAL);
	controlsPanel->SetSizer(controlsSizer);
	// third notebook for control tabs
	wxNotebook *ctbook=new wxNotebook(controlsPanel, mynotebook3, wxPoint(0, 0), wxSize(100,250));
	controlsSizer->Add(ctbook, 1, wxGROW);

	wxPanel *ctsetPanel=new wxPanel(ctbook, -1);
	ctbook->AddPage(ctsetPanel, _("Info"), false);
	wxStaticText *dText2 = new wxStaticText(ctsetPanel, -1, _("Please edit the input mappings by hand by using a texteditor.\nThe input mappings are stored in the following file:\nMy Documents\\Rigs of Rods\\config\\input.map"), wxPoint(10,10));

	wxHyperlinkCtrl *link1 = new wxHyperlinkCtrl(ctsetPanel, -1, _("(more help here)"), "http://www.rigsofrods.com/wiki/pages/Input.map", wxPoint(10, 100));

	wxPanel *ffPanel=new wxPanel(ctbook, -1);
	ctbook->AddPage(ffPanel, _("Force Feedback"), false);

#ifdef NETWORK
	wxPanel *netPanel=new wxPanel(nbook, -1);
	nbook->AddPage(netPanel, _("Network"), false);
#endif
	wxPanel *advancedPanel=new wxPanel(snbook, -1);
	snbook->AddPage(advancedPanel, _("Advanced"), false);

	debugPanel=new wxPanel(snbook, -1);
	snbook->AddPage(debugPanel, _("Debug"), true);


#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	// this is removed for now
	wxPanel *updatePanel=new wxPanel(nbook, -1);
	nbook->AddPage(updatePanel, _("Updates"), false);
#endif
	aboutPanel=new wxScrolledWindow (nbook, -1);
	nbook->AddPage(aboutPanel, "About", false);

#ifdef USE_OPENCL
	wxPanel *GPUPanel=new wxPanel(nbook, -1);
	nbook->AddPage(GPUPanel, _("OpenCL"), false);
#endif // USE_OPENCL

	wxStaticText *dText = 0;

	// clear the renderer settings and fill them later
	dText = new wxStaticText(rsPanel, -1, _("Render System"), wxPoint(10, 28));
	renderer = new wxChoice(rsPanel, EVT_CHANGE_RENDERER, wxPoint(220, 25), wxSize(220, -1), 0);
	// renderer options done, do the rest

	// gamePanel
	int y = 10;
	int x_row1 = 150, x_row2 = 300;

	dText = new wxStaticText(gamePanel, -1, _("Language:"), wxPoint(10, y));
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
		choices.Add(_("English"));
	}
	languageMode=new wxChoice(gamePanel, EVC_LANG, wxPoint(x_row1, y), wxSize(200, -1), choices, wxCB_READONLY);
	languageMode->SetSelection(sel);
	languageMode->SetToolTip(_("This setting overrides the system's default language. You need to restart the configurator to apply the changes."));
	y+=35;

	// Gearbox
	dText = new wxStaticText(gamePanel, -1, _("Default Gearbox mode:"), wxPoint(10,y));
	gearBoxMode=new wxChoice(gamePanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	gearBoxMode->Append(wxT("Automatic shift"));
	gearBoxMode->Append(wxT("Manual shift - Auto clutch"));
	gearBoxMode->Append(wxT("Fully Manual: sequential shift"));
	gearBoxMode->Append(wxT("Fully Manual: stick shift"));
	gearBoxMode->Append(wxT("Fully Manual: stick shift with ranges"));
	gearBoxMode->SetToolTip(_("The default mode for the gearbox when a new vehicle is loaded."));
	y+=25;

	int y1=15;
	wxStaticBox *sb = new wxStaticBox(gamePanel, wxID_ANY, _("Camera Settings"),  wxPoint(10, y), wxSize(450, 150));
	
	dText = new wxStaticText(sb, -1, _("FOV External:"), wxPoint(10,y1+3));
	fovext = new wxTextCtrl(sb, -1, "60", wxPoint(x_row1, y1), wxSize(200, -1), 0);
	y1+=35;

	dText = new wxStaticText(sb, -1, _("FOV Internal:"), wxPoint(10,y1+3));
	fovint = new wxTextCtrl(sb, -1, "75", wxPoint(x_row1, y1), wxSize(200, -1), 0);
	y1+=35;

	extcam=new wxCheckBox(sb, -1, _("Disable Camera Pitching"), wxPoint(x_row1, y1));
	extcam->SetToolTip(_("If you dislike the pitching external vehicle camera, you can disable it here."));
	y1+=25;
	y+=155;

	dText = new wxStaticText(gamePanel, -1, _("User Token: "), wxPoint(10,y+3));
	usertoken=new wxTextCtrl(gamePanel, -1, wxString(), wxPoint(x_row1, y), wxSize(200, -1));
	usertoken->SetToolTip(_("Your rigsofrods.com User Token."));
	btnToken = new wxButton(gamePanel, get_user_token, _("Get Token"), wxPoint(x_row1+210, y), wxSize(90,25));
	y+=35;

	// creak sound?
	creaksound=new wxCheckBox(gamePanel, -1, _("disable creak sound"), wxPoint(x_row1, y));
	creaksound->SetToolTip(_("You can disable the default creak sound by checking this box"));

	// aboutPanel
	y = 10;
	x_row1 = 10;
	x_row2 = 300;

	addAboutTitle(_("Version"), x_row1, y);
	dText = new wxStaticText(aboutPanel, -1, conv(getVersionString()), wxPoint(10, y));
	y+= 120;

	addAboutTitle(_("Authors"), x_row1, y);
	
	// Looong list following
	addAboutEntry("Pierre-Michel-Ricordel (pricorde)", "Physics Genius, Original Author, Core Developer, retired", "http://www.rigsofrods.com/members/18658-pricorde", x_row1, y);
	addAboutEntry("Thomas Fischer (tdev)", "Core Developer", "http://www.rigsofrods.com/members/18656-tdev", x_row1, y);
	y+=20;

	addAboutTitle(_("Code Contributors"), x_row1, y);
	addAboutEntry("Estama", "Physics Core Optimizations, Collision/Friction code, Support Beams", "http://www.rigsofrods.com/members/26673-estama", x_row1, y);
	addAboutEntry("Lifter", "Triggers, Animators, Animated Props, Shocks2", "http://www.rigsofrods.com/members/22371-Lifter", x_row1, y);
	addAboutEntry("Aperion", "Slidesnodes, Axles, Improved Engine code, Rigidifiers, Networking code", "http://www.rigsofrods.com/members/18734-Aperion", x_row1, y);
	addAboutEntry("FlyPiper", "Inertia Code, minor patches", "http://www.rigsofrods.com/members/19789-flypiper", x_row1, y);
	addAboutEntry("knied", "MacOSX Patches", "http://www.rigsofrods.com/members/32234-knied", x_row1, y);
	addAboutEntry("altren", "coded some MyGUI windows", "", x_row1, y);
	addAboutEntry("petern", "repair on spot, linux patches", "", x_row1, y);
	addAboutEntry("imrenagy", "moving chair hardware support, several fixes", "", x_row1, y);
	addAboutEntry("priotr", "several linux fixes", "", x_row1, y);
	addAboutEntry("cptf", "several linux gcc fixes", "", x_row1, y);
	addAboutEntry("88Toyota", "clutch force patches", "", x_row1, y);
	addAboutEntry("synthead", "minor linux fixes", "", x_row1, y);
	y+=20;

	addAboutTitle(_("Core Content Contributors"), x_row1, y);
	addAboutEntry("donoteat", "improved spawner models, terrain work", "http://www.rigsofrods.com/members/18779", x_row1, y);
	addAboutEntry("kevinmce", "character", "http://www.rigsofrods.com/members/18956-kevinmce", x_row1, y);
	y+=20;

	addAboutTitle(_("Mod Contributors"), x_row1, y);
	addAboutEntry("the rigs of rods community", "provided us with lots of mods to play with", "http://www.rigsofrods.com/repository/", x_row1, y);
	y+=20;

	addAboutTitle(_("Testers"), x_row1, y);
	addAboutEntry("invited core team", "the invited members helped us a lot along the way at various corners", "", x_row1, y);
	y+=20;

	addAboutTitle(_("Used Software"), x_row1, y);
	addAboutEntry("Ogre3D", "3D rendering engine", "http://www.ogre3d.org", x_row1, y);
	addAboutEntry("Caelum", "Atmospheric effects", "http://code.google.com/p/caelum/", x_row1, y);
	addAboutEntry("Angelscript", "Scripting Backend", "http://www.angelcode.com/angelscript/", x_row1, y);
	addAboutEntry("LUA", "Scripting Backend", "http://www.lua.org", x_row1, y);
	addAboutEntry("OpenAL Soft", "Sound engine", "http://kcat.strangesoft.net/openal.html", x_row1, y);
	addAboutEntry("MyGUI", "GUI System", "http://www.mygui.info", x_row1, y);
	addAboutEntry("CrashRpt", "Crash Reporting system", "http://code.google.com/p/crashrpt/", x_row1, y);
	addAboutEntry("Hydrax", "Advanced Water System", "http://www.ogre3d.org/addonforums/viewforum.php?f=20", x_row1, y);
	addAboutEntry("mofilereader", "used for Internationalization", "http://code.google.com/p/mofilereader/", x_row1, y);
	addAboutEntry("OIS", "used as Input System", "http://sourceforge.net/projects/wgois/", x_row1, y);
	addAboutEntry("pagedGeometry", "used for foliage (grass, trees, etc)", "http://code.google.com/p/ogre-paged/", x_row1, y);
	addAboutEntry("pthreads", "used for threading", "", x_row1, y);
	addAboutEntry("curl", "used for www-server communication", "http://curl.haxx.se/", x_row1, y);
	addAboutEntry("SocketW", "used as cross-platform socket abstraction", "http://www.digitalfanatics.org/cal/socketw/index.html", x_row1, y);
	addAboutEntry("boost", "used as glue inbetween the components", "http://www.boost.org", x_row1, y);
	addAboutEntry("wxWidgets", "used as cross platform user interface toolkit", "http://www.wxwidgets.org", x_row1, y);
	y+=20;

	addAboutTitle(_("Missing someone?"), x_row1, y);
	addAboutEntry("missing someone?", "if you are missing someone on this list, please drop us a line:\nsupport@rigsofrods.com", "mailto:support@rigsofrods.com", x_row1, y);

	wxSize size = nbook->GetBestVirtualSize();
	size.x = 400;
	aboutPanel->SetVirtualSize(size);
	aboutPanel->SetScrollRate(20, 25);
	
	// complete about panel
	//aboutPanel->FitInside();

	// debugPanel
	y = 10;
	x_row1 = 150;
	x_row2 = 300;

	dText = new wxStaticText(debugPanel, -1, _("These settings are for debugging RoR in various ways.\nIf you dont know how to use, stay away from these."), wxPoint(10, y));
	y+=45;

	ingame_console=new wxCheckBox(debugPanel, -1, _("Ingame Console"), wxPoint(10, y));
	ingame_console->SetToolTip(_("Enables the Scripting Console ingame. Use the ~ key."));
	y+=15;

	dtm=new wxCheckBox(debugPanel, -1, _("Debug Truck Mass"), wxPoint(10, y));
	dtm->SetToolTip(_("Prints all node masses to the RoR.log"));
	y+=15;

	advanced_logging=new wxCheckBox(debugPanel, -1, _("Advanced Logging"), wxPoint(10, y));
	advanced_logging->SetToolTip(_("Enables fancy HTML logs for loaded terrains, objects and trucks"));
	y+=15;

	beam_break_debug=new wxCheckBox(debugPanel, -1, _("Beam Break Debug"), wxPoint(10, y));
	beam_break_debug->SetToolTip(_("Logs the beam info to RoR.log whenever a beam breaks"));
	y+=15;

	beam_deform_debug=new wxCheckBox(debugPanel, -1, _("Beam Deform Debug"), wxPoint(10, y));
	beam_deform_debug->SetToolTip(_("Logs the beam info to RoR.log whenever a beam deforms"));
	y+=15;

	debug_vidcam=new wxCheckBox(debugPanel, -1, _("Debug VideoCameras"), wxPoint(10, y)); //VideoCameraDebug
	debug_vidcam->SetToolTip(_("Adds a virtuals camera mesh that should help you position the camera correctly"));
	y+=15;

	debug_envmap=new wxCheckBox(debugPanel, -1, _("Debug EnvironmentMapping / Chrome"), wxPoint(10, y));
	debug_envmap->SetToolTip(_("Displays an unwrapped cube on what is used to project the reflections on vehicles"));
	y+=15;

	debug_triggers=new wxCheckBox(debugPanel, -1, _("Trigger Debug"), wxPoint(10, y));
	debug_triggers->SetToolTip(_("Enables Trigger debug messages"));
	y+=15;
	
	dcm=new wxCheckBox(debugPanel, -1, _("Debug Collision Meshes"), wxPoint(10, y));
	dcm->SetToolTip(_("Shows all Collision meshes in Red to be able to position them correctly. Only use for debugging!"));
	y+=15;

	beamdebug=new wxCheckBox(debugPanel, -1, _("Enable Visual Beam Debug"), wxPoint(10, y));
	beamdebug->SetToolTip(_("Displays node numbers and more info along nodes and beams."));
	y+=15;

	dofdebug=new wxCheckBox(debugPanel, -1, _("Enable Depth of Field Debug"), wxPoint(10, y));
	dofdebug->SetToolTip(_("Shows the DOF debug display on the screen in order to identify DOF problems"));
	y+=15;

	nocrashrpt=new wxCheckBox(debugPanel, -1, _("Disable Crash Reporting"), wxPoint(10, y));
	nocrashrpt->SetToolTip(_("Disables the crash handling system. Only use for debugging purposes"));
	y+=25;
	
	dText = new wxStaticText(debugPanel, -1, _("Input Grabbing:"), wxPoint(10,y+3));
	inputgrab=new wxChoice(debugPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	inputgrab->Append(conv("All"));
	inputgrab->Append(conv("Dynamically"));
	inputgrab->Append(conv("None"));
	inputgrab->SetToolTip(_("Determines the input capture mode. All is the default, Dynamically is good for windowed mode."));
	inputgrab->SetSelection(0); // All
	y+=35;

    dText = new wxStaticText(debugPanel, -1, _("Preselected Map: "), wxPoint(10, y));
	presel_map=new wxTextCtrl(debugPanel, -1, wxString(), wxPoint(x_row1, y), wxSize(170, -1));
	presel_map->SetToolTip(_("The terrain you want to load upon RoR startup. Might remove the startup selection menu."));
	y+=25;

    dText = new wxStaticText(debugPanel, -1, _("Preselected Truck: "), wxPoint(10,y));
	presel_truck=new wxTextCtrl(debugPanel, -1, wxString(), wxPoint(x_row1, y), wxSize(170, -1));
	presel_truck->SetToolTip(_("The truck you want to load upon RoR startup. Might remove the startup selection menu."));
	y+=25;

	wxHyperlinkCtrl *link = new wxHyperlinkCtrl(debugPanel, -1, _("(read more on how to use these options here)"), "http://www.rigsofrods.com/wiki/pages/Debugging_Trucks", wxPoint(10, y));

	// graphics panel
	y = 10;
	x_row1 = 150;
	x_row2 = 300;
	dText = new wxStaticText(graphicsPanel, -1, _("Texture filtering:"), wxPoint(10,y+3));
	textfilt=new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	textfilt->Append(conv("None (fastest)"));
	textfilt->Append(conv("Bilinear"));
	textfilt->Append(conv("Trilinear"));
	textfilt->Append(conv("Anisotropic (best looking)"));
	textfilt->SetToolTip(_("Most recent hardware can do Anisotropic filtering without significant slowdown.\nUse lower settings only on old or poor video chipsets."));
	textfilt->SetSelection(3); //anisotropic
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Sky type:"), wxPoint(10,y+3));
	sky=new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	sky->Append(conv("Sandstorm (fastest)"));
	sky->Append(conv("Caelum (best looking, slower)"));
	sky->SetToolTip(_("Caelum sky is nice but quite slow unless you have a high-powered GPU."));
	sky->SetSelection(1); //Caelum
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
	sky->SetSelection(1); //Texture shadows
	y+=25;

	dText = new wxStaticText(graphicsPanel, wxID_ANY, _("Sightrange:"), wxPoint(10,y+3));
	sightRange=new wxSlider(graphicsPanel, sightrangeslider, 5000, 1, 5000, wxPoint(x_row1, y), wxSize(200, -1));
	sightRange->SetToolTip(_("determines how far you can see in meters"));
	sightRangeText = new wxStaticText(graphicsPanel, wxID_ANY, _("Unlimited"), wxPoint(x_row1 + 210,y+3));
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
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Vegetation:"), wxPoint(10, y+3));
	vegetationMode=new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	vegetationMode->Append(conv("None (fastest)"));
	vegetationMode->Append(conv("20%"));
	vegetationMode->Append(conv("50%"));
	vegetationMode->Append(conv("Full (best looking, slower)"));
	vegetationMode->SetToolTip(_("This determines how much grass and how many trees (and such objects) should get displayed."));
	vegetationMode->SetSelection(3); // full
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Particle systems:"), wxPoint(10, y));
	particles=new wxCheckBox(graphicsPanel, -1, _("Enable Particle Systems"), wxPoint(x_row1, y));
	particles->SetToolTip(_("This may hurt framerate a bit on old systems, but it looks pretty good."));
	heathaze=new wxCheckBox(graphicsPanel, -1, _("HeatHaze"), wxPoint(x_row2, y));
	heathaze->SetToolTip(_("Heat Haze from engines, major slowdown. (only activate with recent hardware)"));
	heathaze->Disable();
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Cockpit options:"), wxPoint(10, y));
	mirror=new wxCheckBox(graphicsPanel, -1, _("Mirrors"), wxPoint(x_row1, y));
	mirror->SetToolTip(_("Shows the rear view mirrors in 1st person view. May cause compatibility problems for very old video cards."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Visual effects:"), wxPoint(10, y));
	sunburn=new wxCheckBox(graphicsPanel, -1, _("Sunburn"), wxPoint(x_row1, y));
	sunburn->SetToolTip(_("Requires a recent video card. Adds a bluish blinding effect."));
	sunburn->Disable();

	hdr=new wxCheckBox(graphicsPanel, -1, _("HDR"), wxPoint(x_row2, y));
	hdr->SetToolTip(_("Requires a recent video card. Add a lightning effect that simulates the light sensitivity of the human eye."));
	y+=15;

	mblur=new wxCheckBox(graphicsPanel, -1, _("Motion blur"), wxPoint(x_row1, y));
	mblur->SetToolTip(_("Requires a recent video card. Adds a motion blur effect."));
	mblur->Disable();

	skidmarks=new wxCheckBox(graphicsPanel, -1, _("Skidmarks"), wxPoint(x_row2, y));
	skidmarks->SetToolTip(_("Adds tire tracks to the ground."));
	skidmarks->Disable();
	y+=15;

	envmap=new wxCheckBox(graphicsPanel, -1, _("High quality reflective effects"), wxPoint(x_row1, y));
	envmap->SetToolTip(_("Enable high quality reflective effects. Causes a slowdown."));
	y+=15;

	glow=new wxCheckBox(graphicsPanel, -1, _("Glow"), wxPoint(x_row1, y));
	glow->SetToolTip(_("Adds a glow effect to lights"));
	glow->Disable();

	dof=new wxCheckBox(graphicsPanel, -1, _("DOF"), wxPoint(x_row2, y));
	dof->SetToolTip(_("Adds a nice Depth of field effect to the scene."));
	y+=25;

	dText = new wxStaticText(graphicsPanel, -1, _("Screenshot Format:"), wxPoint(10, y));
	screenShotFormat =new wxChoice(graphicsPanel, -1, wxPoint(x_row1, y), wxSize(200, -1), 0);
	screenShotFormat->Append(conv("jpg (smaller, default)"));
	screenShotFormat->Append(conv("png (bigger, no quality loss)"));
	screenShotFormat->SetToolTip(_("In what Format should screenshots be saved?"));

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
	y+=30;

	dText = new wxStaticText(advancedPanel, -1, _("Sound device:"), wxPoint(10,y+3));
	sound=new wxChoice(advancedPanel, -1, wxPoint(x_row1, y), wxSize(280, -1), 0);
	sound->Append(conv("No sound"));
	sound->Append(conv("Default"));
	sound->SetToolTip(_("Select the appropriate sound source.\nLeaving to Default should work most of the time."));
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	//add the rest
	DSoundEnumerate(sound);
#endif
	y+=35;

	dText = new wxStaticText(advancedPanel, -1, _("Thread number:"), wxPoint(10,y+3));
	thread=new wxChoice(advancedPanel, -1, wxPoint(x_row1, y), wxSize(280, -1), 0);
	thread->Append(conv("1 (Standard CPU)"));
	thread->Append(conv("2 (Hyper-Threading or Dual core CPU)"));
	//thread->Append(conv("3 (multi core CPU, one thread per beam)"));
	thread->SetToolTip(_("If you have a Hyper-Threading, or Dual core or multiprocessor computer,\nyou will have a huge gain in speed by choosing the second option.\nBut this mode has some camera shaking issues.\n"));
	y+=35;

	dText = new wxStaticText(advancedPanel, -1, _("Light source effects:"), wxPoint(10, y+3));
	flaresMode=new wxChoice(advancedPanel, -1, wxPoint(x_row1, y), wxSize(280, -1), 0);
//	flaresMode->Append(_("None (fastest)")); //this creates a bug in the autopilot
	flaresMode->Append(conv("No light sources"));
	flaresMode->Append(conv("Only current vehicle, main lights"));
	flaresMode->Append(conv("All vehicles, main lights"));
	flaresMode->Append(conv("All vehicles, all lights"));
	flaresMode->SetToolTip(_("Determines which lights will project light on the environment.\nThe more light sources are used, the slowest it will be."));
	y+=35;

	dText = new wxStaticText(advancedPanel, -1, _("Various Settings:"), wxPoint(10,y+3));
	replaymode=new wxCheckBox(advancedPanel, -1, _("Replay Mode"), wxPoint(x_row1, y));
	replaymode->SetToolTip(_("Replay mode. (Will affect your frame rate)"));
	y+=15;
	posstor=new wxCheckBox(advancedPanel, -1, _("Enable Position Storage"), wxPoint(x_row1, y));
	posstor->SetToolTip(_("Can be used to quick save and load positions of trucks"));
	y+=15;

	autodl=new wxCheckBox(advancedPanel, -1, _("Enable Auto-Downloader"), wxPoint(x_row1, y));
	autodl->SetToolTip(_("This enables the automatic downloading of missing mods when using Multiplayer"));
	autodl->Disable();
	y+=15;

	hydrax=new wxCheckBox(advancedPanel, -1, _("Hydrax Water System"), wxPoint(x_row1, y));
	hydrax->SetToolTip(_("Enables the new water rendering system. EXPERIMENTAL. Overrides any water settings."));
	y+=15;
	rtshader=new wxCheckBox(advancedPanel, -1, _("RT Shader System"), wxPoint(x_row1, y));
	rtshader->SetToolTip(_("Enables the Runtime Shader generation System. EXPERIMENTAL"));
	rtshader->Disable();
	y+=15;
	dismap=new wxCheckBox(advancedPanel, -1, _("Disable Overview Map"), wxPoint(x_row1, y));
	dismap->SetToolTip(_("Disabled the map. This is for testing purposes only, you should not gain any FPS with that."));
	y+=15;

	// controls settings panel
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

	// select default tabs
	// default is settings -> gameplay
	nbook->SetSelection(0);
	snbook->SetSelection(2);

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

void MyDialog::addAboutTitle(wxString name, int &x, int &y)
{
	wxStaticText *dText = new wxStaticText(aboutPanel, wxID_ANY, name, wxPoint(x, y));
	wxFont dfont=dText->GetFont();
	dfont.SetWeight(wxFONTWEIGHT_BOLD);
	dfont.SetPointSize(dfont.GetPointSize()+3);
	dText->SetFont(dfont);
	y += dText->GetSize().y;
}

void MyDialog::addAboutEntry(wxString name, wxString desc, wxString url, int &x, int &y)
{

	wxSize s;
	if(!url.empty())
	{
		wxHyperlinkCtrl *link = new wxHyperlinkCtrl(aboutPanel, wxID_ANY, name, url, wxPoint(x+15, y), wxSize(250, 25), wxHL_ALIGN_LEFT|wxHL_CONTEXTMENU);
		link->SetNormalColour(wxColour("BLACK"));
		link->SetHoverColour(wxColour("LIGHT GREY'"));
		link->SetVisitedColour(wxColour("ORANGE"));
		wxFont dfont=link->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+1);
		link->SetFont(dfont);
		s = link->GetSize();
	} else
	{
		wxStaticText *dText = new wxStaticText(aboutPanel, wxID_ANY, name, wxPoint(x+15, y));
		wxFont dfont=dText->GetFont();
		dfont.SetWeight(wxFONTWEIGHT_BOLD);
		dfont.SetPointSize(dfont.GetPointSize()+1);
		dText->SetFont(dfont);
		s = dText->GetSize();
	}
	y += s.y;
	wxStaticText *dText = new wxStaticText(aboutPanel, wxID_ANY, desc, wxPoint(x+30, y));
	wxFont dfont=dText->GetFont();
	dfont.SetPointSize(dfont.GetPointSize()-1);
	dText->SetFont(dfont);
	y += dText->GetSize().y + 2;

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
		if(e.getSource() == "D3D9RenderSystem::setConfigOption")
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
		shadowOptimizations->Enable();
	} else
	{
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



void MyDialog::updateRendersystems(Ogre::RenderSystem *rs)
{
	// beware: rs may be null if no config file is present (this is normal)
	std::map<Ogre::String, bool> filterOptions;
	filterOptions["Allow NVPerfHUD"]=true;
	filterOptions["Floating-point mode"]=true;
	filterOptions["Resource Creation Policy"]=true;
	filterOptions["VSync Interval"]=true;
	filterOptions["sRGB Gamma Conversion"]=true;
	filterOptions["Colour Depth"]=true;

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
	int y = 55;
	
	wxStaticLine *w = new wxStaticLine(rsPanel, -1, wxPoint(x, y+3), wxSize(440, 2));
	y += 15;

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
		for(Ogre::StringVector::iterator valIt = optIt->second.possibleValues.begin(); valIt != optIt->second.possibleValues.end(); valIt++)
		{
			if(*valIt == optIt->second.currentValue)
				selection = valueCounter;

			// filter video modes
			if(optIt->first == "Video Mode")
			{
				int res_x=-1, res_y=-1, res_d=-1;
				sscanf(valIt->c_str(), "%d x %d @ %d-bit colour", &res_x, &res_y, &res_d);
				
				// discard low resolutions and 16 bit modes
				if(res_d != -1 && res_d < 32)
				{
					wxLogStatus(wxT("discarding resolution as it is below 32 bits: ") + conv(valIt->c_str()));
					continue;
				}
				if(res_x < 800)
				{
					wxLogStatus(wxT("discarding resolution as the x res is below 800 pixels: ") + conv(valIt->c_str()));
					continue;
				}
			}

			valueCounter++;
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
	inputgrab->SetSelection(0); //All
	textfilt->SetSelection(3); //anisotropic
	sky->SetSelection(1);//caelum
	shadow->SetSelection(1);//texture shadows
	shadowOptimizations->SetValue(true);
	sightRange->SetValue(5000); //5k = unlimited
	water->SetSelection(0);//basic water
	waves->SetValue(false); // no waves
	vegetationMode->SetSelection(3); // Full
	particles->SetValue(true);

	flaresMode->SetSelection(2); // all trucks
	//languageMode->SetSelection(0);

	ffEnable->SetValue(true);
	ffOverall->SetValue(100);
	ffHydro->SetValue(100);
	ffCenter->SetValue(50);
	ffCamera->SetValue(0);
	//update textboxes
	wxScrollEvent dummye;
	OnForceFeedbackScroll(dummye);

	screenShotFormat->SetSelection(0);
	gearBoxMode->SetSelection(0);

	replaymode->SetValue(false);
	dtm->SetValue(false);
	presel_map->SetValue(wxString());
	presel_truck->SetValue(wxString());
	fovint->SetValue("75");
	fovext->SetValue("60");
	advanced_logging->SetValue(false);
	ingame_console->SetValue(false);
	dofdebug->SetValue(false);
	nocrashrpt->SetValue(false);
	debug_triggers->SetValue(false);
	debug_vidcam->SetValue(false);
	debug_envmap->SetValue(false);
	beam_break_debug->SetValue(false);
	beam_deform_debug->SetValue(false);
	dcm->SetValue(false);
	//wxCheckBox *dust;
	heathaze->SetValue(false);
	hydrax->SetValue(false);
	rtshader->SetValue(false);
	dismap->SetValue(false);
	autodl->SetValue(false);
	posstor->SetValue(false);
	beamdebug->SetValue(false);
	extcam->SetValue(false);
	mirror->SetValue(true);
	envmap->SetValue(true);
	sunburn->SetValue(false);
	hdr->SetValue(false);
	glow->SetValue(false);
	dof->SetValue(false);
	mblur->SetValue(false);
	skidmarks->SetValue(false);
	creaksound->SetValue(true);
	sound->SetSelection(1);//default
	thread->SetSelection(1);//2 CPUs is now the norm (incl. HyperThreading)

#ifdef NETWORK
	network_enable->SetValue(false);
	nickname->SetValue(_("Anonymous"));
	servername->SetValue(_("127.0.0.1"));
	serverport->SetValue(_("12333"));
	serverpassword->SetValue(wxString());
	usertoken->SetValue(wxString());
#endif

	// update settings
	getSettingsControls();
}

void MyDialog::getSettingsControls()
{
	char tmp[255]="";
	settings["Texture Filtering"] = conv(textfilt->GetStringSelection());
	settings["Input Grab"] = conv(inputgrab->GetStringSelection());

	
	settings["Sky effects"] = conv(sky->GetStringSelection());

	settings["Shadow technique"] = conv(shadow->GetStringSelection());
	sprintf(tmp, "%d", sightRange->GetValue());
	settings["SightRange"] = tmp;
	settings["Shadow optimizations"] = (shadowOptimizations->GetValue()) ? "Yes" : "No";

	settings["Preselected Map"] = presel_map->GetValue();
	settings["Preselected Truck"] = presel_truck->GetValue();

	settings["FOV Internal"] = fovint->GetValue();
	settings["FOV External"] = fovext->GetValue();

	settings["Water effects"] = conv(water->GetStringSelection());
	settings["Waves"] = (waves->GetValue()) ? "Yes" : "No";
	settings["Replay mode"] = (replaymode->GetValue()) ? "Yes" : "No";
	settings["Debug Truck Mass"] = (dtm->GetValue()) ? "Yes" : "No";
	settings["Advanced Logging"] = (advanced_logging->GetValue()) ? "Yes" : "No";
	settings["Enable Ingame Console"] = (ingame_console->GetValue()) ? "Yes" : "No";
	settings["DOFDebug"] = (dofdebug->GetValue()) ? "Yes" : "No";
	settings["NoCrashRpt"] = (nocrashrpt->GetValue()) ? "Yes" : "No";
	settings["Trigger Debug"] = (debug_triggers->GetValue()) ? "Yes" : "No";
	settings["VideoCameraDebug"] = (debug_vidcam->GetValue()) ? "Yes" : "No";
	settings["EnvMapDebug"] = (debug_envmap->GetValue()) ? "Yes" : "No";
	settings["Beam Break Debug"] = (beam_break_debug->GetValue()) ? "Yes" : "No";
	settings["Beam Deform Debug"] = (beam_deform_debug->GetValue()) ? "Yes" : "No";
	settings["Debug Collisions"] = (dcm->GetValue()) ? "Yes" : "No";
	settings["Particles"] = (particles->GetValue()) ? "Yes" : "No";
	settings["HeatHaze"] = "No"; //(heathaze->GetValue()) ? "Yes" : "No";
	settings["Hydrax"] = (hydrax->GetValue()) ? "Yes" : "No";
	//settings["Use RTShader System"] = (rtshader->GetValue()) ? "Yes" : "No";
	settings["disableOverViewMap"] = (dismap->GetValue()) ? "Yes" : "No";
	settings["DebugBeams"] = (beamdebug->GetValue()) ? "Yes" : "No";
	//settings["AutoDownload"] = (autodl->GetValue()) ? "Yes" : "No";
	settings["Position Storage"] = (posstor->GetValue()) ? "Yes" : "No";
	settings["GearboxMode"]= conv(gearBoxMode->GetStringSelection());
	settings["External Camera Mode"] = (extcam->GetValue()) ? "Static" : "Pitching";
	settings["Mirrors"] = (mirror->GetValue()) ? "Yes" : "No";
	settings["Envmap"] = (envmap->GetValue()) ? "Yes" : "No";
	settings["Sunburn"] = "No"; //(sunburn->GetValue()) ? "Yes" : "No";
	settings["HDR"] = (hdr->GetValue()) ? "Yes" : "No";
	settings["Glow"] = "No"; //(glow->GetValue()) ? "Yes" : "No";
	settings["DOF"] = (dof->GetValue()) ? "Yes" : "No";
	settings["Motion blur"] = "No"; //(mblur->GetValue()) ? "Yes" : "No";
	settings["Skidmarks"] = "No"; //(skidmarks->GetValue()) ? "Yes" : "No";
	settings["Creak Sound"] = (creaksound->GetValue()) ? "No" : "Yes";
	settings["Envmap"] = (envmap->GetValue()) ? "Yes" : "No";
	settings["3D Sound renderer"] = conv(sound->GetStringSelection());
	settings["Threads"] = conv(thread->GetStringSelection());

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
	st = settings["Input Grab"]; if (st.length()>0) inputgrab->SetStringSelection(conv(st));
	st = settings["Sky effects"]; if (st.length()>0) sky->SetStringSelection(conv(st));
	st = settings["Shadow technique"]; if (st.length()>0) shadow->SetStringSelection(conv(st));
	st = settings["SightRange"]; if (st.length()>0) sightRange->SetValue((int)atof(st.c_str()));
	st = settings["Shadow optimizations"]; if (st.length()>0) shadowOptimizations->SetValue(st=="Yes");
	st = settings["Water effects"]; if (st.length()>0) water->SetStringSelection(conv(st));
	st = settings["Waves"]; if (st.length()>0) waves->SetValue(st=="Yes");
	st = settings["Replay mode"]; if (st.length()>0) replaymode->SetValue(st=="Yes");
	st = settings["Beam Break Debug"]; if (st.length()>0) beam_break_debug->SetValue(st=="Yes");
	st = settings["Beam Deform Debug"]; if (st.length()>0) beam_deform_debug->SetValue(st=="Yes");
	st = settings["Debug Truck Mass"]; if (st.length()>0) dtm->SetValue(st=="Yes");
	st = settings["Advanced Logging"]; if (st.length()>0) advanced_logging->SetValue(st=="Yes");
	st = settings["Enable Ingame Console"]; if (st.length()>0) ingame_console->SetValue(st=="Yes");
	st = settings["DOFDebug"]; if (st.length()>0) dofdebug->SetValue(st=="Yes");
	st = settings["NoCrashRpt"]; if (st.length()>0) nocrashrpt->SetValue(st=="Yes");
	st = settings["Trigger Debug"]; if (st.length()>0) debug_triggers->SetValue(st=="Yes");
	st = settings["VideoCameraDebug"]; if (st.length()>0) debug_vidcam->SetValue(st=="Yes");
	st = settings["EnvMapDebug"]; if (st.length()>0) debug_envmap->SetValue(st=="Yes");
	st = settings["Debug Collisions"]; if (st.length()>0) dcm->SetValue(st=="Yes");
	st = settings["GearboxMode"]; if (st.length()>0) gearBoxMode->SetStringSelection(conv(st));
	st = settings["Particles"]; if (st.length()>0) particles->SetValue(st=="Yes");
	st = settings["HeatHaze"]; if (st.length()>0) heathaze->SetValue(st=="Yes");
	st = settings["Hydrax"]; if (st.length()>0) hydrax->SetValue(st=="Yes");
	//st = settings["Use RTShader System"]; if (st.length()>0) rtshader->SetValue(st=="Yes");
	st = settings["disableOverViewMap"]; if (st.length()>0) dismap->SetValue(st=="Yes");
	st = settings["External Camera Mode"]; if (st.length()>0) extcam->SetValue(st=="Static");
	//st = settings["AutoDownload"]; if (st.length()>0) autodl->SetValue(st=="Yes");
	st = settings["Position Storage"]; if (st.length()>0) posstor->SetValue(st=="Yes");
	st = settings["DebugBeams"]; if (st.length()>0) beamdebug->SetValue(st=="Yes");
	st = settings["Mirrors"]; if (st.length()>0) mirror->SetValue(st=="Yes");
	st = settings["Creak Sound"]; if (st.length()>0) creaksound->SetValue(st=="No");
	st = settings["Envmap"]; if (st.length()>0) envmap->SetValue(st=="Yes");
	//st = settings["Sunburn"]; if (st.length()>0) sunburn->SetValue(st=="Yes");
	st = settings["HDR"]; if (st.length()>0) hdr->SetValue(st=="Yes");
	//st = settings["Glow"]; if (st.length()>0) glow->SetValue(st=="Yes");
	st = settings["DOF"]; if (st.length()>0) dof->SetValue(st=="Yes");
	//st = settings["Motion blur"]; if (st.length()>0) mblur->SetValue(st=="Yes");
	//st = settings["Skidmarks"]; if (st.length()>0) skidmarks->SetValue(st=="Yes");
	st = settings["3D Sound renderer"]; if (st.length()>0) sound->SetStringSelection(conv(st));
	st = settings["Threads"]; if (st.length()>0) thread->SetStringSelection(conv(st));
	st = settings["Force Feedback"]; if (st.length()>0) ffEnable->SetValue(st=="Yes");
	st = settings["Force Feedback Gain"]; if (st.length()>0) ffOverall->SetValue((int)atof(st.c_str()));
	st = settings["Force Feedback Stress"]; if (st.length()>0) ffHydro->SetValue((int)atof(st.c_str()));
	st = settings["Force Feedback Centering"]; if (st.length()>0) ffCenter->SetValue((int)atof(st.c_str()));
	st = settings["Force Feedback Camera"]; if (st.length()>0) ffCamera->SetValue((int)atof(st.c_str()));

	st = settings["FOV Internal"]; if (st.length()>0) fovint->SetValue(st);
	st = settings["FOV External"]; if (st.length()>0) fovext->SetValue(st);

	st = settings["Preselected Map"]; if (st.length()>0) presel_map->SetValue(st);
	st = settings["Preselected Truck"]; if (st.length()>0) presel_truck->SetValue(st);

	//update textboxes
	wxScrollEvent dummye;
	OnForceFeedbackScroll(dummye);
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
	// update slider text
	OnsightrangesliderScroll(dummye);
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
	if(settings["Colour Depth"] == "") settings["Colour Depth"] = "32";

	// then update the controls!
	updateSettingsControls();
	return false;
}

void MyDialog::SaveConfig()
{
	// first, update the settings map with the actual control values
	getSettingsControls();

	// then set stuff and write configs
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
	strcat(path, "\\updater.exe");
	wxLogStatus(wxT("using updater: ") + wxString(path));

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
	sei.lpParameters = NULL;//wxT("--update");
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

void MyDialog::OnsightrangesliderScroll(wxScrollEvent &e)
{
	wxString s;
	int v = sightRange->GetValue();
	if(v == sightRange->GetMax())
	{
		s = "Unlimited";
	} else
	{
		s.Printf(wxT("%i m"), v);
	}
	sightRangeText->SetLabel(s);
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
	wxString tabname = nbook->GetPageText(event.GetSelection());
	if(tabname == "Updates")
	{
		// try to find our version
		Ogre::String ver = "";
		FILE *f = fopen("version.txt", "r");
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
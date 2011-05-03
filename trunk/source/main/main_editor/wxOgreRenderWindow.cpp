#include "wxOgreRenderWindow.h"
#include "wxOgreRenderWindowListener.h"

#ifdef __WXGTK__
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <wx/gtk/win_gtk.h>
#include <GL/glx.h>
#endif

#include <Ogre.h>
#include "AdvancedOgreFramework.h"

const wxWindowID ID_RENDERTIMER = wxNewId ();

IMPLEMENT_CLASS (wxOgreRenderWindow, wxControl)

BEGIN_EVENT_TABLE (wxOgreRenderWindow, wxControl)
#ifndef __WXMSW__
EVT_PAINT (wxOgreRenderWindow::OnPaint)
#endif
EVT_TIMER (ID_RENDERTIMER, wxOgreRenderWindow::OnRenderTimer)
EVT_SIZE (wxOgreRenderWindow::OnSize)
EVT_MOUSE_EVENTS (wxOgreRenderWindow::OnMouseEvents)
END_EVENT_TABLE ()

Ogre::Root *wxOgreRenderWindow::msOgreRoot = 0;
Ogre::RenderWindow *wxOgreRenderWindow::mRenderWindow = 0;
//------------------------------------------------------------------------------
unsigned int wxOgreRenderWindow::msNextRenderWindowId = 1;
//------------------------------------------------------------------------------
wxOgreRenderWindow::wxOgreRenderWindow (wxWindow *parent, wxWindowID id,
										const wxPoint &pos, const wxSize &size, long style, const wxValidator &validator)
{
											Init ();
											Create (parent, id, pos, size, style, validator);
}
//------------------------------------------------------------------------------
wxOgreRenderWindow::wxOgreRenderWindow ()
{
	Init ();
}
//------------------------------------------------------------------------------
bool wxOgreRenderWindow::Create( wxWindow *parent, wxWindowID id,
								  const wxPoint &pos, const wxSize &size, long style,
									const wxValidator &validator)
{
	 // Error creating the base class
	 if (!wxControl::Create (parent, id, pos, size, style, validator))
		 return false;

	 // Use the told size or let the sizers pick one.
	 SetInitialSize (size);

	 SetBackgroundStyle(wxBG_STYLE_CUSTOM);

	 return true;
}
//------------------------------------------------------------------------------
wxOgreRenderWindow::~wxOgreRenderWindow ()
{
	if (mRenderTimer)
		delete mRenderTimer;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::Init ()
{
	// Callbacks
	mRenderWindowListener = 0;

	mRenderTimer = new wxTimer (this, ID_RENDERTIMER);
	mRenderTimer->Start (10);
}
//------------------------------------------------------------------------------
inline wxSize wxOgreRenderWindow::DoGetBestSize () const
{
	//return wxSize( -1, -1 );
	return wxSize (512, 512);
}
//------------------------------------------------------------------------------
Ogre::Root *wxOgreRenderWindow::GetOgreRoot ()
{
	return msOgreRoot;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::SetOgreRoot (Ogre::Root *root)
{
	msOgreRoot = root;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::SetOgreRenderWindow (Ogre::RenderWindow *rw)
{
	mRenderWindow = rw;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::SetRenderTimerPeriod (int period)
{
	if (!mRenderTimer)
		return;

	if (period <= 0)
		mRenderTimer->Stop ();
	else
		mRenderTimer->Start (period);
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::Update ()
{
	if (msOgreRoot)
	{
		msOgreRoot->renderOneFrame ();
#ifdef __WXGTK__
		//Without this in Linux you'll get a black window, don't know why
		if( mRenderWindow )
			mRenderWindow->update();
#endif
	}
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::setRenderWindowListener(wxOgreRenderWindowListener *listener)
{
	mRenderWindowListener = listener;
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnPaint (wxPaintEvent &evt)
{
	Update ();
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnRenderTimer (wxTimerEvent &evt)
{
	Update ();
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnSize (wxSizeEvent &evt)
{
	if (mRenderWindow)
	{
		// Setting new size;
		int width;
		int height;
		wxSize size = evt.GetSize ();
		width = size.GetWidth ();
		height = size.GetHeight ();

		if(OgreFramework::getSingletonPtr())
			OgreFramework::getSingletonPtr()->resized(Ogre::Vector2(width, height));

		/*
		mRenderWindow->resize (width, height);
		// Letting Ogre know the window has been resized;
		mRenderWindow->windowMovedOrResized ();

		Ogre::Viewport *vp = mRenderWindow->getViewport(0);
		if( vp )
		{
			vp->getCamera()->setAspectRatio( Ogre::Real( vp->getActualWidth() ) /
												Ogre::Real( vp->getActualHeight() ) );
		}
		*/
	}

	//Update ();
}
//------------------------------------------------------------------------------
void wxOgreRenderWindow::OnMouseEvents (wxMouseEvent &evt)
{
	if (mRenderWindowListener)
		mRenderWindowListener->OnMouseEvents( evt );
}
//------------------------------------------------------------------------------
Ogre::String wxOgreRenderWindow::GetOgreHandle ()
{
	return GetOgreHandleForWindow(this);
}

Ogre::String wxOgreRenderWindow::GetOgreHandleForWindow (wxWindow *win)
{
	Ogre::String handle;

#ifdef __WXMSW__
	// Handle for Windows systems
	handle = Ogre::StringConverter::toString((size_t)((HWND)win->GetHandle()));
#elif defined(__WXGTK__)
	// Handle for GTK-based systems

	GtkWidget *widget = (GtkWidget *)win;
	gtk_widget_set_double_buffered (widget, FALSE);
	gtk_widget_realize( widget );

	// Grab the window object
	GdkWindow *gdkWin = GTK_PIZZA (widget)->bin_window;
	Display* display = GDK_WINDOW_XDISPLAY(gdkWin);
	Window wid = GDK_WINDOW_XWINDOW(gdkWin);

	XSync( display, false );		//Added by Dark Sylinc
	XMapWindow( display, wid );		//Added by Dark Sylinc 

	std::stringstream str;

	// Display
	/*Deprecated externalWindowHandle
	str << (unsigned long)display << ':';

	// Screen (returns "display.screen")
	std::string screenStr = DisplayString(display);
	std::string::size_type dotPos = screenStr.find(".");
	screenStr = screenStr.substr(dotPos+1, screenStr.size());
	str << screenStr << ':';

	// XID
	str << wid << ':';

	// Retrieve XVisualInfo
	int attrlist[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, GLX_STENCIL_SIZE, 8, None };
	XVisualInfo* vi = glXChooseVisual(display, DefaultScreen(display), attrlist);
	str << (unsigned long)vi;*/

	//parentWindowHandle just uses the XID
	str << wid;

	handle = str.str();
#else
	// Any other unsupported system
#error Not supported on this platform.
#endif

	return handle;
}

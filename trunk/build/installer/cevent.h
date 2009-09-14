#ifndef CEVENT_H__
#define CEVENT_H__

#include <wx/event.h>

// Could have been DECLARE_EVENT_TYPE( MyFooCommandEvent, -1 )
// Not needed if you only use the event-type in one .cpp file
//extern expdecl const wxEventType MyStatusCommandEvent;
DECLARE_EVENT_TYPE(MyStatusCommandEvent, ID_ANY)
 
// A custom event that transports a whole wxString.
class MyStatusEvent: public wxCommandEvent
{
public:
	MyStatusEvent( wxEventType commandType = MyStatusCommandEvent, int id = 0 )
	:  wxCommandEvent(commandType, id) { }
 
	// You *must* copy here the data to be transported
	MyStatusEvent( const MyStatusEvent &event )
	:  wxCommandEvent(event) { this->SetText( event.GetText() ); }
 
	// Required for sending with wxPostEvent()
	wxEvent* Clone() const { return new MyStatusEvent(*this); }
 
	wxString GetText() const { return m_Text; }
	void SetText( const wxString& text ) { m_Text = text; }
 
private:
	wxString m_Text;
};
 
typedef void (wxEvtHandler::*MyStatusEventFunction)(MyStatusEvent &);
 
// This #define simplifies the one below, and makes the syntax less
// ugly if you want to use Connect() instead of an event table.
#define MyStatusEventHandler(func)                                         \
	(wxObjectEventFunction)(wxEventFunction)(wxCommandEventFunction)\
	wxStaticCastEvent(MyStatusEventFunction, &func)                    
 
// Define the event table entry. Yes, it really *does* end in a comma.
#define EVT_MYSTATUS(id, fn)                                            \
	DECLARE_EVENT_TABLE_ENTRY( MyStatusCommandEvent, id, wxID_ANY,  \
	(wxObjectEventFunction)(wxEventFunction)                     \
	(wxCommandEventFunction) wxStaticCastEvent(                  \
	MyStatusEventFunction, &fn ), (wxObject*) NULL ),
 
// Optionally, you can do a similar #define for EVT_MYFOO_RANGE.
#define EVT_MYSTATUS_RANGE(id1,id2, fn)                                 \
	DECLARE_EVENT_TABLE_ENTRY( MyStatusCommandEvent, id1, id2,      \
	MyStatusEventHandler(fn), (wxObject*) NULL ),
 
// If you want to use the custom event to send more than one sort
// of data, or to more than one place, make it easier by providing
// named IDs in an enumeration.
enum { SE_STARTING, SE_UPDATING, SE_DONE, SE_ERROR };

#endif //CEVENT_H__

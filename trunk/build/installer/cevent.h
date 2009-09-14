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
	MyStatusEvent(wxEventType commandType = MyStatusCommandEvent, int id = 0) :  wxCommandEvent(commandType, id)
	{
	}
 
	// You *must* copy here the data to be transported
	MyStatusEvent(const MyStatusEvent &event) : wxCommandEvent(event)
	{
		this->setText(event.getText());
		this->setProgress(0, event.getProgress(0));
		this->setProgress(1, event.getProgress(1));
	}
 
	// Required for sending with wxPostEvent()
	wxEvent* Clone() const
	{
		return new MyStatusEvent(*this);
	}
 
	// data getter/setter
	wxString getText() const { return m_text; }
	void setText( const wxString& text ) { m_text = text; }
	
	float getProgress(int num) const { assert(num<2); return m_progress[num]; }
	void setProgress(int num, float value) { assert(num<2); m_progress[num] = value; }
 
private:
	// data members
	float m_progress[2];
	wxString m_text;
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

// MSE = my status event ;)
enum { MSE_STARTING, MSE_UPDATING, MSE_DONE, MSE_ERROR };

#endif //CEVENT_H__

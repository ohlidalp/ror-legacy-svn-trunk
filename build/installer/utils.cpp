#include "utils.h"
#include "wxStrel.h"

BEGIN_EVENT_TABLE(myClickBitmap, wxStaticBitmap)
    EVT_LEFT_UP(myClickBitmap::click)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(myClickText, wxStaticText)
    EVT_LEFT_UP(myClickText::click)
END_EVENT_TABLE()

myClickBitmap::myClickBitmap(wxStrel *_s, wxWindow *parent,
                 wxWindowID id,
                 const wxGDIImage& label,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style,
                 const wxString& name) : s(_s), wxStaticBitmap(parent,id,label,pos,size,style,name)
{
}

void myClickBitmap::click(wxMouseEvent &evt)
{
  s->toggle();
}

myClickText::myClickText(wxStrel *_s, wxWindow *parent,
                 wxWindowID id,
                 const wxString& label,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style,
                 const wxString& name) : s(_s), wxStaticText(parent,id,label,pos,size,style,name)
{
}

void myClickText::click(wxMouseEvent &evt)
{
  s->toggle();
}

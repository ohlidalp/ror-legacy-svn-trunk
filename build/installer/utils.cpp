#include "utils.h"
#include "wxStrel.h"

BEGIN_EVENT_TABLE(myClickBitmap, wxStaticBitmap)
    EVT_LEFT_UP(myClickBitmap::click)
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

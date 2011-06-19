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

// written by thomas fischer thomas{AT}thomasfischer{DOT}biz on 18th of June 2011
// to cope the problems of the buggy i18n translations in wxChoice

#ifndef WXVALUECHOICE
#define WXVALUECHOICE

#include <wx/choice.h>
#include <wx/string.h>
#include <map>

//some shortcut
//#define ADD_TRANSLATED_CHOICE(x,y) x->AppendValueItem(wxT(y), _(y));

class wxValueChoice : public wxChoice
{
protected:
	std::map <int, wxString> id_value_map;
	
public:
	wxValueChoice()
	{
		// copied from init()
		m_lastAcceptedSelection = wxID_NONE;
		m_heightOwn = wxDefaultCoord;
	}

	wxValueChoice(wxWindow *parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxChoiceNameStr)
	{
		// copied from init()
		m_lastAcceptedSelection = wxID_NONE;
		m_heightOwn = wxDefaultCoord;
		// create control
		Create(parent, id, pos, size, 0, NULL, style, validator, name);
	}

	void AppendValueItem(wxString valueString, wxString caption)
	{
		int id = this->Append(caption);
		id_value_map[id] = valueString;
	}
	
	void AppendValueItem(wxString captionAndValue)
	{
		int id = this->Append(captionAndValue);
		id_value_map[id] = captionAndValue;
	}

	wxString getSelectedValue()
	{
		int id = this->GetSelection();
		std::map <int, wxString>::iterator it = id_value_map.find(id);
		if(it == id_value_map.end())
		{
			return wxString("error");
		}
		return it->second;
	}

	int setSelectedValue(wxString val)
	{
		if(val.empty()) return -1;
		std::map <int, wxString>::iterator it;
		for(it = id_value_map.begin(); it != id_value_map.end(); it++)
		{
			if(it->second == val)
			{
				this->SetSelection(it->first);
				return it->first;
			}
		}

		this->SetSelection(0);
		return -1;
	}
};

#endif //WXVALUECHOICE

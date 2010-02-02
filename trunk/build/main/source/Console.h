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
#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "Singleton.h"
#include <BaseLayout.h>
#include "rormemory.h"

ATTRIBUTE_CLASS_LAYOUT(Console, "Console.layout");
class Console :
	public wraps::BaseLayout,
	public Singleton2<Console>,
	public MemoryAllocatedObject
{
public:
	Console();
	~Console();

	void setVisible(bool _visible);
	bool getVisible();

	void print(const MyGUI::UString &text);

	//void onKeyPressed(const OIS::KeyEvent &arg);

protected:
	void eventButtonPressed(MyGUI::Widget* _sender, MyGUI::KeyCode _key, MyGUI::Char _char);
	void eventCommandAccept(MyGUI::Edit* _sender);

	ATTRIBUTE_FIELD_WIDGET_NAME(Console, mLogEdit, "Log");
	MyGUI::Edit* mLogEdit;
	ATTRIBUTE_FIELD_WIDGET_NAME(Console, mCommandEdit, "Command");
	MyGUI::Edit* mCommandEdit;

	bool mVisible;

	int mHistoryPosition;
	std::list<MyGUI::UString> lines;
	std::vector<MyGUI::UString> history;
};

#endif // __CONSOLE_H__

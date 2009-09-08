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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of September 2009

#ifndef GUI_FRICTION_H__
#define GUI_FRICTION_H__

#include <MyGUI.h>
#include "OgreSingleton.h"
#include "OgrePrerequisites.h"
#include "Beam.h"

class Collisions;

class GUI_Friction : public Ogre::Singleton< GUI_Friction >
{
public:
	GUI_Friction();
	~GUI_Friction();
	static GUI_Friction& getSingleton(void);
	static GUI_Friction* getSingletonPtr(void);

	bool getVisible();
	void setVisible(bool value);
	
	void setCollisions(Collisions *_col) { col = _col; };
	void setActiveCol(ground_model_t *gm);
protected:
	MyGUI::WindowPtr win;
	Collisions *col;
};

#endif //GUI_FRICTION_H__

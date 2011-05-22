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

#ifndef RORVIEWER_H__
#define RORVIEWER_H__
#include <string>
#include <Ogre.h>

class RigsOfRods;

using namespace std;
using namespace Ogre;

class RoREditor
{
public:
	RoREditor(string meshPath);
	~RoREditor(void);

	bool Initialize(std::string hwndStr, std::string mainhwndStr);
	void Deinitialize(void);
	void Update(void);
	
private:
	bool			initialized;
	RigsOfRods*     app;

	int timeSinceLastFrame;
	int startTime;
};

#endif //RORVIEWER_H__
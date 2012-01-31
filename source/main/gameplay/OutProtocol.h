/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 17th of July 2011

#ifndef OUTPROTCOL_H__
#define OUTPROTCOL_H__

#include "RoRPrerequisites.h"
#include <OgreSingleton.h>

#ifdef USE_SOCKETW
#include "SocketW.h"
#endif //USE_SOCKETW

class OutProtocol : public Ogre::Singleton< OutProtocol >
{
public:
	OutProtocol(void);
	~OutProtocol(void);

	bool update(float dt);
protected:
	int sockfd;
	float delay, timer;
	int mode;
	int id;
	bool working;
	unsigned counter;

	void startup();
};

#endif // OUTPROTCOL_H__

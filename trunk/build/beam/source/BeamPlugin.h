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
#ifndef __BeamPlugin_H__
#define __BeamPlugin_H__

#include "BeamConfig.h"
#include "BeamNode.h"
#include "BeamBeam.h"

#include <pthread.h>
#include <string>
#include <vector>

class BeamPlugin
{
public:
	// IMPORTANT: decide whether to use one plugin per beam or give Beam* upon calc only!
	//
	BeamPlugin(Beam *, BeamPluginManger *mgr);
	~BeamPlugin();

	std::vector<String> getResponsibleKeywords();
	void processLine(Ogre::String line);
};


#endif

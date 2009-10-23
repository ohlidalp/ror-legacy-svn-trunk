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

// ONE PluginManger per Beam instance, thus NO singleton!
class BeamPluginManager 
{
// manager class that manages all existing plugins
public:
	BeamPluginManager(Beam *);

	void addPlugin(BeamPlugin &plug);
	void removePlugin(BeamPlugin &plug);

protected:
	void processPlugins(Beam *, float dt);
};

#endif


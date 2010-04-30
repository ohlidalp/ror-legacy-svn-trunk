/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009,2010 Pierre-Michel Ricordel
Copyright 2007,2008,2009,2010 Thomas Fischer

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

// created on 30th of April 2010 by Thomas Fischer

#ifndef RORPREREQUISITES_H__
#define RORPREREQUISITES_H__

class Airbrake;
class Flexable;
class FlexBody;
class Beam;
class BeamThreadStats;
class FlexMesh;
class FlexObj;
class FlexAirfoil;
class Screwprop;
class Buoyance;
class Collisions;
class DustPool;
class BeamEngine;
class HeightFinder;
class Water;
class Mirrors;
class Turboprop;
class Replay;
class Airfoil;
class Network;
class SlideNode;
class Rail;
class RailGroup;
class PointColDetector;
class Skidmark;
class Autopilot;
class MaterialFunctionMapper;
class CmdKeyInertia;
class SoundScriptInstance;

namespace Ogre
{
	class MovableText;
};

#ifdef USE_LUA
class LuaSystem;
#endif


// some platform fixes
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

// forward typedefs for structs

// little helper macro that should prevent typos and increase readability
#define FWDCLSTRUCT(x) typedef struct x x##_t

FWDCLSTRUCT(node);
FWDCLSTRUCT(beam);
FWDCLSTRUCT(shock);
FWDCLSTRUCT(collcab_rate);
FWDCLSTRUCT(soundsource);
FWDCLSTRUCT(contacter);
FWDCLSTRUCT(rigidifier);
FWDCLSTRUCT(wheel);
FWDCLSTRUCT(vwheel);
FWDCLSTRUCT(ropable);
FWDCLSTRUCT(wing);
FWDCLSTRUCT(command);
FWDCLSTRUCT(rotator);
FWDCLSTRUCT(flare);
FWDCLSTRUCT(prop);
FWDCLSTRUCT(rope);
FWDCLSTRUCT(exhaust);
FWDCLSTRUCT(cparticle);
FWDCLSTRUCT(debugtext);
FWDCLSTRUCT(rig);
FWDCLSTRUCT(collision_box);
FWDCLSTRUCT(tie);
FWDCLSTRUCT(hook);

#endif //RORPREREQUISITES_H__

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
#ifndef __BeamConfig_H__
#define __BeamConfig_H__

#define THREAD_MONO 0
#define THREAD_HT 1
#define THREAD_DUAL 2
#define THREAD_TRI 3
#define THREAD_QUAD 4

#define MAX_NODES 1000
#define MAX_BEAMS 5000
#define MAX_ROTATORS 20
#define MAX_CONTACTERS 2000
#define MAX_HYDROS 64
#define MAX_WHEELS 64
#define MAX_SUBMESHES 500
#define MAX_TEXCOORDS 3000
#define MAX_CABS 3000
#define MAX_SHOCKS MAX_BEAMS
#define MAX_ROPES 64
#define MAX_ROPABLES 64
#define MAX_TIES 64
#define MAX_PROPS 200
#define MAX_COMMANDS 48
#define MAX_CAMERAS 10
#define MAX_RIGIDIFIERS 100

#define MAX_FLEXBODIES 64

#define MAX_AEROENGINES 8

#define MAX_SCREWPROPS 8

#define MAX_SOUNDSCRIPTS_PER_TRUCK 128

#define DEFAULT_RIGIDIFIER_SPRING 1000000.0
#define DEFAULT_RIGIDIFIER_DAMP 50000.0

#define MAX_AIRBRAKES 20

#define DEFAULT_SPRING 9000000.0
//should be 60000
#define DEFAULT_DAMP 12000.0
//#define DEFAULT_DAMP 60000.0
//mars
//#define DEFAULT_GRAVITY -3.8
//earth
#define DEFAULT_GRAVITY -9.8
#define DEFAULT_DRAG 0.05
#define DEFAULT_BEAM_DIAMETER 0.05
#define BEAM_SKELETON_DIAMETER 0.01

#define DEFAULT_WATERDRAG 10.0
//buoyancy force per node in Newton
//#define DEFAULT_BUOYANCY 700.0

// version 1 = pre 0.32
// version 2 = post 0.32
#define TRUCKFILEFORMATVERSION 3

#define IRON_DENSITY 7874.0
#define BEAM_BREAK 1000000.0
#define BEAM_DEFORM 400000.0
#define BEAM_CREAK_DEFAULT  100000.0
#define WHEEL_FRICTION_COEF 2.0
#define CHASSIS_FRICTION_COEF 0.5 //Chassis has 1/4 the friction of wheels.
#define SPEED_STOP 0.2

#define MAX_PRESSURE_BEAMS 4000

#define STAB_RATE 0.1

#define BEAM_NORMAL 0
#define BEAM_HYDRO 1
#define BEAM_VIRTUAL 2
#define BEAM_MARKED 3
#define BEAM_INVISIBLE 4
#define BEAM_INVISIBLE_HYDRO 5

#define NODE_NORMAL 0
#define NODE_LOADED 1

#define MAX_NETFORCE 16

//leading truck
#define ACTIVATED 0
//not leading but active
#define DESACTIVATED 1
//active but wanting to sleep
#define MAYSLEEP 2
//active but ordered to sleep ASAP (synchronously)
#define GOSLEEP 3
//static
#define SLEEPING 4
//network
#define NETWORKED 5
#define RECYCLE 6
#define DELETED 7 // special used when truck pointer is 0
#define TRAFFICED 8

#define MAX_WINGS 40

#define MAX_CPARTICLES 10

#define UNLOCKED 0
#define PRELOCK 1
#define LOCKED 2

#define NOT_DRIVEABLE 0
#define TRUCK 1
#define AIRPLANE 2
#define BOAT 3
#define MACHINE 4

#define DRY 0
#define DRIPPING 1
#define WET 2

#define HYDRO_FLAG_SPEED        0x00000001
#define HYDRO_FLAG_DIR          0x00000002
#define HYDRO_FLAG_AILERON      0x00000004
#define HYDRO_FLAG_RUDDER       0x00000008
#define HYDRO_FLAG_ELEVATOR     0x00000010
#define HYDRO_FLAG_REV_AILERON  0x00000020
#define HYDRO_FLAG_REV_RUDDER   0x00000040
#define HYDRO_FLAG_REV_ELEVATOR 0x00000080


#define SHOCK_FLAG_NORMAL       0x00000001
#define SHOCK_FLAG_INVISIBLE    0x00000002
#define SHOCK_FLAG_LACTIVE      0x00000004
#define SHOCK_FLAG_RACTIVE      0x00000008
#define SHOCK_FLAG_ISSHOCK2     0x00000010
#define SHOCK_FLAG_SOFTBUMP     0x00000020

#endif

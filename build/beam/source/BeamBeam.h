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
#ifndef __BeamBeam_H__
#define __BeamBeam_H__

#include <BeamConfig.h>

typedef struct _beam
{
	node_t *p1;
	node_t *p2;
	Real k; //tensile spring
	Real d; //damping factor
	Real L; //length
	Real refL; //reference length
	Real Lhydro;//hydro reference len
	Real hydroRatio;//hydro rotation ratio
	int hydroFlags;
	int type;
	int broken;
	int bounded;
	Real shortbound;
	Real longbound;
	Real commandRatioLong;
	Real commandRatioShort;
	Real commandShort;
	Real commandLong;
	Real maxtiestress;
	Real stress;
	Real maxposstress;
	Real maxnegstress;
	Real default_deform;
	Real strength;
	Real diameter;
	Vector3 lastforce;
	bool isrope;
	bool iscentering;
	int isOnePressMode;
	bool isforcerestricted;

	int autoMovingMode;
	bool autoMoveLock;
	bool pressedCenterMode;

	float centerLength;

	int disabled;
	float minendmass;
	float update_timer;
	float update_rate;
	float scale;
} beam_t;

#endif

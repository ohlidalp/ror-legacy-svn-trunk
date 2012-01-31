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
#ifndef __Screwprop_H_
#define __Screwprop_H_

#include "RoRPrerequisites.h"

#include "DustPool.h"

class Screwprop
{
private:
	node_t *nodes;
	int nodeback;
	int nodeup;
	float fullpower; //in HP
	Water* water;
	int trucknum;
	DustPool *splashp, *ripplep;

public:
	int noderef;

	bool reverse;
	float throtle;
	float rudder;

	Screwprop(node_t *nd, int nr, int nb, int nu, float power, Water* w, int trucknum);
	void updateForces(int update);
	void setThrottle(float val);
	void setRudder(float val);
	float getThrottle();
	float getRudder();
	void reset();
	void toggleReverse();
};

#endif // __Screwprop_H_

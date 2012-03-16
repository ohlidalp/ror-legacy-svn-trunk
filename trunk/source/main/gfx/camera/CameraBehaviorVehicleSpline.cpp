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
#include "CameraBehaviorVehicleSpline.h"

#include <Ogre.h>
#include "CameraManager.h"
#include "Console.h"
#include "InputEngine.h"
#include "language.h"
#include "Settings.h"

#include "BeamFactory.h"

using namespace Ogre;

CameraBehaviorVehicleSpline::CameraBehaviorVehicleSpline() :
	  splinePos(0)
	, myManualObject(0)
	, myManualObjectNode(0)
	, spline(new SimpleSpline())
{
}

void CameraBehaviorVehicleSpline::activate(cameraContext_t &ctx)
{
	if(!myManualObject)
	{
		myManualObject =  ctx.scm->createManualObject();
		myManualObjectNode = ctx.scm->getRootSceneNode()->createChildSceneNode();

		myManualObject->begin("tracks/transred", Ogre::RenderOperation::OT_LINE_STRIP);
		for(int i=0; i < splineDrawResolution; i++)
			myManualObject->position(0, 0, 0);
		myManualObject->end();

		myManualObjectNode->attachObject(myManualObject);
	}

}

void CameraBehaviorVehicleSpline::updateSplineDisplay()
{
	myManualObject->beginUpdate(0);
	for(int i = 0; i < splineDrawResolution; i++)
	{
		float pos1d = i/(float)splineDrawResolution;
		Vector3 pos3d = spline->interpolate(pos1d);
		//printf(">pos3d> %d >%f,%f,%f\n", i, pos3d.x, pos3d.y, pos3d.z);
		myManualObject->position(pos3d);

	}
	myManualObject->end();
}

void CameraBehaviorVehicleSpline::update(cameraContext_t &ctx)
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if(!curr_truck) return;

	// create a simple spline
	float x = curr_truck->minx + (curr_truck->maxx - curr_truck->minx) * 0.5f;
	float y = curr_truck->miny + (curr_truck->maxy - curr_truck->miny) * 0.5f;

	Vector3 pos1 = Vector3(x, y, curr_truck->minz);
	Vector3 pos2 = pos1+Vector3(10,0,0); //Vector3(x, y, curr_truck->maxz);
	Vector3 pos3 = pos2+Vector3(-10,10,0); //Vector3(x, y, curr_truck->maxz);

	spline->clear();
	spline->addPoint(pos1);
	spline->addPoint(pos2);
	spline->addPoint(pos3);

	updateSplineDisplay();

	// Make all the changes to the camera
	Vector3 dir = curr_truck->nodes[curr_truck->cameranodepos[0]].smoothpos - curr_truck->nodes[curr_truck->cameranodedir[0]].smoothpos;
	dir.normalise();
	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	targetPitch = 0;

	camRatio = 1.0f / (curr_truck->tdt * 4.0f);

	camCenterPoint = curr_truck->getPosition();

	CameraBehaviorOrbit::update(ctx);
}

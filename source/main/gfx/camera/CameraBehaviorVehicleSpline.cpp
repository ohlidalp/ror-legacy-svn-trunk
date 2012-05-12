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

#include "BeamFactory.h"
#include "CameraManager.h"
#include "Console.h"
#include "InputEngine.h"
#include "language.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicleSpline::CameraBehaviorVehicleSpline() :
	  myManualObject(0)
	, myManualObjectNode(0)
	, spline(new SimpleSpline())
	, splinePos(0.5f)
{
}

void CameraBehaviorVehicleSpline::activate(CameraManager::cameraContext_t &ctx)
{
	CameraBehaviorOrbit::activate(ctx);

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
		myManualObject->position(pos3d);

	}
	myManualObject->end();
}

void CameraBehaviorVehicleSpline::update(CameraManager::cameraContext_t &ctx)
{
	Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
	if(!curr_truck) return;

	Vector3 dir = curr_truck->nodes[curr_truck->cameranodepos[0]].smoothpos - curr_truck->nodes[curr_truck->cameranodedir[0]].smoothpos;
	dir.normalise();
	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	targetPitch = 0;
	camRatio = 1.0f / (curr_truck->tdt * 4.0f);

	if(curr_truck->free_camerarail > 0)
	{
		spline->clear();
		for(int i = 0; i < curr_truck->free_camerarail; i++)
		{
			spline->addPoint(curr_truck->nodes[ curr_truck->cameraRail[i] ].AbsPosition);
		}

		updateSplineDisplay();

		camCenterPosition = spline->interpolate(splinePos);
	} else
	{
		// fallback :-/
		camCenterPosition = curr_truck->getPosition();
	}

	CameraBehaviorOrbit::update(ctx);
}

bool CameraBehaviorVehicleSpline::mouseMoved(const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;
	Camera *cam = CameraManager::getSingleton().getCamera();

	if(INPUTENGINE.isKeyDown(OIS::KC_LCONTROL) && ms.buttonDown(OIS::MB_Right))
	{
		splinePos += (float)ms.X.rel * 0.001f;
		if(splinePos < 0) splinePos = 0;
		if(splinePos > 1) splinePos = 1;
		return true;
	}
	else if(ms.buttonDown(OIS::MB_Right))
	{
		camRotX += Degree( (float)ms.X.rel * 0.13f);
		camRotY += Degree(-(float)ms.Y.rel * 0.13f);
		camDist += -(float)ms.Z.rel * 0.02f;
		return true;
	}
	return false;
}

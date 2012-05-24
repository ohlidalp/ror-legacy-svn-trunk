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

#include "Beam.h"
#include "InputEngine.h"
#include "Ogre.h"
#include "Settings.h"

using namespace Ogre;

CameraBehaviorVehicleSpline::CameraBehaviorVehicleSpline() :
	  CameraBehaviorVehicle()
	, myManualObject(0)
	, mySceneNode(0)
	, spline(new SimpleSpline())
	, splinePos(0.5f)
{
}

void CameraBehaviorVehicleSpline::update(const CameraManager::cameraContext_t &ctx)
{
	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[0]].smoothpos
				 - ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[0]].smoothpos).normalisedCopy();

	targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
	targetPitch     = 0.0f;

	camRatio = 1.0f / (ctx.mCurrTruck->tdt * 4.0f);

	if ( ctx.mCurrTruck->free_camerarail > 0 )
	{
		spline->clear();
		for (int i = 0; i < ctx.mCurrTruck->free_camerarail; i++)
		{
			spline->addPoint(ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameraRail[i]].AbsPosition);
		}

		updateSplineDisplay();

		camLookAt = spline->interpolate(splinePos);
	} else
	{
		// fallback :-/
		camLookAt = ctx.mCurrTruck->getPosition();
	}

	CameraBehavior::update(ctx);
}

void CameraBehaviorVehicleSpline::activate(const CameraManager::cameraContext_t &ctx, bool reset /* = true */)
{
	if ( !ctx.mCurrTruck || ctx.mCurrTruck->free_camerarail <= 0 )
	{
		CameraManager::getSingleton().switchToNextBehavior();
		return;
	}

	if ( !myManualObject )
	{
		myManualObject =  ctx.mSceneMgr->createManualObject();
		mySceneNode = ctx.mSceneMgr->getRootSceneNode()->createChildSceneNode();

		myManualObject->begin("tracks/transred", Ogre::RenderOperation::OT_LINE_STRIP);
		for (int i = 0; i < splineDrawResolution; i++)
		{
			myManualObject->position(0, 0, 0);
		}
		myManualObject->end();

		mySceneNode->attachObject(myManualObject);
	}
}

bool CameraBehaviorVehicleSpline::mouseMoved(const CameraManager::cameraContext_t &ctx, const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	if ( INPUTENGINE.isKeyDown(OIS::KC_LCONTROL) && ms.buttonDown(OIS::MB_Right) )
	{
		splinePos += ms.X.rel * 0.001f;
		splinePos  = std::max(0.0f, splinePos);
		splinePos  = std::min(splinePos, 1.0f);
		return true;
	} else
	{
		return CameraBehavior::mouseMoved(ctx, _arg);
	}
}

void CameraBehaviorVehicleSpline::updateSplineDisplay()
{
	myManualObject->beginUpdate(0);
	for (int i = 0; i < splineDrawResolution; i++)
	{
		float pos1d = i / (float)splineDrawResolution;
		Vector3 pos3d = spline->interpolate(pos1d);
		myManualObject->position(pos3d);
	}
	myManualObject->end();
}

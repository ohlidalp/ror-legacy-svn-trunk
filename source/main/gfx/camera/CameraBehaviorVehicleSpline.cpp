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

using namespace Ogre;

CameraBehaviorVehicleSpline::CameraBehaviorVehicleSpline() :
	  CameraBehaviorVehicle()
	, numLinkedBeams(0)
	, spline(new SimpleSpline())
	, splineClosed(false)
	, splineLength(1.0f)
	, splineObject(0)
	, splinePos(0.5f)
{
}

void CameraBehaviorVehicleSpline::update(const CameraManager::cameraContext_t &ctx)
{
	Vector3 dir = (ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodepos[0]].smoothpos
		- ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameranodedir[0]].smoothpos).normalisedCopy();

	targetPitch = 0.0f;

	if ( camPitching )
	{
		targetPitch = -asin(dir.dotProduct(Vector3::UNIT_Y));
	}

	if ( ctx.mCurrTruck->free_camerarail > 0 )
	{
		if ( ctx.mCurrTruck->getAllLinkedBeams().size() != numLinkedBeams )
		{
			createSpline(ctx);
		}
		updateSpline();
		updateSplineDisplay();

		camLookAt = spline->interpolate(splinePos);

		if ( !INPUTENGINE.isKeyDown(OIS::KC_SPACE) )
		{
			Vector3 centerDir = ctx.mCurrTruck->getPosition() - camLookAt;
			if ( centerDir.length() > 1.0f )
			{
				centerDir.normalise();
				targetDirection = -atan2(centerDir.dotProduct(Vector3::UNIT_X), centerDir.dotProduct(-Vector3::UNIT_Z));
			} else
			{
				targetDirection = -atan2(dir.dotProduct(Vector3::UNIT_X), dir.dotProduct(-Vector3::UNIT_Z));
			}
		}
	}

	CameraBehaviorOrbit::update(ctx);
}

bool CameraBehaviorVehicleSpline::mouseMoved(const CameraManager::cameraContext_t &ctx, const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	if ( INPUTENGINE.isKeyDown(OIS::KC_LCONTROL) && ms.buttonDown(OIS::MB_Right) )
	{
		if ( INPUTENGINE.isKeyDown(OIS::KC_LMENU) )
		{
			splinePos += ms.X.rel * std::max(0.0001f, splineLength * 0.0000001f);
		} else
		{
			splinePos += ms.X.rel * std::max(0.00005f, splineLength * 0.0000005f);
		}
		
		if ( ms.X.rel > 0 && splinePos > 0.99f )
		{
			if ( splineClosed )
			{
				splinePos -= 1.0f;
			} else
			{
				// u - turn
			}
		} else if ( ms.X.rel < 0 && splinePos < 0.01f )
		{
			if ( splineClosed )
			{
				splinePos += 1.0f;
			} else
			{
				// u - turn
			}
		}

		splinePos  = std::max(0.0f, splinePos);
		splinePos  = std::min(splinePos, 1.0f);

		camRatio = 0.0f;

		return true;
	} else
	{
		camRatio = 5.0f;

		return CameraBehaviorOrbit::mouseMoved(ctx, _arg);
	}
}

void CameraBehaviorVehicleSpline::activate(const CameraManager::cameraContext_t &ctx, bool reset /* = true */)
{
	if ( !ctx.mCurrTruck || ctx.mCurrTruck->free_camerarail <= 0 )
	{
		CameraManager::getSingleton().switchToNextBehavior();
		return;
	} else if ( reset )
	{
		this->reset(ctx);
		createSpline(ctx);
	}
}

void CameraBehaviorVehicleSpline::reset(const CameraManager::cameraContext_t &ctx)
{
	CameraBehaviorOrbit::reset(ctx);

	camDist = std::min(ctx.mCurrTruck->getMinimalCameraRadius() * 2.0f, 20.0f);
	
	splinePos = 0.5f;
}

void CameraBehaviorVehicleSpline::createSpline(const CameraManager::cameraContext_t &ctx)
{
	splineClosed = false;
	splineLength = 1.0f;

	spline->clear();
	splineNodes.clear();

	for (int i = 0; i < ctx.mCurrTruck->free_camerarail; i++)
	{
		spline->addPoint(ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameraRail[i]].AbsPosition);
		splineNodes.push_back(&ctx.mCurrTruck->nodes[ctx.mCurrTruck->cameraRail[i]]);
	}

	std::list<Beam*> linkedBeams = ctx.mCurrTruck->getAllLinkedBeams();

	numLinkedBeams = linkedBeams.size();

	if ( numLinkedBeams > 0 )
	{
		for (std::list<Beam*>::iterator it = linkedBeams.begin(); it != linkedBeams.end(); ++it)
		{
			if ( (*it)->free_camerarail <= 0 ) break;

			Vector3 lastPoint = spline->getPoint(spline->getNumPoints() - 1);
			Vector3 firstPoint = (*it)->nodes[(*it)->cameraRail[0]].AbsPosition;
			Vector3 secondPoint = (*it)->nodes[(*it)->cameraRail[1]].AbsPosition;

			if ( firstPoint.distance(lastPoint) > 5.0f ) break;

			for (int i = 1; i < (*it)->free_camerarail; i++)
			{
				spline->addPoint((*it)->nodes[(*it)->cameraRail[i]].AbsPosition);
				splineNodes.push_back(&(*it)->nodes[(*it)->cameraRail[i]]);
			}
		}

		Vector3 firstPoint = spline->getPoint(0);
		Vector3 lastPoint  = spline->getPoint(spline->getNumPoints() - 1);

		if ( firstPoint.distance(lastPoint) < 1.0f )
		{
			splineClosed = true;
		}
	}

	for (int i = 1; i < spline->getNumPoints(); i++)
	{
		splineLength += spline->getPoint(i - 1).distance(spline->getPoint(i));
	}

	splineLength /= 2.0f;

	if ( !splineObject )
	{
		splineObject = gEnv->sceneManager->createManualObject();
		SceneNode* splineNode = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();

		splineObject->begin("tracks/transred", Ogre::RenderOperation::OT_LINE_STRIP);
		for (int i = 0; i < splineDrawResolution; i++)
		{
			splineObject->position(0, 0, 0);
		}
		splineObject->end();

		splineNode->attachObject(splineObject);
	}
}

void CameraBehaviorVehicleSpline::updateSpline()
{
	for (int i = 0; i < spline->getNumPoints(); i++)
	{
		spline->updatePoint(i, splineNodes[i]->AbsPosition);
	}
}

void CameraBehaviorVehicleSpline::updateSplineDisplay()
{
	splineObject->beginUpdate(0);
	for (int i = 0; i < splineDrawResolution; i++)
	{
		Real parametricDist = i / (float)splineDrawResolution;
		Vector3 position = spline->interpolate(parametricDist);
		splineObject->position(position);
	}
	splineObject->end();
}

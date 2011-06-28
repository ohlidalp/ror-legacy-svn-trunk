/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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
#include "PreviewRenderer.h"
#include "Ogre.h"
#include "Settings.h"

#include "RoRFrameListener.h"
#include "AdvancedOgreFramework.h"

#include "Beam.h"

PreviewRenderer::PreviewRenderer(Beam *b) : truck(b)
{
	fn = SSETTING("vehicleOutputFile");
	if(fn.empty()) return;

	go();
}

PreviewRenderer::~PreviewRenderer()
{
}

void PreviewRenderer::go()
{
	Ogre::SceneManager *sceneMgr = RoRFrameListener::eflsingleton->getSceneMgr();
	Ogre::Viewport *vp = RoRFrameListener::eflsingleton->getRenderWindow()->getViewport(0);

	// disable skybox
	sceneMgr->setSkyBox(false, "");
	// disable fog
	sceneMgr->setFog(FOG_NONE);
	// disable shadows
	vp->setShadowsEnabled(false);
	// white background
	vp->setBackgroundColour(Ogre::ColourValue::White);
	vp->setClearEveryFrame(true);

	// better mipmapping
	MaterialManager::getSingleton().setDefaultAnisotropy(8);
	MaterialManager::getSingleton().setDefaultTextureFiltering(TFO_ANISOTROPIC);

	// run the beam engine one time
	int tsteps = 100; // 100 steps
	Real dtperstep = 0.01 / (float)tsteps; // at 100 FPS
	for (int i=0; i<tsteps; i++)
	{
		truck->calcForcesEuler(i==tsteps, dtperstep, i, tsteps);
	}

	// calculate min camera radius for truck and the trucks center

	// first: average pos
	truck->updateTruckPosition();

	// then camera radius:
	float minCameraRadius = 0;
	for (int i=0; i < truck->free_node; i++)
	{
		Real dist = truck->nodes[i].AbsPosition.distance(truck->position);
		if(dist > minCameraRadius)
			minCameraRadius = dist;
	}
	minCameraRadius *= 2.1f;

	// position the camera in a good way
	//const Real objDist = minCameraRadius * 100;
	//const Real nearDist = objDist - (minCameraRadius + 1); 
	//const Real farDist = objDist + (minCameraRadius + 1);

	Camera *cam = RoRFrameListener::eflsingleton->getCamera();
	//cam->setAspectRatio(1.0f);
	//cam->setFOVy(Math::ATan((minCameraRadius*2) / minCameraRadius));
	cam->setNearClipDistance(0.1);
	cam->setFarClipDistance(minCameraRadius * 10);
	cam->setFOVy(Ogre::Radian(Ogre::Degree(45.0f)));
	cam->setProjectionType(Ogre::PT_ORTHOGRAPHIC);
	cam->setOrthoWindow(minCameraRadius, minCameraRadius);

	sceneMgr->clearSpecialCaseRenderQueues();
	sceneMgr->addSpecialCaseRenderQueue(Ogre::RENDER_QUEUE_MAIN);  
	sceneMgr->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_INCLUDE);


	for(int o=0;o<2;o++)
	{
		String oext = "ortho.";
		if(o == 1)
		{
			oext = "3d.";
			cam->setProjectionType(Ogre::PT_PERSPECTIVE);
			minCameraRadius *= 1.3f;
		}

		for(int i=0;i<2;i++)
		{
			String ext = "normal.";
			if(i == 0)
			{
				truck->hideSkeleton(true);
			} else if(i == 1)
			{
				ext = "skeleton.";
				// now show the skeleton
				truck->showSkeleton(true, true);
				truck->updateSimpleSkeleton();
			}
			ext = oext + ext;

			// right
			cam->setPosition(Vector3(truck->position.x, truck->position.y, truck->position.z - minCameraRadius));
			cam->lookAt(truck->position);
			render(ext + "right");

			// left
			cam->setPosition(Vector3(truck->position.x, truck->position.y, truck->position.z + minCameraRadius));
			cam->lookAt(truck->position);
			render(ext + "left");

			// front
			cam->setPosition(Vector3(truck->position.x - minCameraRadius, truck->position.y, truck->position.z));
			cam->lookAt(truck->position);
			render(ext + "front");

			// back
			cam->setPosition(Vector3(truck->position.x + minCameraRadius, truck->position.y, truck->position.z));
			cam->lookAt(truck->position);
			render(ext + "back");

			// top
			cam->setPosition(Vector3(truck->position.x, truck->position.y + minCameraRadius, truck->position.z + 0.01f));
			cam->lookAt(truck->position);
			render(ext + "top");

			// bottom
			cam->setPosition(Vector3(truck->position.x, truck->position.y - minCameraRadius, truck->position.z + 0.01f));
			cam->lookAt(truck->position);
			render(ext + "bottom");

		}
	}
}


void PreviewRenderer::render(Ogre::String ext)
{
	// create some screenshot
	RoRWindowEventUtilities::messagePump();
	Ogre::Root::getSingleton().renderOneFrame();
	OgreFramework::getSingletonPtr()->m_pRenderWnd->update();
	OgreFramework::getSingletonPtr()->m_pRenderWnd->writeContentsToFile(fn+"."+ext+".jpg");
}
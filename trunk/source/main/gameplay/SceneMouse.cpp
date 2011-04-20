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
#include "SceneMouse.h"
#include "RoRFrameListener.h"
#include "Beam.h"
#include "BeamFactory.h"
#include <Ogre.h>

#ifdef USE_MYGUI
# include <MyGUI.h>
#endif //USE_MYGUI

using namespace Ogre;

template<> SceneMouse *Singleton < SceneMouse >::ms_Singleton = 0;

SceneMouse::SceneMouse(Ogre::SceneManager *scm, RoRFrameListener *rfl) : scm(scm), rfl(rfl)
{
	mouseGrabForce = 100000.0f;
	mouseGrabState = 0;
	
	// load 3d line for mouse picking
	pickLine =  scm->createManualObject("PickLineObject");
	pickLineNode = scm->getRootSceneNode()->createChildSceneNode("PickLineNode");

	MaterialPtr pickLineMaterial = MaterialManager::getSingleton().create("PickLineMaterial",ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	pickLineMaterial->setReceiveShadows(false);
	pickLineMaterial->getTechnique(0)->setLightingEnabled(true);
	pickLineMaterial->getTechnique(0)->getPass(0)->setDiffuse(0,0,1,0);
	pickLineMaterial->getTechnique(0)->getPass(0)->setAmbient(0,0,1);
	pickLineMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(0,0,1);

	pickLine->begin("PickLineMaterial", Ogre::RenderOperation::OT_LINE_LIST);
	pickLine->position(0, 0, 0);
	pickLine->position(0, 0, 0);
	pickLine->end();
	pickLineNode->attachObject(pickLine);
	pickLineNode->setVisible(false);
}

SceneMouse::~SceneMouse()
{
}

bool SceneMouse::mouseMoved(const OIS::MouseEvent& _arg)
{
	const OIS::MouseState ms = _arg.state;

	// experimental mouse hack
	if(ms.buttonDown(OIS::MB_Left))
	{
		// get the camera
		Camera *cam = rfl->getCamera();
		Viewport *vp = cam->getViewport();
		// get a camera to scene ray
		Ray mouseRay = cam->getCameraToViewportRay((float)ms.X.abs/(float)vp->getWidth(), (float)ms.Y.abs/(float)vp->getHeight());
		// get current truck
		Beam *curr_truck = BeamFactory::getSingleton().getCurrentTruck();
		// check if its valid and not sleeping
		if(curr_truck && curr_truck->state == ACTIVATED)
		{

			float mindist = 30000.0f;
			int   minnode = -1;
			// walk all nodes
			for (int i = 0; i < curr_truck->free_node; i++)
			{
				// check if our ray intersects with the node
				std::pair<bool, Real> pair = mouseRay.intersects(Sphere(curr_truck->nodes[i].smoothpos, 0.1f));
				if (pair.first)
				{
					// we hit it, check if its the nearest node
					if (pair.second < mindist)
					{
						mindist = pair.second;
						minnode = i;
					}
				}
			}

			// check if we hit a node
			if(minnode != -1)
			{
				// get correct ray
				Ray m = cam->getCameraToViewportRay((float)ms.X.abs/(float)vp->getWidth(), (float)ms.Y.abs/(float)vp->getHeight());

				// get position
				Vector3 pos = m.getPoint(mindist);

				// then display
				pickLine->beginUpdate(0);
				pickLine->position(curr_truck->nodes[minnode].AbsPosition);
				pickLine->position(pos);
				pickLine->end();
				pickLineNode->setVisible(true);

				// add forces
				curr_truck->mouseMove(minnode, pos, 10000.0f);
			} else
			{
				// hide mouse line
				pickLineNode->setVisible(false);
			}
			// not fixed
			return false;
		}
	}
	

	if(ms.buttonDown(OIS::MB_Right))
	{
		rfl->camRotX += Degree(-(float)ms.X.rel * 0.13f);
		rfl->camRotY += Degree(-(float)ms.Y.rel * 0.13f);
#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setPointer("hand");
#endif // USE_MYGUI
		return true;
	} else if(rfl->cameramode == CAMERA_FREE)
	{
		rfl->getCamera()->yaw(Degree(-(float)ms.X.rel * 0.13f));
		rfl->getCamera()->pitch(Degree(-(float)ms.Y.rel * 0.13f));
#ifdef USE_MYGUI
		MyGUI::PointerManager::getInstance().setPointer("hand");
#endif // USE_MYGUI
		return true;
	}
	return false;
}

bool SceneMouse::mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return true;
}

bool SceneMouse::mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id)
{
	return true;
}

bool SceneMouse::keyPressed(const OIS::KeyEvent& _arg)
{
	return true;
}

bool SceneMouse::keyReleased(const OIS::KeyEvent& _arg)
{
	return true;
}

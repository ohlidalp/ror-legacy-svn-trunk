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
#include "IngameEditor.h"
#include "ExampleFrameListener.h"
#include "Beam.h"
#include "engine.h"
#include "OISMouse.h"
#include "ResourceBuffer.h"
#include "language.h"

using namespace OIS;
//using namespace Ogre;
using namespace std;

// singleton pattern
IngameEditor* IngameEditor::myInstance = 0;

IngameEditor & IngameEditor::Instance () 
{
	if (myInstance == 0)
		myInstance = new IngameEditor;
	return *myInstance;
}

IngameEditor::IngameEditor() : mefl(0), mTimeUntilNextToggle(0), placeMode(0), attachNodeNumber(-1), savedNearClip(-1), hideFrontBeams(false), initiated(false), truckNumber(-1), beamType(0)
{
	editorMode = IE_GEOMETRY;
	offPos = Ogre::Vector3(-2,2,5);
}

void IngameEditor::setup(ExampleFrameListener *efl)
{
	virtualPosition = Ogre::Vector3(0,0,0);
	mefl = efl;

	gridsize = Vector2(10, 10);

	//initiate Beam
	//int tnum = mefl->addTruck("semi.truck", basePosition);
	//truck = mefl->getTruck(tnum);
	truck = 0;

	// create grid
	;
	gridPlane.normal = Ogre::Vector3::UNIT_Z;
	gridPlane.d = 0;

	MeshManager::getSingleton().createPlane("GridPlane1Mesh",
			ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
			gridPlane,
			gridsize.x, gridsize.y ,1,1,true,1,1,1,Ogre::Vector3::UNIT_Y);
	Entity* ent = mefl->getSceneMgr()->createEntity( "GridPlane1", "GridPlane1Mesh" );
	ent->setMaterialName("tracks/IngameEditor/Grid1");
	ent->setVisible(true);

	gridNode = mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode("IngameEditorGrid1");
	gridNode->attachObject(ent);

	// create selection sphere
	Entity *ss = mefl->getSceneMgr()->createEntity("IngameEditorSelectionEntity", "simplesphere.mesh");
	ss->setMaterialName("tracks/IngameEditor/Selector1");
	selectionNode = mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode("IngameEditorSelectionNode");
	selectionNode->attachObject(ss);
	selectionNode->setScale(0.08, 0.08, 0.08);

	// create temporary sphere
	Entity *ts = mefl->getSceneMgr()->createEntity("IngameEditorSelectionEntityTemp", "simplesphere.mesh");
	ts->setMaterialName("tracks/IngameEditor/Selector2");
	selectionNode2 = mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode("IngameEditorSelectionNode2");
	selectionNode2->attachObject(ts);
	selectionNode2->setScale(0.08, 0.08, 0.08);

	// create second temporary sphere
	Entity *ts2 = mefl->getSceneMgr()->createEntity("IngameEditorSelectionEntityTemp2", "simplesphere.mesh");
	ts2->setMaterialName("tracks/IngameEditor/Selector3");
	selectionNode3 = mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode("IngameEditorSelectionNode3");
	selectionNode3->attachObject(ts2);
	selectionNode3->setScale(0.06, 0.06, 0.06);

	// create lines

	// constrution line
	lineManualObject =  mefl->getSceneMgr()->createManualObject("manualLine"); 
	lineNode = mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode("manualLineNode"); 
	MaterialPtr myManualObjectMaterial = MaterialManager::getSingleton().create("manualLineMaterial",ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,true,new ResourceBuffer()); 
	myManualObjectMaterial->setReceiveShadows(false); 
	myManualObjectMaterial->getTechnique(0)->setLightingEnabled(true); 
	myManualObjectMaterial->getTechnique(0)->getPass(0)->setDiffuse(1,1,0,0); 
	myManualObjectMaterial->getTechnique(0)->getPass(0)->setAmbient(1,1,0); 
	myManualObjectMaterial->getTechnique(0)->getPass(0)->setSelfIllumination(1,1,0); 
	lineManualObject->begin("manualLineMaterial", Ogre::RenderOperation::OT_LINE_LIST); 
	lineManualObject->position(0, 0, 0); 
	lineManualObject->position(0, 0, 0); 
	lineManualObject->end(); 
	lineNode->attachObject(lineManualObject);

	// help line 1
	lineManualObject2 =  mefl->getSceneMgr()->createManualObject("manualLine2"); 
	lineNode2 = mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode("manualLineNode2"); 
	MaterialPtr myManualObjectMaterial2 = MaterialManager::getSingleton().create("manualLineMaterialHelp1",ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,true,new ResourceBuffer()); 
	myManualObjectMaterial2->setReceiveShadows(false); 
	myManualObjectMaterial2->getTechnique(0)->setLightingEnabled(true);
	myManualObjectMaterial2->getTechnique(0)->getPass(0)->setDiffuse(0,1,1,0.7);
	myManualObjectMaterial2->getTechnique(0)->getPass(0)->setAmbient(0,1,1);
	myManualObjectMaterial2->getTechnique(0)->getPass(0)->setSelfIllumination(0,1,1);
	myManualObjectMaterial2->getTechnique(0)->getPass(0)->setDepthCheckEnabled(false);
	lineManualObject2->begin("manualLineMaterialHelp1", Ogre::RenderOperation::OT_LINE_LIST);
	lineManualObject2->position(0, 0, 0); 
	lineManualObject2->position(0, 0, 0); 
	lineManualObject2->end(); 
	lineNode2->attachObject(lineManualObject2);

	// beam type text
	beamTypeText = new MovableText("beamType_text", "normal");
	beamTypeText->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
	beamTypeText->setAdditionalHeight(0.2);
	beamTypeText->showOnTop(true);
	beamTypeText->setCharacterHeight(2);
	selectionNode2->attachObject(beamTypeText);

	// coordinate text
	selectorText = new MovableText("selector_text", "normal");
	selectorText->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
	selectorText->setAdditionalHeight(0.2);
	selectorText->showOnTop(true);
	selectorText->setCharacterHeight(2);
	selectionNode->attachObject(selectorText);
	selectorText->setVisible(false);
	initiated = true;
}


void IngameEditor::getTruckDirection()
{
		Ogre::Vector3 idir=truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranodedir[0]].RelPosition;
		truckangle=atan2(idir.dotProduct(Ogre::Vector3::UNIT_X), idir.dotProduct(-Ogre::Vector3::UNIT_Z));
}


bool IngameEditor::show(bool visible)
{
	if(visible)
	{
		truckNumber = mefl->getCurrentTruckNumber();
		if(truckNumber < 0)
			return false;

		truck = mefl->getCurrentTruck();
		if(!truck)
			return false;
		truck->desactivate();
		savedSkeletonMode = truck->skeleton;
		truck->showSkeleton(false);
		selectorText->setVisible(true);

		// reset truck at current position
		Ogre::Vector3 ipos=truck->nodes[0].AbsPosition;
		//truck->reset();
		//truck->resetPosition(ipos.x, ipos.z, false);

		truck->updateVisual();
		mefl->showDashboardOverlays(false, TRUCK);

		// rotate everything to match truck rotation
		getTruckDirection();
		Quaternion rotationQuaternion(Radian(-truckangle) - Degree(90), Ogre::Vector3::UNIT_Y);
		baseRotation = rotationQuaternion;
		//truck->reset();
		this->basePosition = truck->getPosition();

		//mefl->setCurrentTruck(-1);
		mefl->cameramode = CAMERA_EXT;
		Camera *cam = mefl->getCamera();

		//cam->setProjectionType(PT_ORTHOGRAPHIC);
		savedNearClip = cam->getNearClipDistance();
		cam->setNearClipDistance(0.01);
		//mefl->getPersonNode()->setVisible(false);
		if(truck->engine)
			truck->engine->stop();
		mefl->setCameraRotation(Degree(-90), Degree(12), 10);
		lineNode->setVisible(false);
		lineNode2->setVisible(true);
		virtualPosition = Ogre::Vector3(0,0,0);

		positionChanged(true);
	} else {
		if(!truck)
			return false;
		hideFrontBeams=false;
		positionChanged();
		mefl->cameramode = CAMERA_EXT;
		mefl->getCamera()->setProjectionType(PT_PERSPECTIVE);
		if(savedNearClip != -1)
			mefl->getCamera()->setNearClipDistance(savedNearClip);
		if(!savedSkeletonMode)
			truck->hideSkeleton();
		//mefl->getPersonNode()->setVisible(true);
		if(truckNumber != -1)
			mefl->setCurrentTruck(truckNumber);
		if(truck && truck->engine)
			truck->engine->start();
		if(truck)
			truck->activate();
		lineNode->setVisible(false);
		lineNode2->setVisible(false);
		if(truck)
			if(truck->driveable == AIRPLANE)
				mefl->showDashboardOverlays(true, AIRPLANE);
			else if(truck->driveable == TRUCK)
				mefl->showDashboardOverlays(true, TRUCK);
			else if(truck->driveable == BOAT)
				mefl->showDashboardOverlays(true, BOAT);
		selectorText->setVisible(false);
	}
	gridNode->setVisible(visible);
	selectionNode->setVisible(visible);
	selectionNode2->setVisible(false);
	selectionNode3->setVisible(false);
	beamTypeText->setVisible(false);
	return true;
}

void IngameEditor::positionChanged(bool rotationChanged)
{
	if(!truck)
		return;
	// how much the grid will move with the cursor
	gridTranslation = Vector2(ceil((virtualPosition.x-gridsize.x/2)/gridsize.x),ceil((virtualPosition.y-gridsize.y/2)/gridsize.y));
	//LogManager::getSingleton().logMessage("position: "+StringConverter::toString(virtualPosition));
	gridNode->setPosition(basePosition + baseRotation * Ogre::Vector3(gridsize.x*gridTranslation.x, gridsize.y*gridTranslation.y, virtualPosition.z));
	if(rotationChanged)
		gridNode->setOrientation(baseRotation);

	// update help line
	lineManualObject2->beginUpdate(0); 
	lineManualObject2->position(basePosition + baseRotation * (virtualPosition - Ogre::Vector3(0, 0, 5)));
	lineManualObject2->position(basePosition + baseRotation * (virtualPosition + Ogre::Vector3(0, 0, 5))); 
	lineManualObject2->end();

	// FIXME: get the plane normalised, so the getDistance function works!
	//gridPlane.redefine(basePosition + baseRotation * Ogre::Vector3(1,0,0), basePosition + baseRotation * Ogre::Vector3(0,1,0), basePosition + baseRotation * Ogre::Vector3(0,0,1));
	//gridPlane.redefine(basePosition + baseRotation * Ogre::Vector3(0,0,0), basePosition + baseRotation * Ogre::Vector3(0,1,0), basePosition + baseRotation * Ogre::Vector3(1,0,0));
	Ogre::Vector3 a = (baseRotation * Ogre::Vector3(0, 0, -virtualPosition.z));
	gridPlane.redefine(a.normalisedCopy(), basePosition);
	gridPlane.d = 0;
	//gridPlane.normal = baseRotation * Ogre::Vector3::UNIT_Z;
	gridPlane.normalise();

	// place main selection node
	selectionNode->setPosition(basePosition + baseRotation * virtualPosition);

	if(placeMode == 0 || placeMode >= 5)
	{
		selectionNode2->setVisible(false);
	}
	else
	{
		selectionNode2->setVisible(true);
		if(tmpPosition == Ogre::Vector3::ZERO && tmpNodeNumber >= 0)
		{
			selectionNode2->setPosition(truck->nodes[tmpNodeNumber].AbsPosition);
		}
		else
		{
			selectionNode2->setPosition(basePosition + baseRotation * tmpPosition);
		}
		
	}

	// move node stuff
	if(placeMode == 5)
	{
		truck->nodes[tmpNodeNumber].AbsPosition = basePosition + baseRotation * virtualPosition; //maybe I broke something here
		truck->nodes[tmpNodeNumber].iPosition = basePosition + baseRotation * virtualPosition;
		// TODDO: fixme
		// recalculate length of all connected beams
		for(int i=0; i<truck->getBeamCount();i++)
		{
			if(truck->beams[i].p1->id == tmpNodeNumber || truck->beams[i].p2->id == tmpNodeNumber)
			{
				Ogre::Vector3 t;
				t=truck->beams[i].p1->AbsPosition;
				t=t-truck->beams[i].p2->AbsPosition;
				truck->beams[i].L=t.length();
				truck->beams[i].refL=t.length();
				truck->beams[i].Lhydro=t.length();
			}
		}
	}

	// find a near node
	float distance = gridsize.x / 10 / 5;
	attachNodeNumber = -1;
	for(int i=0; i<truck->getNodeCount();i++)
	{
		if (fabs(truck->nodes[i].AbsPosition.distance(basePosition + baseRotation * virtualPosition)) <= distance)
		{
			attachNodeNumber = i;
			break;
		}
	}
	if(attachNodeNumber != -1)
	{
		selectionNode3->setVisible(true);
		selectionNode3->setPosition(truck->nodes[attachNodeNumber].AbsPosition);
	}else
		selectionNode3->setVisible(false);

	// update construction line
	if(placeMode != 0 && placeMode < 5)
	{
		lineManualObject->setVisible(true);
		lineManualObject->beginUpdate(0); 
		if(tmpPosition == Ogre::Vector3::ZERO && tmpNodeNumber >= 0)
			lineManualObject->position(truck->nodes[tmpNodeNumber].AbsPosition);
		else
			lineManualObject->position(basePosition + baseRotation * tmpPosition);
		lineManualObject->position(basePosition + baseRotation * virtualPosition); 
		lineManualObject->end();
	}else
		lineManualObject->setVisible(false);
	
	//LogManager::getSingleton().logMessage(StringConverter::toString(beamType));
	switch(beamType) {
		case 0: beamTypeText->setCaption("normal"); break;
		case 1: beamTypeText->setCaption("invisible"); break;
		case 2: beamTypeText->setCaption("rope"); break;
	}

	// update selector text
	char tmp[255];
	sprintf(tmp,"(%.02f / %.02f / %.02f)", virtualPosition.x, virtualPosition.y, virtualPosition.z);
	String msg = String(tmp);
	if(attachNodeNumber != -1)
		msg += String("\nnode " + StringConverter::toString(attachNodeNumber)+"            ");
	else
		msg += String("\n             ");
	selectorText->setCaption(msg);

	// update camera
	Camera *cam = mefl->getCamera();
	cam->setPosition(basePosition + baseRotation * virtualPosition + baseRotation * Ogre::Vector3(offPos.x, offPos.y, offPos.z));
	cam->lookAt(basePosition + baseRotation * virtualPosition);

	// color beams, so that we can see in what depth we are working
	Ogre::Vector3 curPos = basePosition + baseRotation * virtualPosition;
	float zdistance = distance * 4;
	for (int i=0; i<truck->getBeamCount(); i++)
	{
		if(!truck->beams[i].mEntity || !truck->beams[i].p1 || !truck->beams[i].p2)
			continue;		
		if(curPos.distance(truck->beams[i].p1->AbsPosition) <= zdistance && curPos.distance(truck->beams[i].p2->AbsPosition) <= zdistance)
		{
			truck->beams[i].mEntity->setVisible(true);
			truck->beams[i].mEntity->setMaterialName("tracks/IngameEditor/mat-beam-fit");
		}
		else if(curPos.distance(truck->beams[i].p1->AbsPosition) <= zdistance || curPos.distance(truck->beams[i].p2->AbsPosition) <= zdistance)
		{
				truck->beams[i].mEntity->setVisible(true);
				truck->beams[i].mEntity->setMaterialName("tracks/IngameEditor/mat-beam-cross");
		}
		else 
		{
			if(hideFrontBeams)
			{
				truck->beams[i].mEntity->setVisible(false);
			}
			else
			{
				truck->beams[i].mEntity->setVisible(true);
				truck->beams[i].mEntity->setMaterialName("tracks/IngameEditor/mat-beam-trans");
			}
		}
	}
	truck->updateVisual();
}

void IngameEditor::applyBeamType(beam_t *b)
{
	if(beamType == 0)
	{
		b->type=BEAM_NORMAL;
	}
	else if(beamType == 1)
	{
		if(b->mSceneNode)
			b->mSceneNode->detachAllObjects();
		b->type=BEAM_INVISIBLE;
	}
	else if(beamType == 2)
	{
		b->type=BEAM_NORMAL;
		b->isrope = true;
	}
}


void IngameEditor::updateInput()
{
	bool posChanged = false, rotationChanged = false;
	float stepsizex = gridsize.x / 10 / 5;
	float stepsizey = gridsize.y / 10 / 5;
	float stepsizez = 1.0 / 5.0;

	// handle mouse
	const OIS::MouseState mstate=INPUTENGINE.getMouseState();
	//LogManager::getSingleton().logMessage(StringConverter::toString(KEYMANAGER.isModifierDown(KC_LCONTROL)));
	
	/*
	// no more base rotation
	if(!INPUTENGINE.isKeyDown(KC_LSHIFT) && INPUTENGINE.isKeyDown(KC_LCONTROL))
	{
		if(mstate.X.rel != 0 && !mstate.buttonDown(MB_Right))
		{
			Quaternion rotationQuaternion(Degree(mstate.X.rel), Ogre::Vector3::UNIT_Y);
			baseRotation = baseRotation * rotationQuaternion;
			rotationChanged = true;
		}
	}
	if(INPUTENGINE.isKeyDown(KC_LSHIFT) && INPUTENGINE.isKeyDown(KC_LCONTROL))
	{
		if(mstate.Y.rel != 0 && !mstate.buttonDown(MB_Right))
		{
			Quaternion rotationQuaternion(Degree(mstate.Y.rel), Ogre::Vector3::UNIT_X);
			baseRotation = baseRotation * rotationQuaternion;
			rotationChanged = true;
		}
	}
	*/

	// moving functions -- do nothing while rotating!
	if(!rotationChanged && !INPUTENGINE.isKeyDown(KC_LCONTROL))
	{
		float a = mefl->getCameraRotationX().valueDegrees() - Radian(truckangle).valueDegrees() - 90;
		//LogManager::getSingleton().logMessage(StringConverter::toString(a));
		bool reverseMouseX = false; //!(abs((int)a % 180) > 90);
		bool reverseMouseY = false;

		if(mstate.X.rel != 0 && !mstate.buttonDown(MB_Right))
		{
			virtualPosition2 += Ogre::Vector3(stepsizex * (reverseMouseX?-1:1) * (float)mstate.X.rel / 10.0 , 0,0);
			posChanged = true;
		}
		if(mstate.Y.rel != 0 && !mstate.buttonDown(MB_Right))
		{
			virtualPosition2 += Ogre::Vector3(0, stepsizey * (reverseMouseY?-1:1) * (float)-mstate.Y.rel / 10.0, 0);
			posChanged = true;
		}
		if(mstate.Z.rel != 0 && !mstate.buttonDown(MB_Right))
		{
			//LogManager::getSingleton().logMessage(StringConverter::toString(mstate.Z.rel));
			virtualPosition2 += Ogre::Vector3(0, 0, 0.2 * (float)-mstate.Z.rel / 120.0);
			posChanged = true;
		}

		// handle (mostly) keyboard
		if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_MODE_GEO") && mTimeUntilNextToggle <= 0)
		{
			switchmode(IE_GEOMETRY);
			mTimeUntilNextToggle = 0.1;
		}
		if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_MODE_FLARE") && mTimeUntilNextToggle <= 0)
		{
			switchmode(IE_flares);
			mTimeUntilNextToggle = 0.1;
		}
		if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_UP") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 += Ogre::Vector3(0,stepsizey,0);
			posChanged = true;
			mTimeUntilNextToggle = 0.1;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_DOWN") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 += Ogre::Vector3(0,-stepsizey,0);
			posChanged = true;
			mTimeUntilNextToggle = 0.1;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_LEFT") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 += Ogre::Vector3(-stepsizex,0,0);
			posChanged = true;
			mTimeUntilNextToggle = 0.1;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_RIGHT") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 += Ogre::Vector3(stepsizex,0,0);
			posChanged = true;
			mTimeUntilNextToggle = 0.1;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_BACKWARD") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 += Ogre::Vector3(0,0,-stepsizez);
			posChanged = true;
			mTimeUntilNextToggle = 0.1;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_FORWARD") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 += Ogre::Vector3(0,0,stepsizez);
			posChanged = true;
			mTimeUntilNextToggle = 0.1;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_RESET") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 = Ogre::Vector3(0,0,0);
			baseRotation = Quaternion(Degree(0), Ogre::Vector3::UNIT_X);
			rotationChanged = true;
			posChanged = true;
			mTimeUntilNextToggle = 0.2;
		}

		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_RESET") && mTimeUntilNextToggle <= 0)
		{
			virtualPosition2 = Ogre::Vector3(0,0,0);
			posChanged = true;
			mTimeUntilNextToggle = 0.2;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_TRANS_FRONT") && mTimeUntilNextToggle <= 0)
		{
			hideFrontBeams = ! hideFrontBeams;
			posChanged = true;
			mTimeUntilNextToggle = 0.2;
		}	
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_BEAM_TYPE") && mTimeUntilNextToggle <= 0)
		{
			beamType++;
			if(beamType > 2)
				beamType = 0;
			posChanged = true;
			mTimeUntilNextToggle = 0.2;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_CHANGE_NODE_TYPE") && mTimeUntilNextToggle <= 0)
		{
			// TODO: implement me
			mTimeUntilNextToggle = 0.2;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_SHOW_COORDS") && mTimeUntilNextToggle <= 0)
		{
			selectorText->setVisible(!selectorText->getVisible());
			posChanged = true;
			mTimeUntilNextToggle = 0.3;
		}	

	}

	// apply position (grid aligned or not)
	if(INPUTENGINE.isKeyDown(KC_LMENU))
	{
		// grid align
		virtualPosition = Ogre::Vector3(stepsizex * floor(virtualPosition2.x / stepsizex), \
								  stepsizey * floor(virtualPosition2.y / stepsizey), \
								  stepsizez * floor(virtualPosition2.z / stepsizez));
	}
	else
	{
		// not grid aligned
		virtualPosition = virtualPosition2;
	}		

	// editing functions
	if(!rotationChanged)
	{
		if((INPUTENGINE.getEventBoolValue("INGAMEEDITOR_MOVE_NODE") || (mstate.buttonDown(OIS::MB_Left) && INPUTENGINE.isKeyDown(KC_LSHIFT))) && mTimeUntilNextToggle <= 0)
		{
			if(placeMode == 0 && attachNodeNumber != -1)
			{
				tmpNodeNumber = attachNodeNumber;
				placeMode = 5;
			}
			else if(placeMode == 5)
			{
				placeMode = 0;
			}
			posChanged = true;
			mTimeUntilNextToggle = 1;
		}	
		else if((INPUTENGINE.getEventBoolValue("INGAMEEDITOR_PLACE_BEAM") || mstate.buttonDown(OIS::MB_Left)) && mTimeUntilNextToggle <= 0)
		{
			if(placeMode == 0 && attachNodeNumber == -1)
			{
				tmpNodeNumber = -1;
				tmpPosition = virtualPosition;
				placeMode = 1;
			}
			else if(placeMode == 0 && attachNodeNumber != -1)	// first node exist
			{
				tmpNodeNumber = attachNodeNumber;
				tmpPosition = Ogre::Vector3::ZERO; // truck->nodes[tmpNodeNumber].AbsPosition - basePosition;
				placeMode = 2;
			}
			else if (placeMode == 1) // both nodes new
			{
				if(tmpPosition == virtualPosition)
					return;
				// init the nodes first
				node_t *node1 = truck->addNode(basePosition + baseRotation * tmpPosition);
				node_t *node2 = truck->addNode(basePosition + baseRotation * virtualPosition);
				truck->setMass(truck->getTotalMass() + node1->mass + node2->mass);
				truck->calc_masses2(truck->getTotalMass());

				// now place a beam inbetween
				beam_t *b = truck->addBeam(node1->id, node2->id);
				applyBeamType(b);

				truck->updateVisual();

				placeMode = 0;
				tmpNodeNumber = -1;
			}
			else if (placeMode == 2 && attachNodeNumber != -1 && attachNodeNumber != tmpNodeNumber) // first and second node exist
			{
				beam_t *b = truck->addBeam(tmpNodeNumber, attachNodeNumber);
				applyBeamType(b);

				truck->updateVisual();

				placeMode = 0;
				tmpNodeNumber = -1;
			}
			else if (placeMode == 2 && attachNodeNumber == -1 && tmpNodeNumber != -1) // only 2nd node new
			{
				// init the nodes first
				node_t *node2 = truck->addNode(basePosition + baseRotation * virtualPosition);
				truck->setMass(truck->getTotalMass() + node2->mass);
				truck->calc_masses2(truck->getTotalMass());
				// now place a beam inbetween
				beam_t *b = truck->addBeam(tmpNodeNumber, node2->id);
				applyBeamType(b);

				truck->updateVisual();

				placeMode = 0;
				tmpNodeNumber = -1;
			}
			posChanged = true;
			mTimeUntilNextToggle = 0.5;
		}
		else if(INPUTENGINE.getEventBoolValue("INGAMEEDITOR_KEY_INFO") && mTimeUntilNextToggle <= 0)
		{
			String msg = _L("Keytable for the truck editor:\nY - exit/enter editor\nR - Reset Plane and cursors\nSPACE / Left Mouse Button - Place Beams\nG - Switch Beam type\nH - Show coordinates information\nM / Right Mouse Button - Move selected Node\nN - change node type\nMouse - move curor\nNUMPAD 4682/93/5 - rotate/zoom/reset view\nPAGE UP/DOWN - Move Editingg Plane forwards/backwards\nUP/LEFT/RIGHT/DOWN - move red cursor up/left/right/down\nPlease note that you do not drag with the mouse. Rather click twice!\n" );
			mefl->flashMessage(const_cast<char*>(msg.c_str()), 15, 0.04);
			mTimeUntilNextToggle = 0.1;
		}
	}

	if(posChanged || rotationChanged)
		positionChanged(rotationChanged);
}

void IngameEditor::switchmode(int newMode)
{
	editorMode = newMode;

}

void IngameEditor::frameStep(const FrameEvent& evt)
{
	if(!truck)
		return;

	updateInput();

	if (mTimeUntilNextToggle >= 0) 
		mTimeUntilNextToggle -= evt.timeSinceLastFrame;
	//truck->updateVisual();
}

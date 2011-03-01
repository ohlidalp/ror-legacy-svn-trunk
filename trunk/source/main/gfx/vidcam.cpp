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
#include "vidcam.h"
#include "ResourceBuffer.h"
#include <Ogre.h>

#include "utils.h"

using namespace Ogre;

int VideoCamera::counter = 0;

VideoCamera::VideoCamera(Ogre::SceneManager *mSceneMgr, Ogre::Camera *camera, Beam *truck) : mSceneMgr(mSceneMgr), camera(camera), truck(truck)
	, mVidCam()
	, rttTex(0)
	, mat()
{
}

void VideoCamera::init()
{
	mat = Ogre::MaterialManager::getSingleton().getByName(materialName);

	mVidCam = mSceneMgr->createCamera(materialName + "_camera");

	Ogre::TexturePtr rttTexPtr = Ogre::TextureManager::getSingleton().createManual(materialName + "_texture"
			, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
			, Ogre::TEX_TYPE_2D
			, textureSize.x
			, textureSize.y
			, 0
			, Ogre::PF_R8G8B8
			, Ogre::TU_RENDERTARGET
			, new ResourceBuffer());
	rttTex = rttTexPtr->getBuffer()->getRenderTarget();
		
	mVidCam->setNearClipDistance(minclip);
	mVidCam->setFarClipDistance(maxclip);
	mVidCam->setFOVy(Ogre::Degree(fov));
	mVidCam->setAspectRatio((float)textureSize.x/(float)textureSize.y);
	Ogre::Viewport *v = rttTex->addViewport(mVidCam);
	v->setClearEveryFrame(true);
	v->setBackgroundColour(camera->getViewport()->getBackgroundColour());

	disabledTexture = mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->getTextureName();

	mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(materialName + "_texture");
	mat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
	v->setOverlaysEnabled(false);
}

void VideoCamera::setActive(bool state)
{
	rttTex->setActive(state);
	if (state)
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(materialName + "_texture");
	else
	{
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName(disabledTexture);
		// mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureRotate(Degree(180));
	}
}

void VideoCamera::update(float dt)
{
	// get the normal of the camera plane now
	Vector3 normal=(-(truck->nodes[nref].smoothpos - truck->nodes[nx].smoothpos)).crossProduct(-(truck->nodes[nref].smoothpos - truck->nodes[ny].smoothpos));
	normal.normalise();

	// add user set offset
	Vector3 pos = truck->nodes[nx].smoothpos + 
		(offset.x * normal) + 
		(offset.y * (truck->nodes[nref].smoothpos - truck->nodes[ny].smoothpos)) + 
		(offset.z * (truck->nodes[nref].smoothpos - truck->nodes[nx].smoothpos));

	//orientation of camera
	Vector3 refx = truck->nodes[nx].smoothpos - truck->nodes[nref].smoothpos;
	refx.normalise();
	Vector3 refy = -(refx.crossProduct(normal));
	refx *= -1.0f;
	Quaternion rot = Quaternion(refx, refy, -normal); // rotate towards the cam direction

	// set the new position / orientation to the camera
	mVidCam->setPosition(pos);
	mVidCam->setOrientation(rot * rotation); // add the user rotation to this
}



VideoCamera *VideoCamera::parseLine(Ogre::SceneManager *mSceneMgr, Ogre::Camera *camera, Beam *truck, const char *fname, char *line, int linecounter)
{
	// sample rate / isMirror / rotxyz ???
	int nx=-1, ny=-1, nref=-1, texx=256, texy=256, cammode=-1;
	float fov=-1.0f, minclip=-1.0f, maxclip=-1.0f, offx=0.0f, offy=0.0f, offz=0.0f, rotx=0.0f, roty=0.0f, rotz=0.0f;
	char materialname[255] = "";
	int result = sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %f, %i, %i, %f, %f, %i, %s", &nx, &ny, &nref, &offx, &offy, &offz, &rotx, &roty, &rotz, &fov, &texx, &texy, &minclip, &maxclip, &cammode, materialname);
	if (result < 16 || result == EOF)
	{
		LogManager::getSingleton().logMessage("Error parsing File (videocamera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}

	if (nx < 0 || nx >= truck->free_node || ny < 0 || ny >= truck->free_node || nref < 0 || nref >= truck->free_node)
	{
		LogManager::getSingleton().logMessage("Error parsing File (videocamera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Wrong node definition. trying to continue ...");
		return 0;
	}

	if (texx <= 0 || !isPowerOfTwo(texx) || texy <= 0 || !isPowerOfTwo(texy))
	{
		LogManager::getSingleton().logMessage("Error parsing File (videocamera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Wrong texture size definition (needs to be 2^n). trying to continue ...");
		return 0;
	}

	if (minclip < 0 || minclip > maxclip || maxclip < 0)
	{
		LogManager::getSingleton().logMessage("Error parsing File (videocamera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". Wrong clipping definition. trying to continue ...");
		return 0;
	}

	// TODO check for correct and resonable values

	VideoCamera *v = new VideoCamera(mSceneMgr, camera, truck);
	v->fov = fov;
	v->minclip = minclip;
	v->maxclip = maxclip;
	v->nx = nx;
	v->ny = ny;
	v->nref = nref;

	v->offset      = Vector3(offx, offy, offz);
	v->rotation    = Quaternion(Degree(rotz), Vector3::UNIT_Z) * Quaternion(Degree(roty), Vector3::UNIT_Y) * Quaternion(Degree(rotx), Vector3::UNIT_X);
	v->textureSize = Vector2(texx, texy);

	v->cammode = cammode;
	v->materialName = String(materialname);

	v->init();

	return v;
}
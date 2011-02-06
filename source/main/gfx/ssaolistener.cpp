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
#include "ssaolistener.h"
#include "Ogre.h"

// constructor
SSAOListener::SSAOListener(Ogre::SceneManager *mgr, Ogre::Camera *cam) : mgr(mgr), cam(cam)
{
}

// this callback will be used to modify SSAO parameters
void SSAOListener::notifyMaterialRender(Ogre::uint32 pass_id, Ogre::MaterialPtr &mat)
{
	if (pass_id != 42) // not SSAO, return
		return;

	// calculate the far-top-right corner in view-space
	Ogre::Vector3 farCorner = cam->getViewMatrix(true) * cam->getWorldSpaceCorners()[4];

	// get the pass
	Ogre::Pass *pass = mat->getBestTechnique()->getPass(0);

	// get the vertex shader parameters
	Ogre::GpuProgramParametersSharedPtr params = pass->getVertexProgramParameters();
	// set the camera's far-top-right corner
	if (params->_findNamedConstantDefinition("farCorner"))
		params->setNamedConstant("farCorner", farCorner);

	// get the fragment shader parameters
	params = pass->getFragmentProgramParameters();
	// set the projection matrix we need
	static const Ogre::Matrix4 CLIP_SPACE_TO_IMAGE_SPACE(
		0.5,    0,    0,  0.5,
		0,   -0.5,    0,  0.5,
		0,      0,    1,    0,
		0,      0,    0,    1);
	if (params->_findNamedConstantDefinition("ptMat"))
		params->setNamedConstant("ptMat", CLIP_SPACE_TO_IMAGE_SPACE * cam->getProjectionMatrixWithRSDepth());
	if (params->_findNamedConstantDefinition("far"))
		params->setNamedConstant("far", cam->getFarClipDistance());
}

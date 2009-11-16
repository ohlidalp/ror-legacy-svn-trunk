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
#include "mirrors.h"
#include "ResourceBuffer.h"

Camera *mloCamera;

class MirrorsListener : public RenderTargetListener
{
public:
    void preRenderTargetUpdate(const RenderTargetEvent& evt)
    {
		//mloCamera->setFOVy(mloCamera->getFOVy()/2.0);
    }
    void postRenderTargetUpdate(const RenderTargetEvent& evt)
    {
		//mloCamera->setFOVy(mloCamera->getFOVy()*2.0);
    }

};

MirrorsListener mMirrorsListener;

Mirrors::Mirrors(SceneManager *mSceneMgr, RenderWindow *mWindow, Camera *camera)
{
	/*	Entity *teo = mSceneMgr->createEntity("mirroro", "beam.mesh");
		tnodeo=mSceneMgr->getRootSceneNode()->createChildSceneNode();
		tnodeo->attachObject(teo);
		tnodeo->setScale(0.001, 0.001, 0.001);

		Entity *tei = mSceneMgr->createEntity("mirrori", "beam.mesh");
		tei->setMaterialName("tracks/beam");
		tnoden=mSceneMgr->getRootSceneNode()->createChildSceneNode();
		tnoden->attachObject(tei);
		tnoden->setScale(0.001, 0.001, 0.001);
*/
		//mScene=mSceneMgr;
		rttTex=0;
		mloCamera=camera;
		mCamera=camera;

		TexturePtr rttTexPtr = TextureManager::getSingleton().createManual("mirrortexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, TEX_TYPE_2D, 128, 256, 0, PF_R8G8B8, TU_RENDERTARGET, new ResourceBuffer());
		rttTex = rttTexPtr->getBuffer()->getRenderTarget();
		{
			mMirrorCam = mSceneMgr->createCamera("MirrorCam");
			mMirrorCam->setNearClipDistance(0.2);
			mMirrorCam->setFarClipDistance(mCamera->getFarClipDistance());
			mMirrorCam->setFOVy(Degree(50));
			mMirrorCam->setAspectRatio(
				((Real)mWindow->getViewport(0)->getActualWidth() / 
				(Real)mWindow->getViewport(0)->getActualHeight())/2.0);

			Viewport *v = rttTex->addViewport( mMirrorCam );
			v->setClearEveryFrame( true );
			v->setBackgroundColour( camera->getViewport()->getBackgroundColour() );

			mat = MaterialManager::getSingleton().getByName("mirror");
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirrortexture");
			//mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setProjectiveTexturing(true, mMirrorCam);
			//mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAddressingMode(TextureUnitState::TAM_CLAMP);
			mat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
			v->setOverlaysEnabled(false);

			//rttTex->addListener(&mMirrorsListener);

			// set up linked reflection
			//mMirrorCam->enableReflection(mirrorPlane);
	//		mMirrorCam->enableCustomNearClipPlane(mirrorPlane);
		}
	}

void Mirrors::setActive(bool state)
{
	rttTex->setActive(state);
	if (state)
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirrortexture");
	else
		mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("mirror.dds");

}

void Mirrors::update(Vector3 normal, Vector3 center, Radian rollangle)
{
    //mMirrorCam->setOrientation(mCamera->getOrientation());
    //mMirrorCam->setPosition(mCamera->getPosition());
	//mMirrorCam->setFOVy(mCamera->getFOVy());
//	mMirrorCam->setNearClipDistance(mCamera->getNearClipDistance());
//	mMirrorCam->setFarClipDistance(mCamera->getFarClipDistance());
	//mirrorPlane=mplane;
//	mMirrorCam->enableReflection(mplane);
	Plane plane=Plane(normal, center);
	//mMirrorCam->enableCustomNearClipPlane(plane);
	//Vector3 project=plane.projectVector(center-mCamera->getPosition());
	//Vector3 perp=center-mCamera->getPosition()-project;
//	mMirrorCam->setPosition(2.0*center-mCamera->getPosition()+2.0*project); 
	//mMirrorCam->setPosition(mCamera->getPosition()+2*perp); 
	//mMirrorCam->lookAt(center);
	//mMirrorCam->setFOVy();
	//tnodeo->setPosition(center);
	//tnoden->setPosition(mMirrorCam->getPosition());
	mMirrorCam->setPosition(center); 
	Vector3 project=plane.projectVector(mCamera->getPosition()-center);
//	Vector3 upvec=mCamera->getUp(); 
//	Vector3 dirvec=center-(mCamera->getPosition()-2*project);
//	mMirrorCam->setOrientation(Quaternion(upvec.crossProduct(dirvec), upvec, dirvec)); 
	mMirrorCam->lookAt(mCamera->getPosition()-2*project); 
	mMirrorCam->roll(rollangle);
}

void Mirrors::prepareShutdown()
{
	//if (rttTex) rttTex->removeListener(&mMirrorsListener);
}


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
#include "ShadowManager.h"

#include "Ogre.h"
#include "OgreShadowCameraSetup.h"

#include "OgreTerrain.h"
#include "OgreTerrainMaterialGeneratorA.h"

#include "Settings.h"

//using namespace std;
//using namespace Ogre;

//---------------------------------------------------------------------
ShadowManager::ShadowManager(Ogre::SceneManager *scene, Ogre::RenderWindow *window, Ogre::Camera *camera) :
	mSceneMgr(scene), mWindow(window), mCamera(camera), mPSSMSetup()
{
	setSingleton(this);
	mDepthShadows = true;
}

ShadowManager::~ShadowManager()
{
}

void ShadowManager::loadConfiguration()
{
	Ogre::String s = SSETTING("Shadow technique", "Parallel-split Shadow Maps");
	if (s == "Stencil shadows")            changeShadowTechnique(Ogre::SHADOWTYPE_STENCIL_MODULATIVE);
	if (s == "Texture shadows")            changeShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE);
	if (s == "Parallel-split Shadow Maps") changeShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED);
}

int ShadowManager::changeShadowTechnique(Ogre::ShadowTechnique tech)
{
	float shadowFarDistance = std::min(200.0f, (FSETTING("SightRange", 2000)* 0.8f));
	float scoef=0.2;
	mSceneMgr->setShadowColour(Ogre::ColourValue(0.563+scoef, 0.578+scoef, 0.625+scoef));

	mSceneMgr->setShadowTechnique(tech);
	mSceneMgr->setShadowFarDistance(shadowFarDistance);
	mSceneMgr->setShowDebugShadows(false);

	if(tech == Ogre::SHADOWTYPE_STENCIL_MODULATIVE)
	{
		//		mSceneMgr->setShadowIndexBufferSize(2000000);
		mSceneMgr->setShadowDirectionalLightExtrusionDistance(100);

		//important optimization
		mSceneMgr->getRenderQueue()->getQueueGroup(Ogre::RENDER_QUEUE_WORLD_GEOMETRY_1)->setShadowsEnabled(false);

		//		mSceneMgr->setUseCullCamera(false);
		//		mSceneMgr->setShowBoxes(true);
		//		mSceneMgr->showBoundingBoxes(true);
	} else if(tech == Ogre::SHADOWTYPE_TEXTURE_MODULATIVE)
	{
		mSceneMgr->setShadowTextureSettings(2048,2);
	} else if(tech == Ogre::SHADOWTYPE_TEXTURE_MODULATIVE_INTEGRATED)
	{
#if OGRE_VERSION>0x010602



		// General scene setup

		// 3 textures per directional light (PSSM)
		int num = 3;

		mSceneMgr->setShadowTextureCountPerLightType(Ogre::Light::LT_DIRECTIONAL, num);

		if (mPSSMSetup.isNull())
		{
			// shadow camera setup
			Ogre::PSSMShadowCameraSetup* pssmSetup = new Ogre::PSSMShadowCameraSetup();
			pssmSetup->setSplitPadding(mCamera->getNearClipDistance());
			pssmSetup->calculateSplitPoints(3, mCamera->getNearClipDistance(), mSceneMgr->getShadowFarDistance());
			for (int i=0; i < num; ++i)
			{	int size = i==0 ? 2048 : 1024;
				const Ogre::Real cAdjfA[5] = {2, 1, 0.5, 0.25, 0.125};
				pssmSetup->setOptimalAdjustFactor(i, cAdjfA[std::min(i, 4)]);
			}
			mPSSMSetup.bind(pssmSetup);

		}
		mSceneMgr->setShadowCameraSetup(mPSSMSetup);
		
		
		mSceneMgr->setShadowTextureCount(num);
		for (int i=0; i < num; ++i)
		{	int size = i==0 ? 2048 : 1024;
			mSceneMgr->setShadowTextureConfig(i, size, size, mDepthShadows ? Ogre::PF_FLOAT32_R : Ogre::PF_X8B8G8R8);
		}

		mSceneMgr->setShadowTextureSelfShadow(mDepthShadows);
		mSceneMgr->setShadowCasterRenderBackFaces(false);

		mSceneMgr->setShadowTextureCasterMaterial(mDepthShadows?"PSSM/shadow_caster":Ogre::StringUtil::BLANK);

		updatePSSM();

#else
		showError("Parallel-split Shadow Maps as shadow technique is only available when you build with Ogre 1.6 support.", "PSSM error");
		exit(1);
#endif //OGRE_VERSION
	}
	return 0;
}

void ShadowManager::updatePSSM(Ogre::Terrain* terrain)
{
	if (!mPSSMSetup.get())  return;

	Ogre::TerrainMaterialGeneratorA::SM2Profile *matProfile  = 0;
	if(Ogre::TerrainGlobalOptions::getSingletonPtr())
	{
		matProfile = static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(Ogre::TerrainGlobalOptions::getSingleton().getDefaultMaterialGenerator()->getActiveProfile());
		matProfile->setReceiveDynamicShadowsEnabled(true);
		matProfile->setReceiveDynamicShadowsLowLod(true);
		matProfile->setGlobalColourMapEnabled(false);
	}


	Ogre::PSSMShadowCameraSetup* pssmSetup = static_cast<Ogre::PSSMShadowCameraSetup*>(mPSSMSetup.get());
	const Ogre::PSSMShadowCameraSetup::SplitPointList& splitPointList = pssmSetup->getSplitPoints();

	Ogre::Vector4 splitPoints;
	for (size_t i = 0; i < /*3*/splitPointList.size(); ++i)
		splitPoints[i] = splitPointList[i];

	// TODO: fix this
	//setMaterialSplitPoints("road", splitPoints);
	//setMaterialSplitPoints("road2", splitPoints);


	if (matProfile && terrain)
	{
		matProfile->generateForCompositeMap(terrain);
		matProfile->setReceiveDynamicShadowsDepth(mDepthShadows);
		matProfile->setReceiveDynamicShadowsPSSM(static_cast<Ogre::PSSMShadowCameraSetup*>(mPSSMSetup.get()));
	}
}

void ShadowManager::setMaterialSplitPoints(Ogre::String materialName, Ogre::Vector4 &splitPoints)
{
	Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().getByName(materialName);
	if (!mat.isNull())
	{
		unsigned short np = mat->getTechnique(0)->getNumPasses()-1;  // last
		try {
			mat->getTechnique(0)->getPass(np)->getFragmentProgramParameters()->setNamedConstant("pssmSplitPoints", splitPoints);
		} catch(...)
		{
			// this material is not prepared for PSSM usage !
		}
	}
}
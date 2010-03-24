#include "PlayerColours.h"
#include "Ogre.h"

using namespace Ogre;

// some colours with a good contrast inbetween
ColourValue cvals[] =
{
	ColourValue(0.0,0.8,0.0),
	ColourValue(0.0,0.4,0.701960784314),
	ColourValue(1.0,0.501960784314,0.0),
	ColourValue(1.0,0.8,0.0),
	ColourValue(0.2,0.0,0.6),
	ColourValue(0.6,0.0,0.6),
	ColourValue(0.8,1.0,0.0),
	ColourValue(1.0,0.0,0.0),
	ColourValue(0.501960784314,0.501960784314,0.501960784314),
	ColourValue(0.0,0.560784313725,0.0),
	ColourValue(0.0,0.282352941176,0.490196078431),
	ColourValue(0.701960784314,0.352941176471,0.0),
	ColourValue(0.701960784314,0.560784313725,0.0),
	ColourValue(0.419607843137,0.0,0.419607843137),
	ColourValue(0.560784313725,0.701960784314,0.0),
	ColourValue(0.701960784314,0.0,0.0),
	ColourValue(0.745098039216,0.745098039216,0.745098039216),
	ColourValue(0.501960784314,1.0,0.501960784314),
	ColourValue(0.501960784314,0.788235294118,1.0),
	ColourValue(1.0,0.752941176471,0.501960784314),
	ColourValue(1.0,0.901960784314,0.501960784314),
	ColourValue(0.666666666667,0.501960784314,1.0),
	ColourValue(0.933333333333,0.0,0.8),
	ColourValue(1.0,0.501960784314,0.501960784314),
	ColourValue(0.4,0.4,0.0),
	ColourValue(1.0,0.749019607843,1.0),
	ColourValue(0.0,1.0,0.8),
	ColourValue(0.8,0.4,0.6),
	ColourValue(0.6,0.6,0.0),
};

template<> PlayerColours *Singleton < PlayerColours >::ms_Singleton = 0;

PlayerColours::PlayerColours()
{
	updatePlayerColours();
}

PlayerColours::~PlayerColours()
{
}

Ogre::String PlayerColours::getColourMaterial(int colourNum)
{
	return "tracks/PlayerColours/"+StringConverter::toString(colourNum);
}

void PlayerColours::updateMaterial(int colourNum, String materialName, int textureUnitStateNum)
{
    int numColours = sizeof(cvals) / sizeof(ColourValue);
    if(colourNum < 0 || colourNum >= numColours) return;
    
	ColourValue cval = cvals[colourNum];
    
    MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
    if(mat.isNull()) return;
    if(mat->getNumTechniques()>0 && mat->getTechnique(0)->getNumPasses()>0 && mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates() > 0)
    {
        mat->getTechnique(0)->getPass(0)->getTextureUnitState(textureUnitStateNum)->setAlphaOperation(LBX_BLEND_CURRENT_ALPHA , LBS_MANUAL, LBS_CURRENT, 0.8);
        mat->getTechnique(0)->getPass(0)->getTextureUnitState(textureUnitStateNum)->setColourOperationEx(LBX_BLEND_CURRENT_ALPHA , LBS_MANUAL, LBS_CURRENT, cval, cval, 1);
    }
}

void PlayerColours::updatePlayerColours()
{
    int numColours = sizeof(cvals) / sizeof(ColourValue);
	for(int i=0;i<numColours;i++)
	{
		ColourValue cval = cvals[i];
		MaterialPtr mat = MaterialManager::getSingleton().getByName("tracks/PlayerColours/"+StringConverter::toString(i));
        if(mat.isNull()) continue;
		if(mat->getNumTechniques()>0 && mat->getTechnique(0)->getNumPasses()>0 && mat->getTechnique(0)->getPass(0)->getNumTextureUnitStates() > 0)
		{
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setAlphaOperation(LBX_BLEND_CURRENT_ALPHA , LBS_MANUAL, LBS_CURRENT, 0.8);
			mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setColourOperationEx(LBX_BLEND_CURRENT_ALPHA , LBS_MANUAL, LBS_CURRENT, cval, cval, 1);
		}
	}
}
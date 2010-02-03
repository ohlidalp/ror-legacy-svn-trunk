#ifndef ETM_H_
#define ETM_H_

#include <Ogre.h>
#include "ETTerrainManager.h"
#include "ETTerrainInfo.h"
#include "ETBrush.h"
#include "ETSplattingManager.h"

#define ETM EditableTerrain::getInstance()

class EditableTerrain
{
public:
	static EditableTerrain & getInstance();

	enum SmoothSampleType {SST_Small, SST_Large};

	bool deform(Ogre::Vector3 pos, float amount);
	bool smooth(Ogre::Vector3 pos, SmoothSampleType type);
	bool paint(Ogre::Vector3 pos, int pattern, float pressure);
	bool loadEditBrush(Ogre::String brushname);
	void updateLightmap(Ogre::Vector3 sunDirection=Ogre::Vector3::ZERO, Ogre::ColourValue lightColour=Ogre::ColourValue::ZERO, Ogre::ColourValue ambientColour=Ogre::ColourValue::ZERO);
	void setup(Ogre::SceneManager *smgr);
	void loadTerrain(Ogre::String geom, Ogre::Camera *cam);
	bool saveTerrain();
	Ogre::Material *getTerrainMaterial();
	ET::TerrainInfo *getTerrainInfo();

	~EditableTerrain();
protected:
	EditableTerrain();
	EditableTerrain(const EditableTerrain&);
	EditableTerrain& operator= (const EditableTerrain&);

	ET::Brush boxFilterPatch(int x, int z, int w, int h, enum SmoothSampleType type, ET::Brush intensity);
	float buildFactor(ET::Brush heights, int x, int y, float &factor);

	Ogre::SceneManager *mSceneMgr;
	Ogre::Camera *mCamera;
	ET::Brush mEditBrush;
	Ogre::String terrainConfigFile;
	ET::TerrainManager *mTerrainMgr;
	ET::TerrainInfo *terrainInfo;
	ET::SplattingManager* mSplatMgr;
	int width, height, scaleX, scaleZ, maxHeight;
	Ogre::String heighmapname, worldtext;
	
private:
    static EditableTerrain *myInstance;

};

#endif


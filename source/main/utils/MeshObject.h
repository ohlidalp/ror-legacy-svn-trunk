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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 1st of May 2010

#ifndef MESHOBJECT_H__
#define MESHOBJECT_H__

#include "RoRPrerequisites.h"

#include "Ogre.h"

#include "materialFunctionMapper.h"
#include "skin.h"
#include "MaterialReplacer.h"
#include "Settings.h"


class MeshObject : public Ogre::ResourceBackgroundQueue::Listener, public Ogre::Resource::Listener
{
public:
	MeshObject(Ogre::SceneManager *smgr, Ogre::String meshName, Ogre::String entityName, Ogre::SceneNode *sceneNode=0, Skin *s=0, bool backgroundLoading=false);
	~MeshObject();

	void setSimpleMaterialColour(Ogre::ColourValue c);
	void setMaterialFunctionMapper(MaterialFunctionMapper *m, MaterialReplacer *mr);
	void setMaterialName(Ogre::String m);
	void setCastShadows(bool b);
	void setMeshEnabled(bool e);
	void setVisible(bool b);
	Ogre::MeshPtr getMesh() { return mesh; };
	Ogre::Entity *getEntity() { return ent; };

protected:
	MaterialReplacer *mr;
	Ogre::SceneManager *smgr;
	Ogre::String meshName;
	Ogre::String entityName;
	Ogre::SceneNode *sceneNode;
	Ogre::Entity *ent;
	Ogre::MeshPtr mesh;
	Ogre::BackgroundProcessTicket ticket;
	bool backgroundLoading;
	bool loaded;
	
	Ogre::ColourValue simpleMatColour;
	bool enableSimpleMaterial;
	Ogre::String materialName;
	Skin *skin;
	bool castshadows;
	MaterialFunctionMapper *mfm;
	bool enabled;
	bool visible;

	void postProcess();
	void loadMesh();

	void operationCompleted(BackgroundProcessTicket ticket, const BackgroundProcessResult& result);
	void backgroundLoadingComplete(Resource *r);
	void backgroundPreparingComplete(Resource *r);
	void loadingComplete(Resource *r);
	void preparingComplete(Resource *r);
	void unloadingComplete(Resource *r);
};

#endif //MESHOBJECT_H__

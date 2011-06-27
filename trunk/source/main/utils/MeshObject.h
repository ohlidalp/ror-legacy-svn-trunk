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
	MeshObject(Ogre::SceneManager *smgr, Ogre::String meshName, Ogre::String entityName, Ogre::SceneNode *sceneNode=0, Skin *s=0, bool backgroundLoading=false)
		: mr(0)
		, smgr(smgr)
		, meshName(meshName)
		, entityName(entityName)
		, sceneNode(sceneNode)
		, ent(0)
		, backgroundLoading(backgroundLoading)
		, loaded(false)
		, enableSimpleMaterial(false)
		, materialName()
		, skin(s)
		, castshadows(true)
		, mfm(0)
		, enabled(true)
		, visible(true)
	{
		// create a new sceneNode if not existing
		if(!sceneNode)
			sceneNode = smgr->getRootSceneNode()->createChildSceneNode();

		loadMesh();
	}

	~MeshObject()
	{
		if(backgroundLoading && !mesh.isNull())
			mesh->unload();
	}

	void setSimpleMaterialColour(Ogre::ColourValue c)
	{
		simpleMatColour = c;
		enableSimpleMaterial = true;
		if(loaded && ent)
		{
			// already loaded, so do it afterwards manually
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ent, simpleMatColour);
		}
	}

	void setMaterialFunctionMapper(MaterialFunctionMapper *m, MaterialReplacer *mr)
	{
		if(!m) return;
		mfm = m;
		this->mr = mr;
		if(loaded && ent)
		{
			// already loaded, so do it afterwards manually
			mfm->replaceMeshMaterials(ent);
			mr->replaceMeshMaterials(ent);
		}
	}

	void setMaterialName(Ogre::String m)
	{
		if(m.empty()) return;
		materialName = m;
		if(loaded && ent)
		{
			ent->setMaterialName(materialName);
		}
	}

	void setCastShadows(bool b)
	{
		castshadows=b;
		if(loaded && sceneNode && ent && sceneNode->numAttachedObjects())
		{
			sceneNode->getAttachedObject(0)->setCastShadows(b);
		}
	}

	void setMeshEnabled(bool e)
	{
		setVisible(e);
		enabled = e;
	}

	void setVisible(bool b)
	{
		if(!enabled) return;
		visible = b;
		if(loaded && sceneNode)
			sceneNode->setVisible(b);
	}


	Ogre::MeshPtr getMesh() { return mesh; };
	Ogre::Entity *getEntity() { return ent; };
	//Ogre::SceneNode *getSceneNode() { return sceneNode; };

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



	void postProcess()
	{
		loaded=true;
		if(!sceneNode) return;

		// important: you need to add the LODs before creating the entity
		// now find possible LODs, needs to be done before calling createEntity()
		if(!mesh.isNull())
		{
			String basename, ext;
			StringUtil::splitBaseFilename(meshName, basename, ext);
			for(int i=1; i<4;i++)
			{
				String fn = basename + "_lod" + TOSTRING(i) + ".mesh";
				bool exists = ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(fn);
				if(!exists) continue;


				float distance = 3;

				// we need to tune this according to our sightrange
				float sightrange = PARSEREAL(SSETTING("SightRange"));

				if(sightrange > 4999)
				{
					// unlimited
					if     (i == 1) distance =  200;
					else if(i == 2) distance =  600;
					else if(i == 3) distance = 2000;
					else if(i == 4) distance = 5000;
				} else
				{
					// limited
					if     (i == 1) distance = std::max(20.0f, sightrange * 0.1f);
					else if(i == 2) distance = std::max(20.0f, sightrange * 0.2f);
					else if(i == 3) distance = std::max(20.0f, sightrange * 0.3f);
					else if(i == 4) distance = std::max(20.0f, sightrange * 0.4f);
				}

				Ogre::MeshManager::getSingleton().load(fn, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME),
					mesh->createManualLodLevel(distance, fn);
			}
		}

		// now create an entity around the mesh and attach it to the scene graph
		try
		{
			if(entityName.empty())
				ent = smgr->createEntity(meshName);
			else
				ent = smgr->createEntity(entityName, meshName);
			if(ent)
				sceneNode->attachObject(ent);
		} catch(Ogre::Exception& e)
		{
			LOG("error loading mesh: " + meshName + ": " + e.getFullDescription());
			return;
		}

		// then modify some things
		if(enableSimpleMaterial)
			MaterialFunctionMapper::replaceSimpleMeshMaterials(ent, simpleMatColour);

		if(skin)
			skin->replaceMeshMaterials(ent);

		if(mfm)
			mfm->replaceMeshMaterials(ent);

		if(!materialName.empty())
			ent->setMaterialName(materialName);

		// only set it if different from default (true)
		if(!castshadows && sceneNode && sceneNode->numAttachedObjects() > 0)
			sceneNode->getAttachedObject(0)->setCastShadows(castshadows);

		sceneNode->setVisible(visible);
	}

	void loadMesh()
	{
		Ogre::String resourceGroup = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
		mesh = static_cast<Ogre::MeshPtr>(Ogre::MeshManager::getSingleton().create(meshName, resourceGroup));
		if(backgroundLoading)
		{
			mesh->setBackgroundLoaded(true);
			mesh->addListener(this);
			ticket = Ogre::ResourceBackgroundQueue::getSingleton().load(
				Ogre::MeshManager::getSingletonPtr()->getResourceType(), 
				mesh->getName(), 
				resourceGroup,
				false,
				0,
				0,
				0);

			// try to load its textures in the background
			for(int i=0; i<mesh->getNumSubMeshes(); i++)
			{
				SubMesh *sm = mesh->getSubMesh(i);
				String materialName = sm->getMaterialName();
				Ogre::MaterialPtr mat = static_cast<Ogre::MaterialPtr>(Ogre::MaterialManager::getSingleton().getByName(materialName)); //, resourceGroup));
				if(mat.isNull()) continue;
				for(int tn=0; tn<mat->getNumTechniques(); tn++)
				{
					Technique *t = mat->getTechnique(tn);
					for(int pn=0; pn<t->getNumPasses(); pn++)
					{
						Pass *p = t->getPass(pn);
						for(int tun=0; tun<p->getNumTextureUnitStates(); tun++)
						{
							TextureUnitState *tu = p->getTextureUnitState(tun);
							String textureName = tu->getTextureName();
							// now add this texture to the background loading queue
							Ogre::TexturePtr tex = static_cast<Ogre::TexturePtr>(Ogre::TextureManager::getSingleton().create(textureName, resourceGroup));
							tex->setBackgroundLoaded(true);
							tex->addListener(this);
							ticket = Ogre::ResourceBackgroundQueue::getSingleton().load(
									Ogre::TextureManager::getSingletonPtr()->getResourceType(),
									tex->getName(),
									resourceGroup,
									false,
									0,
									0,
									0);

						}
					}
                            
				}
			}
		}
		
		if(!backgroundLoading)
			postProcess();
	}

	void operationCompleted(BackgroundProcessTicket ticket, const BackgroundProcessResult& result)
	{
		// NOT USED ATM
		LOG("operationCompleted: " + meshName);
		if(ticket == this->ticket)
			postProcess();
	}

	void backgroundLoadingComplete(Resource *r)
	{
		// deprecated, use loadingComplete instead
	}

	void backgroundPreparingComplete(Resource *r)
	{
		// deprecated, use preparingComplete instead
	}

	void loadingComplete(Resource *r)
	{
		LOG("loadingComplete: " + r->getName());
		postProcess();
	}

	void preparingComplete(Resource *r)
	{
		LOG("preparingComplete: " + r->getName());
	}

	void unloadingComplete(Resource *r)
	{
		LOG("unloadingComplete: " + r->getName());
	}
};

#endif //MESHOBJECT_H__

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

#include "skin.h"

#include "OgreLogManager.h"
#include "Ogre.h"
#include "CacheSystem.h"

using namespace Ogre;

Skin::Skin(ResourceManager* creator, const String& name, ResourceHandle handle, const String& group, bool isManual, ManualResourceLoader* loader) : 
	Ogre::Resource(creator, name, handle, group, isManual, loader)
{
	this->name = name;
	this->thumbnail = "";
	this->description = "";
	this->authorName = "";
	this->authorID = -1;
	//LOG("Skin::Skin("+name+")");
}

Skin::~Skin()
{
	//LOG("Skin::~Skin("+name+")");
}

void Skin::loadImpl()
{
	//LOG("Skin::loadImpl("+name+")");
}

void Skin::unloadImpl()
{
	//LOG("Skin::unloadImpl("+name+")");
}

size_t Skin::calculateSize() const
{
	//LOG("Skin::calculateSize("+name+")");
	return 0;
}

int Skin::addMaterialReplace(Ogre::String from, Ogre::String to)
{
	//LOG("Skin::addMaterialReplace("+from+","+to+")");
	replaceMaterials[from] = to;
	return 0;
}

int Skin::addTextureReplace(Ogre::String from, Ogre::String to)
{
	replaceTextures[from] = to;
	return 0;
}

int Skin::hasReplacementForMaterial(Ogre::String material)
{
	return (int)replaceMaterials.count(material);
}

int Skin::hasReplacementForTexture(Ogre::String texture)
{
	return (int)replaceTextures.count(texture);
}

Ogre::String Skin::getReplacementForTexture(Ogre::String texture)
{
	//LOG("Skin::getReplacementForMaterial("+material+") = " + replaceMaterials[material]);
	String res = replaceTextures[texture];
	if(res.empty()) res = texture;
	return res;
}

Ogre::String Skin::getReplacementForMaterial(Ogre::String material)
{
	//LOG("Skin::getReplacementForMaterial("+material+") = " + replaceMaterials[material]);
	String res = replaceMaterials[material];
	if(res.empty()) res = material;
	return res;
}

void Skin::replaceMaterialTextures(Ogre::String materialName)
{
	MaterialPtr mat = MaterialManager::getSingleton().getByName(materialName);
	if(!mat.isNull())
	{
		for(int t = 0; t < mat->getNumTechniques(); t++)
		{
			Technique *tech = mat->getTechnique(0);
			if(!tech) continue;
			for(int p=0; p < tech->getNumPasses(); p++)
			{
				Pass *pass = tech->getPass(p);
				if(!pass) continue;
				for(int tu = 0; tu < pass->getNumTextureUnitStates(); tu++)
				{
					TextureUnitState *tus = pass->getTextureUnitState(tu);
					if(!tus) continue;

					//if(tus->getTextureType() != TEX_TYPE_2D) continue; // only replace 2d images
					// walk the frames, usually there is only one
					for(int fr=0; fr<tus->getNumFrames(); fr++)
					{
						String textureName = tus->getFrameTextureName(fr);
						std::map<Ogre::String, Ogre::String>::iterator it = replaceTextures.find(textureName);
						if(it != replaceTextures.end())
						{
							textureName = it->second; //getReplacementForTexture(textureName);
							tus->setFrameTextureName(textureName, fr);
						}
					}
				}
			}
		}
	}
}

void Skin::replaceMeshMaterials(Ogre::Entity *e)
{
	if(!e) return;
	MeshPtr m = e->getMesh();
	if(!m.isNull())
	{
		for(int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			String materialName = sm->getMaterialName();
			std::map<Ogre::String, Ogre::String>::iterator it = replaceMaterials.find(materialName);
			if(it != replaceMaterials.end())
			{
				materialName = it->second;
				sm->setMaterialName(materialName);
			} else
			{
				// look for texture replacements
				replaceMaterialTextures(sm->getMaterialName());
			}
		}
	}

	for(int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		String materialName = subent->getMaterialName();
		std::map<Ogre::String, Ogre::String>::iterator it = replaceMaterials.find(materialName);
		if(it != replaceMaterials.end())
		{
			materialName = it->second;
			subent->setMaterialName(materialName);
		} else
		{
			// look for texture replacements
			replaceMaterialTextures(subent->getMaterialName());
		}
	}

}


bool Skin::operator==(const Skin& other) const
{
	return (this->name == other.name && this->thumbnail == other.thumbnail && this->authorID == other.authorID && this->authorName == other.authorName && this->description == other.description);
}

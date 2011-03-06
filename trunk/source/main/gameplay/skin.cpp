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

int Skin::hasReplacementForMaterial(Ogre::String material)
{
	//LOG("Skin::hasReplacementForMaterial("+material+") = " + TOSTRING((int)replaceMaterials.count(material)));
	int res = (int)replaceMaterials.count(material);
	if(!res)
		return (int)replaceMaterials.count(material);
	return res;
}

Ogre::String Skin::getReplacementForMaterial(Ogre::String material)
{
	//LOG("Skin::getReplacementForMaterial("+material+") = " + replaceMaterials[material]);
	String res = replaceMaterials[material];
	if(res.empty())
		return replaceMaterials[material];
	return res;
}

int Skin::serialize(Ogre::String &dst)
{
	std::map<Ogre::String, Ogre::String>::iterator it;
	dst += "\tName=" + this->name + "\n";
	if(!this->thumbnail.empty())   dst += "\tThumbnail=" + this->thumbnail + "\n";
	if(this->authorID != -1)       dst += "\tAuthorID=" + TOSTRING(this->authorID) + "\n";
	if(!this->authorName.empty())  dst += "\tAuthorName=" + this->authorName + "\n";
	if(!this->description.empty()) dst += "\tDescription=" + this->description + "\n";

	for(it = replaceMaterials.begin(); it != replaceMaterials.end(); it++)
		dst += "\tReplaceMaterial=" + it->first + " " + it->second + "\n";
	return 0;
}

bool Skin::operator==(const Skin& other) const
{
	return (this->name == other.name && this->thumbnail == other.thumbnail && this->authorID == other.authorID && this->authorName == other.authorName && this->description == other.description);
}

void Skin::replaceMeshMaterials(Ogre::Entity *e)
{

	MeshPtr m = e->getMesh();
	if(!m.isNull())
	{
		for(int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			SubMesh *sm = m->getSubMesh(n);
			if(this->hasReplacementForMaterial(sm->getMaterialName()))
			{
				String newMat = this->getReplacementForMaterial(sm->getMaterialName());
				//LOG("Skin: replaced mesh material " + sm->getMaterialName() + " with new new material " + newMat + " on entity " + e->getName());
				sm->setMaterialName(newMat);
			}
		}
	}

	for(int n=0; n<(int)e->getNumSubEntities();n++)
	{
		SubEntity *subent = e->getSubEntity(n);
		if(this->hasReplacementForMaterial(subent->getMaterialName()))
		{
			String newMat = this->getReplacementForMaterial(subent->getMaterialName());
			//LOG("Skin: replaced mesh material " + subent->getMaterialName() + " with new new material " + newMat + " on entity " + e->getName());
			subent->setMaterialName(newMat);
		}
	}

}


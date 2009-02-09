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

#include "skin.h"

#include "OgreLogManager.h"

using namespace Ogre;

Skin::Skin(ResourceManager* creator, const String& name, ResourceHandle handle, const String& group, bool isManual, ManualResourceLoader* loader)
{
	this->name = name;
	this->thumbnail = "";
	LogManager::getSingleton().logMessage("skin loaded");
}

Skin::~Skin()
{
	LogManager::getSingleton().logMessage("skin unloaded");
}

void Skin::loadImpl()
{
	LogManager::getSingleton().logMessage("skin loadImpl");
}

void Skin::unloadImpl()
{
	LogManager::getSingleton().logMessage("skin unloadImpl");
}

size_t Skin::calculateSize() const
{
	LogManager::getSingleton().logMessage("skin calculateSize");
	return 0;
}

int Skin::addMaterialReplace(Ogre::String from, Ogre::String to)
{
	replaceMaterials[from] = to;
	return 0;
}

String Skin::getName()
{
	return this->name;
}

int Skin::hasReplacementForMaterial(Ogre::String material)
{
	return (int)replaceMaterials.count(material);
}

Ogre::String Skin::getReplacementForMaterial(Ogre::String material)
{
	return replaceMaterials[material];
}

void Skin::setThumbnailImage(Ogre::String thumbnail)
{
	this->thumbnail = thumbnail;
}

Ogre::String Skin::getThumbnailImage()
{
	return this->thumbnail;
}

int Skin::serialize(Ogre::String &dst)
{
	std::map<Ogre::String, Ogre::String>::iterator it;
	dst += "\tname=" + this->name + "\n";
	dst += "\tthumbnail=" + this->thumbnail + "\n";
	dst += "\tsource=\n";
	for(it = replaceMaterials.begin(); it != replaceMaterials.end(); it++)
	{
		dst += "\treplaceMaterial=" + it->first + ", " + it->second + "\n";
	}
	return 0;
}
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

#include "OgreStableHeaders.h"

#include "skinmanager.h"
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include "OgreStringVector.h"
#include "OgreException.h"
#include "OgreResourceGroupManager.h"
#include "CacheSystem.h"\

using namespace Ogre;

//---------------------------------------------------------------------
template<> SkinManager * Singleton< SkinManager >::ms_Singleton = 0;
SkinManager* SkinManager::getSingletonPtr(void)
{
	return ms_Singleton;
}
SkinManager& SkinManager::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}
//---------------------------------------------------------------------
SkinManager::SkinManager() : ResourceManager()
{
	// Loading order
	mLoadOrder = 200.0f;
	// Scripting is supported by this manager
	mScriptPatterns.push_back("*.skin");
	// Register scripting with resource group manager
	ResourceGroupManager::getSingleton()._registerScriptLoader(this);

	// Resource type
	mResourceType = "RoRVehicleSkins";

	// Register with resource group manager
	ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);


}
//---------------------------------------------------------------------
SkinManager::~SkinManager()
{
	// Unregister with resource group manager
	ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
	// Unegister scripting with resource group manager
	ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);

}
//---------------------------------------------------------------------
Resource* SkinManager::createImpl(const String& name, ResourceHandle handle,
	const String& group, bool isManual, ManualResourceLoader* loader,
	const NameValuePairList* params)
{
	return new Skin(this, name, handle, group, isManual, loader);
}
//---------------------------------------------------------------------
void SkinManager::parseScript(DataStreamPtr& stream, const String& groupName)
{
	String line;
	SkinPtr pSkin;
	LogManager::getSingleton().logMessage("SkinManager::parseScript");

	while( !stream->eof() )
	{
		line = stream->getLine();
		// Ignore blanks & comments
		if( !line.length() || line.substr( 0, 2 ) == "//" )
		{
			continue;
		}
		else
		{
			if (pSkin.isNull())
			{
				// No current skin
				// So first valid data should be skin name
				pSkin = create(line, groupName);
				pSkin->_notifyOrigin(stream->getName());
				// Skip to and over next {
				stream->skipLine("{");
			}
			else
			{
				// Already in skin
				if (line == "}")
				{
					// Finished
					pSkin.setNull();
					// NB skin isn't loaded until required
				}
				else
				{
					parseAttribute(line, pSkin);
				}
			}
		}
	}
}

//---------------------------------------------------------------------
void SkinManager::parseAttribute(const String& line, SkinPtr& pSkin)
{
	std::vector<String> params = StringUtil::split(line);
	String& attrib = params[0];
	StringUtil::toLowerCase(attrib);
	if (attrib == "replacematerial")
	{
		// Check params
		if (params.size() != 3)
		{
			logBadAttrib(line, pSkin);
			return;
		}
		// Set
		pSkin->addMaterialReplace(params[1], params[2]);
	}
	if (attrib == "thumbnail")
	{
		// Check params
		if (params.size() != 2)
		{
			logBadAttrib(line, pSkin);
			return;
		}
		// Set
		pSkin->setThumbnailImage(params[1]);
	}
}

//---------------------------------------------------------------------
void SkinManager::logBadAttrib(const String& line, SkinPtr& pSkin)
{
	LogManager::getSingleton().logMessage("Bad attribute line: " + line +
		" in skin " + pSkin->getName());

}

int SkinManager::getMaterialAlternatives(Ogre::String materialName, std::vector<SkinPtr> &skinVector)
{
	Ogre::ResourceManager::ResourceMapIterator it = SkinManager::getSingleton().getResourceIterator();
	while (it.hasMoreElements())
	{
	   SkinPtr skin = it.getNext();

	   if (skin->hasReplacementForMaterial(materialName))
	   {
		  skinVector.push_back(skin);
	   }
	}
	return 0; // no errors
}


int SkinManager::getSkinCount()
{
	return (int)mResourcesByHandle.size();
}

int SkinManager::serialize(Ogre::String &dst)
{
	ResourceManager::ResourceHandleMap::iterator it;
	for(it = mResourcesByHandle.begin(); it != mResourcesByHandle.end(); it++)
	{
		dst += "skin\n{\n";
		int sres = ((SkinPtr)(it->second))->serialize(dst);
		dst += "\torigin=" + it->second->getOrigin() + "\n";
		String source = CACHE.getSkinSource(it->second->getOrigin());
		dst += "\tsource=" + source + "\n";
		dst += "}\n\n";
	}
	return 0;
}

int SkinManager::clear()
{
	mResourcesByHandle.clear();
	return 0;
}

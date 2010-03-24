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
#ifndef _Skin_H__
#define _Skin_H__

#include "OgrePrerequisites.h"
#include "OgreResource.h"
#include "OgreTexture.h"
#include "OgreMaterial.h"
#include "OgreCommon.h"

class Skin : public Ogre::Resource
{
public:
	/** Constructor.
	@see Resource
	*/
	Skin(Ogre::ResourceManager* creator, const Ogre::String& name, Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual = false, Ogre::ManualResourceLoader* loader = 0);
	virtual ~Skin();

	// we are lazy and wont use separate get/set
	Ogre::String name;
	Ogre::String thumbnail;
	Ogre::String description;
	Ogre::String authorName;
	Ogre::String skintype;
	Ogre::String source;
	Ogre::String sourcetype;
	Ogre::String origin;
	int authorID;
	bool loaded;

	int addMaterialReplace(Ogre::String from, Ogre::String to);
	int hasReplacementForMaterial(Ogre::String material);
	Ogre::String getReplacementForMaterial(Ogre::String material);
	void replaceMeshMaterials(Ogre::Entity *e);

	int serialize(Ogre::String &dst);
	bool operator==(const Skin& other) const;
protected:
	std::map<Ogre::String, Ogre::String> replaceMaterials;

	void loadImpl(void);
	void unloadImpl(void);
	size_t calculateSize(void) const;
};

/** Specialisation of SharedPtr to allow SharedPtr to be assigned to SkinPtr 
@note Has to be a subclass since we need operator=.
We could templatise this instead of repeating per Resource subclass, 
except to do so requires a form VC6 does not support i.e.
ResourceSubclassPtr<T> : public SharedPtr<T>
*/
class SkinPtr : public Ogre::SharedPtr<Skin> 
{
public:
	SkinPtr() : Ogre::SharedPtr<Skin>() {}
	explicit SkinPtr(Skin* rep) : Ogre::SharedPtr<Skin>(rep) {}
	SkinPtr(const SkinPtr& r) : Ogre::SharedPtr<Skin>(r) {} 
	SkinPtr(const Ogre::ResourcePtr& r) : Ogre::SharedPtr<Skin>()
	{
		// lock & copy other mutex pointer
		OGRE_MUTEX_CONDITIONAL(r.OGRE_AUTO_MUTEX_NAME)
		{
			OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
			OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
			pRep = static_cast<Skin*>(r.getPointer());
			pUseCount = r.useCountPointer();
			if (pUseCount)
			{
				++(*pUseCount);
			}
		}
	}

	/// Operator used to convert a ResourcePtr to a SkinPtr
	SkinPtr& operator=(const Ogre::ResourcePtr& r)
	{
		if (pRep == static_cast<Skin*>(r.getPointer()))
			return *this;
		release();
		// lock & copy other mutex pointer
		OGRE_MUTEX_CONDITIONAL(r.OGRE_AUTO_MUTEX_NAME)
		{
			OGRE_LOCK_MUTEX(*r.OGRE_AUTO_MUTEX_NAME)
			OGRE_COPY_AUTO_SHARED_MUTEX(r.OGRE_AUTO_MUTEX_NAME)
			pRep = static_cast<Skin*>(r.getPointer());
			pUseCount = r.useCountPointer();
			if (pUseCount)
			{
				++(*pUseCount);
			}
		}
		else
		{
			// RHS must be a null pointer
			assert(r.isNull() && "RHS must be null if it has no mutex!");
			setNull();
		}
		return *this;
	}
};

#endif

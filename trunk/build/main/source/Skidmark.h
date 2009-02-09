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
#ifndef SkidMark_H
#define SkidMark_H

#include "OgrePrerequisites.h"
#include "DynamicRenderable.h"
#include <vector>


class Skidmark : public DynamicRenderable
{
	typedef Ogre::Vector3 Vector3;
	typedef Ogre::Quaternion Quaternion;
	typedef Ogre::SceneManager SceneManager;
	typedef Ogre::Camera Camera;
	typedef Ogre::Real Real;
	typedef Ogre::RenderOperation::OperationType OperationType;

public:
	/// Constructor - see setOperationType() for description of argument.
	Skidmark(SceneManager *scm, OperationType opType=Ogre::RenderOperation::OT_LINE_STRIP);
	virtual ~Skidmark();

	/// Add a point to the point list
	void addPoint(const Ogre::Vector3 &p);
	/// Add a point to the point list
	void addPoint(Real x, Real y, Real z);

	/// Change the location of an existing point in the point list
	void setPoint(unsigned short index, const Vector3 &value);

	void setPoint2(const Vector3 &value);

	/// Return the location of an existing point in the point list
	const Vector3& getPoint(unsigned short index) const;

	/// Return the total number of points in the point list
	unsigned short getNumPoints(void) const;

	/// Remove all points from the point list
	void clear();

	/// Call this to update the hardware buffer after making changes.  
	void update();

	/** Set the type of operation to draw with.
	* @param opType Can be one of 
	*    - RenderOperation::OT_LINE_STRIP
	*    - RenderOperation::OT_LINE_LIST
	*    - RenderOperation::OT_POINT_LIST
	*    - RenderOperation::OT_TRIANGLE_LIST
	*    - RenderOperation::OT_TRIANGLE_STRIP
	*    - RenderOperation::OT_TRIANGLE_FAN
	*    The default is OT_LINE_STRIP.
	*/
	void setOperationType(OperationType opType);
	OperationType getOperationType() const;

protected:
	/// Implementation DynamicRenderable, creates a simple vertex-only decl
	virtual void createVertexDeclaration();
	/// Implementation DynamicRenderable, pushes point list out to hardware memory
	virtual void fillHardwareBuffers();

private:
	std::vector<Vector3> mPoints;
	bool mDirty;
	SceneManager *scm;
	Ogre::SceneNode *mNode;
	int pos;
};


#endif


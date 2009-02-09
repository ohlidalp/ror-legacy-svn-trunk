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

#include "Skidmark.h"
#include <Ogre.h>
#include <cassert>
#include <cmath>

using namespace Ogre;

enum {
  POSITION_BINDING,
  TEXCOORD_BINDING
};

Skidmark::Skidmark(SceneManager *scm, OperationType opType) : scm(scm), pos(0)
{
	initialize(opType,false);
	setMaterial("BaseWhiteNoLighting");
	mDirty = true;

	mNode = scm->getRootSceneNode()->createChildSceneNode();
	mNode->attachObject(this);
	
	for(int i=0; i<100; i++)
		addPoint(Vector3::ZERO);
	update();
}

Skidmark::~Skidmark()
{
}

void Skidmark::setOperationType(OperationType opType)
{
  mRenderOp.operationType = opType;
}

RenderOperation::OperationType Skidmark::getOperationType() const
{
  return mRenderOp.operationType;
}

void Skidmark::addPoint(const Vector3 &p)
{
   mPoints.push_back(p);
   mDirty = true;
}
void Skidmark::addPoint(Real x, Real y, Real z)
{
   mPoints.push_back(Vector3(x,y,z));
   mDirty = true;
}
const Vector3& Skidmark::getPoint(unsigned short index) const
{
   assert(index < mPoints.size() && "Point index is out of bounds!!");
   return mPoints[index];
}
unsigned short Skidmark::getNumPoints(void) const
{
  return (unsigned short)mPoints.size();
}

void Skidmark::setPoint(unsigned short index, const Vector3 &value)
{
  assert(index < mPoints.size() && "Point index is out of bounds!!");

  mPoints[index] = value;
  mDirty = true;
}

void Skidmark::setPoint2(const Vector3 &value)
{
	if(pos>=99)
		pos=0;
	setPoint(pos++, value);
}

void Skidmark::clear()
{
  mPoints.clear();
  mDirty = true;
}

void Skidmark::update()
{
  if (mDirty) fillHardwareBuffers();
  //mNode->needUpdate();
}

void Skidmark::createVertexDeclaration()
{
  VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;
  decl->addElement(POSITION_BINDING, 0, VET_FLOAT3, VES_POSITION);
}

void Skidmark::fillHardwareBuffers()
{
  int size = mPoints.size();

  prepareHardwareBuffers(size,0);

  if (!size) { 
	mBox.setExtents(Vector3::ZERO,Vector3::ZERO);
	mDirty=false;
	return;
  }
  
  Vector3 vaabMin = mPoints[0];
  Vector3 vaabMax = mPoints[0];

  HardwareVertexBufferSharedPtr vbuf =
	mRenderOp.vertexData->vertexBufferBinding->getBuffer(0);

  Real *prPos = static_cast<Real*>(vbuf->lock(HardwareBuffer::HBL_DISCARD));
  {
   for(int i = 0; i < size; i++)
   {
	  *prPos++ = mPoints[i].x;
	  *prPos++ = mPoints[i].y;
	  *prPos++ = mPoints[i].z;

	  if(mPoints[i].x < vaabMin.x)
		 vaabMin.x = mPoints[i].x;
	  if(mPoints[i].y < vaabMin.y)
		 vaabMin.y = mPoints[i].y;
	  if(mPoints[i].z < vaabMin.z)
		 vaabMin.z = mPoints[i].z;

	  if(mPoints[i].x > vaabMax.x)
		 vaabMax.x = mPoints[i].x;
	  if(mPoints[i].y > vaabMax.y)
		 vaabMax.y = mPoints[i].y;
	  if(mPoints[i].z > vaabMax.z)
		 vaabMax.z = mPoints[i].z;
   }
  }
  vbuf->unlock();

  mBox.setExtents(vaabMin, vaabMax);

  mDirty = false;
}

/*
void Skidmark::getWorldTransforms(Matrix4 *xform) const
{
   // return identity matrix to prevent parent transforms
   *xform = Matrix4::IDENTITY;
}
*/
/*
const Quaternion &Skidmark::getWorldOrientation(void) const
{
   return Quaternion::IDENTITY;
}

const Vector3 &Skidmark::getWorldPosition(void) const
{
   return Vector3::ZERO;
}
*/

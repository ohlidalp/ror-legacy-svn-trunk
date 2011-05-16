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
// created on 31th of July 2009 by Thomas Fischer
#include "as_ogre.h"

using namespace Ogre;
using namespace AngelScript;

// helper/wrapper functions first
static void Vector3DefaultConstructor(Vector3 *self)
{
	new(self) Vector3();
}

static void Vector3CopyConstructor(const Vector3 &other, Vector3 *self)
{
	new(self) Vector3(other);
}

static void Vector3InitConstructor(float x, float y, float z, Vector3 *self)
{
	new(self) Vector3(x,y,z);
}

static void Vector3InitConstructorScaler(float s, Vector3 *self)
{
	new(self) Vector3(s,s,s);
}

static int Vector3Cmp(const Vector3 &a, const Vector3 &b)
{
	if( (a.x > b.x) && (a.y > b.y) && (a.z > b.z) )
		return 1;
	else if( (a.x < b.x) && (a.y < b.y) && (a.z < b.z) )
		return -1;
	else
		return 0;
}

// main registration method
void registerOgreObjects(AngelScript::asIScriptEngine *engine)
{
	int r;

	// note: this is sorted by the appearance in OgreVector3.h
	// Ogre::Vector3
	r = engine->RegisterObjectType("vector3", sizeof(Ogre::Vector3), asOBJ_VALUE | asOBJ_POD | asOBJ_APP_CLASS_CA); assert( r >= 0 );

	// Register the object properties
	r = engine->RegisterObjectProperty("vector3", "float x", offsetof(Ogre::Vector3, x)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("vector3", "float y", offsetof(Ogre::Vector3, y)); assert( r >= 0 );
	r = engine->RegisterObjectProperty("vector3", "float z", offsetof(Ogre::Vector3, z)); assert( r >= 0 );

	// Register the object constructors
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,  "void f()",                    asFUNCTION(Vector3DefaultConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,  "void f(float, float, float)", asFUNCTION(Vector3InitConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,  "void f(const vector3 &in)",   asFUNCTION(Vector3CopyConstructor), asCALL_CDECL_OBJLAST); assert( r >= 0 );
	r = engine->RegisterObjectBehaviour("vector3", asBEHAVE_CONSTRUCT,  "void f(float)",               asFUNCTION(Vector3InitConstructorScaler), asCALL_CDECL_OBJLAST); assert( r >= 0 );

	// Register the object operators
	r = engine->RegisterObjectMethod("vector3", "float opIndex(int) const",                  asMETHODPR(Vector3, operator[], (size_t) const, float), asCALL_THISCALL);assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &f(const vector3 &in)",             asMETHODPR(Vector3, operator =, (const Vector3 &), Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "bool opEquals(const vector3 &in) const",    asMETHODPR(Vector3, operator==,(const Vector3&) const, bool), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("vector3", "vector3 opAdd(const vector3 &in) const",    asMETHODPR(Vector3, operator+,(const Vector3&) const, Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opSub(const vector3 &in) const",    asMETHODPR(Vector3, operator-,(const Vector3&) const, Vector3), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "vector3 opMul(const float &in) const",      asMETHODPR(Vector3, operator*,(const float) const, Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opMul(const vector3 &in) const",    asMETHODPR(Vector3, operator*,(const Vector3&) const, Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(const float &in) const",      asMETHODPR(Vector3, operator/,(const float) const, Vector3), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opDiv(const vector3 &in) const",    asMETHODPR(Vector3, operator/,(const Vector3&) const, Vector3), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("vector3", "vector3 opAdd() const",                     asMETHODPR(Vector3, operator+,() const, const Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 opSub() const",                     asMETHODPR(Vector3, operator-,() const, Vector3), asCALL_THISCALL); assert( r >= 0 );

	//r = engine->RegisterObjectMethod("vector3", "vector3 opMul(const float &in, const vector3 &in)", asMETHODPR(Vector3, operator*,(const float, const Vector3&), Vector3), asCALL_THISCALL); assert( r >= 0 );

	r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(const vector3 &in)",   asMETHODPR(Vector3,operator+=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opAddAssign(const float &in)",     asMETHODPR(Vector3,operator+=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(const vector3 &in)",   asMETHODPR(Vector3,operator-=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opSubAssign(const float &in)",     asMETHODPR(Vector3,operator-=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(const vector3 &in)",   asMETHODPR(Vector3,operator*=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opMulAssign(const float &in)",     asMETHODPR(Vector3,operator*=,(float),Vector3&), asCALL_THISCALL); assert( r >= 0 );

	//r = engine->RegisterObjectMethod("vector3", "vector3& operator @= ( const vector3& rkVector f( const Vector3& rkVector )", asMETHOD(Ogre::Vector3, f), asCALL_THISCALL); assert(r>=0);
	
	r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(const vector3 &in)",   asMETHODPR(Vector3,operator/=,(const Vector3 &),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 &opDivAssign(const float &in)",     asMETHODPR(Vector3,operator/=,(float),Vector3&), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "int opCmp(const vector3 &in) const",        asFUNCTION(Vector3Cmp), asCALL_CDECL_OBJFIRST); assert( r >= 0 );
	
	// Register the object methods
	// r = engine->RegisterObjectMethod("vector3", "void swap(vector3 &inout)",  asMETHOD(Vector3,swap), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "float length() const",        asMETHOD(Vector3,length), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "float squaredLength() const", asMETHOD(Vector3,squaredLength), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "float distance(const vector3 &in) const",        asMETHOD(Vector3,distance), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "float squaredDistance(const vector3 &in) const", asMETHOD(Vector3,squaredDistance), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "float dotProduct(const vector3 &in) const",    asMETHOD(Vector3,dotProduct), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "float absDotProduct(const vector3 &in) const", asMETHOD(Vector3,absDotProduct), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "float normalise()", asMETHOD(Vector3,normalise), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "float crossProduct(const vector3 &in) const", asMETHOD(Vector3,crossProduct), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 midPoint(const vector3 &in) const", asMETHOD(Vector3,midPoint), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "void makeFloor(const vector3 &in)", asMETHOD(Vector3,makeFloor), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "void makeCeil(const vector3 &in)", asMETHOD(Vector3,makeCeil), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 perpendicular() const", asMETHOD(Vector3,perpendicular), asCALL_THISCALL); assert( r >= 0 );
	// r = engine->RegisterObjectMethod("vector3", "vector3 randomDeviant(const radian &in, const vector3 &in) const", asMETHOD(Vector3,randomDeviant), asCALL_THISCALL); assert( r >= 0 );
	// r = engine->RegisterObjectMethod("vector3", "radian angleBetween(const vector3 &in)", asMETHOD(Vector3,angleBetween), asCALL_THISCALL); assert( r >= 0 );
	// r = engine->RegisterObjectMethod("vector3", "quaternion getRotationTo(const vector3 &in, const vector3 &in) const", asMETHOD(Vector3,getRotationTo), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "bool isZeroLength() const", asMETHOD(Vector3,isZeroLength), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 normalisedCopy() const", asMETHOD(Vector3,normalisedCopy), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "vector3 reflect(const vector3 &in) const", asMETHOD(Vector3,reflect), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "bool positionEquals(const vector3 &in, float &in) const",  asMETHOD(Vector3,positionEquals), asCALL_THISCALL); assert( r >= 0 );
	r = engine->RegisterObjectMethod("vector3", "bool positionCloses(const vector3 &in, float &in) const",  asMETHOD(Vector3,positionCloses), asCALL_THISCALL); assert( r >= 0 );
	// r = engine->RegisterObjectMethod("vector3", "bool directionEquals(const vector3 &in, radian &in) const", asMETHOD(Vector3,directionEquals), asCALL_THISCALL); assert( r >= 0 );
	
	r = engine->RegisterObjectMethod("vector3", "bool isNaN() const", asMETHOD(Vector3,isNaN), asCALL_THISCALL); assert( r >= 0 );
}

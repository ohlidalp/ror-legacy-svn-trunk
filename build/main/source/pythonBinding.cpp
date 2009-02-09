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

#ifdef PYTHONSCRIPT

#include "pythonBinding.h"
#include "ExampleFrameListener.h"
#include "gui_loader.h"
#include "collisions.h"
#include "Beam.h"

#include <Ogre.h>
#include <boost/python.hpp>
#include <frameobject.h> // for python's _frame object (used in traceback function)

using namespace boost::python;
using namespace Ogre;
using namespace std;

#define MAX_PYTHON_FILE_SIZE 12800 // = 100 kb

// boost <--> RoR interface Binding Class
BOOST_PYTHON_MODULE(RigsOfRods)
{
	class_<TruckCallBackWrap, boost::noncopyable>("TruckCallBack")
		.def("enterTruck", &TruckCallBack::enterTruck, &TruckCallBackWrap::default_enterTruck)
		.def("exitTruck", &TruckCallBack::exitTruck, &TruckCallBackWrap::default_exitTruck)
	;

	class_<Collisions>("collisions")
		.def("printStats", &Collisions::printStats)
	;

	enum_<blinktype>("blinktype")
		.value("none", BLINK_NONE)
		.value("left", BLINK_LEFT)
		.value("right", BLINK_RIGHT)
		.value("warn", BLINK_WARN)
	;


	class_<Beam>("beamtruck")
		.def("getTruckName", &Beam::getTruckName)
		.def("showSkeleton", &Beam::showSkeleton)
		.def("parkingbrakeToggle", &Beam::parkingbrakeToggle)
		.def("beaconsToggle", &Beam::beaconsToggle)
		.def("toggleCustomParticles", &Beam::toggleCustomParticles)
		.def("parkingbrakeToggle", &Beam::parkingbrakeToggle)
		.def("getBrakeLightVisible", &Beam::getBrakeLightVisible)
		.def("getReverseLightVisible", &Beam::getReverseLightVisible)
		.def("getCustomLightVisible", &Beam::getCustomLightVisible)
		.def("setCustomLightVisible", &Beam::setCustomLightVisible)
		.def("getBeaconMode", &Beam::getBeaconMode)
		.def("setBlinkType", &Beam::setBlinkType)
		.def("getBlinkType", &Beam::getBlinkType)
	
		.def("getHeadingDirectionAngle", &Beam::getHeadingDirectionAngle)
		.def("getCustomParticleMode", &Beam::getCustomParticleMode)
		.def("getLowestNode", &Beam::getLowestNode)
		.def_readwrite("brake", &Beam::brake)
		.def_readwrite("affforce", &Beam::affforce)
		.def_readwrite("ffforce", &Beam::ffforce)
		.def_readwrite("label", &Beam::label)
		.def_readwrite("WheelSpeed", &Beam::WheelSpeed)
		.def_readwrite("skeleton", &Beam::skeleton)
		//.def_readwrite("dircommand", &Beam::dircommand)
		//.def_readwrite("dirstate", &Beam::dirstate)
		.def_readwrite("replaymode", &Beam::replaymode)
		.def_readwrite("replaylen", &Beam::replaylen)
		.def_readwrite("replaypos", &Beam::replaypos)
		.def_readwrite("locked", &Beam::locked)
		.def_readwrite("cparticle_enabled", &Beam::cparticle_enabled)
		.def_readwrite("hookId", &Beam::hookId)
		.def_readwrite("free_node", &Beam::free_node)
		.def_readwrite("tied", &Beam::tied)
		.def_readwrite("canwork", &Beam::canwork)
		.def_readwrite("hashelp", &Beam::hashelp)
		.def_readwrite("minx", &Beam::minx)
		.def_readwrite("maxx", &Beam::maxx)
		.def_readwrite("miny", &Beam::miny)
		.def_readwrite("maxy", &Beam::maxy)
		.def_readwrite("minz", &Beam::minz)
		.def_readwrite("maxz", &Beam::maxz)
		.def_readwrite("state", &Beam::state)
		.def_readwrite("sleepcount", &Beam::sleepcount)
		.def_readwrite("driveable", &Beam::driveable)
		.def_readwrite("importcommands", &Beam::importcommands)
		.def_readwrite("rescuer", &Beam::rescuer)
		.def_readwrite("parkingbrake", &Beam::parkingbrake)
		.def_readwrite("lights", &Beam::lights)
		.def_readwrite("editorId", &Beam::editorId)
		.def_readwrite("leftMirrorAngle", &Beam::leftMirrorAngle)
		.def_readwrite("rightMirrorAngle", &Beam::rightMirrorAngle)
		.def_readwrite("refpressure", &Beam::refpressure)
		.def_readwrite("elevator", &Beam::elevator)
		.def_readwrite("rudder", &Beam::rudder)
		.def_readwrite("aileron", &Beam::aileron)
		.def_readwrite("flap", &Beam::flap)
		.def_readwrite("fusedrag", &Beam::fusedrag)
		.def_readwrite("fadeDist", &Beam::fadeDist)
		.def_readwrite("currentcamera", &Beam::currentcamera)
		.def_readwrite("freecinecamera", &Beam::freecinecamera)
		.def_readwrite("brakeforce", &Beam::brakeforce)
		.def_readwrite("ispolice", &Beam::ispolice)
		.def_readwrite("loading_finished", &Beam::loading_finished)
		.def_readwrite("freecamera", &Beam::freecamera)
		.def_readwrite("first_wheel_node", &Beam::first_wheel_node)
		.def_readwrite("netbuffersize", &Beam::netbuffersize)
		.def_readwrite("nodebuffersize", &Beam::nodebuffersize)
		.def_readwrite("free_wheel", &Beam::free_wheel)
		.def_readwrite("speedoMax", &Beam::speedoMax)
	;

	class_<pythonCommonInterface>("common")
		.def("registerTruckCallback", &pythonCommonInterface::registerTruckCallback)
		.def("logMessage", &pythonCommonInterface::logMessage)
		.def("getConfigPath", &pythonCommonInterface::getRoRConfigPath)
		.def("getInterfaceVersion", &pythonCommonInterface::getInterfaceVersion)
		.def("getRoRVersion", &pythonCommonInterface::getRoRVersion)
		.def("getRoRRevision", &pythonCommonInterface::getRoRRevision)
		.def("getNetProtocolVersion", &pythonCommonInterface::getNetProtocolVersion)
		.def("startTimer", &pythonCommonInterface::startTimer)
		.def("stopTimer", &pythonCommonInterface::stopTimer)
		.def("getTime", &pythonCommonInterface::getTime)
		.def("showChooser", &pythonCommonInterface::showChooser)
		.def("repairVehicle", &pythonCommonInterface::repairVehicle)
		.def("spawnObject", &pythonCommonInterface::spawnObject)
		.def("spawnBeam", &pythonCommonInterface::spawnBeam)
		.def("pointArrow", &pythonCommonInterface::pointArrow)
		//.def("getSceneMgr", &pythonCommonInterface::getSceneMgr)
		//.def("getRenderWindow", &pythonCommonInterface::getRenderWindow)
		//.def("getCollisions", &pythonCommonInterface::getCollisions)
		.def("getCurrentTruckNumber", &pythonCommonInterface::getCurrentTruckNumber)
		.def("getTruckCount", &pythonCommonInterface::getTruckCount)
		
		// pointer !
		.def("getLastLoadedTruck", &pythonCommonInterface::getLastLoadedTruck, return_value_policy<manage_new_object>())
		// pointer !
		.def("getTruck", &pythonCommonInterface::getTruck, return_value_policy<manage_new_object>())
		
		.def("runScript", &pythonCommonInterface::runScript)	
    ;

	class_<pythonGUIInterface>("gui")
		// thins wrappers to simulate default arguments
		.def("flashMessage", &pythonGUIInterface::flashMessage3)
		.def("flashMessage", &pythonGUIInterface::flashMessage2)
		.def("flashMessage", &pythonGUIInterface::flashMessage)
    ;

	class_<node_t>("node")
		.def_readwrite("mass", &node_t::mass)
		.def_readwrite("iPosition", &node_t::iPosition)
		.def_readwrite("AbsPosition", &node_t::AbsPosition)
		.def_readwrite("RelPosition", &node_t::RelPosition)
		.def_readwrite("smoothpos", &node_t::smoothpos)
		.def_readwrite("Velocity", &node_t::Velocity)
		.def_readwrite("Forces", &node_t::Forces)
		.def_readwrite("locked", &node_t::locked)
		.def_readwrite("iswheel", &node_t::iswheel)
		.def_readwrite("masstype", &node_t::masstype)
		.def_readwrite("contactless", &node_t::contactless)
		.def_readwrite("contacted", &node_t::contacted)
		.def_readwrite("friction", &node_t::friction)
		.def_readwrite("buoyancy", &node_t::buoyancy)
		.def_readwrite("lastdrag", &node_t::lastdrag)
		.def_readwrite("gravimass", &node_t::gravimass)
		.def_readwrite("lockednode", &node_t::lockednode)
		.def_readwrite("lockedPosition", &node_t::lockedPosition)
		.def_readwrite("lockedVelocity", &node_t::lockedVelocity)
		.def_readwrite("lockedForces", &node_t::lockedForces)
		.def_readwrite("wetstate", &node_t::wetstate)
		.def_readwrite("wettime", &node_t::wettime)
		.def_readwrite("isHot", &node_t::isHot)
		.def_readwrite("overrideMass", &node_t::overrideMass)
		.def_readwrite("buoyanceForce", &node_t::buoyanceForce)
		.def_readwrite("id", &node_t::id)
		.def_readwrite("colltesttimer", &node_t::colltesttimer)
		.def_readwrite("toblock", &node_t::toblock)
	;

	class_<beam_t>("beam")
		.def_readwrite("k", &beam_t::k)
		.def_readwrite("d", &beam_t::d)
		.def_readwrite("L", &beam_t::L)
		.def_readwrite("refL", &beam_t::refL)
		.def_readwrite("Lhydro", &beam_t::Lhydro)
		.def_readwrite("hydroRatio", &beam_t::hydroRatio)
		//.def_readwrite("hydroSpeed", &beam_t::hydroSpeed)
		.def_readwrite("broken", &beam_t::broken)
		.def_readwrite("bounded", &beam_t::bounded)
		.def_readwrite("shortbound", &beam_t::shortbound)
		.def_readwrite("longbound", &beam_t::longbound)
		.def_readwrite("commandRatioLong", &beam_t::commandRatioLong)
		.def_readwrite("commandRatioShort", &beam_t::commandRatioShort)
		.def_readwrite("commandShort", &beam_t::commandShort)
		.def_readwrite("commandLong", &beam_t::commandLong)
		.def_readwrite("stress", &beam_t::stress)
		.def_readwrite("maxposstress", &beam_t::maxposstress)
		.def_readwrite("maxnegstress", &beam_t::maxnegstress)
		.def_readwrite("default_deform", &beam_t::default_deform)
		.def_readwrite("strength", &beam_t::strength)
		.def_readwrite("diameter", &beam_t::diameter)
		.def_readwrite("lastforce", &beam_t::lastforce)
		.def_readwrite("isrope", &beam_t::isrope)
		.def_readwrite("iscentering", &beam_t::iscentering)
		.def_readwrite("isOnePressMode", &beam_t::isOnePressMode)
		.def_readwrite("isforcerestricted", &beam_t::isforcerestricted)
		.def_readwrite("autoMovingMode", &beam_t::autoMovingMode)
		.def_readwrite("autoMoveLock", &beam_t::autoMoveLock)
		.def_readwrite("pressedCenterMode", &beam_t::pressedCenterMode)
		.def_readwrite("centerLength", &beam_t::centerLength)
		.def_readwrite("disabled", &beam_t::disabled)
		.def_readwrite("minendmass", &beam_t::minendmass)
		.def_readwrite("update_timer", &beam_t::update_timer)
		.def_readwrite("update_rate", &beam_t::update_rate)
	;

	class_<flare_t>("flare")
		.def_readwrite("noderef", &flare_t::noderef)
		.def_readwrite("nodex", &flare_t::nodex)
		.def_readwrite("nodey", &flare_t::nodey)
		.def_readwrite("offsetx", &flare_t::offsetx)
		.def_readwrite("offsety", &flare_t::offsety)
		.def_readwrite("type", &flare_t::type)
		.def_readwrite("controlnumber", &flare_t::controlnumber)
		.def_readwrite("controltoggle_status", &flare_t::controltoggle_status)
		.def_readwrite("blinkdelay", &flare_t::blinkdelay)
		.def_readwrite("blinkdelay_curr", &flare_t::blinkdelay_curr)
		.def_readwrite("blinkdelay_state", &flare_t::blinkdelay_state)
		.def_readwrite("size", &flare_t::size)
		.def_readwrite("isVisible", &flare_t::isVisible)
	;

	class_<prop_t>("prop")
		.def_readwrite("noderef", &prop_t::noderef)
		.def_readwrite("nodex", &prop_t::nodex)
		.def_readwrite("nodey", &prop_t::nodey)
		.def_readwrite("offsetx", &prop_t::offsetx)
		.def_readwrite("offsety", &prop_t::offsety)
		.def_readwrite("offsetz", &prop_t::offsetz)
		.def_readwrite("rot", &prop_t::rot)
		.def_readwrite("wheelpos", &prop_t::wheelpos)
		.def_readwrite("mirror", &prop_t::mirror)
		.def_readwrite("beacontype", &prop_t::beacontype)
		.def_readwrite("pale", &prop_t::pale)
		.def_readwrite("spinner", &prop_t::spinner)
	;

	class_<cparticle_t>("particle")
		.def_readwrite("emitterNode", &cparticle_t::emitterNode)
		.def_readwrite("directionNode", &cparticle_t::directionNode)
		.def_readwrite("active", &cparticle_t::active)
	;

};

// boost <--> Ogre interface Binding Class
BOOST_PYTHON_MODULE(Ogre)
{
	// SceneManager Class

	// Vector3 Class
	class_<Ogre::Vector3>("Vector3")
		.def(init<float, float, float>())
		.def("length", &Ogre::Vector3::length)
		.def("squaredLength", &Ogre::Vector3::squaredLength)
		.def("distance", &Ogre::Vector3::distance)
		.def("squaredDistance", &Ogre::Vector3::squaredDistance)
		.def("dotProduct", &Ogre::Vector3::dotProduct)
		.def("absDotProduct", &Ogre::Vector3::absDotProduct)
		.def("normalise", &Ogre::Vector3::normalise)
		.def("crossProduct", &Ogre::Vector3::crossProduct)
		.def("midPoint", &Ogre::Vector3::midPoint)
		.def("crossProduct", &Ogre::Vector3::crossProduct)
		.def("makeFloor", &Ogre::Vector3::makeFloor)
		.def("makeCeil", &Ogre::Vector3::makeCeil)
		.def("perpendicular", &Ogre::Vector3::perpendicular)
		.def("randomDeviant", &Ogre::Vector3::randomDeviant)
		.def("getRotationTo", &Ogre::Vector3::getRotationTo)
		.def("isZeroLength", &Ogre::Vector3::isZeroLength)
		.def("normalisedCopy", &Ogre::Vector3::normalisedCopy)
		.def("reflect", &Ogre::Vector3::reflect)
		.def("positionEquals", &Ogre::Vector3::positionEquals)
		.def("positionCloses", &Ogre::Vector3::positionCloses)
		.def("directionEquals", &Ogre::Vector3::directionEquals)
		.def("positionCloses", &Ogre::Vector3::positionCloses)
		.def("positionCloses", &Ogre::Vector3::positionCloses)
		.def("positionCloses", &Ogre::Vector3::positionCloses)
		.def("positionCloses", &Ogre::Vector3::positionCloses)
		.def("positionCloses", &Ogre::Vector3::positionCloses)
		.def("positionCloses", &Ogre::Vector3::positionCloses)
		.def_readonly("ZERO", &Ogre::Vector3::ZERO)
		.def_readonly("UNIT_X", &Ogre::Vector3::UNIT_X)
		.def_readonly("UNIT_Y", &Ogre::Vector3::UNIT_Y)
		.def_readonly("UNIT_Z", &Ogre::Vector3::UNIT_Z)
		.def_readonly("NEGATIVE_UNIT_X", &Ogre::Vector3::NEGATIVE_UNIT_X)
		.def_readonly("NEGATIVE_UNIT_Y", &Ogre::Vector3::NEGATIVE_UNIT_Y)
		.def_readonly("NEGATIVE_UNIT_Z", &Ogre::Vector3::NEGATIVE_UNIT_Z)
		.def_readonly("UNIT_SCALE", &Ogre::Vector3::UNIT_SCALE)
		.def(self_ns::str(self)) // self_ns workaround: http://osdir.com/ml/python.c++/2003-05/msg00256.html
		.def(self + self)
		.def(self - self)
		.def(self * float())
		.def(self * self)
		.def(self / float())
		.def(self / self)
		.def(self += float())
		.def(self += self)
		.def(self -= float())
		.def(self -= self)
		.def(self *= float())
		.def(self *= self)
		.def(self /= float())
		.def(self < self)
		.def(self > self)
    ;
}

// Interface Class implementation below
pythonCommonInterface::pythonCommonInterface()
{
	this->efl = SCRIPTINGENGINE.getEflInstance();
}

int pythonCommonInterface::registerTruckCallback(int truckNum, TruckCallBackWrap callback)
{
	return SCRIPTINGENGINE.registerTruckCallback(truckNum, callback);
}

void pythonCommonInterface::logMessage(char *msg)
{
	if(!msg) return;
	LogManager::getSingleton().logMessage("PYTHON| "+String(msg));
}

Ogre::String pythonCommonInterface::getRoRConfigPath()
{
	return ExampleFrameListener::getRoRConfigPath();
}

Ogre::String pythonCommonInterface::getInterfaceVersion()
{
	return String("0.1.0");
}

void pythonCommonInterface::startTimer()
{
	if(!efl) return;
	efl->startTimer();
}

float pythonCommonInterface::stopTimer()
{
	if(!efl) return 0;
	return efl->stopTimer();
}

float pythonCommonInterface::getTime()
{
	if(!efl) return 0;
	return efl->getTime();
}

int pythonCommonInterface::showChooser(char *type, char *instance, char *box)
{
	if(!type || !instance || !box || !efl) return -1;
	int ntype=-1;
	if (!strcmp("vehicle", type))   ntype = GUI_Loader::LT_Vehicle;
	if (!strcmp("truck", type))     ntype = GUI_Loader::LT_Truck;
	if (!strcmp("car", type))       ntype = GUI_Loader::LT_Truck;
	if (!strcmp("boat", type))      ntype = GUI_Loader::LT_Boat;
	if (!strcmp("airplane", type))  ntype = GUI_Loader::LT_Airplane;
	if (!strcmp("heli", type))      ntype = GUI_Loader::LT_Heli;
	if (!strcmp("trailer", type))   ntype = GUI_Loader::LT_Trailer;
	if (!strcmp("load", type))      ntype = GUI_Loader::LT_Load;
	if (!strcmp("extension", type)) ntype = GUI_Loader::LT_Extension;
	if (ntype!=-1) 
		efl->showLoad(ntype, instance, box);
	return 0;
}

int pythonCommonInterface::repairVehicle(char *instance, char *box)
{
	if(!instance || !box || !efl) return -1;
	efl->repairTruck(instance, box);
	return 0;
}

int pythonCommonInterface::spawnObject(char *objectName, char *instanceName, Ogre::Vector3 position, Ogre::Vector3 rotation)
{
	if(!objectName || !instanceName || !efl) return -1;
	SceneNode *bakeNode=efl->getSceneMgr()->getRootSceneNode()->createChildSceneNode();
	efl->loadObject(objectName, position.x, position.y, position.z, rotation.x, rotation.y, rotation.z, bakeNode, instanceName, true, -1, objectName);
	return 0;
}

int pythonCommonInterface::spawnBeam(char *objectName, char *instanceName, Ogre::Vector3 position, Ogre::Vector3 rotation)
{
	if(!objectName || !instanceName || !efl) return -1;
	Ogre::LogManager::getSingleton().logMessage("PYTHON| loading beam " + String(objectName));
	efl->addTruck(objectName, position);
	return 0;
}

int pythonCommonInterface::pointArrow(Ogre::Vector3 position, char *text)
{
	efl->setDirectionArrow(text, position);
	return 0;
}

Ogre::String pythonCommonInterface::getNetProtocolVersion()
{
	return String(RORNET_VERSION);
}

Ogre::String pythonCommonInterface::getRoRVersion()
{
	return String(ROR_VERSION_STRING);
}

Ogre::String pythonCommonInterface::getRoRRevision()
{
	return String(REVISION);
}

Collisions *pythonCommonInterface::getCollisions()
{
	if(!efl) return 0;
	return efl->getCollisions();
}

Beam *pythonCommonInterface::getTruck(int number)
{
	if(!efl || number<0) return NULL;
	return efl->getTruck(number);
}

Beam *pythonCommonInterface::getLastLoadedTruck()
{
	if(!efl) return NULL;
	int number = getTruckCount();
	if(number<0) return NULL;
	return efl->getTruck(number);
}

int pythonCommonInterface::getCurrentTruckNumber()
{
	if(!efl) return -1;
	return efl->getCurrrentTruckNumber(); // d'oh damn typo :|
}

int pythonCommonInterface::getTruckCount()
{
	if(!efl) return -1;
	return efl->getTruckCount();
}

int pythonCommonInterface::runScript(char *filename)
{
	if(!filename) return -1;
	return SCRIPTINGENGINE.runScript(String(filename));
}


pythonGUIInterface::pythonGUIInterface()
{
	this->efl = SCRIPTINGENGINE.getEflInstance();
}

int pythonGUIInterface::flashMessage3(char *msg, float time, float charHeight)
{
	if(!msg || !efl) return -1;
	efl->flashMessage(msg, time, charHeight);
	return 0;
}

int pythonGUIInterface::flashMessage2(char *msg, float time)
{
	// thin wrapper
	if(!msg || !efl) return -1;
	return flashMessage3(msg, time);
}

int pythonGUIInterface::flashMessage(char *msg)
{
	// thin wrapper
	if(!msg || !efl) return -1;
	return flashMessage3(msg);
}


// scriptingEngine below
ScriptingEngine* ScriptingEngine::myInstance = 0;

ScriptingEngine & ScriptingEngine::Instance () 
{
	if (myInstance == 0)
		myInstance = new ScriptingEngine;
	return *myInstance;
}

// constructor
ScriptingEngine::ScriptingEngine() : mefl(0)
{
}

// called to setup
void ScriptingEngine::setup(ExampleFrameListener *efl)
{
	mefl = efl;
	// http://www.python.org/doc/current/api/initialization.html#l2h-652
	Py_Initialize();

	initClasses();

	// run base python file
	runScript("base.py");
}

ScriptingEngine::~ScriptingEngine()
{
	Py_Finalize();
}

void ScriptingEngine::initClasses()
{
	// init our python interface class
	try
	{
		PyImport_AddModule("__main__");
		main_module = import("__main__");
		initRigsOfRods();
		initOgre();
	} catch(...)
	{
		Ogre::LogManager::getSingleton().logMessage(getPythonException());
		PyErr_Print();
  	}
}

int ScriptingEngine::registerTruckCallback(int truckNum, TruckCallBackWrap &callback)
{
	Ogre::LogManager::getSingleton().logMessage("new callback registered for truck "+Ogre::StringConverter::toString(truckNum));
	truckCallbacks[truckNum].push_back(callback);
	//Ogre::LogManager::getSingleton().logMessage("calling...");
	//Ogre::LogManager::getSingleton().logMessage(Ogre::StringConverter::toString(callback.enter()));
	return 0;
}

int ScriptingEngine::fireTruckEvent(int truckNum, Ogre::String type)
{
	Ogre::LogManager::getSingleton().logMessage("fireTruckEvent: "+Ogre::StringConverter::toString(truckNum)+" / "+type);
	std::vector<TruckCallBackWrap>::iterator it;
	for(it=truckCallbacks[truckNum].begin(); it!=truckCallbacks[truckNum].end(); it++)
	{
	// init our python interface class
	try
	{
		if(type == "enterTruck")
			(*it).enterTruck();
		else if(type == "exitTruck")
			(*it).exitTruck();
	} catch(...)
	{
		Ogre::LogManager::getSingleton().logMessage(getPythonException());
		PyErr_Print();
  	}
	}
	return 0;
}

int ScriptingEngine::runScript(Ogre::String filename)
{
	String content="";
	int res = readFile(filename, content);
	if(res)
		return res;
	try
	{
		// get main module
		object main_dict = main_module.attr("__dict__");
		//Ogre::LogManager::getSingleton().logMessage("content: >>"+content+"<<");
		handle<> ignored((PyRun_String(content.c_str(), Py_file_input, main_dict.ptr(), main_dict.ptr())));
	} catch(...)
	{
		Ogre::LogManager::getSingleton().logMessage(getPythonException());
		PyErr_Print();
  	}
	return 0;
}

ExampleFrameListener *ScriptingEngine::getEflInstance()
{
	return mefl;
}

int ScriptingEngine::readFile(Ogre::String filename, Ogre::String &content)
{
	Ogre::LogManager::getSingleton().logMessage(" ** trying to read python File: " + filename);
	String group="";
	try
	{
		group = ResourceGroupManager::getSingleton().findGroupContainingResource(String(filename));
	}catch(...)
	{
	}
	if(group == "")
	{
		Ogre::LogManager::getSingleton().logMessage(" ** File not found: " + filename);
		return -1;
	}
	// create some buffer
	char *buffer = (char*)malloc(MAX_PYTHON_FILE_SIZE);
	memset(buffer,0, MAX_PYTHON_FILE_SIZE);

	// open file and read the content into the buffer
	DataStreamPtr ds=ResourceGroupManager::getSingleton().openResource(filename, group);
	
	// just to make gcc happy:
	//size_t size = ds->read(buffer, ds->size());
	ds->read(buffer, ds->size());
	
	// important: convert windows \r\n newlines to linux \n otherwise the script parsing will fail
	// we will convert \r\n to ' \n' which should work out well
	char *ptr = buffer;
	while (*ptr!=0 && *(ptr+1)!=0)
	{
		if (*ptr=='\r' && *(ptr+1)=='\n') 
		{
			*ptr=' ';
			ptr++;
		}
		ptr++;
	}
	// done converting
	content = String(buffer);
	free(buffer);
	Ogre::LogManager::getSingleton().logMessage(" ** File successfully read: " + filename);
	return 0;
}

std::string ScriptingEngine::printTB(PyObject *f)
{
        PyTracebackObject *tb = (PyTracebackObject *)f;
        int err = 0;
        std::ostringstream os;
        while (tb != NULL && err == 0)
        {
                os << "  File\x22"<<PyString_AsString(tb->tb_frame->f_code->co_filename)
                   << "\x22, line "<<tb->tb_lineno<<", in "
                   << PyString_AsString(tb->tb_frame->f_code->co_name)
                   << "\n";
                tb = tb->tb_next;
                if (err == 0)
                       err = PyErr_CheckSignals();
        }
        os.flush();
       return os.str();
}

std::string ScriptingEngine::getPythonException()
{
        PyObject* type = 0;
        PyObject* value = 0;
        PyObject* tb = 0;
        PyErr_Fetch(&type,&value,&tb);
        try {
                handle<> type_o( allow_null(type) );
                handle<> value_o(value);
                handle<> tb_o( allow_null(tb) );

				handle<> type_str(PyObject_Str(type_o.get()));
                handle<> value_str(PyObject_Str(value_o.get()));
                handle<> tb_str(PyObject_Str(tb_o.get()));

                std::string ret = "Error in python execution:\n";
                if (tb_str.get()!=0)
                        ret += "Traceback (most recent call last):\n" +
printTB(tb_o.get());

                std::string type_string =
PyString_AsString(type_str.get());
                if (type_string.find("exceptions.")==0)
                        type_string=type_string.substr(11);
                ret += type_string + ": ";
                ret += PyString_AsString(value_str.get());
                ret += "\n";
                return ret;
        }
        catch (error_already_set&)
        {
                return "Error during python exception elucidation.";
        }
}


#endif // PYTHONSCRIPT

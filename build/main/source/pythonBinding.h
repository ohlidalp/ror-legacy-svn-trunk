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

#ifndef PYTHONBINDING_H
#define PYTHONBINDING_H

// $LastChangedDate: 2009-02-08 19:49:52 +0000 (Sun, 08 Feb 2009) $
// $LastChangedRevision: 1891 $
// $LastChangedBy: pricorde $
// $Id: pythonBinding.h 1891 2009-02-08 19:49:52Z pricorde $
// $Rev: 1891 $
#define REVISION "$Rev: 1891 $"


#include <OgrePrerequisites.h>
#include <Python.h> // for pyObject
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/utility.hpp>
#include <string>

// forward declarations
class ExampleFrameListener;
class Collisions;
class Beam;

// Callback class for trucks
struct TruckCallBack
{
	virtual ~TruckCallBack() {}
	virtual int enterTruck() { return -1; }
	virtual int exitTruck() { return -1; }
};

// the wrapper for it
struct TruckCallBackWrap : TruckCallBack, boost::python::wrapper<TruckCallBack>
{
	int enterTruck()
	{
		if (boost::python::override enterTruck = this->get_override("enterTruck"))
			return enterTruck();
		return TruckCallBack::enterTruck();
	}

	int exitTruck()
	{
		if (boost::python::override exitTruck = this->get_override("exitTruck"))
			return exitTruck();
		return TruckCallBack::exitTruck();
	}

	int default_enterTruck() { return this->TruckCallBack::enterTruck(); }
	int default_exitTruck() { return this->TruckCallBack::exitTruck(); }
};

// exposed classes
class pythonCommonInterface
{
protected:
	ExampleFrameListener *efl;
public:
	pythonCommonInterface();
    Ogre::String getRoRConfigPath();
    Ogre::String getInterfaceVersion();
    Ogre::String getRoRVersion();
    Ogre::String getRoRRevision();
    Ogre::String getNetProtocolVersion();
    void logMessage(char *msg);
	void startTimer();
	float stopTimer();
	float getTime();
	int showChooser(char *type, char *instance, char *box);
	int repairVehicle(char *instance, char *box);
	int spawnObject(char *objectName, char *instanceName, Ogre::Vector3 position, Ogre::Vector3 rotation);
	int spawnBeam(char *objectName, char *instanceName, Ogre::Vector3 position, Ogre::Vector3 rotation);
	int pointArrow(Ogre::Vector3 position, char *text);
	Collisions *getCollisions();
	int getCurrentTruckNumber();
	int getTruckCount();
	Beam *getTruck(int number);
	Beam *getLastLoadedTruck();
	int runScript(char *filename);
	int registerTruckCallback(int truckNum, TruckCallBackWrap callback);
};

class pythonGUIInterface
{
protected:
	ExampleFrameListener *efl;
public:
	pythonGUIInterface();
	// thin wrapper to simulate default arguments
	int flashMessage(char *msg);
	int flashMessage2(char *msg, float time);
	int flashMessage3(char *msg, float time=1, float charHeight=-1);
};

// main script handling class
#define SCRIPTINGENGINE ScriptingEngine::Instance()
class ScriptingEngine
{
public:
	static ScriptingEngine & Instance();
	void setup(ExampleFrameListener *efl);

	int runScript(Ogre::String filename);

	ExampleFrameListener *getEflInstance();

	int registerTruckCallback(int truckNum, TruckCallBackWrap &cb);
	int fireTruckEvent(int truckNum, Ogre::String type);

protected:
	ScriptingEngine();
	~ScriptingEngine();
	ScriptingEngine(const ScriptingEngine&);
	ScriptingEngine& operator= (const ScriptingEngine&);
	static ScriptingEngine* myInstance;

	ExampleFrameListener *mefl;
	int readFile(Ogre::String filename, Ogre::String &content);
	void initClasses();
	std::string getPythonException();
	std::string printTB(PyObject *f);
	boost::python::object main_module;
	std::map<int, std::vector<TruckCallBackWrap> > truckCallbacks;
};

#endif // PYTHONBINDING_H

#else // PYTHONSCRIPT
// nice workaround to disable the shortcut completely
#define SCRIPTINGENGINE
#endif // PYTHONSCRIPT

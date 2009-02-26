/* zlib/libpng license below:
Copyright (c) 2004-2009 Pierre-Michel Ricordel (pricorde{AT}rigsofrods{DOT}com), Thomas Fischer (thomas{AT}rigsofrods{DOT}com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

#ifndef __InputEngine_H__
#define __InputEngine_H__


#include "OISEvents.h"
#include "OISInputManager.h"
#include "OISMouse.h"
#include "OISKeyboard.h"
#include "OISJoyStick.h"
//#include <OgreFrameListener.h>
#include <OgreWindowEventUtilities.h>
#include <map>

//class ExampleFrameListener;
// some shortcut
#define INPUTENGINE InputEngine::Instance()

// config filename
#define CONFIGFILENAME "input.map"

enum grabtypes {
	GRAB_ALL=0,
	GRAB_DYNAMICALLY,
	GRAB_NONE
};

enum eventtypes
{
	ET_NONE=0,
	ET_Keyboard,
	ET_MouseButton,
	ET_MouseAxisX,
	ET_MouseAxisY,
	ET_MouseAxisZ,
	ET_JoystickButton,
	ET_JoystickAxisAbs,
	ET_JoystickAxisRel,
	ET_JoystickPov,
	ET_JoystickSliderX,
	ET_JoystickSliderY
};

typedef struct
{
	// general
	enum eventtypes eventtype;
	// keyboard
	int keyCode;
	bool explicite;
	bool ctrl;
	bool shift;
	bool alt;
	//mouse
	int mouseButtonNumber;
	//joystick buttons
	int joystickNumber;
	int joystickButtonNumber;
	// joystick axis
	int joystickAxisNumber;
	float joystickAxisDeadzone;
	float joystickAxisLinearity;
	int  joystickAxisRegion;
	bool joystickAxisReverse;
	bool joystickAxisHalf;
	bool joystickAxisUseDigital;
	// POVs
	int joystickPovNumber;
	int joystickSliderNumber;
	
	//others
	std::string configline;
	std::string group;
	std::string tmp_eventname;
	std::string comments;
	int suid; //session unique id
} event_trigger_t;

class InputEngine : public OIS::MouseListener, public OIS::KeyListener, public OIS::JoyStickListener
{
public:
	static InputEngine & Instance();
	void Capture();
	float getEventValue(Ogre::String eventName);
	bool getEventBoolValue(Ogre::String eventName);
	static bool instanceExists() ;	
	// we need to use hwnd here, as we are also using this in the configurator
	bool setup(size_t hwnd, bool capture=false, bool capturemouse=false, int grabMode=0);
	Ogre::String getKeyForCommand(Ogre::String evName);
	bool isKeyDown(OIS::KeyCode mod);

	std::map<std::string, std::vector<event_trigger_t> > &getEvents() { return events; };
	std::string getEventTypeName(int type);

	int getCurrentKeyCombo(std::string *combo);
	int getCurrentJoyButton();
	std::string getKeyNameForKeyCode(OIS::KeyCode keycode);
	void resetKeys();
	OIS::JoyStickState *getCurrentJoyState();
	void smoothValue(float &ref, float value, float rate);
	bool saveMapping(std::string outfile=CONFIGFILENAME);
	bool appendLineToConfig(std::string line, std::string outfile=CONFIGFILENAME);
	bool loadMapping(std::string outfile=CONFIGFILENAME, bool append=false);

	void destroy();

	Ogre::String getEventCommand(Ogre::String eventName);
	event_trigger_t *getEventBySUID(int suid);
	
	bool isEventDefined(Ogre::String eventName);
	void addEvent(Ogre::String eventName, event_trigger_t t);
	bool deleteEventBySUID(int suid);
	Ogre::String getKeyLine() { return keyInput; };
	void resetKeyLine() { keyInput=""; };
	void setRecordInput(bool value) { recordChat=value; };
	bool getInputsChanged() { return inputsChanged; };
	void prepareShutdown();
	OIS::MouseState getMouseState();
	// some custom methods
	void windowResized(Ogre::RenderWindow* rw);
	
	bool reloadConfig(std::string outfile=CONFIGFILENAME);
	bool updateConfigline(event_trigger_t *t);

	void grabMouse(bool enable);
	void hideMouse(bool visible);
	void setMousePosition(int x, int y, bool padding=true);

	int getKeboardKeyForCommand(Ogre::String val);

protected:
	InputEngine();
	~InputEngine();
	InputEngine(const InputEngine&);
	InputEngine& operator= (const InputEngine&);
	static InputEngine* myInstance;

	//OIS Input devices
	OIS::InputManager* mInputManager;
	OIS::Mouse*    mMouse;
	OIS::Keyboard* mKeyboard;
	OIS::JoyStick* mJoy;

	// JoyStickListener
	bool buttonPressed( const OIS::JoyStickEvent &arg, int button );
	bool buttonReleased( const OIS::JoyStickEvent &arg, int button );
	bool axisMoved( const OIS::JoyStickEvent &arg, int axis );
	bool sliderMoved( const OIS::JoyStickEvent &, int );
	bool povMoved( const OIS::JoyStickEvent &, int );

	// KeyListener
	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );

	// MouseListener
	bool mouseMoved( const OIS::MouseEvent &arg );
	bool mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id );
	bool mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id );

	// this stores the key/button/axis values
	std::map<int, bool> keyState;
	OIS::JoyStickState joyState;
	OIS::MouseState mouseState;

	// define event aliases
	std::map<std::string, std::vector<event_trigger_t> > events;


	bool processLine(char *line);
	bool captureMode;

	//ExampleFrameListener *mefl;

	void initAllKeys();
	std::map<std::string, OIS::KeyCode> allkeys;
	std::map<std::string, OIS::KeyCode>::iterator allit;

	float deadZone(float axis, float dz);
	float logval(float val);
	std::string getEventGroup(std::string eventName);
	bool mappingLoaded;
	
	Ogre::String keyInput;
	bool recordChat;
	bool inputsChanged;
	bool fileExists(char* filename);
	int grabMode;
};

#endif

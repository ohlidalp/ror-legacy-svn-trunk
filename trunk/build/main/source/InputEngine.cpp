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
#include "InputEngine.h"

#include <Ogre.h>
#include <OgreStringConverter.h>
#include <OgreException.h>
#include <OgreWindowEventUtilities.h>
#include "Settings.h"

#ifndef NOOGRE
#include "IngameConsole.h"
#include "gui_manager.h"
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
#define strnlen(str,len) strlen(str)
#endif

// using head for all OS's
#define OIS_HEAD 1


//Use this define to signify OIS will be used as a DLL
//(so that dll import/export macros are in effect)
#define OIS_DYNAMIC_LIB
#include <OIS.h>

#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
#include <X11/Xlib.h>
#include <linux/LinuxMouse.h>
#endif

#ifndef NOOGRE
#include "ogreconsole.h"
#endif

using namespace std;
using namespace Ogre;
using namespace OIS;

// singleton pattern
InputEngine* InputEngine::myInstance = 0;

InputEngine & InputEngine::Instance () 
{
	if (myInstance == 0)
		myInstance = new InputEngine;
	return *myInstance;
}

bool InputEngine::instanceExists() 
{
	return (myInstance != 0);
}
// Constructor takes a RenderWindow because it uses that to determine input context
InputEngine::InputEngine() : mInputManager(0), mMouse(0), mKeyboard(0), mJoy(0), captureMode(false), mappingLoaded(false)
{
#ifndef NOOGRE
	LogManager::getSingleton().logMessage("*** Loading OIS ***");
#endif
	initAllKeys();
}

InputEngine::~InputEngine()
{
#ifndef NOOGRE
	LogManager::getSingleton().logMessage("*** Terminating destructor ***");
#endif
	destroy();
}

void InputEngine::destroy()
{
	if( mInputManager )
	{
#ifndef NOOGRE
		LogManager::getSingleton().logMessage("*** Terminating OIS ***");
#endif
		if(mMouse)
		{
			mInputManager->destroyInputObject( mMouse );
			mMouse=0;
		}
		if(mKeyboard)
		{
			mInputManager->destroyInputObject( mKeyboard );
			mKeyboard=0;
		}
		if(mJoy)
		{
			mInputManager->destroyInputObject( mJoy );
			mJoy=0;
		}

		OIS::InputManager::destroyInputSystem(mInputManager);
		mInputManager = 0;
	}
}


bool InputEngine::setup(size_t hwnd, bool capture, bool capturemouse, int _grabMode)
{
	grabMode = _grabMode;
#ifndef NOOGRE
	LogManager::getSingleton().logMessage("*** Initializing OIS ***");
#endif
	//try to delete old ones first (linux can only handle one at a time)
	destroy();
	captureMode = capture;
	recordChat=false;
	if(captureMode)
	{
		//size_t hWnd = 0;
		//win->getCustomAttribute("WINDOW", &hWnd);
		ParamList pl;
		String hwnd_str = Ogre::StringConverter::toString(hwnd);
		pl.insert(OIS::ParamList::value_type("WINDOW", hwnd_str));

		if(grabMode == GRAB_ALL)
		{
		} else if(grabMode == GRAB_DYNAMICALLY || grabMode == GRAB_NONE)
		{
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
			pl.insert(OIS::ParamList::value_type("x11_mouse_hide", "false"));
			pl.insert(OIS::ParamList::value_type("XAutoRepeatOn", "false"));
			pl.insert(OIS::ParamList::value_type("x11_mouse_grab", "false"));
			pl.insert(OIS::ParamList::value_type("x11_keyboard_grab", "false"));
#endif
		}
#ifndef NOOGRE
		LogManager::getSingleton().logMessage("*** OIS WINDOW: "+hwnd_str);
#endif
		
		mInputManager = OIS::InputManager::createInputSystem(pl);

		//Create all devices (We only catch joystick exceptions here, as, most people have Key/Mouse)
		mKeyboard = static_cast<Keyboard*>(mInputManager->createInputObject( OISKeyboard, true ));
		mKeyboard->setTextTranslation(OIS::Keyboard::Ascii);
		
		if(capturemouse)
			mMouse = static_cast<Mouse*>(mInputManager->createInputObject( OISMouse, true ));
		try 
		{
			mJoy = static_cast<JoyStick*>(mInputManager->createInputObject( OISJoyStick, true ));
		}
		catch(...)
		{
#ifndef NOOGRE
			LogManager::getSingleton().logMessage("*** No Joystick detected! ***");
#endif
			mJoy = 0;
		}

		//Set initial mouse clipping size
		//windowResized(win);

		// set Callbacks
		mKeyboard->setEventCallback(this);
		if(capturemouse)
			mMouse->setEventCallback(this);
		if(mJoy)
			mJoy->setEventCallback(this);

		// init states (not required for keyboard)
		if(capturemouse)
			mouseState = mMouse->getMouseState();
		if(mJoy)
			joyState = mJoy->getJoyStickState();
	}
	//this becomes more and more convoluted!
#ifdef NOOGRE
	// we will load the mapping manually
#else
	if(!mappingLoaded)
		if (!loadMapping())
		return false;
#endif
	return true;
}

void InputEngine::grabMouse(bool enable)
{
	static int lastmode = -1;
	if(!mMouse)
		return;
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	if((enable && lastmode == 0) || (!enable && lastmode == 1) || (lastmode == -1))
	{
		LogManager::getSingleton().logMessage("*** mouse grab: " + StringConverter::toString(enable));
		((LinuxMouse *)mMouse)->grab(enable);
		lastmode = enable?1:0;
	}
#endif
}

void InputEngine::hideMouse(bool visible)
{
	static int mode = -1;
	if(!mMouse)
		return;
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	if((visible && mode == 0) || (!visible && mode == 1) || mode == -1)
	{
		((LinuxMouse *)mMouse)->hide(visible);
		mode = visible?1:0;
	}
#endif
}

void InputEngine::setMousePosition(int x, int y, bool padding)
{
	if(!mMouse)
		return;
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
	// padding ensures that the mouse has a safety area at the window's borders
//	((LinuxMouse *)mMouse)->setMousePosition(x, y, padding);
#endif
}

OIS::MouseState InputEngine::getMouseState()
{
	static float mX=999999, mY=999999;
	static int mode = -1;
	if(mode == -1)
	{
#ifndef NOOGRE
		// try to find the correct location!
		mX = StringConverter::parseReal(SETTINGS.getSetting("MouseSensX"));
		mY = StringConverter::parseReal(SETTINGS.getSetting("MouseSensY"));
		LogManager::getSingleton().logMessage("Mouse X sens: " + StringConverter::toString((Real)mX));
		LogManager::getSingleton().logMessage("Mouse Y sens: " + StringConverter::toString((Real)mY));
		mode = 1;
		if(mX == 0 || mY ==0)
			mode = 2;
#else
		// no scaling without ogre
		mode = 2;
#endif
	}
	
	OIS::MouseState m;
	if(mMouse)
	{
		m = mMouse->getMouseState();
		if(mode == 1)
		{
			m.X.rel = (int)((float)m.X.rel * mX);
			m.Y.rel = (int)((float)m.X.rel * mY);
		}
	}
	return m;
}

bool InputEngine::fileExists(char* filename)
{
	FILE* f = fopen(filename, "rb");
	if(f != NULL) {
		fclose(f);
		return true;
	}
	return false;
}

Ogre::String InputEngine::getKeyNameForKeyCode(OIS::KeyCode keycode)
{
	if(keycode == KC_LSHIFT || keycode == KC_RSHIFT)
		return "SHIFT";
	if(keycode == KC_LCONTROL || keycode == KC_RCONTROL)
		return "CTRL";
	if(keycode == KC_LMENU || keycode == KC_RMENU)
		return "ALT";
	for(allit = allkeys.begin();allit != allkeys.end();allit++)
	{
		if(allit->second == keycode)
			return allit->first;
	}
	return "unkown";
}

void InputEngine::Capture()
{
	if(mKeyboard) mKeyboard->capture();
	if(mMouse) mMouse->capture();
	if(mJoy) mJoy->capture();
}

void InputEngine::windowResized(RenderWindow* rw)
{
	if(!mMouse)
		return;
	//update mouse area
	unsigned int width, height, depth;
	int left, top;
	rw->getMetrics(width, height, depth, left, top);
	const OIS::MouseState &ms = mMouse->getMouseState();
	ms.width = width;
	ms.height = height;
}

/* --- Joystik Events ------------------------------------------ */
bool InputEngine::buttonPressed( const OIS::JoyStickEvent &arg, int button )
{
	inputsChanged=true;
	//LogManager::getSingleton().logMessage("*** buttonPressed " + StringConverter::toString(button));
	joyState = arg.state;
	return true;
}

bool InputEngine::buttonReleased( const OIS::JoyStickEvent &arg, int button )
{
	inputsChanged=true;
	//LogManager::getSingleton().logMessage("*** buttonReleased " + StringConverter::toString(button));
	joyState = arg.state;
	return true;
}

bool InputEngine::axisMoved( const OIS::JoyStickEvent &arg, int axis )
{
	inputsChanged=true;
	//LogManager::getSingleton().logMessage("*** axisMoved " + StringConverter::toString(axis) + " / " + StringConverter::toString((int)(arg.state.mAxes[axis].abs / (float)(mJoy->MAX_AXIS/100))));
	joyState = arg.state;
	return true;
}

bool InputEngine::sliderMoved( const OIS::JoyStickEvent &, int )
{
	inputsChanged=true;
	//LogManager::getSingleton().logMessage("*** sliderMoved");
	return true;
}

bool InputEngine::povMoved( const OIS::JoyStickEvent &, int )
{
	inputsChanged=true;
	//LogManager::getSingleton().logMessage("*** povMoved");
	return true;
}

/* --- Key Events ------------------------------------------ */
bool InputEngine::keyPressed( const OIS::KeyEvent &arg )
{
#ifndef NOOGRE
# ifdef ANGELSCRIPT
	if(OgreConsole::getSingleton().getVisible())
	{
		OgreConsole::getSingleton().onKeyPressed(arg);
		return true;
	}
# endif
#endif

	//LogManager::getSingleton().logMessage("*** keyPressed");
	if(keyState[arg.key] != 1)
		inputsChanged=true;
	keyState[arg.key] = 1;

	if(recordChat)
	{
		// chatting stuff
		if (keyInput.size()<255)
		{
			if ((arg.key==OIS::KC_DELETE || arg.key==OIS::KC_BACK) && keyInput.size()>0)
			{
				keyInput = keyInput.substr(0, keyInput.size()-1);
#ifndef NOOGRE
				NETCHAT.setEnterText("^7"+keyInput, true, true);
#endif
			}

			// exclude control keys
			if(arg.text > 20)
			{
				keyInput.push_back(arg.text);
#ifndef NOOGRE
				NETCHAT.setEnterText("^7"+keyInput, true, true);
#endif
			}
		}
	}

#ifndef NOOGRE
	MYGUI.keyPressed(arg);
#endif

	return true;
}

bool InputEngine::keyReleased( const OIS::KeyEvent &arg )
{
	//LogManager::getSingleton().logMessage("*** keyReleased");
	if(keyState[arg.key] != 0)
		inputsChanged=true;
	keyState[arg.key] = 0;
#ifndef NOOGRE
	MYGUI.keyReleased(arg);
#endif
	return true;
}

/* --- Mouse Events ------------------------------------------ */
bool InputEngine::mouseMoved( const OIS::MouseEvent &arg )
{
	//APP.mGUISystem->injectOISMouseMove(arg);
	//LogManager::getSingleton().logMessage("*** mouseMoved");
	inputsChanged=true;
	mouseState = arg.state;
#ifndef NOOGRE
	MYGUI.mouseMoved(arg);
#endif
	return true;
}

bool InputEngine::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	//APP.mGUISystem->injectOISMouseButtonDown(id);
	//LogManager::getSingleton().logMessage("*** mousePressed");
	inputsChanged=true;
	mouseState = arg.state;
#ifndef NOOGRE
	MYGUI.mousePressed(arg, id);
#endif
	return true;
}

bool InputEngine::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
	//APP.mGUISystem->injectOISMouseButtonUp(id);
	//LogManager::getSingleton().logMessage("*** mouseReleased");
	inputsChanged=true;
	mouseState = arg.state;
#ifndef NOOGRE
	MYGUI.mouseReleased(arg, id);
#endif
	return true;
}

/* --- Custom Methods ------------------------------------------ */
void InputEngine::prepareShutdown()
{
#ifndef NOOGRE
	LogManager::getSingleton().logMessage("*** Inputsystem prepare for shutdown ***");
#endif
	destroy();
}


void InputEngine::resetKeys()
{
	for ( map<int, bool>::iterator iter = keyState.begin(); iter != keyState.end(); ++iter)
	{
		iter->second = false;
	}
}

bool InputEngine::getEventBoolValue(String eventName)
{
	return (getEventValue(eventName) > 0.5);
}

float InputEngine::deadZone(float axisValue, float dz)
{
	// no deadzone?
	if(dz < 0.0001)
		return axisValue;

	// check for deadzone
	if (fabs(axisValue) < dz)
		// dead zone case
		return 0.0;
	else
		// non-deadzone, remap the remaining part
		return axisValue / (1.0/(1.0-dz));
	return axisValue;
}

float InputEngine::logval(float val)
{
	if (val>0) return log10(1.0/(1.1-val))/1.0;
	if (val==0) return 0;
	return -log10(1.0/(1.1+val))/1.0;
}

void InputEngine::smoothValue(float &ref, float value, float rate)
{
	if(value < -1) value = -1;
	if(value > 1) value = 1;
	// smooth!
	if (ref > value)
	{
		ref -= rate;
		if (ref < value) 
			ref = value;
	}
	else if (ref < value)
		ref += rate;
}

String InputEngine::getEventCommand(String eventName)
{
	std::vector<event_trigger_t> t_vec = events[eventName];
	if(t_vec.size() > 0)
		return String(t_vec[0].configline);
	return "";
}

event_trigger_t *InputEngine::getEventBySUID(int suid)
{
	std::map<Ogre::String, std::vector<event_trigger_t> >::iterator a;
	std::vector<event_trigger_t>::iterator b;
	for(a = events.begin();a != events.end(); a++)
	{
		for(b = a->second.begin();b != a->second.end(); b++)
		{
			if(b->suid == suid)
				return &(*b);
		}
	}
	return 0;
}

bool InputEngine::deleteEventBySUID(int suid)
{
	std::map<Ogre::String, std::vector<event_trigger_t> >::iterator a;
	std::vector<event_trigger_t>::iterator b;
	for(a = events.begin();a != events.end(); a++)
	{
		for(b = a->second.begin();b != a->second.end(); b++)
		{
			if(b->suid == suid)
			{
				a->second.erase(b);
				return true;
			}
		}
	}
	return false;
}

bool InputEngine::isEventDefined(String eventName)
{
	std::vector<event_trigger_t> t_vec = events[eventName];
	if(t_vec.size() > 0)
	{
		if(t_vec[0].eventtype != ET_NONE)
			return true;
	}
	return false;
}

int InputEngine::getKeboardKeyForCommand(Ogre::String eventName)
{
	float returnValue = 0;
	std::vector<event_trigger_t> t_vec = events[eventName];
	for(std::vector<event_trigger_t>::iterator i = t_vec.begin(); i != t_vec.end(); i++)
	{
		event_trigger_t t = *i;
		float value = 0;
		if(t.eventtype == ET_Keyboard)
			return t.keyCode;
		return -1;
	}
}

float InputEngine::getEventValue(String eventName)
{
	float returnValue = 0;
	std::vector<event_trigger_t> t_vec = events[eventName];
	for(std::vector<event_trigger_t>::iterator i = t_vec.begin(); i != t_vec.end(); i++)
	{
		event_trigger_t t = *i;
		float value = 0;
		switch(t.eventtype)
		{
			case ET_NONE:
				break;
			case ET_Keyboard:
				if(!keyState[t.keyCode])
					break;

				// only use explicite mapping, if two keys with different modifiers exist, i.e. F1 and SHIFT+F1.
				// check for modificators
				if (t.explicite)
				{
					if(t.ctrl != (keyState[KC_LCONTROL] || keyState[KC_RCONTROL]))
						break;
					if(t.shift != (keyState[KC_LSHIFT] || keyState[KC_RSHIFT]))
						break;
					if(t.alt != (keyState[KC_LMENU] || keyState[KC_RMENU]))
						break;
				} else {
					if(t.ctrl && !(keyState[KC_LCONTROL] || keyState[KC_RCONTROL]))
						break;
					if(t.shift && !(keyState[KC_LSHIFT] || keyState[KC_RSHIFT]))
						break;
					if(t.alt && !(keyState[KC_LMENU] || keyState[KC_RMENU]))
						break;
				}
				value = 1;
				break;
			case ET_MouseButton:
				//if(t.mouseButtonNumber == 0)
				// TODO: FIXME
				value = mouseState.buttonDown(MB_Left);
				break;
			case ET_MouseAxisX:
				value = mouseState.X.abs / mJoy->MAX_AXIS;
				break;
			case ET_MouseAxisY:
				value = mouseState.Y.abs / mJoy->MAX_AXIS;
				break;
			case ET_MouseAxisZ:
				value = mouseState.Z.abs / mJoy->MAX_AXIS;
				break;
			case ET_JoystickButton:
				{
					if(!mJoy)
					{
						value=0;
						continue;
					}
#ifdef OIS_HEAD
					if(t.joystickButtonNumber >= (int)mJoy->getNumberOfComponents(OIS_Button))
#else
					if(t.joystickButtonNumber >= (int)mJoy->buttons())
#endif
					{
#ifndef NOOGRE
#ifdef OIS_HEAD
						LogManager::getSingleton().logMessage("*** Joystick has not enough buttons for mapping: need button "+StringConverter::toString(t.joystickButtonNumber) + ", availabe buttons: "+StringConverter::toString(mJoy->getNumberOfComponents(OIS_Button)));
#else
						LogManager::getSingleton().logMessage("*** Joystick has not enough buttons for mapping: need button "+StringConverter::toString(t.joystickButtonNumber) + ", availabe buttons: "+StringConverter::toString(mJoy->buttons()));
#endif
#endif
						value=0;
						continue;
					}
#ifdef OIS_HEAD
					value = joyState.mButtons[t.joystickButtonNumber];
#else
					value = joyState.buttonDown(t.joystickButtonNumber);
#endif
				}
				break;
			case ET_JoystickAxisRel:
			case ET_JoystickAxisAbs:
				{
					if(!mJoy)
					{
						value=0;
						continue;
					}
					if(t.joystickAxisNumber >= (int)joyState.mAxes.size())
					{
#ifndef NOOGRE
						LogManager::getSingleton().logMessage("*** Joystick has not enough axis for mapping: need axe "+StringConverter::toString(t.joystickAxisNumber) + ", availabe axis: "+StringConverter::toString(joyState.mAxes.size()));
#endif
						value=0;
						continue;
					}
					Axis axe = joyState.mAxes[t.joystickAxisNumber];
					
					if(t.eventtype == ET_JoystickAxisRel)
					{
						value = (float)axe.rel / (float)mJoy->MAX_AXIS;
					}
					else
					{
						value = (float)axe.abs / (float)mJoy->MAX_AXIS;
						switch(t.joystickAxisRegion)
						{
						case 0:
							// normal case, full axis used
							value = (value + 1)/2;
							break;
						case -1:
							// lower range used
							if(value > 0)
								value = 0;
							else
								value = value * -1.0;
							break;
						case 1:
							// upper range used
							if(value < 0)
								value = 0;
							break;
						}

						if(t.joystickAxisHalf)
						{
							// XXX: TODO: write this
							//float a = (double)((value+1.0)/2.0);
							//float b = (double)(1.0-(value+1.0)/2.0);
							//LogManager::getSingleton().logMessage("half: "+StringConverter::toString(value)+" / "+StringConverter::toString(a)+" / "+StringConverter::toString(b));
							//no dead zone in half axis
							value = (1.0 + value) / 2.0;
							if (t.joystickAxisReverse)
								value = 1.0 - value;
							if (t.joystickAxisLinearity < 0.5)
								value = log10(1.0/(1.1-value));
						}else
						{
							//LogManager::getSingleton().logMessage("not half: "+StringConverter::toString(value)+" / "+StringConverter::toString(deadZone(value, t.joystickAxisDeadzone)) +" / "+StringConverter::toString(t.joystickAxisDeadzone) );
							value = deadZone(value, t.joystickAxisDeadzone);
							if (t.joystickAxisReverse)
								value = 1-value;
							if (t.joystickAxisLinearity < 0.5)
								value = logval(value);
						}
						// digital mapping of analog axis
						if(t.joystickAxisUseDigital)
							if(value >= 0.5)
								value = 1;
							else
								value = 0;
					}
				}
				break;
			case ET_JoystickPov:
				{
					if(!mJoy)
					{
						value=0;
						continue;
					}
#ifdef OIS_HEAD
					if(t.joystickButtonNumber >= (int)mJoy->getNumberOfComponents(OIS_POV))
#else
					if(t.joystickPovNumber >= (int)mJoy->hats())
#endif
					{
#ifndef NOOGRE
#ifdef OIS_HEAD
						LogManager::getSingleton().logMessage("*** Joystick has not enough POVs for mapping: need POV "+StringConverter::toString(t.joystickPovNumber) + ", availabe POVs: "+StringConverter::toString(mJoy->getNumberOfComponents(OIS_POV)));
#else
						LogManager::getSingleton().logMessage("*** Joystick has not enough POVs for mapping: need POV "+StringConverter::toString(t.joystickPovNumber) + ", availabe POVs: "+StringConverter::toString(mJoy->hats()));
#endif
#endif
						value=0;
						continue;
					}
					value = joyState.mPOV[t.joystickPovNumber].direction;
				}
				break;
			case ET_JoystickSliderX:
				{
					if(!mJoy)
					{
						 value=0;
						 continue;
					}
					value = joyState.mSliders[t.joystickSliderNumber].abX / mJoy->MAX_AXIS;
				}
				break;
			case ET_JoystickSliderY:
				{
					if(!mJoy)
					{
						value=0;
						continue;
					}
					value = joyState.mSliders[t.joystickSliderNumber].abY / mJoy->MAX_AXIS;
				}
				break;
		}
		// only return if grater zero, otherwise check all other bombinations
		if(value > returnValue)
			returnValue = value;
	}
	return returnValue;
}

bool InputEngine::isKeyDown(OIS::KeyCode key)
{
	return this->mKeyboard->isKeyDown(key);
}

Ogre::String InputEngine::getEventTypeName(int type)
{
	switch(type)
	{
	case ET_NONE: return "None";
	case ET_Keyboard: return "Keyboard";
	case ET_MouseButton: return "MouseButton";
	case ET_MouseAxisX: return "MouseAxisX";
	case ET_MouseAxisY: return "MouseAxisY";
	case ET_MouseAxisZ: return "MouseAxisZ";
	case ET_JoystickButton: return "JoystickButton";
	case ET_JoystickAxisAbs: return "JoystickAxis";
	case ET_JoystickAxisRel: return "JoystickAxis";
	case ET_JoystickPov: return "JoystickPov";
	case ET_JoystickSliderX: return "JoystickSliderX";
	case ET_JoystickSliderY: return "JoystickSliderY";
	}
	return "unkown";
}

void InputEngine::addEvent(String eventName, event_trigger_t t)
 {
    static int counter=0;
    counter++;
    t.suid = counter;
    events[eventName].push_back(t);
}

bool InputEngine::processLine(char *line)
{
	static Ogre::String cur_comment = "";

	char eventName[255]="", evtype[255]="";
	const char delimiters[] = "+";
	size_t linelen = strnlen(line, 1024);
	enum eventtypes eventtype = ET_NONE;

	int joyNo = 0;
	float defaultDeadzone = 0.1;
	if (line[0]==';' || linelen < 5)
	{
		cur_comment += line;;
		return false;
	}
	sscanf(line, "%s %s", eventName, evtype);
	if(strnlen(eventName, 255) == 0 || strnlen(evtype, 255) == 0)
		return false;

	if(!strncmp(evtype, "Keyboard", 8)) eventtype = ET_Keyboard;
	else if(!strncmp(evtype, "MouseButton", 10)) eventtype = ET_MouseButton;
	else if(!strncmp(evtype, "MouseAxisX", 9)) eventtype = ET_MouseAxisX;
	else if(!strncmp(evtype, "MouseAxisY", 9)) eventtype = ET_MouseAxisY;
	else if(!strncmp(evtype, "MouseAxisZ", 9)) eventtype = ET_MouseAxisZ;
	else if(!strncmp(evtype, "JoystickButton", 14)) eventtype = ET_JoystickButton;
	else if(!strncmp(evtype, "JoystickAxis", 12)) eventtype = ET_JoystickAxisAbs;
	//else if(!strncmp(evtype, "JoystickAxis", 250)) eventtype = ET_JoystickAxisRel;
	else if(!strncmp(evtype, "JoystickPov", 11)) eventtype = ET_JoystickPov;
	else if(!strncmp(evtype, "JoystickSliderX", 15)) eventtype = ET_JoystickSliderX;
	else if(!strncmp(evtype, "JoystickSliderY", 15)) eventtype = ET_JoystickSliderY;
	else if(!strncmp(evtype, "None", 4)) eventtype = ET_NONE;

	switch(eventtype)
	{
	case ET_Keyboard:
		{
			char keycodes[255], *keycode=0;
			OIS::KeyCode key = KC_UNASSIGNED;
			sscanf(line, "%s %s %s", eventName, evtype, keycodes);
			// seperate all keys and construct the key combination
			//LogManager::getSingleton().logMessage("try to add key: " + String(keyname)+","+ String(evtype)+","+String(keycodes));
			bool alt=false;
			bool shift=false;
			bool ctrl=false;
			bool expl=false;
			char *keycodes_work = strdup(keycodes);
			char *token = strtok(keycodes_work, delimiters);
			while (token != NULL)
			{
				if (strncmp(token, "SHIFT", 5) == 0)
					shift=true;
				else if (strncmp(token, "CTRL", 4) == 0)
					ctrl=true;
				else if (strncmp(token, "ALT", 3) == 0)
					alt=true;
				else if (strncmp(token, "EXPL", 4) == 0)
					expl=true;
				keycode = token;
				token = strtok(NULL, delimiters);
			}

			allit = allkeys.find(keycode);
			if(allit == allkeys.end())
			{
#ifndef NOOGRE
				LogManager::getSingleton().logMessage("unkown key: " + string(keycodes));
#endif
				key = KC_UNASSIGNED;
			} else {
				//LogManager::getSingleton().logMessage("found key: " + string(keycode) + " = " + StringConverter::toString((int)key));
				key = allit->second;
			}
			event_trigger_t t_key;
			//memset(&t_key, 0, sizeof(event_trigger_t));
			t_key.eventtype = ET_Keyboard;
			t_key.shift = shift;
			t_key.ctrl = ctrl;
			t_key.alt = alt;
			t_key.explicite = expl;
			t_key.keyCode = key;
			t_key.configline = keycodes;
			t_key.group = getEventGroup(eventName);
			t_key.tmp_eventname=eventName;
			t_key.comments = cur_comment;
			cur_comment = "";
			addEvent(eventName, t_key);
			
#ifndef NOOGRE
			//LogManager::getSingleton().logMessage("adding: " + String(eventName) + ": "+StringConverter::toString((int)key));
#endif
			return true;
		}
	case ET_JoystickButton:
		{
			int buttonNo=0;
			char tmp2[255];
			memset(tmp2, 0 ,255);
			sscanf(line, "%s %s %d %s", eventName, evtype, &buttonNo, tmp2);
			event_trigger_t t_joy;
			//memset(&t_joy, 0, sizeof(event_trigger_t));
			t_joy.eventtype = ET_JoystickButton;
			t_joy.joystickNumber = joyNo;
			t_joy.joystickButtonNumber = buttonNo;
			if(!strcmp(tmp2, "!NEW!"))
			{
				t_joy.configline = Ogre::String(tmp2);
			} else
			{
				char tmp[255];
				sprintf(tmp, "%d", buttonNo);
				t_joy.configline = Ogre::String(tmp);
			}
			t_joy.group = getEventGroup(eventName);
			t_joy.tmp_eventname=eventName;
			t_joy.comments = cur_comment;
			cur_comment = "";
			addEvent(eventName, t_joy);
			return true;
		}
	case ET_JoystickAxisRel:
	case ET_JoystickAxisAbs:
		{
			int axisNo=0;
			char options[250];
			memset(options, 0, 250);
			sscanf(line, "%s %s %d %s", eventName, evtype, &axisNo, options);

			bool half=false;
			bool reverse=false;
			bool linear=false;
			bool relative=false;
			bool usedigital=false;
			float deadzone=defaultDeadzone;
			float linearity=1;
			int jAxisRegion=0;
			// 0 = all
			// -1 = lower
			// 1 = upper
			char *tmp = strdup(options);
			char *token = strtok(tmp, delimiters);
			while (token != NULL)
			{
				if (strncmp(token, "HALF", 4) == 0)
					half=true;
				else if (strncmp(token, "REVERSE", 7) == 0)
					reverse=true;
				else if (strncmp(token, "LINEAR", 6) == 0)
					linear=true;
				else if (strncmp(token, "UPPER", 5) == 0)
					jAxisRegion = 1;
				else if (strncmp(token, "LOWER", 5) == 0)
					jAxisRegion = -1;
				else if (strncmp(token, "RELATIVE", 8) == 0)
					relative=true;
				else if (strncmp(token, "DIGITAL", 7) == 0)
					usedigital=true;
				else if (strncmp(token, "DEADZONE", 8) == 0 && strnlen(token, 250) > 9)
				{
					char tmp2[255];
					strcpy(tmp2,token+9);
					deadzone = atof(tmp2);
					//LogManager::getSingleton().logMessage("got deadzone: " + StringConverter::toString(deadzone)+", "+String(tmp2));
				}
				else if (strncmp(token, "LINEARITY", 9) == 0 && strnlen(token, 250) > 10)
				{
					char tmp2[255];
					strcpy(tmp2,token+10);
					linearity = atof(tmp2);
				}
				token = strtok(NULL, delimiters);
			}

			if(relative)
				eventtype = ET_JoystickAxisRel;

			event_trigger_t t_joy;
			//memset(&t_joy, 0, sizeof(event_trigger_t));
			t_joy.eventtype = eventtype;
			t_joy.joystickAxisRegion = jAxisRegion;
			t_joy.joystickAxisUseDigital = usedigital;
			t_joy.joystickAxisDeadzone = deadzone;
			t_joy.joystickAxisHalf = half;
			t_joy.joystickAxisLinearity = linearity;
			t_joy.joystickAxisReverse = reverse;
			t_joy.joystickAxisNumber = axisNo;
			t_joy.joystickNumber = joyNo;
			t_joy.configline = Ogre::String(options);
			t_joy.group = getEventGroup(eventName);
			t_joy.tmp_eventname=eventName;
			t_joy.comments = cur_comment;
			cur_comment = "";
			addEvent(eventName, t_joy);
			//LogManager::getSingleton().logMessage("added axis: " + StringConverter::toString(axisNo));
			return true;
		}
	case ET_NONE:
		{
			event_trigger_t t_none;
			t_none.eventtype = eventtype;
			t_none.configline = "";
			t_none.group = getEventGroup(eventName);
			t_none.tmp_eventname=eventName;
			t_none.comments = cur_comment;
			cur_comment = "";
			addEvent(eventName, t_none);
			return true;
		}
	case ET_MouseButton:
	case ET_MouseAxisX:
	case ET_MouseAxisY:
	case ET_MouseAxisZ:
	case ET_JoystickPov:
	case ET_JoystickSliderX:
	case ET_JoystickSliderY:
	default:
		return false;
	}
	return false;
}

int InputEngine::getCurrentJoyButton()
{
	for(int i=0;i<(int)joyState.mButtons.size();i++)
#ifdef OIS_HEAD
		if(joyState.mButtons[i])
#else
		if(joyState.buttonDown(i))
#endif
			return i;
	return -1;
}

JoyStickState *InputEngine::getCurrentJoyState()
{
	return &joyState;
}


int InputEngine::getCurrentKeyCombo(Ogre::String *combo)
{
	std::map<int, bool>::iterator i;
	int keyCounter = 0;
	int modCounter = 0;
	
	// list all modificators first
	for(i = keyState.begin();i!=keyState.end();i++)
	{
		if(i->second)
		{
			if(i->first != KC_LSHIFT && i->first != KC_RSHIFT && i->first != KC_LCONTROL && i->first != KC_RCONTROL && i->first != KC_LMENU && i->first != KC_RMENU)
				continue;
			modCounter++;
			Ogre::String keyName = getKeyNameForKeyCode((OIS::KeyCode)i->first);
			if(*combo == "")
				*combo = keyName;
			else
				*combo = *combo + "+" + keyName;
		}
	}

	// now list all keys
	for(i = keyState.begin();i!=keyState.end();i++)
	{
		if(i->second)
		{
			if(i->first == KC_LSHIFT || i->first == KC_RSHIFT || i->first == KC_LCONTROL || i->first == KC_RCONTROL || i->first == KC_LMENU || i->first == KC_RMENU)
				continue;
			Ogre::String keyName = getKeyNameForKeyCode((OIS::KeyCode)i->first);
			if(*combo == "")
				*combo = keyName;
			else
				*combo = *combo + "+" + keyName;
			keyCounter++;
		}
	}

	//
	if(modCounter > 0 && keyCounter == 0)
	{
		return -modCounter;
	} else if(keyCounter==0 && modCounter == 0)
	{
		*combo = "(Please press a key)";
		return 0;
	}
	return keyCounter;
}

Ogre::String InputEngine::getEventGroup(Ogre::String eventName)
{
	const char delimiters[] = "_";
	char *tmp = strdup(eventName.c_str());
	char *token = strtok(tmp, delimiters);
	while (token != NULL)
		return Ogre::String(token);
	return "";
}

bool InputEngine::appendLineToConfig(std::string line, std::string outfile)
{
	FILE *f = fopen(const_cast<char *>(outfile.c_str()),"a");
	if(!f)
		return false;
	fprintf(f, "%s\n", line.c_str());
	fclose(f);
	return true;
}

bool InputEngine::reloadConfig(std::string outfile)
{
	events.clear();
	loadMapping(outfile);
	return true;
}

bool InputEngine::updateConfigline(event_trigger_t *t)
{
	if(t->eventtype != ET_JoystickAxisAbs)
		return false;
	t->configline = "";
	if(t->joystickAxisRegion==1)
		t->configline += "UPPER";
	else if(t->joystickAxisRegion==-1)
		t->configline += "LOWER";
	
	if(t->joystickAxisReverse && t->configline != "")
		t->configline += "+REVERSE";
	else if(t->joystickAxisReverse && t->configline == "")
		t->configline += "REVERSE";

	if(fabs(t->joystickAxisDeadzone-0.1) > 0.0001)
	{
		char tmp[255]="";
		memset(tmp, 0, 255);
		sprintf(tmp, "DEADZONE=%0.2f", t->joystickAxisDeadzone);
		if(t->configline != "")
			t->configline += "+" + std::string(tmp);
		else
			t->configline += std::string(tmp);
	}
	return true;
}

bool InputEngine::saveMapping(Ogre::String outfile)
{
	FILE *f = fopen(const_cast<char *>(outfile.c_str()),"w");
	std::map<Ogre::String, std::vector<event_trigger_t> > controls = getEvents();
	std::map<Ogre::String, std::vector<event_trigger_t> >::iterator mapIt;
	std::vector<event_trigger_t>::iterator vecIt;

	int counter = 0;
	
	Ogre::String curGroup = "";
	for(mapIt = controls.begin(); mapIt != controls.end(); mapIt++)
	{
		std::vector<event_trigger_t> vec = mapIt->second;

		for(vecIt = vec.begin(); vecIt != vec.end(); vecIt++, counter++)
		{
			if(vecIt->group != curGroup)
			{
				curGroup = vecIt->group;
				// group title:
				fprintf(f, "\n; %s\n", curGroup.c_str());
			}

			// no user comments for now!
			//if(vecIt->comments!="")
			//	fprintf(f, "%s", vecIt->comments.c_str());

			// print event name
			fprintf(f, "%-30s ", mapIt->first.c_str());
			// print event type
			fprintf(f, "%-20s ", getEventTypeName(vecIt->eventtype).c_str());

			if(vecIt->eventtype == ET_Keyboard)
			{
				fprintf(f, "%s ", vecIt->configline.c_str());
			} else if(vecIt->eventtype == ET_JoystickAxisAbs || vecIt->eventtype == ET_JoystickAxisRel)
			{
				fprintf(f, "%d ", vecIt->joystickAxisNumber);
				fprintf(f, "%s ", vecIt->configline.c_str());
			} else if(vecIt->eventtype == ET_JoystickButton)
			{
				fprintf(f, "%d ", vecIt->joystickButtonNumber);
			}
			// end this line
			fprintf(f, "\n");
		}
	}
	fclose(f);
	return true;
}


bool InputEngine::loadMapping(Ogre::String outfile, bool append)
{
	char line[1025] = "";

	if(!append)
	{
		// clear everything
		resetKeys();
		events.clear();
	}

#ifndef NOOGRE
	LogManager::getSingleton().logMessage("Loading input mapping...");
	{
		DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(outfile, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		while (!ds->eof())
		{
			size_t size = 1024;
			if(ds->tell() + size >= ds->size())
				size = ds->size()-ds->tell();
			if(ds->tell() >= ds->size())
				break;
			size_t readnum = ds->readLine(line, size);
			if(readnum > 5)
				processLine(line);
		}
	}
#else
	FILE *f = fopen(outfile.c_str(), "r");
	if(!f)
		return false;
	while(fgets(line, 1024, f)!=NULL)
	{ 
		if(strnlen(line, 1024) > 5)
			processLine(line);
	}
	fclose(f);

#endif

	mappingLoaded = true;
	
#ifndef NOOGRE
	LogManager::getSingleton().logMessage("key map successfully loaded!");
#endif
	return true;
}

void InputEngine::initAllKeys()
{
	allkeys["0"] = KC_0;
	allkeys["1"] = KC_1;
	allkeys["2"] = KC_2;
	allkeys["3"] = KC_3;
	allkeys["4"] = KC_4;
	allkeys["5"] = KC_5;
	allkeys["6"] = KC_6;
	allkeys["7"] = KC_7;
	allkeys["8"] = KC_8;
	allkeys["9"] = KC_9;
	allkeys["A"] = KC_A ;
	allkeys["ABNT_C1"] = KC_ABNT_C1;
	allkeys["ABNT_C2"] = KC_ABNT_C2;
	allkeys["ADD"] = KC_ADD;
	allkeys["APOSTROPHE"] = KC_APOSTROPHE;
	allkeys["APPS"] = KC_APPS;
	allkeys["AT"] = KC_AT;
	allkeys["AX"] = KC_AX;
	allkeys["B"] = KC_B;
	allkeys["BACK"] = KC_BACK;
	allkeys["BACKSLASH"] = KC_BACKSLASH;
	allkeys["C"] = KC_C;
	allkeys["CALCULATOR"] = KC_CALCULATOR;
	allkeys["CAPITAL"] = KC_CAPITAL;
	allkeys["COLON"] = KC_COLON;
	allkeys["COMMA"] = KC_COMMA;
	allkeys["CONVERT"] = KC_CONVERT;
	allkeys["D"] = KC_D;
	allkeys["DECIMAL"] = KC_DECIMAL;
	allkeys["DELETE"] = KC_DELETE;
	allkeys["DIVIDE"] = KC_DIVIDE;
	allkeys["DOWN"] = KC_DOWN;
	allkeys["E"] = KC_E;
	allkeys["END"] = KC_END;
	allkeys["EQUALS"] = KC_EQUALS;
	allkeys["ESCAPE"] = KC_ESCAPE;
	allkeys["F"] = KC_F;
	allkeys["F1"] = KC_F1;
	allkeys["F10"] = KC_F10;
	allkeys["F11"] = KC_F11;
	allkeys["F12"] = KC_F12;
	allkeys["F13"] = KC_F13;
	allkeys["F14"] = KC_F14;
	allkeys["F15"] = KC_F15;
	allkeys["F2"] = KC_F2;
	allkeys["F3"] = KC_F3;
	allkeys["F4"] = KC_F4;
	allkeys["F5"] = KC_F5;
	allkeys["F6"] = KC_F6;
	allkeys["F7"] = KC_F7;
	allkeys["F8"] = KC_F8;
	allkeys["F9"] = KC_F9;
	allkeys["G"] = KC_G;
	allkeys["GRAVE"] = KC_GRAVE;
	allkeys["H"] = KC_H;
	allkeys["HOME"] = KC_HOME;
	allkeys["I"] = KC_I;
	allkeys["INSERT"] = KC_INSERT;
	allkeys["J"] = KC_J;
	allkeys["K"] = KC_K;
	allkeys["KANA"] = KC_KANA;
	allkeys["KANJI"] = KC_KANJI;
	allkeys["L"] = KC_L;
	allkeys["LBRACKET"] = KC_LBRACKET;
	allkeys["LCONTROL"] = KC_LCONTROL;
	allkeys["LEFT"] = KC_LEFT;
	allkeys["LMENU"] = KC_LMENU;
	allkeys["LSHIFT"] = KC_LSHIFT;
	allkeys["LWIN"] = KC_LWIN;
	allkeys["M"] = KC_M;
	allkeys["MAIL"] = KC_MAIL;
	allkeys["MEDIASELECT"] = KC_MEDIASELECT;
	allkeys["MEDIASTOP"] = KC_MEDIASTOP;
	allkeys["MINUS"] = KC_MINUS;
	allkeys["MULTIPLY"] = KC_MULTIPLY;
	allkeys["MUTE"] = KC_MUTE;
	allkeys["MYCOMPUTER"] = KC_MYCOMPUTER;
	allkeys["N"] = KC_N;
	allkeys["NEXTTRACK"] = KC_NEXTTRACK;
	allkeys["NOCONVERT"] = KC_NOCONVERT;
	allkeys["NUMLOCK"] = KC_NUMLOCK;
	allkeys["NUMPAD0"] = KC_NUMPAD0;
	allkeys["NUMPAD1"] = KC_NUMPAD1;
	allkeys["NUMPAD2"] = KC_NUMPAD2;
	allkeys["NUMPAD3"] = KC_NUMPAD3;
	allkeys["NUMPAD4"] = KC_NUMPAD4;
	allkeys["NUMPAD5"] = KC_NUMPAD5;
	allkeys["NUMPAD6"] = KC_NUMPAD6;
	allkeys["NUMPAD7"] = KC_NUMPAD7;
	allkeys["NUMPAD8"] = KC_NUMPAD8;
	allkeys["NUMPAD9"] = KC_NUMPAD9;
	allkeys["NUMPADCOMMA"] = KC_NUMPADCOMMA;
	allkeys["NUMPADENTER"] = KC_NUMPADENTER;
	allkeys["NUMPADEQUALS"] = KC_NUMPADEQUALS;
	allkeys["O"] = KC_O;
	allkeys["OEM_102"] = KC_OEM_102;
	allkeys["P"] = KC_P;
	allkeys["PAUSE"] = KC_PAUSE;
	allkeys["PERIOD"] = KC_PERIOD;
	allkeys["PGDOWN"] = KC_PGDOWN;
	allkeys["PGUP"] = KC_PGUP;
	allkeys["PLAYPAUSE"] = KC_PLAYPAUSE;
	allkeys["POWER"] = KC_POWER;
	allkeys["PREVTRACK"] = KC_PREVTRACK;
	allkeys["Q"] = KC_Q;
	allkeys["R"] = KC_R;
	allkeys["RBRACKET"] = KC_RBRACKET;
	allkeys["RCONTROL"] = KC_RCONTROL;
	allkeys["RETURN"] = KC_RETURN;
	allkeys["RIGHT"] = KC_RIGHT;
	allkeys["RMENU"] = KC_RMENU;
	allkeys["RSHIFT"] = KC_RSHIFT;
	allkeys["RWIN"] = KC_RWIN;
	allkeys["S"] = KC_S;
	allkeys["SCROLL"] = KC_SCROLL;
	allkeys["SEMICOLON"] = KC_SEMICOLON;
	allkeys["SLASH"] = KC_SLASH;
	allkeys["SLEEP"] = KC_SLEEP;
	allkeys["SPACE"] = KC_SPACE;
	allkeys["STOP"] = KC_STOP;
	allkeys["SUBTRACT"] = KC_SUBTRACT;
	allkeys["SYSRQ"] = KC_SYSRQ;
	allkeys["T"] = KC_T;
	allkeys["TAB"] = KC_TAB;
	allkeys["U"] = KC_U;
	//allkeys["UNASSIGNED"] = KC_UNASSIGNED;
	allkeys["UNDERLINE"] = KC_UNDERLINE;
	allkeys["UNLABELED"] = KC_UNLABELED;
	allkeys["UP"] = KC_UP;
	allkeys["V"] = KC_V;
	allkeys["VOLUMEDOWN"] = KC_VOLUMEDOWN;
	allkeys["VOLUMEUP"] = KC_VOLUMEUP;
	allkeys["W"] = KC_W;
	allkeys["WAKE"] = KC_WAKE;
	allkeys["WEBBACK"] = KC_WEBBACK;
	allkeys["WEBFAVORITES"] = KC_WEBFAVORITES;
	allkeys["WEBFORWARD"] = KC_WEBFORWARD;
	allkeys["WEBHOME"] = KC_WEBHOME;
	allkeys["WEBREFRESH"] = KC_WEBREFRESH;
	allkeys["WEBSEARCH"] = KC_WEBSEARCH;
	allkeys["WEBSTOP"] = KC_WEBSTOP;
	allkeys["X"] = KC_X;
	allkeys["Y"] = KC_Y;
	allkeys["YEN"] = KC_YEN;
	allkeys["Z"] = KC_Z;
}


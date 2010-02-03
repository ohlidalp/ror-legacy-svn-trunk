/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#ifndef __KeyManager_H__
#define __KeyManager_H__

#include "Ogre.h"
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <windows.h>
#endif
#include <string>
#include <math.h>
#include <stdio.h>
#include <map>
#include "OISEvents.h"
#include "OISInputManager.h"
#include "OISMouse.h"
#include "OISKeyboard.h"
#include "OISJoyStick.h"

// some shortcut
#define KEYMANAGER KeyManager::Instance()

typedef struct {
	OIS::KeyCode key;
	bool explicite;
	bool ctrl;
	bool shift;
	bool alt;
} key_combo_t;

class KeyManager
{
public:
	/** returns the singleton instance
     */
	static KeyManager & Instance();
	
	/** sets up the class
	 * @param mKeyboard the keyboard instace from OIs
     */
	void setup(OIS::Keyboard* mKeyboard);

	/** adds a Keys to the List
	 * @param name unique identifier of the key
	 * @param defaultkeycode default keycode
     */
	void addKey(std::string name, OIS::KeyCode key, bool ctrl, bool alt, bool shift, bool explicite);

	/** returns the state of a key
	 * @param name unique identifier of the key
     */
	bool isKeyDown(std::string name);
	bool isModifierDown(OIS::KeyCode mod);

	/** returns a string which labels the key
	 * @param key the keycode to search for
	 */
	std::string getNameForKey(OIS::KeyCode key);

	std::string getKeyForCommand(std::string name);

protected:
	KeyManager();
	KeyManager(const KeyManager&);
	KeyManager& operator= (const KeyManager&);

private:
	void initAllKeys();
	bool loadMapping();
	static KeyManager* myInstance;
	std::map<std::string, key_combo_t> keys;
	std::map<std::string, key_combo_t>::iterator it;
	std::map<std::string, OIS::KeyCode> allkeys;
	std::map<std::string, OIS::KeyCode>::iterator allit;
	OIS::Keyboard* mKeyboard;
};


#endif

/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#include "KeyManager.h"

using namespace Ogre;
using namespace OIS;
using namespace std;


// singleton pattern
KeyManager* KeyManager::myInstance = 0;

KeyManager & KeyManager::Instance () 
{
	if (myInstance == 0)
		myInstance = new KeyManager;
	return *myInstance;
}

KeyManager::KeyManager() : mKeyboard(0)
{
	initAllKeys();
	loadMapping();
}

void KeyManager::setup(Keyboard* mKeyboard)
{
	this->mKeyboard = mKeyboard;
}

void KeyManager::addKey(string name, KeyCode key, bool ctrl, bool alt, bool shift, bool explicite)
{
	/*
	// check if the same key is already existing!
	bool explicite = false;
	for ( map<string, key_combo_t>::iterator iter = keys.begin(); iter != keys.end(); ++iter) {
		if(iter->second.key == key) {
			iter->second.explicite = true;
			explicite = true;
			//LogManager::getSingleton().logMessage("explicit keys: " + string(iter->first));
		}
	}
	*/

	key_combo_t combo;
	combo.key = key;
	combo.explicite = explicite;
	combo.ctrl = ctrl;
	combo.alt = alt;
	combo.shift = shift;
	keys[name] = combo;
	LogManager::getSingleton().logMessage("key: " + name + "=" + getNameForKey(key) + ", ctrl=" + StringConverter::toString((int)ctrl) + ", alt=" + StringConverter::toString((int)alt) + ", shift=" + StringConverter::toString((int)shift)+ ", explicite=" + StringConverter::toString((int)explicite));
}

bool KeyManager::isModifierDown(OIS::KeyCode mod)
{
	if(mod != KC_LCONTROL && mod != KC_RCONTROL && mod != KC_LSHIFT && mod != KC_LSHIFT && mod != KC_LMENU && mod != KC_RMENU)
		return false;
	return this->mKeyboard->isKeyDown(mod);
}


bool KeyManager::isKeyDown(string name)
{
	//prevent use before initialisation
	if (this->mKeyboard == 0)
		return false;
	it = keys.find(name);
	if(it == keys.end())
	{
		LogManager::getSingleton().logMessage("unmapped key event: " + string(name) + ". You will need to map this key in order to use it!");
		return false;
	}
	
	key_combo_t combo = it->second;
	if(this->mKeyboard->isKeyDown(combo.key) == false)
		return false;
	// only use explicite mapping, if two keys with different modifiers exist, i.e. F1 and SHIFT+F1.
	if (combo.explicite) {
		if(combo.ctrl != (this->mKeyboard->isKeyDown(KC_LCONTROL) || this->mKeyboard->isKeyDown(KC_RCONTROL)))
			return false;
		if(combo.shift != (this->mKeyboard->isKeyDown(KC_LSHIFT) || this->mKeyboard->isKeyDown(KC_RSHIFT)))
			return false;
		if(combo.alt != (this->mKeyboard->isKeyDown(KC_LMENU) || this->mKeyboard->isKeyDown(KC_RMENU)))
			return false;
	} else {
		if(combo.ctrl && !(this->mKeyboard->isKeyDown(KC_LCONTROL) || this->mKeyboard->isKeyDown(KC_RCONTROL)))
			return false;
		if(combo.shift && !(this->mKeyboard->isKeyDown(KC_LSHIFT) || this->mKeyboard->isKeyDown(KC_RSHIFT)))
			return false;
		if(combo.alt && !(this->mKeyboard->isKeyDown(KC_LMENU) || this->mKeyboard->isKeyDown(KC_RMENU)))
			return false;
	}
	return true;
}

bool KeyManager::loadMapping()
{
	LogManager::getSingleton().logMessage("Loading key mapping...");
	char line[1024];
	char keyname[1024];
	char keycodes[255], *keycode=0;
	char *keycodes_work, *token;
	KeyCode key = KC_UNASSIGNED;
	bool alt=false, shift=false, ctrl=false, expl=false;
	const char delimiters[] = "+";
	size_t linelen = 0;

	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource("keymap.txt", "General");
	while (!ds->eof())
	{
		linelen = ds->readLine(line, 1023);
		if (line[0]==';' || linelen == 0)
			continue;
		sscanf(line, "%s %s", keyname, keycodes);

		// seperate all keys and construct the key combination
		alt=false;
		shift=false;
		ctrl=false;
		expl=false;
		keycodes_work = strdup(keycodes);
		token = strtok(keycodes_work, delimiters);
		while (token != NULL) {
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
			LogManager::getSingleton().logMessage("unkown key: " + string(keycodes));
			key = KC_UNASSIGNED;
			continue;
		} else {
			key = allit->second;
			//LogManager::getSingleton().logMessage("found key: " + string(keycode) + " = " + StringConverter::toString((int)key));
		}
		
		// add it to the list
		addKey(keyname, key, ctrl, alt, shift, expl);
	}

	// ds closes automatically, so do not close it explicitly here:
	//ds->close();
	
	LogManager::getSingleton().logMessage("key map successfully loaded!");
	return true;
}

string KeyManager::getNameForKey(KeyCode key)
{
	for ( map<string, KeyCode>::iterator iter = allkeys.begin(); iter != allkeys.end(); ++iter) {
		if(iter->second == key)
			return iter->first;
	}
	return "unkown";
}

string KeyManager::getKeyForCommand(string name)
{
	it = keys.find(name);
	if(it == keys.end())
		return string("");
	key_combo_t combo = it->second;
	return getNameForKey(combo.key);
}


void KeyManager::initAllKeys()
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


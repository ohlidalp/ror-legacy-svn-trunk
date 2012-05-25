/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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
#include "GameScript.h"

// AS addons start
#include "scriptstdstring/scriptstdstring.h"
#include "scriptmath/scriptmath.h"
#include "contextmgr/contextmgr.h"
#include "scriptany/scriptany.h"
#include "scriptarray/scriptarray.h"
#include "scripthelper/scripthelper.h"
#include "scriptstring/scriptstring.h"
// AS addons end

#ifdef USE_CURL
#define CURL_STATICLIB
#include <stdio.h>
#include <curl/curl.h>
//#include <curl/types.h>
#include <curl/easy.h>
#endif //USE_CURL

#include "Beam.h"
#include "BeamEngine.h"
#include "BeamFactory.h"
#include "Character.h"
#include "Console.h"
#include "heightfinder.h"
#include "IWater.h"
#include "language.h"
#include "network.h"
#include "RoRFrameListener.h"
#include "RoRVersion.h"
#include "SelectorWindow.h"
#include "Settings.h"
#include "SkyManager.h"
#include "utils.h"

using namespace Ogre;

/* class that implements the interface for the scripts */
GameScript::GameScript(ScriptEngine *se, RoRFrameListener *efl) : mse(se), mefl(efl), apiThread()
{
}

GameScript::~GameScript()
{
}

void GameScript::log(std::string &msg)
{
	SLOG(msg);
}

double GameScript::getTime()
{
	return this->mefl->getTime();
}

void GameScript::setPersonPosition(Vector3 &vec)
{
	if(mefl && mefl->person) mefl->person->setPosition(Vector3(vec.x, vec.y, vec.z));
}

void GameScript::loadTerrain(std::string &terrain)
{
	if(mefl) mefl->loadTerrain(terrain);
}

Vector3 GameScript::getPersonPosition()
{
	if(mefl && mefl->person)
	{
		Vector3 ov = mefl->person->getPosition();
		return Vector3(ov.x, ov.y, ov.z);
	}
	return Vector3::ZERO;
}

void GameScript::movePerson(Vector3 vec)
{
	if(mefl && mefl->person) mefl->person->move(Vector3(vec.x, vec.y, vec.z));
}

std::string GameScript::getCaelumTime()
{
#ifdef USE_CAELUM
	return SkyManager::getSingleton().getPrettyTime();
#else
	return "";
#endif // USE_CAELUM
}

void GameScript::setCaelumTime(float value)
{
#ifdef USE_CAELUM
	SkyManager::getSingleton().setTimeFactor(value);
#endif // USE_CAELUM
}

bool GameScript::getCaelumAvailable()
{
#ifdef USE_CAELUM
	return SkyManager::getSingletonPtr() != 0;
#else
	return false;
#endif // USE_CAELUM
}

float GameScript::stopTimer()
{
	if(mefl) return mefl->stopTimer();
	return 0.0;
}

void GameScript::startTimer()
{
	if(mefl) mefl->startTimer();
}

void GameScript::setWaterHeight(float value)
{
	if(mefl && mefl->w) mefl->w->setHeight(value);
}

float GameScript::getGroundHeight(Vector3 &v)
{
	if(RoRFrameListener::hfinder)
		return RoRFrameListener::hfinder->getHeightAt(v.x, v.z);
	return -1;
}

float GameScript::getWaterHeight()
{
	if(mefl && mefl->w) return mefl->w->getHeight();
	return 0;
}

Beam *GameScript::getCurrentTruck()
{
	return BeamFactory::getSingleton().getCurrentTruck();
}

float GameScript::getGravity()
{
	if(mefl) return mefl->getGravity();
	return 0;
}

void GameScript::setGravity(float value)
{
	if(mefl) mefl->setGravity(value);
}

Beam *GameScript::getTruckByNum(int num)
{
	return BeamFactory::getSingleton().getTruck(num);
}

int GameScript::getNumTrucks()
{
	return BeamFactory::getSingleton().getTruckCount();
}

int GameScript::getNumTrucksByFlag(int flag)
{
	int res = 0;
	for(int i=0; i< BeamFactory::getSingleton().getTruckCount(); i++)
	{
		Beam *truck = BeamFactory::getSingleton().getTruck(i);
		if(!truck && !flag)
			res++;
		if(!truck) continue;
		if(truck->state == flag)
			res++;
	}
	return res;
}

int GameScript::getCurrentTruckNumber()
{
	if(mefl) return BeamFactory::getSingleton().getCurrentTruckNumber();
	return -1;
}

void GameScript::registerForEvent(int eventValue)
{
	if(!mse) return;
	mse->eventMask += eventValue;
}

void GameScript::flashMessage(std::string &txt, float time, float charHeight)
{
#ifdef USE_MYGUI
	Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, "script_code_red.png");
#endif // USE_MYGUI
}

void GameScript::message(std::string &txt, std::string &icon, float timeMilliseconds, bool forceVisible)
{
#ifdef USE_MYGUI
	Console::getSingleton().putMessage(Console::CONSOLE_MSGTYPE_SCRIPT, Console::CONSOLE_SYSTEM_NOTICE, txt, icon, timeMilliseconds, forceVisible);
#endif // USE_MYGUI
}

void GameScript::setDirectionArrow(std::string &text, Vector3 &vec)
{
	if(mefl) mefl->setDirectionArrow(const_cast<char*>(text.c_str()), Vector3(vec.x, vec.y, vec.z));
}

int GameScript::getChatFontSize()
{
	return 0; //NETCHAT.getFontSize();
}

void GameScript::setChatFontSize(int size)
{
	//NETCHAT.setFontSize(size);
}

void GameScript::showChooser(std::string &type, std::string &instance, std::string &box)
{
#ifdef USE_MYGUI
	SelectorWindow::LoaderType ntype = SelectorWindow::LT_None;
	
	if (type == "airplane")    ntype = SelectorWindow::LT_Airplane;
	if (type == "all")         ntype = SelectorWindow::LT_AllBeam;
	if (type == "boat")        ntype = SelectorWindow::LT_Boat;
	if (type == "car")         ntype = SelectorWindow::LT_Car;
	if (type == "extension")   ntype = SelectorWindow::LT_Extension;
	if (type == "heli")        ntype = SelectorWindow::LT_Heli;
	if (type == "load")        ntype = SelectorWindow::LT_Load;
	if (type == "trailer")     ntype = SelectorWindow::LT_Trailer;
	if (type == "train")       ntype = SelectorWindow::LT_Train;
	if (type == "truck")       ntype = SelectorWindow::LT_Truck;
	if (type == "vehicle")     ntype = SelectorWindow::LT_Vehicle;
	
	if (ntype != SelectorWindow::LT_None)
	{
		mefl->showLoad(ntype, const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
	}
#endif //USE_MYGUI
}

void GameScript::repairVehicle(std::string &instance, std::string &box, bool keepPosition)
{
	BeamFactory::getSingleton().repairTruck(mefl->getCollisions(), const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()), keepPosition);
}

void GameScript::removeVehicle(std::string &instance, std::string &box)
{
	BeamFactory::getSingleton().removeTruck(mefl->getCollisions(), const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
}


void GameScript::destroyObject(const std::string &instanceName)
{
	mefl->unloadObject(const_cast<char*>(instanceName.c_str()));
}

void GameScript::spawnObject(const std::string &objectName, const std::string &instanceName, Vector3 &pos, Vector3 &rot, const std::string &eventhandler, bool uniquifyMaterials)
{
	AngelScript::asIScriptModule *mod=0;
	try
	{
		mod = mse->getEngine()->GetModule(mse->moduleName, AngelScript::asGM_ONLY_IF_EXISTS);
	}catch(std::exception e)
	{
		SLOG("Exception in spawnObject(): " + String(e.what()));
		return;
	}
	if(!mod) return;
	int functionPtr = mod->GetFunctionIdByName(eventhandler.c_str());

	// trying to create the new object
	SceneNode *bakeNode=mefl->getSceneMgr()->getRootSceneNode()->createChildSceneNode();
	mefl->loadObject(const_cast<char*>(objectName.c_str()), pos.x, pos.y, pos.z, rot.x, rot.y, rot.z, bakeNode, const_cast<char*>(instanceName.c_str()), true, functionPtr, const_cast<char*>(objectName.c_str()), uniquifyMaterials);
}

void GameScript::hideDirectionArrow()
{
	if(mefl) mefl->setDirectionArrow(0, Vector3::ZERO);
}

int GameScript::setMaterialAmbient(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setAmbient(red, green, blue);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialAmbient(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::setMaterialDiffuse(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setDiffuse(red, green, blue, alpha);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialDiffuse(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::setMaterialSpecular(const std::string &materialName, float red, float green, float blue, float alpha)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSpecular(red, green, blue, alpha);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialSpecular(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::setMaterialEmissive(const std::string &materialName, float red, float green, float blue)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 0;
		m->setSelfIllumination(red, green, blue);
	} catch(Exception e)
	{
		SLOG("Exception in setMaterialEmissive(): " + e.getFullDescription());
		return 0;
	}
	return 1;
}

int GameScript::getSafeTextureUnitState(TextureUnitState **tu, const std::string materialName, int techniqueNum, int passNum, int textureUnitNum)
{
	try
	{
		MaterialPtr m = MaterialManager::getSingleton().getByName(materialName);
		if(m.isNull()) return 1;

		// verify technique
		if(techniqueNum < 0 || techniqueNum > m->getNumTechniques()) return 2;
		Technique *t = m->getTechnique(techniqueNum);
		if(!t) return 2;

		//verify pass
		if(passNum < 0 || passNum > t->getNumPasses()) return 3;
		Pass *p = t->getPass(passNum);
		if(!p) return 3;

		//verify texture unit
		if(textureUnitNum < 0 || textureUnitNum > p->getNumTextureUnitStates()) return 4;
		TextureUnitState *tut = p->getTextureUnitState(textureUnitNum);
		if(!tut) return 4;

		*tu = tut;
		return 0;
	} catch(Exception e)
	{
		SLOG("Exception in getSafeTextureUnitState(): " + e.getFullDescription());
	}
	return 1;
}

int GameScript::setMaterialTextureName(const std::string &materialName, int techniqueNum, int passNum, int textureUnitNum, const std::string &textureName)
{
	TextureUnitState *tu = 0;
	int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
	if(res == 0 && tu != 0)
	{
		// finally, set it
		tu->setTextureName(textureName);
	}
	return res;
}

int GameScript::setMaterialTextureRotate(const std::string &materialName, int techniqueNum, int passNum, int textureUnitNum, float rotation)
{
	TextureUnitState *tu = 0;
	int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
	if(res == 0 && tu != 0)
	{
		tu->setTextureRotate(Degree(rotation));
	}
	return res;
}

int GameScript::setMaterialTextureScroll(const std::string &materialName, int techniqueNum, int passNum, int textureUnitNum, float sx, float sy)
{
	TextureUnitState *tu = 0;
	int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
	if(res == 0 && tu != 0)
	{
		tu->setTextureScroll(sx, sy);
	}
	return res;
}

int GameScript::setMaterialTextureScale(const std::string &materialName, int techniqueNum, int passNum, int textureUnitNum, float u, float v)
{
	TextureUnitState *tu = 0;
	int res = getSafeTextureUnitState(&tu, materialName, techniqueNum, passNum, textureUnitNum);
	if(res == 0 && tu != 0)
	{
		tu->setTextureScale(u, v);
	}
	return res;
}


float GameScript::rangeRandom(float from, float to)
{
	return Math::RangeRandom(from, to);
}

int GameScript::getLoadedTerrain(std::string &result)
{
	result = mefl->loadedTerrain;
	return 0;
}

void GameScript::clearEventCache()
{
	mefl->getCollisions()->clearEventCache();
}

void GameScript::setCameraPosition(Vector3 &pos)
{
	// TODO: TOFIX
	//mefl->getCamera()->setPosition(Vector3(pos.x, pos.y, pos.z));
}

void GameScript::setCameraDirection(Vector3 &rot)
{
	// TODO: TOFIX
	//mefl->getCamera()->setDirection(Vector3(rot.x, rot.y, rot.z));
}

void GameScript::setCameraYaw(float rotX)
{
	// TODO: TOFIX
	//mefl->getCamera()->yaw(Degree(rotX));
}

void GameScript::setCameraPitch(float rotY)
{
	// TODO: TOFIX
	//mefl->getCamera()->pitch(Degree(rotY));
}

void GameScript::setCameraRoll(float rotZ)
{
	// TODO: TOFIX
	//mefl->getCamera()->roll(Degree(rotZ));
}

Vector3 GameScript::getCameraPosition()
{
	// TODO: TOFIX
	//Vector3 pos = mefl->getCamera()->getPosition();
	return Vector3::ZERO; //Vector3(pos.x, pos.y, pos.z);
}

Vector3 GameScript::getCameraDirection()
{
	// TODO: TOFIX
	//Vector3 rot = mefl->getCamera()->getDirection();
	return Vector3::ZERO; //(rot.x, rot.y, rot.z);
}

void GameScript::cameraLookAt(Vector3 &pos)
{
	// TODO: TOFIX
	//mefl->getCamera()->lookAt(Vector3(pos.x, pos.y, pos.z));
}

#ifdef USE_CURL
//hacky hack to fill memory with data for curl
// from: http://curl.haxx.se/libcurl/c/getinmemory.html
static size_t curlWriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct curlMemoryStruct *mem = (struct curlMemoryStruct *)data;

  mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}
#endif //USE_CURL

void *onlineAPIThread(void *_params)
{
	// copy over params
	GameScript::OnlineAPIParams_t params = *(GameScript::OnlineAPIParams_t *)_params;

	// call the function
	params.cls->useOnlineAPIDirectly(params);

	// free the params
	params.dict->Release();
	free(_params);
	_params = NULL;

	pthread_exit(NULL);
	return NULL;
}

int GameScript::useOnlineAPIDirectly(OnlineAPIParams_t params)
{
#ifdef USE_CURL
	struct curlMemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	// construct post fields
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	curl_global_init(CURL_GLOBAL_ALL);

	std::map<std::string, AngelScript::CScriptDictionary::valueStruct>::const_iterator it;
	for(it = params.dict->dict.begin(); it != params.dict->dict.end(); it++)
	{
		int typeId = it->second.typeId;
		if(typeId == mse->getEngine()->GetTypeIdByDecl("string"))
		{
			// its a std::string
			std::string *str = (std::string *)it->second.valueObj;
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, str->c_str(), CURLFORM_END);
		}
		else if(typeId == AngelScript::asTYPEID_INT8 \
			|| typeId == AngelScript::asTYPEID_INT16 \
			|| typeId == AngelScript::asTYPEID_INT32 \
			|| typeId == AngelScript::asTYPEID_INT64)
		{
			// its an integer
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((int)it->second.valueInt).c_str(), CURLFORM_END);
		}
		else if(typeId == AngelScript::asTYPEID_UINT8 \
			|| typeId == AngelScript::asTYPEID_UINT16 \
			|| typeId == AngelScript::asTYPEID_UINT32 \
			|| typeId == AngelScript::asTYPEID_UINT64)
		{
			// its an unsigned integer
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((unsigned int)it->second.valueInt).c_str(), CURLFORM_END);
		}
		else if(typeId == AngelScript::asTYPEID_FLOAT || typeId == AngelScript::asTYPEID_DOUBLE)
		{
			// its a float or double
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, TOSTRING((float)it->second.valueFlt).c_str(), CURLFORM_END);
		}
	}

	// add some hard coded values
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_Name", CURLFORM_COPYCONTENTS, RoRFrameListener::eflsingleton->terrainName.c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_FileName", CURLFORM_COPYCONTENTS, RoRFrameListener::eflsingleton->terrainFileName.c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_FileHash", CURLFORM_COPYCONTENTS, RoRFrameListener::eflsingleton->terrainFileHash.c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ModHash", CURLFORM_COPYCONTENTS, RoRFrameListener::eflsingleton->terrainModHash.c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptName", CURLFORM_COPYCONTENTS, mse->getScriptName().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptHash", CURLFORM_COPYCONTENTS, mse->getScriptHash().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_NickName", CURLFORM_COPYCONTENTS, SSETTING("Nickname", "Anonymous").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Language", CURLFORM_COPYCONTENTS, SSETTING("Language", "English").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Token", CURLFORM_COPYCONTENTS, SSETTING("User Token Hash", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionString", CURLFORM_COPYCONTENTS, ROR_VERSION_STRING, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVN", CURLFORM_COPYCONTENTS, SVN_REVISION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVNID", CURLFORM_COPYCONTENTS, SVN_ID, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_ProtocolVersion", CURLFORM_COPYCONTENTS, RORNET_VERSION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_BinaryHash", CURLFORM_COPYCONTENTS, SSETTING("BinaryHash", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_GUID", CURLFORM_COPYCONTENTS, SSETTING("GUID", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerName", CURLFORM_COPYCONTENTS, SSETTING("Server name", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerPort", CURLFORM_COPYCONTENTS, SSETTING("Server port", "-").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_NetworkEnabled", CURLFORM_COPYCONTENTS, SSETTING("Network enable", "No").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "APIProtocolVersion", CURLFORM_COPYCONTENTS, "2", CURLFORM_END);

	if(BeamFactory::getSingleton().getCurrentTruck())
	{
		Beam *truck = BeamFactory::getSingleton().getCurrentTruck();
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_Name",     CURLFORM_COPYCONTENTS, truck->getTruckName().c_str(), CURLFORM_END);
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_FileName", CURLFORM_COPYCONTENTS, truck->getTruckFileName().c_str(), CURLFORM_END);
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_Hash",     CURLFORM_COPYCONTENTS, truck->getTruckHash().c_str(), CURLFORM_END);

		// look for any locked trucks
		int i = 0;
		for(std::vector<hook_t>::iterator it = truck->hooks.begin(); it != truck->hooks.end(); it++, i++)
		{
			Beam *trailer = it->lockTruck;
			if (trailer && trailer->getTruckName() != trailer->getTruckName())
			{
				String name = "Trailer_" + TOSTRING(i) + "_Name";
				curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, name.c_str(),     CURLFORM_COPYCONTENTS, trailer->getTruckName().c_str(), CURLFORM_END);
				String filename = "Trailer_" + TOSTRING(i) + "_FileName";
				curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, filename.c_str(), CURLFORM_COPYCONTENTS, trailer->getTruckFileName().c_str(), CURLFORM_END);
				String hash = "Trailer_" + TOSTRING(i) + "_Hash";
				curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, hash.c_str(),     CURLFORM_COPYCONTENTS, trailer->getTruckHash().c_str(), CURLFORM_END);
			}
		}
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Trailer_Count",     CURLFORM_COPYCONTENTS, TOSTRING(i).c_str(), CURLFORM_END);
	}

	const RenderTarget::FrameStats& stats = mefl->getRenderWindow()->getStatistics();
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "AVG_FPS", CURLFORM_COPYCONTENTS, TOSTRING(stats.avgFPS).c_str(), CURLFORM_END);



	CURLcode res;
	CURL *curl = curl_easy_init();
	if(!curl)
	{
		LOG("ERROR: failed to init curl");
		return 1;
	}

	char *curl_err_str[CURL_ERROR_SIZE];
	memset(curl_err_str, 0, CURL_ERROR_SIZE);

	std::string url = "http://" + std::string(REPO_SERVER) + params.apiquery;
	curl_easy_setopt(curl, CURLOPT_URL,              url.c_str());

	/* send all data to this function  */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteMemoryCallback);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	// set post options
	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	// logging stuff
	//curl_easy_setopt(curl, CURLOPT_STDERR,           LogManager::getsin InstallerLog::getSingleton()->getLogFilePtr());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER,      curl_err_str[0]);

	// http related settings
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,   1); // follow redirects
	curl_easy_setopt(curl, CURLOPT_AUTOREFERER,      1); // set the Referrer: field in requests where it follows a Location: redirect.
	curl_easy_setopt(curl, CURLOPT_MAXREDIRS,        20);
	curl_easy_setopt(curl, CURLOPT_USERAGENT,        "RoR");
	curl_easy_setopt(curl, CURLOPT_FILETIME,         1);

	// TO BE DONE: ADD SSL
	// see: http://curl.haxx.se/libcurl/c/simplessl.html
	// curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);

	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	//printf("%lu bytes retrieved\n", (long)chunk.size);

	curl_formfree(formpost);

	std::string result;

	if(chunk.memory)
	{
		// convert memory into std::string now
		result = std::string(chunk.memory);

		// then free
		free(chunk.memory);
	}

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	if(res != CURLE_OK)
	{
		const char *errstr = curl_easy_strerror(res);
		result = "ERROR: " + std::string(errstr);
	}

	LOG("online API result: " + result);

#ifdef USE_MYGUI
	Console *con = Console::getSingletonPtrNoCreation();
	if(con)
		con->putMessage(Console::CONSOLE_MSGTYPE_HIGHSCORE, Console::CONSOLE_SYSTEM_NOTICE, ANSI_TO_UTF(result));
#endif // USE_MYGUI
#endif //USE_CURL
	return 0;
}

int GameScript::useOnlineAPI(const std::string &apiquery, const AngelScript::CScriptDictionary &d, std::string &result)
{
	// malloc this, so we are safe from this function scope
	OnlineAPIParams_t *params = (OnlineAPIParams_t *)malloc(sizeof(OnlineAPIParams_t));
	if (!params)
	{
		free(params);
		return 1;
	}
	params->cls      = this;
	strncpy(params->apiquery, apiquery.c_str(), 2048);

	//wrap a new dict around this, as we dont know if or when the script will release it
	AngelScript::CScriptDictionary *newDict = new AngelScript::CScriptDictionary(mse->getEngine());
	// copy over the dict, the main data
	newDict->dict    = d.dict;
	// assign it to the data container
	params->dict     = newDict;
	// tell the script that there will be no direct feedback
	result           = "asynchronous";

#ifdef USE_MYGUI
	Console *con = Console::getSingletonPtrNoCreation();
	if(con) con->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, _L("using Online API..."), "information.png", 2000);
#endif // USE_MYGUI

	// fix the std::string objects in the dict
	// why we need to do this: when we copy the std::map (dict) over, we calso jsut copy the pointers to std::string in it.
	// when this continues and forks, AS releases the strings.
	// so we will allocate new strings that are persistent.
	std::map<std::string, AngelScript::CScriptDictionary::valueStruct>::iterator it;
	for(it = params->dict->dict.begin(); it != params->dict->dict.end(); it++)
	{
		int typeId = it->second.typeId;
		if(typeId == mse->getEngine()->GetTypeIdByDecl("string"))
		{
			// its a std::string, copy it over
			std::string *str = (std::string *)it->second.valueObj;
			it->second.valueObj = (void *)new std::string(*str);
		}
	}

	// create the thread
	LOG("creating thread for online API usage...");
	int rc = pthread_create(&apiThread, NULL, onlineAPIThread, (void *)params);
	if(rc)
	{
		LOG("useOnlineAPI/pthread error code: " + TOSTRING(rc));
		return 1;
	}

	return 0;
}

void GameScript::boostCurrentTruck(float factor)
{
    // add C++ code here
        Beam *b = BeamFactory::getSingleton().getCurrentTruck();
        if(b && b->engine)
        {
                float rpm = b->engine->getRPM();
				rpm += 2000.0f * factor;
				b->engine->setRPM(rpm);
        }

}

int GameScript::addScriptFunction(const std::string &arg)
{
	return mse->addFunction(arg);
}

int GameScript::scriptFunctionExists(const std::string &arg)
{
	return mse->functionExists(arg);
}

int GameScript::deleteScriptFunction(const std::string &arg)
{
	return mse->deleteFunction(arg);
}

int GameScript::addScriptVariable(const std::string &arg)
{
	return mse->addVariable(arg);
}

int GameScript::deleteScriptVariable(const std::string &arg)
{
	return mse->deleteVariable(arg);
}

int GameScript::sendGameCmd(const std::string& message)
{
	Network *net = mefl->getNetwork();
	if(!net) return -11;
	else return net->sendScriptMessage(const_cast<char*>(message.c_str()), (unsigned int)message.size());
}

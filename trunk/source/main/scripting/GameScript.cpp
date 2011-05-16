#include "GameScript.h"
#include "Ogre.h"
#include "RoRFrameListener.h"
#include "BeamFactory.h"
#include "ScriptEngine.h"

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
#include <curl/types.h>
#include <curl/easy.h>
#endif //USE_CURL

#include "rornet.h"
#include "water.h"
#include "Beam.h"
#include "Settings.h"
#include "Console.h"
#include "OverlayWrapper.h"
#include "SkyManager.h"
#include "CacheSystem.h"
#include "as_ogre.h"
#include "SelectorWindow.h"
#include "sha1.h"
#include "collisions.h"

/* class that implements the interface for the scripts */
GameScript::GameScript(ScriptEngine *se, RoRFrameListener *efl) : mse(se), mefl(efl)
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

void GameScript::setPersonPosition(Ogre::Vector3 vec)
{
	if(mefl && mefl->person) mefl->person->setPosition(Ogre::Vector3(vec.x, vec.y, vec.z));
}

void GameScript::loadTerrain(std::string &terrain)
{
	if(mefl) mefl->loadTerrain(terrain);
}

Ogre::Vector3 GameScript::getPersonPosition()
{
	if(mefl && mefl->person)
	{
		Ogre::Vector3 ov = mefl->person->getPosition();
		return Ogre::Vector3(ov.x, ov.y, ov.z);
	}
	return Ogre::Vector3::ZERO;
}

void GameScript::movePerson(Ogre::Vector3 vec)
{
	if(mefl && mefl->person) mefl->person->move(Ogre::Vector3(vec.x, vec.y, vec.z));
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

float GameScript::getGroundHeight(Ogre::Vector3 v)
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
	//mse->eventMask += eventValue;
}

void GameScript::flashMessage(std::string &txt, float time, float charHeight)
{
	if(mefl && mefl->getOverlayWrapper())
		mefl->getOverlayWrapper()->flashMessage(txt, time, charHeight);
}

void GameScript::setDirectionArrow(std::string &text, Ogre::Vector3 vec)
{
	if(mefl) mefl->setDirectionArrow(const_cast<char*>(text.c_str()), Ogre::Vector3(vec.x, vec.y, vec.z));
}

int GameScript::getChatFontSize()
{
	return 0; //NETCHAT.getFontSize();
}

void GameScript::setChatFontSize(int size)
{
	//NETCHAT.setFontSize(size);
}

void GameScript::showChooser(string &type, string &instance, string &box)
{
#ifdef USE_MYGUI
	SelectorWindow::LoaderType ntype = SelectorWindow::LT_None;
	if (type == "vehicle")   ntype = SelectorWindow::LT_Vehicle;
	if (type == "truck")     ntype = SelectorWindow::LT_Truck;
	if (type == "car")       ntype = SelectorWindow::LT_Truck;
	if (type == "boat")      ntype = SelectorWindow::LT_Boat;
	if (type == "airplane")  ntype = SelectorWindow::LT_Airplane;
	if (type == "heli")      ntype = SelectorWindow::LT_Heli;
	if (type == "trailer")   ntype = SelectorWindow::LT_Trailer;
	if (type == "load")      ntype = SelectorWindow::LT_Load;
	if (type == "extension") ntype = SelectorWindow::LT_Extension;
	if (ntype != SelectorWindow::LT_None)
		mefl->showLoad(ntype, const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
#endif //USE_MYGUI
}

void GameScript::repairVehicle(string &instance, string &box, bool keepPosition)
{
	BeamFactory::getSingleton().repairTruck(mefl->getSSM(), mefl->getCollisions(), const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()), keepPosition);
}

void GameScript::removeVehicle(string &instance, string &box)
{
	BeamFactory::getSingleton().removeTruck(mefl->getCollisions(), const_cast<char*>(instance.c_str()), const_cast<char*>(box.c_str()));
}


void GameScript::destroyObject(const std::string &instanceName)
{
	mefl->unloadObject(const_cast<char*>(instanceName.c_str()));
}

void GameScript::spawnObject(const std::string &objectName, const std::string &instanceName, Ogre::Vector3 pos, Ogre::Vector3 rot, const std::string &eventhandler, bool uniquifyMaterials)
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
	if(mefl) mefl->setDirectionArrow(0, Ogre::Vector3::ZERO);
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

float GameScript::rangeRandom(float from, float to)
{
	return Ogre::Math::RangeRandom(from, to);
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

void GameScript::setCameraPosition(Ogre::Vector3 pos)
{
	mefl->getCamera()->setPosition(Ogre::Vector3(pos.x, pos.y, pos.z));
}

void GameScript::setCameraDirection(Ogre::Vector3 rot)
{
	mefl->getCamera()->setDirection(Ogre::Vector3(rot.x, rot.y, rot.z));
}

void GameScript::setCameraYaw(float rotX)
{
	mefl->getCamera()->yaw(Ogre::Degree(rotX));
}

void GameScript::setCameraPitch(float rotY)
{
	mefl->getCamera()->pitch(Ogre::Degree(rotY));
}

void GameScript::setCameraRoll(float rotZ)
{
	mefl->getCamera()->roll(Ogre::Degree(rotZ));
}

Ogre::Vector3 GameScript::getCameraPosition()
{
	Ogre::Vector3 pos = mefl->getCamera()->getPosition();
	return Ogre::Vector3(pos.x, pos.y, pos.z);
}

Ogre::Vector3 GameScript::getCameraDirection()
{
	Ogre::Vector3 rot = mefl->getCamera()->getDirection();
	return Ogre::Vector3(rot.x, rot.y, rot.z);
}

void GameScript::cameraLookAt(Ogre::Vector3 pos)
{
	mefl->getCamera()->lookAt(Ogre::Vector3(pos.x, pos.y, pos.z));
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

int GameScript::useOnlineAPI(const std::string &apiquery, const AngelScript::CScriptDictionary &d, std::string &result)
{
#ifdef USE_CURL1
	struct curlMemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */

	// construct post fields
	struct curl_httppost *formpost=NULL;
	struct curl_httppost *lastptr=NULL;
	curl_global_init(CURL_GLOBAL_ALL);

	std::map<std::string, AngelScript::CScriptDictionary::valueStruct>::const_iterator it;
	for(it = d.dict.begin(); it != d.dict.end(); it++)
	{
		int typeId = it->second.typeId;
		if(typeId == mse->getEngine()->GetTypeIdByDecl("string"))
		{
			// its a string
			curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, it->first.c_str(), CURLFORM_COPYCONTENTS, ((std::string *)it->second.valueObj)->c_str(), CURLFORM_END);
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
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_Name", CURLFORM_COPYCONTENTS, mse->getTerrainName().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_Hash", CURLFORM_COPYCONTENTS, SSETTING("TerrainHash").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "terrain_ScriptHash", CURLFORM_COPYCONTENTS, mse->getTerrainScriptHash().c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_NickName", CURLFORM_COPYCONTENTS, SSETTING("Nickname").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Language", CURLFORM_COPYCONTENTS, SSETTING("Language").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "User_Token", CURLFORM_COPYCONTENTS, SSETTING("User Token Hash").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionString", CURLFORM_COPYCONTENTS, ROR_VERSION_FULL_STRING, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVN", CURLFORM_COPYCONTENTS, SVN_REVISION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_VersionSVNID", CURLFORM_COPYCONTENTS, SVN_ID, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_ProtocolVersion", CURLFORM_COPYCONTENTS, RORNET_VERSION, CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_BinaryHash", CURLFORM_COPYCONTENTS, SSETTING("BinaryHash").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "RoR_GUID", CURLFORM_COPYCONTENTS, SSETTING("GUID").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerName", CURLFORM_COPYCONTENTS, SSETTING("Server name").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_ServerPort", CURLFORM_COPYCONTENTS, SSETTING("Server port").c_str(), CURLFORM_END);
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "MP_NetworkEnabled", CURLFORM_COPYCONTENTS, SSETTING("Network enable").c_str(), CURLFORM_END);

	if(BeamFactory::getSingleton().getCurrentTruck())
	{
		Beam *truck = BeamFactory::getSingleton().getCurrentTruck();

		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_Name", CURLFORM_COPYCONTENTS, truck->getTruckName().c_str(), CURLFORM_END);
		//curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_ModHash", CURLFORM_COPYCONTENTS, truck->cacheEntryInfo.hash.c_str(), CURLFORM_END);
		//curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "Truck_ModFileName", CURLFORM_COPYCONTENTS, truck->cacheEntryInfo.fname.c_str(), CURLFORM_END);
	}

	const RenderTarget::FrameStats& stats = mefl->getRenderWindow()->getStatistics();
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "AVG_FPS", CURLFORM_COPYCONTENTS, TOSTRING(stats.avgFPS).c_str(), CURLFORM_END);



	CURLcode res;
	CURL *curl = curl_easy_init();
	if(!curl)
	{
		result = "ERROR: failed to init curl";
		return 1;
	}

	char *curl_err_str[CURL_ERROR_SIZE];
	memset(curl_err_str, 0, CURL_ERROR_SIZE);

	string url = "http://" + string(REPO_SERVER) + apiquery;
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

	if(chunk.memory)
	{
		// convert memory into std::string now
		result = string(chunk.memory);

		// then free
		free(chunk.memory);
	}

	/* we're done with libcurl, so clean it up */
	curl_global_cleanup();

	if(res != CURLE_OK)
	{
		const char *errstr = curl_easy_strerror(res);
		result = "ERROR: " + string(errstr);
		return 1;
	}

	return 0;
#endif //USE_CURL
	return 1;
}

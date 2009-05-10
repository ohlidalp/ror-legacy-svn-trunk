#ifndef __Gamescript_H__
#define __Gamescript_H__

/**
 *  @brief Proxy class that can be called by script functions
 */
class GameScript
{
public:
	/**
	 * writes a message to the games log (RoR.log)
	 * @param msg string to log
	 */
	void log(std::string &msg)
	{ printf("%-30s| %s\n", __FUNCTION__, msg.c_str()); };

	/**
	 * returns the time in seconds since the game was started
	 * @return time in seconds
	 * 
	 * @par Example Usage:
	 * @code
	 * game.log("" + game.getTime())
	 * @endcode
	 */
	double getTime()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * sets the character position
	 * @param x X position on the terrain
	 * @param y Y position on the terrain
	 * @param z Z position on the terrain
	 */
	void setPersonPosition(float x, float y, float z)
	{ printf("%-30s| %f, %f, %f\n", __FUNCTION__, x, y, z); };

	/**
	 * moves the person relative
	 * @param x X translation
	 * @param y Y translation
	 * @param z Z translation
	 */
	void movePerson(float x, float y, float z)
	{ printf("%-30s| %f, %f, %f\n", __FUNCTION__, x, y, z); };

	/**
	 * gets the time of the day in seconds
	 * @return day time in seconds
	 */
	float getCaelumTime()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };
	
	/**
	 * sets the time of the day in seconds
	 * @param value day time in seconds
	 */
	void setCaelumTime(float value)
	{ printf("%-30s| %f\n", __FUNCTION__, value); };
	
	/**
	 * returns the current base water level (without waves)
	 * @return water height in meters
	 */
	float getWaterHeight()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * sets the base water height
	 * @param value base height in meters
	 */
	void setWaterHeight(float value)
	{ printf("%-30s| %f\n", __FUNCTION__, value); };

	/**
	 * returns the current selected truck, 0 if in person mode
	 * @return reference to Beam object that is currently in use
	 */
	Beam *getCurrentTruck()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * returns a truck by index, get max index by calling getNumTrucks
	 * @return reference to Beam object that the selected slot
	 */
	Beam *getTruckByNum(int num)
	{ printf("%-30s| %d\n", __FUNCTION__, num); return 0; };

	/**
	 * returns the current amount of loaded trucks
	 * @return integer value representing the amount of loaded trucks
	 */
	int getNumTrucks()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * returns the current truck number. >=0 when using a truck, -1 when in person mode
	 * @return integer truck number
	 */
	int getCurrentTruckNumber()
	{ printf("%-30s|\n", __FUNCTION__); return -1; };
	
	/**
	 * returns the currently set upo gravity
	 * @return float number describing gravity terrain wide.
	 */
	float getGravity()
	{ printf("%-30s|\n", __FUNCTION__);return 0; };
	
	/**
	 * sets the gravity terrain wide. This is an expensive call, since the masses of all trucks are recalculated.
	 * @param value new gravity terrain wide (default is -9.81)
	 */
	void setGravity(float value)
	{ printf("%-30s| %f\n", __FUNCTION__, value); };

	/**
	 * registers for a new event to be received by the scripting system
	 * @param eventValue \see enum scriptEvents
	 */
	void registerForEvent(int eventValue)
	{ printf("%-30s| %d\n", __FUNCTION__, eventValue); };

	/**
	 * shows a message to the user
	 * @param txt text to be displayed. "" to hide the text
	 * @param time display time in seconds. default: 1
	 * @param charHeight in percent of the screen. use -1 for default
	 */
	void flashMessage(std::string &txt, float time, float charHeight)
	{ printf("%-30s| %s, %f, %f\n", __FUNCTION__, txt.c_str(), time, charHeight); };

	/**
	 * set direction arrow
	 * @param text text to be displayed. "" to hide the text
	 * @param positionx x position on terrain to point to
	 * @param positiony y position on terrain to point to
	 * @param positionz z position on terrain to point to
	 */
	void setDirectionArrow(std::string &text, float positionx, float positiony, float positionz)
	{ printf("%-30s| %s, %f, %f, %f\n", __FUNCTION__, text.c_str(), positionx, positiony, positionz); };


	/**
	 * returns the size of the font used by the chat box
	 * @return pixel size of the chat text
	 */
	int getChatFontSize()
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	/**
	 * changes the font size of the chat box
	 * @param size font size in pixels
	 */
	void setChatFontSize(int size)
	{ printf("%-30s| %d\n", __FUNCTION__, size); };

	// new things, not documented yet
	void showChooser(std::string &type, std::string &instance, std::string &box)
	{ printf("%-30s|\n", __FUNCTION__); };


	void repairVehicle(std::string &instance, std::string &box)
	{ printf("%-30s|\n", __FUNCTION__); };

	void spawnObject(const std::string &objectName, const std::string instanceName, float px, float py, float pz, float rx, float ry, float rz, const std::string &eventhandler)
	{ printf("%-30s|\n", __FUNCTION__); };

	int getNumTrucksByFlag(int flag)
	{ printf("%-30s|\n", __FUNCTION__); return 0; };

	bool getCaelumAvailable()
	{ printf("%-30s|\n", __FUNCTION__); return false; };

	std::string getSetting(std::string str)
	{ printf("%-30s|\n", __FUNCTION__); };

	void hideDirectionArrow()
	{ printf("%-30s|\n", __FUNCTION__); };


};

#endif //__Gamescript_H__

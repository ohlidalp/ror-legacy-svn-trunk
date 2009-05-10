#ifndef __Settings_H__
#define __Settings_H__

/**
 *  @brief Settings class
 */
class Settings
{
public:

	/**
	 * retrieves a setting out of the RoR settings class
	 * @param key the setting to retrieve. 
	 * @return value of the key. If the key is not existing, the function will return an empty string
	 */
	std::string getSetting(std::string key)
	{ printf("%-30s| %s\n", __FUNCTION__, key.c_str()); return ""; };


	// AS utils
	void addRef(){};
	void release(){};
};

#endif //__Settings_H__

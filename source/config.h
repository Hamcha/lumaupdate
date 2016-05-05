#pragma once

#include "libs.h"

/*! \brief Configuration error reasons
 */
enum LoadConfigError {
	CFGE_NONE,       /*< No errors found            */
	CFGE_NOTEXISTS,  /*< Config file does not exist */
	CFGE_UNREADABLE, /*< Config file is unreadable  */
	CFGE_MALFORMED,  /*< Config file is malformed   */
};

/*! \brief Configuration loader
 */
class Config {
private:
	std::map<std::string, std::string> values;

public:
	/*! \brief Loads and parses a configuration file
	 *
	 *  \param path Path to config file
	 *
	 *  \return true if file exists and was parsed successfully, false otherwise
	 */
	LoadConfigError LoadFile(const std::string& path);

	/*! \brief Checks wether a property exists in the configuration file
	 *
	 *  \param key Property name to check
	 *
	 *  \return true if the property is found, false otherwise
	 */
	bool Has(const std::string& key);

	/*! \brief Gets the value of a property by key
	 *
	 *  \param key      Property name to get
	 *  \param fallback Default value to fall back to if the property doesn't exist
	 *
	 *  \return Either the value of the property or the fallback value
	 */
	std::string Get(const std::string& key, const std::string& fallback);
};
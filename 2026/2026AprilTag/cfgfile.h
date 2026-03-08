#pragma once
#include <map>

class ConfigFile
{
	std::map<std::string,std::string>	m_mSettings;

	void vStripWhiteSpace( char *psz );

public:
	ConfigFile();
	~ConfigFile();

	bool bLoad( char const *psz );

	bool bGetSetting( std::string const &key, std::string &val ) const;
	std::string const &operator[](std::string const &key) const;
};

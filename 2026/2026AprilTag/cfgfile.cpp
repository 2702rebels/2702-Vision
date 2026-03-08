#include <map>
#include <string>
#include <cstring>
#include <stdexcept>

#include "2702util.h"

#include "cfgfile.h"

ConfigFile::ConfigFile()
{
}

ConfigFile::~ConfigFile()
{
}

void ConfigFile::vStripWhiteSpace( char *psz )
{
	// Clean up the tail end of the string
	size_t sLen = strlen(psz);
	while( sLen && isspace(psz[sLen-1]) ) {
		sLen--;
		psz[sLen] = '\0';
	}

	// now clean up the start of the string.
	char *pch = psz;
	while (*pch && isspace(*pch)) {
		pch++;
	}

	if (psz == pch) {
		return;
	}

	while( *psz++ = *pch++ ) {
	}
}

bool ConfigFile::bLoad( char const *pszFile )
{
	FILE *fp = fopen( pszFile, "r" );
	if (!fp) {
		fprintf( stderr, "Field: Unable to open %s", pszFile );
		return false;
	}

	char szTmp[512];
	while( fgets( szTmp, sizeof(szTmp), fp ) )
	{
		char *pchKey = szTmp;
		char *pch = szTmp;
		// Strip comments whch start with a ;
		while (*pch && *pch != ';') {
			pch++;
		}
		if (*pch == ';')
			*pch = '\0';

		vStripWhiteSpace( szTmp );

		if (!szTmp[0]) {
			// blank line
			continue;
		}
		pch = szTmp;
		while( *pch && *pch != ':' ){
			pch++;
		}

		if( *pch ) {
			*pch++ = '\0';
		}
		else {
			fprintf( stderr, "Invalid key/value : %s\n", szTmp );
			continue;
		}

		char *pchVal = pch;
		if (*pchKey) {
			vStripWhiteSpace( pchKey );
			vStripWhiteSpace( pchVal );
			
		}

		m_mSettings[pchKey] = pchVal;
	}
	fclose( fp );
	return true;
}

bool ConfigFile::bGetSetting( std::string const &key, std::string &val ) const
{
	auto search = m_mSettings.find(key);
	if (search != m_mSettings.end()) {
		val = search->second;
		return true;
	}

	return false;
}

std::string const &ConfigFile::operator[]( std::string const &str ) const
{
	auto search = m_mSettings.find(str);
	if (search != m_mSettings.end()) {
		return search->second;
	}

	throw std::runtime_error( strPrintf( "ConfigFile: key '%s' not found\n", str.c_str() ) );
}

#ifdef TESTING
int main( int argc, char **argv )
{
	if (argc < 3) {
		printf( "Usage: %s cfgfile setting [setting ...]\n", argv[0] );
		return 1;
	}

	ConfigFile cf;

	if( cf.bLoad( argv[1] )) {
		for( int i = 2; i < argc; i++ ) {
			std::string str;
			if( cf.bGetSetting( argv[i], str ) ) {
				printf( "%s:'%s'\n", argv[i], str.c_str() );
			}
		}
	}
	return 0;
}
#endif

#include <string>
#include <stdarg.h>

std::string strPrintf( char const *psz, ... )
{
	va_list va;

	va_start( va, psz );
	int len = vsnprintf( 0, 0, psz, va );
	va_end( va );

	char szTmp[len + 1];
	
	va_start( va, psz );
	len = vsnprintf( szTmp, len + 1, psz, va );
	va_end( va );
	return std::string(szTmp);
}

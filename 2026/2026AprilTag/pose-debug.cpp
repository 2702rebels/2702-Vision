#include <stdexcept>

#include "field.h"
#include "FrameLogger.h"
#include "PoseSolver.h"
#include "cfgfile.h"

#include "2702camera.h"

ConfigFile g_cf;
MultiCam g_mcCam;
Field g_field;
int main( int argc, char **argv )
{
	char const *pszCfg = "jane.cfg";
	char const *pszFrm = "foo";
#if 0
	if (argc != 3) {
		fprintf( stderr, "Usage: %s config-file infile\n", argv[0] );
		return 1;
	}
#else
	if (argc == 1 ) {
	}
	else if (argc == 3 ) {
		pszCfg = argv[1];
		pszFrm = argv[2];
	}
	else {
		fprintf( stderr, "Usage: %s config-file infile\n", argv[0] );
		return 1;
	}
#endif

	if (!g_cf.bLoad( pszCfg )) {
		fprintf( stderr, "Unable to load config file '%s'\n", argv[1] );
		return 1;
	}


	try {
		g_field = Field( g_cf["FieldFile"].c_str() );
		if( !g_mcCam.bLoad( g_cf["camfile"].c_str() ) ) {
			return 1;
		}

		PoseSolver ps( g_field, g_mcCam );

		CFrameLogger fl;
		if (!fl.bOpenLog( pszFrm ) ) {
			return 1;
		}

		std::vector<ATFrame *> vatf;
		while( fl.bReadFrame( vatf ) ) {
			printf( "%u\n", vatf[0]->u32Seq() );
			Rebels::QXfrm qxfm;
			if (ps.bEstimate( qxfm, vatf )) {
				ps.bSolve( qxfm, vatf );
			}
		}
		
	}
	catch( std::exception &e ) {
		fprintf( stderr, "Error: %s\n", e.what() );
		return 1;
	}

	return 0;
}

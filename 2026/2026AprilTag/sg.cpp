#include "syncgen.h"

int main( int argc, char **argv )
{
	SyncGen sg;

	if (!sg.bConnect( "/dev/ttyUSB0" )) {
	}
	sg.bStart( 50 );
	sg.bStop();
}

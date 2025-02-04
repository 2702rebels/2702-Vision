#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <string>
#include <stdexcept>

#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace std;

#include "v4lcam.h"


#define RES_ROWS 800
#define RES_COLS 1280

void printinfo( v4lcam &cam )
{
	cam.printCaps();
	cam.printStandards();
	cam.printControls();
	printf( "---------------\n" );
}

int main( int argc, char **argv )
{
	cv::Mat abc( 800, 1280, CV_8UC4 );

	printf( "%d x %d -> %ld %ld %ld\n", abc.cols, abc.rows, abc.total(), abc.elemSize(), *abc.step.p );
	try {
		v4lcam mycam0( "/dev/video0" );
		printinfo( mycam0 );

		v4lcam mycam1( "/dev/video1" );
		printinfo( mycam1 );
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}
	return 0;
}

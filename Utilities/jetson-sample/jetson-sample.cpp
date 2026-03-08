#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <curses.h>

#include <X11/Xlib.h>
//#include <cameraserver/CameraServer.h>
//#include <networktables/NetworkTableInstance.h>

#include <linux/videodev2.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>

#include <string>
#include <stdexcept>

#include <cameraserver/CameraServer.h>
#include <networktables/NetworkTableInstance.h>
#include <drm/drm_fourcc.h>

using namespace std;

extern "C" {
#include "genio.h"
};

#include "v4lcam.h"
#include "fbdisp.h"
#include "nvcam.h"

#include <signal.h>

static char const *g_pszImagPat = "img%04d.png";

void printinfo( v4lcam &cam )
{
	cam.printCaps();
	cam.printStandards();
	cam.printControls();
	printf( "---------------\n" );
}

bool bStop = false;

void sighandler( int nSig )
{
	printf( "Received Signal %d\n", nSig );
	bStop = true;
}

class imgProc : public nvFrmProcessor
{
	std::mutex	m_mtxCS;
	bool		m_bFirstVid = true;
	cs::CvSource 	m_outputStream;

	volatile bool	m_bCapture = false;
	int				m_nCapture = 0;

public:
	imgProc()
	{
	}
	virtual ~imgProc()
	{

	}
	virtual void ProcessFrame( nvCamera *pCam,  const nvFrame *pcf )
	{
		if (m_mtxCS.try_lock()) { 
			cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC1, (void *)pcf->pu8Image() );
			if (m_bFirstVid) {
				m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows() );
				m_bFirstVid = false;
			}
			m_outputStream.PutFrame(frame);

			if (m_bCapture) {
				m_bCapture = false;
				char szTmp[128];
				sprintf( szTmp, g_pszImagPat, ++m_nCapture );
				cv::imwrite( szTmp, frame );
			}
			m_mtxCS.unlock();
		}
	}
	void vCaptureFrame()
	{
		m_bCapture = true;
	}
	int nCaptures() const
	{
		return m_nCapture;
	}
};

void vSetIntegration( nvCamera &nvc )
{
	char szTmp[128];

	GetNumber( szTmp, sizeof(szTmp), "Integration", "Please enter the integration Value" );
	unsigned u = atoi(szTmp);
	nvc.bSetExposure( u );
}

void vSetGain( nvCamera &nvc )
{
	char szTmp[128];

	GetNumber( szTmp, sizeof(szTmp), "Gain", "Please enter the gain Value" );
	unsigned u = atoi(szTmp);
	nvc.bSetGain( u );
}

int main( int argc, char **argv )
{
	char const *pszVideoDev = "/dev/video0";

	if (argc != 3) {
		fprintf( stderr, "Usage: %s videodev filepattern\n", argv[0] );
		fprintf( stderr, "    videodev     path to video device.  e.g. /dev/video0\n" );
		fprintf( stderr, "    filepattern  format string for image files. e.g. sensor1-%%04d.png\n" );
		return 1;
		
	}

	pszVideoDev = argv[1];
	g_pszImagPat = argv[2];

	signal( SIGINT, sighandler );

	InitScreen();
	try {
		nodelay( stdscr, 1 );

	//	auto ntinst = nt::NetworkTableInstance::GetDefault();
	//	ntinst.StartClientTeam( 2702 );
	//	ntinst.StartServer();
	//		RPICamera mycam0( "/dev/video0", 0, 800, 600, DRM_FORMAT_BGR888, proc );
	//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		imgProc proc;
//		nvCamera mycam0( "/dev/video0", 0, 1280, 800, V4L2_PIX_FMT_RGB24, proc );
		nvCamera mycam0( pszVideoDev, 0, 1280, 800, V4L2_PIX_FMT_GREY, 0, proc );
	//	nvCamera mycam1( "/dev/video1", 4, 1 );

		printinfo( mycam0 );
		
			std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
		bool bRefresh = true;
		while( !bStop ) {
			if (bRefresh) {
				clear();
				int nLine = 4;
				mvprintw( 2, 1, "Images Captured: %d", proc.nCaptures() );
				mvprintw( nLine++, 1, "[ESC]    Quit" );
				mvprintw( nLine++, 1, "q        Quit" );
				mvprintw( nLine++, 1, "c        Capture Frame" );
				mvprintw( nLine++, 1, "i        set integration time" );
				mvprintw( nLine++, 1, "g        set gain" );
				refresh();
				bRefresh = false;
			}
			int  ch = getch();
			switch (ch) {
			case -1:
				break;
			case 0x1b:
			case 'q':
				bStop = true;
				break;
			case 'c':
				proc.vCaptureFrame();
				bRefresh = true;
				break;
			case 'i':
				vSetIntegration( mycam0 );
				break;
			case 'g':
				vSetGain( mycam0 );
				break;
			}
			std::this_thread::sleep_for( std::chrono::milliseconds(200) );
		}
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}

	endwin();
	return 0;
}

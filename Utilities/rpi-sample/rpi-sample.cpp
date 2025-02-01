#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <span>

//#include <X11/Xlib.h>
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

#include "rpicam.h"
#include "VisLog.h"

using namespace std;

#include <signal.h>

#if 0
void printinfo( v4lcam &cam )
{
	cam.printCaps();
	cam.printStandards();
	cam.printControls();
	printf( "---------------\n" );
}
#endif

bool bStop = false;

void sighandler( int nSig )
{
	printf( "Received Signal %d\n", nSig );
	bStop = true;
}

class imgProc : public FrmProcessor
{
	VisionLog	m_vl;
	std::mutex	m_mtxLog;
	std::mutex	m_mtxCS;
	bool		m_bFirstVid = true;
	cs::CvSource 	m_outputStream;
public:
	imgProc( char const *pszLogFile ) :
		FrmProcessor(),
		m_vl( pszLogFile, true )
		
	{
	}
	virtual ~imgProc()
	{

	}
	virtual void ProcessFrame( int nCam, CameraFrame const *pcf )
	{
		cv::Mat frame( pcf->nRows(), pcf->nCols(), CV_8UC3, (void *)pcf->pu8Image() );
		printf( "Have Frame from %d:%d\n", nCam, pcf->u32Sequence() );

		if (m_mtxLog.try_lock()) {
			cv::Mat img( pcf->nRows(), pcf->nCols(), CV_8UC3, (void *)pcf->pu8Image() );
			m_vl.bWritePNG( img, pcf->u32Sequence(), pcf->u32Exposure() );
			m_mtxLog.unlock();
		}
		if (m_mtxCS.try_lock()) { 
			if (m_bFirstVid) {
				m_outputStream = frc::CameraServer::PutVideo("Vision", pcf->nCols(), pcf->nRows() );
				m_bFirstVid = false;
			}
			m_outputStream.PutFrame(frame);
			m_mtxCS.unlock();
		}
	}
};

int main( int argc, char **argv )
{
	char szLog[1024] = "test.log";

	if (access("/data/logs", F_OK) == 0) {
		sprintf( szLog, "/data/logs/test.log" );
	}

	signal( SIGINT, sighandler );
	if (argc == 2) {
		strcpy( szLog, argv[1] );
	}

#if 0
	char szImgName[100];
	sprintf( szImgName, "Testing" );
	cv::namedWindow( szImgName, cv::WINDOW_AUTOSIZE );
	cv::Mat img = cv::imread("img.png", cv::IMREAD_COLOR);
	printf( "img size: %d x %d\n", img.rows, img.cols );
	cv::imshow( szImgName, img );
	cv::waitKey(0);
#endif
	try {
		auto ntinst = nt::NetworkTableInstance::GetDefault();
		ntinst.StartClient4( "2702" );
		ntinst.StartServer();
		imgProc proc( szLog );
//		RPICamera mycam0( "/dev/video0", 0, 800, 600, V4L2_PIX_FMT_RGB24, proc );
		RPICamera mycam0( "/dev/video0", 0, 800, 600, DRM_FORMAT_BGR888, proc );

		while( !bStop ) {
			std::this_thread::sleep_for( std::chrono::milliseconds(200) );
		}
	}
	catch (std::exception &e) {
		printf( "Exception: %s\n", e.what() );
	}

	return 0;
}

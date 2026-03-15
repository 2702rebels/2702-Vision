#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <asm/termbits.h>
#include <stdio.h>
#include <errno.h>
#include <thread>


class SyncGen
{
	int	m_fd = -1;
public:
	SyncGen() :
		m_fd( -1 )
	{
	}
	~SyncGen()
	{
		if (m_fd >= 0) {
			close( m_fd );
		}
	}
	bool bConnect( char const *pszPath )
	{
		struct termios2 newtio;

		m_fd = open( pszPath, O_RDWR );
		if (m_fd == -1) {
			printf( "Failed to open %s\n", pszPath );
			return false;
		}

		//tcflush(m_fd, TCIFLUSH);
		bzero(&newtio, sizeof(newtio));
		newtio.c_cflag = BOTHER | CS8 | CLOCAL | CREAD;
		newtio.c_iflag = IGNPAR;
		newtio.c_oflag = 0;
		newtio.c_lflag = 0;         /* set input mode (non-canonical, no echo,...) */
		newtio.c_cc[VTIME] = 5;   /* inter-character timer unused */
		newtio.c_cc[VMIN] = 0;      /* blocking read until 1 char received */
		newtio.c_ispeed = 115200;
		newtio.c_ospeed = 115200;
		if (ioctl(m_fd, TCSETS2, &newtio) != 0) {
			printf( "Unable to set termio: %d\n", errno );
			return false;
		}
		return true;
	}

	void vStart( int freq )
	{
		if (m_fd >= 0) {
			char szTmp[128];
			sprintf( szTmp, "%d\n", freq );
			std::this_thread::sleep_for(  std::chrono::milliseconds(250) );
			write( m_fd, szTmp, strlen(szTmp) );
			int n = read( m_fd, szTmp, sizeof(szTmp) - 1 );
			if (n > 0) {
				szTmp[n] = '\0';
				printf( "Read %s\n", szTmp );
			}
			else {
				printf( "Failed reading from SG\n" );
				_exit(1);
			}
		}
	}

	void vStop()
	{
		vStart( 0 );
	}
	
};

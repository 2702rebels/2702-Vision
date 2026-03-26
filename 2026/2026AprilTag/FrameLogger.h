#pragma once
#include <sys/time.h>
#include "ATFrame.h"

class CFrameLogger
{
	bool		m_bWrite = false;
	time_t		m_tEndLog = 0;
	bool		m_bMatchLog = false;
	FILE 		*m_fp = 0;
public:
	CFrameLogger()
	{
	}
	~CFrameLogger()
	{
		if (m_fp) {
			fclose( m_fp );
			m_fp = 0;
		}
	}

	bool bCreateLog( char const *psz )
	{
		m_fp = fopen( psz, "wt" );
		if (!m_fp) {
			fprintf( stderr, "Unable to open log file '%s' for writing\n", psz );
			return false;
		}
		m_bWrite = true;
		return true;
	}

	bool bCreateMatchLog( char const *pszDir )
	{
		// If log file is already open, don't start match log.
		if (m_fp) {
			return true;
		}
		try {
			char szTmp[128];
			for (int i = 0; ; i++ )
			{
				snprintf( szTmp, sizeof(szTmp), "%s/FrmLog-%04d.log", pszDir, i );
				if (access( szTmp, F_OK ) == -1) 
					break;
			}
			printf( "Opening log file at %s\n", szTmp );

			m_fp = fopen( szTmp, "wt" );
			if (!m_fp) {
				fprintf( stderr, "Unable to open log file '%s' for writing\n", szTmp );
				return false;
			}
			m_bMatchLog = true;
			m_tEndLog = time(0) + 160;
			m_bWrite = true;
			return true;
		}
		catch( exception &e ) {
			if (m_fp) {
				fclose( m_fp );
				m_fp = 0;
			}
			printf( "Unable to start logging: %s\n", e.what() );
		}
		return true;
	}
	bool bOpenLog( char const *psz )
	{
		m_bWrite = false;
		m_fp = fopen( psz, "rt" );
		if (!m_fp) {
			fprintf( stderr, "Unable to open log file '%s'\n", psz );
			return false;
		}
		return true;
	}

	void vLogFrame( std::vector<ATFrame *> const &vaf )
	{
		if (m_fp && m_bWrite) {
			fprintf( m_fp, "S:%u\n", vaf[0]->u32Seq() );
			fprintf( m_fp, "Cams:%lu\n", vaf.size() );
			for( size_t s = 0; s < vaf.size(); s++ ) {
				auto f = *vaf[s];
				fprintf( m_fp, "T:%lu\n", f.sTags() );
				// Loop through Tags
				for (size_t sT = 0; sT < f.sTags(); sT++) {
					ATDetection const &atd = f[sT];
					fprintf( m_fp, "%d", atd.uId() );
					for( int i = 0; i < 4; i++ ) {
						fprintf( m_fp, ",%8.3f,%8.3f", atd[i].x, atd[i].y );
					}
					fprintf( m_fp, "\n" );
				}
			}
			if (m_bMatchLog && time(0) > m_tEndLog) {
				m_bWrite = false;
				fclose( m_fp );
				m_fp = 0;
			}
		}
	}

	bool bReadFrame( std::vector<ATFrame *> &vaf )
	{
		char szTmp[512];

		// Delete the current frames;
		for( size_t s = 0; s < vaf.size(); s++ ) {
			delete vaf[s];
		}
		vaf.clear();

		if (m_fp && !m_bWrite) {
			unsigned uSeq;
			unsigned uCams;
			if (!fgets( szTmp, sizeof(szTmp), m_fp) || sscanf( szTmp, "S:%u", &uSeq ) != 1) {
				fprintf( stderr, "Unable to read frame #... end of file?" );
				return false;
			}
			if (!fgets( szTmp, sizeof(szTmp), m_fp) || sscanf( szTmp, "Cams:%u", &uCams ) != 1) {
				fprintf( stderr, "Unable to read # of cameras\n" );
				return false;
			}
			for (unsigned uCam = 0; uCam < uCams; uCam++) {
				timeval ts = {};
				ts.tv_sec = uSeq/50;
				ts.tv_usec = (uSeq % 50) * 20000;
				unsigned uTags = 0;
				ATFrame *patf = new ATFrame( uCam, uSeq, &ts );
				if (!fgets( szTmp, sizeof(szTmp), m_fp) || sscanf(szTmp, "T:%u", &uTags ) != 1) {
					fprintf( stderr, "Unable to read # of tags\n" );
					return false;
				}
				for (unsigned uTag = 0; uTag < uTags; uTag++) {
					int nId;
					double grC[4][2];
					if (!fgets( szTmp, sizeof(szTmp), m_fp) ||
						sscanf(szTmp, "%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf", &nId,
								&grC[0][0], &grC[0][1], &grC[1][0], &grC[1][1],
								&grC[2][0], &grC[2][1], &grC[3][0], &grC[3][1] ) != 9)
					{
						fprintf( stderr, "@%u: Unable to read Cam %d Tag %d\n", uSeq, uCam, uTag );
						return false;
					}
					ATDetection atd( nId, grC );
					patf->vAddDetection( atd );
				}
				vaf.push_back( patf );
			}
		}
		return true;
	}
};

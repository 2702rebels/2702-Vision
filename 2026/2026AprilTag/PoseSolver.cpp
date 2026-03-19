#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui.hpp>

#include "PoseSolver.h"

#define __cminpack_float__
#include "cminpack/cminpack.h"


PoseSolver::PoseSolver( Field const &fld, MultiCam const &mc ) :
	m_Field( fld ),
	m_mc( mc )
{
}

PoseSolver::~PoseSolver()
{
}

int PoseSolver::eval( void *p, int m, int n, float const *x, float *fv, int iflag )
{
	PoseSolver *pps = static_cast<PoseSolver *>(p);
	
	return pps->eval( m, n, x, fv, iflag );
}

int PoseSolver::eval( int m, int n, float const *px, float *pv, int iflag )
{
//	printf( "%.4f %.4f %.4f : %.4f %.4f %.4f\n", px[0], px[1], px[2], px[3], px[4], px[5] );
	Euler e( px[0], px[1], px[2] );
	Quaternion q( e );
	Point3d p( px[3], px[4], px[5] );

	float fErr;
	float fErr2 = 0.;
	int nCnt = 0;
	QXfrm qxF2R( q, p );  // 
	QXfrm qxF2C = m_mc.qxmR2C() * qxF2R;
	for (auto f : *m_pvatf ) {
		size_t sTags = f->sTags();
		int nCam = f->nCam();
		CamParm const &cp = m_mc[nCam];

		for (size_t sTag = 0; sTag < sTags; sTag++) {
			ATDetection const &atd = (*f)[sTag];
			unsigned uTagId = atd.uId();

			FieldTag const &ft = m_Field[uTagId - 1];
			if (atd.bUse()) {
				Point3d const *pCorners = ft.Corners();
				for (int i = 0; i < 4; i++ ) {
					// Transform the corner point into the "Multi-Camera" frame of reference.
					Point3d p3dCam = qxF2C.TransformPoint( pCorners[i] );
	//				printf( "p2dCam -> %.4f %.4f %.4f\n", p3dCam.x(), p3dCam.y(), p3dCam.z() );

					Point2d p2d = cp.ProjectPoint( p3dCam );

					// Get the U/V position of this corner point.
					auto p2f = atd[i];
					
	//				printf( "%.4f -> %.4f  ,  %.4f -> %.4f\n", p2d.u(), p2f.x, p2d.v(), p2f.y );
					fErr = p2d.u() - p2f.x;
					fErr2 += fErr * fErr;
					*pv++ = fErr;
					fErr = p2d.v() - p2f.y;
					fErr2 += fErr * fErr;
					*pv++ = fErr;
					nCnt++;
				}
			}
		}
	}
	if (iflag == 1) {
//		printf( "iflag: %d -> %f\n", iflag, sqrt(fErr2/nCnt) );
	}
	return 1;
}

void PoseSolver::vSetMinTagPixels( uint32_t u32MinPixels )
{
	m_u32MinTagPixels = u32MinPixels;
}

void PoseSolver::vSetMaxDistance( uint32_t fMaxDist )
{
	m_fMaxDistSqd = fMaxDist * fMaxDist;
}

#define USE_MOST_PIXELS 1

bool PoseSolver::bFilterTags( std::vector<ATFrame *> &vatf )
{
	int nMostTags = 0;
	int nMostPixels = 0;
	int nMostIdx = 0;
	// run through all frames (1 for each camera)
	for (int nFrm = 0; nFrm < vatf.size(); nFrm++) {
		int nUsableTags = 0;
		// run through the detected tags for this camera
		for (size_t t = 0; t < vatf[nFrm]->sTags(); t++ ) {
			ATDetection &d = (*vatf[nFrm])[t];

			// Get the Field information for this tag
			FieldTag const &ft = m_Field[d.uId() - 1];

			// if the tag is to be ignored, set flag and move on
			if (ft.bIgnore()) {
				d.vSetUse( false );
				continue;
			}
				
			float fSizeU = fabs( d[2].x - d[0].x );
			float fSizeV = fabs( d[2].y - d[0].y );
			uint32_t u32Pixels = uint32_t(fSizeU * fSizeV);
			if (u32Pixels < m_u32MinTagPixels) {
				d.vSetUse( false );
				continue;
			}

			cv::Mat objPoints(4, 1, CV_32FC3);
			cv::Mat imgPoints(4, 1, CV_32FC2);

			for (int c = 0; c < 4; c++) {
				Rebels::Point3d pn = ft.RawCorners()[c];
				objPoints.ptr<cv::Vec3f>(0)[c] = cv::Vec3f(pn.x(), pn.y(), pn.z() );
				imgPoints.ptr<cv::Point2f>(0)[c] = d[c];
			}

			cv::Mat rvec( 3, 1, CV_64F );
			cv::Mat tvec( 3, 1, CV_64F );

			cv::Mat m( 3, 3, CV_32F );
			auto cp = m_mc[nFrm];
			for( int nR = 0; nR < 3; nR++ ) {
				for( int nC = 0; nC < 3; nC++ ) {
					m.at<float>( nR, nC ) = cp.pinhole( nR, nC );
				}
			}

			cv::Mat md( 5, 1, CV_64F );
			for (int i = 0; i < 5; i++) {
				md.at<double>(i) = cp.dist(i);
			}

	//		if (!solvePnP( objPoints, imgPoints, m, md, rvec, tvec, false, cv::SOLVEPNP_IPPE_SQUARE )) {
			if (!solvePnP( objPoints, imgPoints, m, md, rvec, tvec )) {
				d.vSetUse( false );
			}

			float fX = float(tvec.at<double>(0));
			float fY = float(tvec.at<double>(1));
			float fZ = float(tvec.at<double>(2));
			
			float fDistSqd = fX * fX + fY * fY + fZ * fZ;

			
//			printf( "%-5u   C%d   T%d  Dist = %.2f\n", vatf[nFrm]->u32Seq(), vatf[nFrm]->nCam(), d.uId(), sqrt(fDistSqd));
			if (fDistSqd > m_fMaxDistSqd) {
//				printf( "Removed\n" );
				d.vSetUse( false );
			}
		}
	}
	return true;
}

bool PoseSolver::bEstimate( QXfrm &xfm, std::vector<ATFrame *> &vatf )
{
	// Find the Sensor with the most AprilTags in view
	int nMostTags = 0;
	int nMostPixels = 0;
	int nMostIdx = 0;
	for (int i = 0; i < vatf.size(); i++) {
		int nUsableTags = 0;
		int nPixels = 0;
		for (size_t t = 0; t < vatf[i]->sTags(); t++ ) {
			ATDetection const &d = (*vatf[i])[t];
			FieldTag const &ft = m_Field[d.uId() - 1];
			if (d.bUse()) {
				nUsableTags++;
				float fSizeU = fabs( d[2].x - d[0].x );
				float fSizeV = fabs( d[2].y - d[0].y );
				nPixels += fSizeU * fSizeV;
			}
		}
#if USE_MOST_PIXELS
		if (nPixels > nMostPixels && nUsableTags > 1) {
			nMostPixels = nPixels;
			nMostIdx = i;
			nMostTags = nUsableTags;
		}
#else
		if (nUsableTags > nMostTags) {
			nMostTags = nUsableTags;
			nMostIdx = i;
		}
#endif
	}

#if USE_MOST_PIXELS
	if (nMostPixels <= 1) {
		return false;
	}
#else
	if (nMostTags <= 1 ) {
		return false;
	}
#endif

	ATFrame *patf = vatf[nMostIdx];
	cv::Mat objPoints(4 * nMostTags, 1, CV_32FC3);
	cv::Mat imgPoints(4 * nMostTags, 1, CV_32FC2);

//	printf( "Using Camera %d", vatf[nMostIdx]->nCam() );
	int idx = 0;
	for (size_t t = 0; t < vatf[nMostIdx]->sTags(); t++ )
	{
		ATDetection const &d = (*vatf[nMostIdx])[t];
		FieldTag const &ft = m_Field[d.uId() - 1];
		if (d.bUse()) {
//printf( "Have Detection %d\n", d.uId() );
//printf( " %02d", d.uId() );
			for (int c = 0; c < 4; c++, idx++ ) {
				Rebels::Point3d pn = ft.Corners()[c];
	//			Rebels::Point3d pn = ft.RawCorner(c);
	//printf( "%.3f %.3f %.3f -> %.2f %.2f\n", pn.x(), pn.y(), pn.z(), d[c].x, d[c].y );
				objPoints.ptr<cv::Vec3f>(0)[idx] = cv::Vec3f(pn.x(), pn.y(), pn.z() );
				imgPoints.ptr<cv::Point2f>(0)[idx] = d[c];
			}
		}
	}
//	printf( "\n" );

	bool bHaveSolution = false;

	cv::Mat rvec( 3, 1, CV_64F );
	cv::Mat tvec( 3, 1, CV_64F );

	cv::Mat m( 3, 3, CV_32F );
	auto cp = m_mc[nMostIdx];
	for( int nR = 0; nR < 3; nR++ ) {
		for( int nC = 0; nC < 3; nC++ ) {
			m.at<float>( nR, nC ) = cp.pinhole( nR, nC );
		}
	}

	cv::Mat md( 5, 1, CV_64F );
	for (int i = 0; i < 5; i++) {
		md.at<double>(i) = cp.dist(i);
	}

	if (solvePnP( objPoints, imgPoints, m, md, rvec, tvec )) {
		cv::Mat reprojectPoints( objPoints.rows, 1, CV_32FC2);
		cv::projectPoints(objPoints, rvec, tvec, m, md, reprojectPoints);

		float sum = 0.;
		sum = cv::norm(reprojectPoints, imgPoints);

//printf( "Sum = %.4f\n", sum );
		Rebels::Point3d p3Rvec( rvec.at<double>(0), rvec.at<double>(1), rvec.at<double>(2) );
		Rebels::Point3d p3Tvec( tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2) );
		Rebels::Quaternion qRot( p3Rvec );
		Rebels::QXfrm qxmF2S( qRot, p3Tvec );

#if 0
		for (size_t t = 0; t < vatf[nMostIdx]->sTags(); t++ )
		{
			ATDetection const &d = (*vatf[nMostIdx])[t];
			FieldTag const &ft = m_Field[d.uId() - 1];
			if (d.bUse()) {
				printf( "For Tag %d\n", d.uId() );
				for (int c = 0; c < 4; c++, idx++ ) {
					Rebels::Point3d pn = qxmF2S.TransformPoint(ft.Corners()[c]);
	//				Rebels::Point3d pn = qxmF2S.TransformPoint(ft.RawCorner(c));
					Rebels::QXfrm qi = cp.getXfrm().Inverse();
					Point3d pn1 = qi.TransformPoint(pn);
					Rebels::Point2d p2 = cp.ProjectPoint( pn1);

					printf( "%.3f %.3f %.3f -> %.2f %.2f\n", pn.x(), pn.y(), pn.z(), p2.u(), p2.v() );
				}
			}
		}
#endif
		bool bUseSolve = sum < 20.;
		if (bUseSolve) {
#if 1
			// the xfrm we have just computed goes from the Field coordinate system to the sensor's.
			// We need the coordinate system from the Robot to the Field
			// We can get that by inverting the computed xfrm to get Sensor->Field
			// We can combine that with the Camera->Sensor and then the Robot->Camera
			QXfrm qxmS2F = qxmF2S.Inverse();
			QXfrm qxmC2F = qxmS2F*cp.getXfrm();
			QXfrm qxmR2F = qxmC2F*m_mc.qxmR2C();
			xfm = qxmR2F;
#else
			xfm = qxmF2S.Inverse();
#endif
		}
		return true;
	}

	return false;
}

bool PoseSolver::bSolve( QXfrm &xfm, std::vector<ATFrame *> &vatf )
{
#if 1
	// count the number of unknowns...
	int nCams = 0;
	int m = 0;
	for (auto f : vatf ) {
		bool bHasUsableTag = false;
		for ( size_t s = 0; s < f->sTags(); s++) {
			ATDetection const &d = (*f)[s];
			FieldTag const &ft = m_Field[d.uId() - 1];
			if (d.bUse()) {
				m += 8;  // there are 4 corners per tag and a u,v coordinate pair per corner
				bHasUsableTag = true;
			}
		}
		if (bHasUsableTag) {
			nCams++;
		}
	}

	if (!m || nCams < 2) {
		return false;
	}

	m_pvatf = &vatf;


	// The transform we want is from Robot to Field, but for solving
	// we need the Field to Robot xfrm
	QXfrm qxInv = xfm.Inverse();

	// Convert the Quaternion rotation to Euler format for solving
	Euler eu( qxInv.Rotation() );
	int n = 6; // Number of unknowns
	float grfX[6];
	grfX[0] = eu.fX(); // Roll
	grfX[1] = eu.fY(); // Pitch
	grfX[2] = eu.fZ(); // Yaww
	grfX[3] = qxInv.Translation().x();
	grfX[4] = qxInv.Translation().y() + 5;
	grfX[5] = qxInv.Translation().z() + 10;

	float *pfVals = new float[m];
	int *pniwa = new int[m];
	int wa = m * n + 5 * n + m;
	float *plwa = new float[wa];

	slmdif1( eval, static_cast<void *>(this), m, n, grfX, pfVals, .01, pniwa, plwa, wa  );

	delete plwa;
	delete pniwa;
	delete pfVals;
	Euler euNew( grfX[0], grfX[1], grfX[2] );
	Point3d p3( grfX[3], grfX[4], grfX[5] );
	QXfrm qxNew( euNew, p3 );

	xfm = qxNew.Inverse();
#endif

	return true;
}

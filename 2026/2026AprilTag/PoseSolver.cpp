#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui.hpp>

#include "PoseSolver.h"
#include "rb.h"

#define __cminpack_float__
#include "cminpack/cminpack.h"

static float fCalcRMS( QXfrm const &qxm, std::vector<Rebels::Point3d> const &vp3Meas, std::vector<Rebels::Point3d> const &vp3Ref, double &fMax )
{
	double fSumSqd = 0.;

	fMax = 0.;
	size_t stNum = vp3Meas.size();
	for (size_t stIdx = 0; stIdx < stNum; stIdx++) {
		Rebels::Point3d p3Xfmd = qxm.TransformPoint( vp3Meas[stIdx] );
		Rebels::Point3d p3Diff = vp3Ref[stIdx] - p3Xfmd;
		double fSqd = p3Diff.x() * p3Diff.x() + p3Diff.y() * p3Diff.y() + p3Diff.z() * p3Diff.z();
		fSumSqd += fSqd;
		double fErr = sqrt(fSqd);
		
		if (fErr > fMax) {
			fMax = fErr;
		}
	}

	return sqrt( fSumSqd / stNum );
}


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

bool PoseSolver::bTagPNP( unsigned uCam, ATDetection &d )
{
	cv::Mat objPoints(4, 1, CV_32FC3);
	cv::Mat imgPoints(4, 1, CV_32FC2);

	unsigned uTagId = d.uId();
	FieldTag const &ft = m_Field[uTagId - 1];
	for (int c = 0; c < 4; c++) {
		Rebels::Point3d pn = ft.RawCorners()[c];
		objPoints.ptr<cv::Vec3f>(0)[c] = cv::Vec3f(pn.x(), pn.y(), pn.z() );
		imgPoints.ptr<cv::Point2f>(0)[c] = d[c];
	}

	cv::Mat rvec( 3, 1, CV_64F );
	cv::Mat tvec( 3, 1, CV_64F );

	auto cp = m_mc[uCam];
	cv::Mat m( 3, 3, CV_32F, (void *)cp.intrinsics() );

	cv::Mat md( 5, 1, CV_32F, (void *)cp.distortion() );

	if (!solvePnP( objPoints, imgPoints, m, md, rvec, tvec )) {
		d.vSetUse( false );
		return false;
	}

	Rebels::Point3d p3Rvec( rvec.at<double>(0), rvec.at<double>(1), rvec.at<double>(2) );
	Rebels::Point3d p3Tvec( tvec.at<double>(0), tvec.at<double>(1), tvec.at<double>(2) );
	Rebels::Quaternion qRot( p3Rvec );

	// Form the quaternion from Tag to Sensor
	Rebels::QXfrm qxmT2S( qRot, p3Tvec );

	d.vSetQxm( qxmT2S );

	return true;
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

			if (uTagId < 1 || uTagId > 32) {
				continue;
			}
			if (atd.bUse()) {
				FieldTag const &ft = m_Field[uTagId - 1];
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

int PoseSolver::nEnumerateEqnsUV( std::vector<ATFrame *> const &vatf, int &nCams, uint64_t &u64TagMask ) const
{
	// count the number of equations...
	int m = 0;
	nCams = 0;
	for (auto f : vatf ) {
		bool bHasUsableTag = false;
		for ( size_t s = 0; s < f->sTags(); s++) {
			ATDetection const &d = (*f)[s];
			FieldTag const &ft = m_Field[d.uId() - 1];
			u64TagMask |= (1 <<(d.uId() - 1));

			if (m_bSolveUseAll || d.bUse()) {
				m += 8;  // there are 4 corners per tag and a u,v coordinate pair per corner
				bHasUsableTag = true;
			}
		}
		if (bHasUsableTag) {
			nCams++;
		}
	}

	return m;
}

int PoseSolver::nEnumerateEqns3D( std::vector<ATFrame *> const &vatf, int &nCams, uint64_t &u64TagMask ) const
{
	// count the number of equations...
	int m = 0;
	nCams = 0;
	for (auto f : vatf ) {
		bool bHasUsableTag = false;
		for ( size_t s = 0; s < f->sTags(); s++) {
			ATDetection const &d = (*f)[s];
			FieldTag const &ft = m_Field[d.uId() - 1];
			u64TagMask |= (1 <<(d.uId() - 1));

			if (m_bSolveUseAll || d.bUse()) {
				m += 12;  // there are 4 corners per tag and an x,y,z coordinate pair per corner
				bHasUsableTag = true;
			}
		}
		if (bHasUsableTag) {
			nCams++;
		}
	}

	return m;
}

int PoseSolver::nCalcResidualsUV( QXfrm &xfm, std::vector<ATFrame *> &vatf, float *pfRes )
{
	int nRes = 0;
	int nCnt = 0;
	QXfrm qxF2R = xfm.Inverse();
	QXfrm qxF2C = m_mc.qxmR2C() * qxF2R;
	for (auto f : vatf ) {
		size_t sTags = f->sTags();
		int nCam = f->nCam();
		CamParm const &cp = m_mc[nCam];

		for (size_t sTag = 0; sTag < sTags; sTag++) {
			ATDetection const &atd = (*f)[sTag];
			unsigned uTagId = atd.uId();

			FieldTag const &ft = m_Field[uTagId - 1];
			if (m_bSolveUseAll || atd.bUse()) {
				Point3d const *pCorners = ft.Corners();
				nRes += 8;

				if (pfRes) {
					for (int i = 0; i < 4; i++ ) {
						// Transform the corner point into the "Multi-Camera" frame of reference.
						Point3d p3dCam = qxF2C.TransformPoint( pCorners[i] );
//						printf( "p2dCam -> %.4f %.4f %.4f\n", p3dCam.x(), p3dCam.y(), p3dCam.z() );

						Point2d p2d = cp.ProjectPoint( p3dCam );

						// Get the U/V position of this corner point.
						auto p2f = atd[i];
						
//						printf( "%.4f -> %.4f  ,  %.4f -> %.4f\n", p2d.u(), p2f.x, p2d.v(), p2f.y );
						float fResU = p2d.u() - p2f.x;
						*pfRes++ = fResU;
						float fResV = p2d.v() - p2f.y;
						*pfRes++ = fResV;
						if (fResU > 500 || fResV > 500) {
							printf( "Res: %f %f\n", fResU, fResV );
						}
						nCnt++;
					}
				}
			}
		}
	}
	return nRes;
}

int PoseSolver::nCalcResiduals3D( QXfrm &xfm, std::vector<ATFrame *> &vatf, float *pfRes )
{
	int nRes = 0;
	int nCnt = 0;

	for (auto f : vatf ) {
		size_t sTags = f->sTags();
		int nCam = f->nCam();
		CamParm const &cp = m_mc[nCam];

		Rebels::QXfrm qxmR2S = cp.getXfrm() * m_mc.qxmR2C();
		Rebels::QXfrm qxmS2R = qxmR2S.Inverse();

		for (size_t sTag = 0; sTag < sTags; sTag++) {
			ATDetection const &atd = (*f)[sTag];
			unsigned uTagId = atd.uId();

			FieldTag const &ft = m_Field[uTagId - 1];
			if (m_bSolveUseAll || atd.bUse()) {
				Point3d const *pCorners = ft.Corners();
				nRes += 12;

				Rebels::QXfrm const &qxmT2S = atd;
				if (pfRes) {
					Rebels::QXfrm const qxmT2R = qxmS2R * qxmT2S;
					Rebels::QXfrm const qxmT2F = xfm * qxmT2R;

					for (int i = 0; i < 4; i++ ) {
						// Determine Position of Tag Corners in Sensor FOR
						Rebels::QXfrm const &qxmT2S = atd;
						Rebels::Point3d p3n = qxmT2F.TransformPoint(ft.RawCorner(i));
						
						Rebels::QXfrm const &qxField = ft;

						Rebels::Point3d p3Diff = p3n - qxField.Translation();
						
//						printf( "%8.3f  %8.3f  %8.3f\n", p3Diff.x(), p3Diff.y(), p3Diff.z() );
						*pfRes++ = p3Diff.x();
						*pfRes++ = p3Diff.y();
						*pfRes++ = p3Diff.z();
						nCnt++;
					}
				}
			}
		}
	}
	return nRes;
}

void PoseSolver::vSetMinTagPixels( uint32_t u32MinPixels )
{
	m_u32MinTagPixels = u32MinPixels;
}

void PoseSolver::vSetMaxDistance( uint32_t fMaxDist )
{
	m_fMaxDistSqd = fMaxDist * fMaxDist;
}

void PoseSolver::vSetSolveMode( bool bUseAll )
{
	m_bSolveUseAll = bUseAll;
}
void PoseSolver::vSetMaxRMS( float fRMS )
{
	m_fMaxRMS = fRMS;
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
				continue;
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

bool PoseSolver::bPose2D( QXfrm &xfm, std::vector<ATFrame *>&vatf )
{
	Rebels::Point3d	p3AvgField;
	Rebels::Point3d	p3AvgRobot;
	int nTagsUsed = 0;

	// Compute the average of all of the Tag center coordinates
	for (int nFrm = 0; nFrm < vatf.size(); nFrm++) {
		auto a = vatf[nFrm];

		auto &cp = m_mc[a->nCam()];

		Rebels::QXfrm qxmR2S = cp.getXfrm() * m_mc.qxmR2C();
		Rebels::QXfrm qxmS2R = qxmR2S.Inverse();

		for (size_t t = 0; t < a->sTags(); t++) {
			ATDetection &d = (*a)[t];
			if (d.uId() < 1 || d.uId() > 32) {
				d.vSetUse( false );
				continue;
			}
			FieldTag const &ft = m_Field[d.uId()-1];

			if (!ft.bIgnore()) {
				if (!d.bXfmValid()) {
					if (!bTagPNP( a->nCam(), d )) {
						d.vSetUse( false );
					}
				}
				if (d.bUse()) {
					// Determine Position of Tag in Sensor FOR
					Rebels::QXfrm const &qxmT2S = d;
					Rebels::Point3d pn = qxmT2S.Translation();

					//printf( "%.3f %.3f %.3f : ", pn.x(), pn.y(), pn.z() );
					pn = qxmS2R.TransformPoint(pn);
					//printf( "%.3f %.3f %.3f : ", pn.x(), pn.y(), pn.z() );

					p3AvgRobot += pn;

					// Take the translation part of the Transform which will be
					// the centre of the tag.
					Rebels::QXfrm const &qxmT2F = ft;

					p3AvgField += qxmT2F.Translation();
					pn = qxmT2F.Translation();
					//printf( "%.3f %.3f %.3f\n", pn.x(), pn.y(), pn.z() );
					nTagsUsed++;
				}
			}
		}
	}

	if (nTagsUsed < 2) {
		return false;
	}

	//printf( "%.3f %.3f %.3f / %.3f %.3f %.3f\n", p3AvgRobot.x(), p3AvgRobot.y(), p3AvgRobot.z(), p3AvgField.x(), p3AvgField.y(), p3AvgField.z() );
	p3AvgRobot /= nTagsUsed;
	p3AvgField /= nTagsUsed;
	Rebels::Point3d p3Dist = p3AvgRobot - p3AvgField;

	//printf( "%.3f %.3f %.3f / %.3f %.3f %.3f -> %.3f %.3f %.3f\n", p3AvgRobot.x(), p3AvgRobot.y(), p3AvgRobot.z(), p3AvgField.x(), p3AvgField.y(), p3AvgField.z(), p3Dist.x(), p3Dist.y(), p3Dist.z() );

	// Now run through all of the Tags again and compute the average rotation about the Z axis
	float fRotSum = 0.F;
	float nRot = 0;
	for (int nFrm = 0; nFrm < vatf.size(); nFrm++) {
		auto a = vatf[nFrm];

		auto &cp = m_mc[a->nCam()];

		Rebels::QXfrm qxmR2S = cp.getXfrm() * m_mc.qxmR2C();
		Rebels::QXfrm qxmS2R = qxmR2S.Inverse();

		for (size_t t = 0; t < a->sTags(); t++) {
			ATDetection &d = (*a)[t];
			FieldTag const &ft = m_Field[d.uId()-1];

			if (!ft.bIgnore()) {
				if (!d.bXfmValid()) {
					if (!bTagPNP( a->nCam(), d )) {
						d.vSetUse( false );
					}
				}
				if (d.bUse()) {
					// Determine Position of Tag in Sensor FOR
					Rebels::QXfrm const &qxmT2S = d;
					Rebels::Point3d pn = qxmT2S.Translation();

					pn = qxmS2R.TransformPoint(pn);

					// compute the vector from the avg position to this tag in robot FOR
					Rebels::Point3d p3VectRobot = pn - p3AvgRobot;
					Rebels::Point3d p3VectR2D( p3VectRobot.x(), p3VectRobot.y(), 0.F );
					p3VectR2D.Normalize();
					//printf( "%.3f %.3f %.3f : ", p3VectR2D.x(), p3VectR2D.y(), p3VectR2D.z() );

					// Take the translation part of the Transform which will be
					// the centre of the tag.
					Rebels::QXfrm const &qxmT2F = ft;

					Rebels::Point3d p3VectField = qxmT2F.Translation() - p3AvgField;
					Rebels::Point3d p3VectF2D( p3VectField.x(), p3VectField.y(), 0.f );
					p3VectF2D.Normalize();
					//printf( "%.3f %.3f %.3f : ", p3VectF2D.x(), p3VectF2D.y(), p3VectF2D.z() );

					float fDot = p3VectR2D.DotProduct( p3VectF2D );
					float fRot = acos( fDot );
					//printf( "%.3f\n", fRot * 180 / PI );
					fRotSum += fRot;
					nRot++;
				}
			}
		}
	}
	
	// Compute the average rotoation.
	if (!nRot) {
		return false;
	}
	fRotSum /= nRot;
	fRotSum *= -1;

	Rebels::Quaternion qRot( fRotSum, Rebels::Point3d( 0, 0, 1 ) );
	Rebels::Point3d p3XfmdRobot = qRot.RotatePoint( p3AvgRobot );

	Rebels::Point3d p3Pos = p3AvgField - p3XfmdRobot;
	xfm.vSetTranslation( p3Pos );
	xfm.vSetRotation( qRot );
	//printf( "--> %.4f %.4f %.4f %.4f : %.3f  %.3f  %3.f\n", qRot.q0(), qRot.x(), qRot.y(), qRot.z(), p3Pos.x(), p3Pos.y(), p3Pos.z() );

#if 0
	for (int nFrm = 0; nFrm < vatf.size(); nFrm++) {
		auto a = vatf[nFrm];

		auto &cp = m_mc[a->nCam()];

		Rebels::QXfrm qxmR2S = cp.getXfrm() * m_mc.qxmR2C();
		Rebels::QXfrm qxmS2R = qxmR2S.Inverse();

		for (size_t t = 0; t < a->sTags(); t++) {
			ATDetection &d = (*a)[t];
			FieldTag const &ft = m_Field[d.uId()-1];

			if (!ft.bIgnore()) {
				if (!d.bXfmValid()) {
					if (!bTagPNP( a->nCam(), d )) {
						d.vSetUse( false );
					}
				}
				if (d.bUse()) {
					// Determine Position of Tag in Sensor FOR
					Rebels::QXfrm const &qxmT2S = d;
					Rebels::Point3d pn = qxmT2S.Translation();
					printf( "%.3f %.3f %.3f : ", pn.x(), pn.y(), pn.z() );

					pn = qxmS2R.TransformPoint(pn);
					printf( "%.3f %.3f %.3f : ", pn.x(), pn.y(), pn.z() );
					pn = xfm.TransformPoint(pn);
					printf( "%.3f %.3f %.3f : ", pn.x(), pn.y(), pn.z() );

					// Take the translation part of the Transform which will be
					// the centre of the tag.
					Rebels::QXfrm const &qxmT2F = ft;


					Rebels::Point3d p3Err = pn - qxmT2F.Translation();

					pn = qxmT2F.Translation();
					printf( "%.3f %.3f %.3f : ", pn.x(), pn.y(), pn.z() );

					printf( "%.3f %.3f %.3f\n", p3Err.x(), p3Err.y(), p3Err.z());
				}
			}
		}
	}
#endif
	
	return true;
}

bool PoseSolver::bPoseHorn( QXfrm &xfm, std::vector<ATFrame *>&vatf )
{
	std::vector<Rebels::Point3d> vp3Field;
	std::vector<Rebels::Point3d> vp3Robot;

	// Build an array of the Tag corners in Field and Robot coordinate systems
	for (int nFrm = 0; nFrm < vatf.size(); nFrm++) {
		auto a = vatf[nFrm];

		auto &cp = m_mc[a->nCam()];

		Rebels::QXfrm qxmR2S = cp.getXfrm() * m_mc.qxmR2C();
		Rebels::QXfrm qxmS2R = qxmR2S.Inverse();

		for (size_t t = 0; t < a->sTags(); t++) {
			ATDetection &d = (*a)[t];
			FieldTag const &ft = m_Field[d.uId()-1];

			if (!d.bXfmValid()) {
				if (!bTagPNP( a->nCam(), d )) {
					d.vSetUse( false );
				}
			}
			if (d.bUse()) {
				for (int c = 0; c < 4; c++) {
					// Determine Position of Tag Corners Sensor FOR
					Rebels::QXfrm const &qxmT2S = d;
					Rebels::Point3d pn = qxmT2S.TransformPoint(ft.RawCorner(c));

					pn = qxmS2R.TransformPoint(pn);

					vp3Robot.push_back( pn );

					vp3Field.push_back(ft.Corners()[c]);
				}
			}
		}
	}

	// We want at least 2 apriltags * 4 corners
	while (vp3Robot.size() >= 8) {
		if (!bIterativeQ3drigid( vp3Robot, vp3Field, xfm)) {
			printf( "bIter returned false\n" );
			return false;
		}

		bool bUpdated = false;
		size_t stIdx = 0;
		while (stIdx < vp3Robot.size()) {
			Rebels::Point3d p3Xfmd = xfm.TransformPoint( vp3Robot[stIdx] );
			Rebels::Point3d p3Diff = vp3Field[stIdx] - p3Xfmd;
			
//			printf( "3DErr : %8.3f\n", p3Diff.length() );
			if (p3Diff.length() > m_fMax3DErr ) {
//				printf( "Discarding\n" );
				vp3Field.erase(vp3Field.begin() + stIdx);
				vp3Robot.erase(vp3Robot.begin() + stIdx);
				bUpdated = true;
			}
			else {
				stIdx++;
			}
		}

		if (!bUpdated) {
			double fMax;
			double fRMS = fCalcRMS( xfm, vp3Robot, vp3Field, fMax );
//			printf( "--> %.4f %.4f %.4f %.4f : %.3f  %.3f  %.3f  -- N: %d Max: %.3f  RMS: %.3f\n", xfm.Rotation().q0(), xfm.Rotation().x(), xfm.Rotation().y(), xfm.Rotation().z(), xfm.Translation().x(), xfm.Translation().y(), xfm.Translation().z(), vp3Robot.size(), fMax, fRMS );
			if (fRMS < m_fMaxRMS) {
				return true;
			}
			return false;
		}
	}
//	printf( "Not enough tags\n" );
	return false;

#if 0
	if (bCalcHorn( xfm, vp3Field, vp3Robot )) {
#if 1
		for (size_t i = 0; i < vp3Robot.size(); i++) {
			Rebels::Point3d p3n = xfm.TransformPoint(vp3Robot[i]);
			Rebels::Point3d p3Err = p3n - vp3Field[i];

			printf( "%.3f  %.3f  %.3f\n", p3Err.x(), p3Err.y(), p3Err.z() );
		}
#endif
		return true;
	}
#else
//	if (bIterativeQ3drigid( vp3Field, vp3Robot, xfm)) {
	if (bIterativeQ3drigid( vp3Robot, vp3Field, xfm)) {
		double fMax;
		double fRMS = fCalcRMS( xfm, vp3Robot, vp3Field, fMax );
		printf( "--> %.4f %.4f %.4f %.4f : %.3f  %.3f  %3.f  -- Max: %.3f  RMS: %.3f\n", xfm.Rotation().q0(), xfm.Rotation().x(), xfm.Rotation().y(), xfm.Rotation().z(), xfm.Translation().x(), xfm.Translation().y(), xfm.Translation().z(), fMax, fRMS );
		if (fRMS < 10.f) {
			return true;
		}
	}
#endif
	return false;
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
		sum = cv::norm(reprojectPoints, imgPoints, cv::NORM_L2SQR);
		sum = sum / idx;

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
		bool bUseSolve = sqrt(sum) < m_fMaxRMS;
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
			return true;
		}
		else {
//			printf( "RMS2 = %.2f\n", sum );
		}
	}
	else {
//		printf( "SolvePNP failed\n" );
	}

	return false;
}

bool PoseSolver::bGuessPose( QXfrm &xfm, std::vector<ATFrame *>&vatf )
{
	// Determine Min/Max X/Y locations for robot based on which Tags are seen
	float fMaxX = 600;
	float fMinX = 0;
	float fMaxY = 300;
	float fMinY = 0;

	Rebels::Point3d vXNorm( 1., 0., 0. );
	Rebels::Point3d vZNorm( 0., 0., 1. );
	
	float fThetaSum = 0.;
	int nTags = 0;
	for (int i = 0; i < vatf.size(); i++) {
		for (size_t t = 0; t < vatf[i]->sTags(); t++ ) {
			ATDetection const &d = (*vatf[i])[t];
			FieldTag const &ft = m_Field[d.uId() - 1];
			if (d.bUse()) {
				nTags++;
				Rebels::QXfrm const &q = ft;
				Rebels::Point3d vDir = q.Rotation().RotatePoint( vXNorm );

				// is tag pointing along the field or across???
				if (fabs(vDir.x()) > fabs(vDir.y())) {
					// tag pointing positively down field, so robot can't be any closer than this tag
					if (vDir.x() > 0) {
						if (fMinX < q.Translation().x()) {
							fMinX = q.Translation().x();
						}
					}
					else {
						if (fMaxX > q.Translation().x()) {
							fMaxX = q.Translation().x();
						}
					}
				}
				else {
					if (vDir.y() > 0) {
						if (fMinY < q.Translation().y()) {
							fMinY = q.Translation().y();
						}
					}
					else {
						if (fMaxY > q.Translation().y()) {
							fMaxY = q.Translation().y();
						}
					}
				}

				auto const &cp = m_mc[vatf[i]->nCam()];
				Quaternion qCam = cp.getRotation().Inverse();
				Rebels::Point3d p3Cam = qCam.RotatePoint( vZNorm ) * -1;

				// both vectors are unit vectors...
				float fCosTheta = vXNorm.DotProduct(p3Cam);
				float fTheta = acos( fCosTheta );
				if (fTheta < 0) {
					printf( "Theta < 0\n" );
				}
				fThetaSum += fTheta;
			}
		}
	}

	if (!nTags)
		return false;

	Rebels::Point3d p3Robot( (fMaxX + fMinX) / 2, (fMaxY + fMinY) / 2, 0. );
	Rebels::Quaternion qRobot( fThetaSum/nTags, Rebels::Point3d( 0., 0., 1. ) );

	xfm = QXfrm( qRobot, p3Robot );
	return true;
}

bool PoseSolver::bSolve( QXfrm &xfm, std::vector<ATFrame *> &vatf )
{
	// count the number of equations...
	int nCams;
	int m;
	uint64_t u64Mask;

	m = nEnumerateEqnsUV( vatf, nCams, u64Mask );

//	if (!m || nCams < 2) {
	if (m < 8) {
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

	return true;
}


#include <opencv2/core.hpp>

static Rebels::Quaternion SolveMatrix( double M[4][4] )
{
	cv::Mat m( 4, 4, CV_64FC1, M );
	cv::Mat e;
	cv::Mat ev;

	cv::eigen( m, e, ev );

	Rebels::Quaternion q( ev.at<double>(0, 0), ev.at<double>(0, 1), ev.at<double>(0,2), ev.at<double>(0,3) );
	q.Normalize();

	return q;
}

bool PoseSolver::bCalcHorn( QXfrm &xfm, std::vector<Rebels::Point3d> const &vp3Meas, std::vector<Rebels::Point3d> const &vp3Ref )
{
	Rebels::Point3d p3AvgRef;
	Rebels::Point3d p3AvgMeas;

	size_t n = vp3Ref.size();
	for (size_t i = 0; i < n; i++) {
		p3AvgRef += vp3Ref[i];
		p3AvgMeas += vp3Meas[i];
	}

	p3AvgRef /= n;
	p3AvgMeas /= n;

    // Compute cross-covariance elements
    double Sxx = 0, Sxy = 0, Sxz = 0;
    double Syx = 0, Syy = 0, Syz = 0;
    double Szx = 0, Szy = 0, Szz = 0;

    for (size_t i = 0; i < n; ++i) {
        Rebels::Point3d a = vp3Ref[i] - p3AvgRef;
        Rebels::Point3d b = vp3Meas[i] - p3AvgMeas;
        Sxx += a.x() * b.x(); Sxy += a.x() * b.y(); Sxz += a.x() * b.z();
        Syx += a.y() * b.x(); Syy += a.y() * b.y(); Syz += a.y() * b.z();
        Szx += a.z() * b.x(); Szy += a.z() * b.y(); Szz += a.z() * b.z();
    }

    // Construct the N matrix (4x4)
    double N[4][4];
    
    // Trace elements
    double tr = Sxx + Syy + Szz;
    
    // Constructing symmetric matrix N based on Horn's paper
    N[0][0] = tr;
    N[0][1] = Syz - Szy;
    N[0][2] = Szx - Sxz;
    N[0][3] = Sxy - Syx;

    N[1][0] = N[0][1];
    N[1][1] = Sxx - Syy - Szz;
    N[1][2] = Sxy + Syx;
    N[1][3] = Szx + Sxz;

    N[2][0] = N[0][2];
    N[2][1] = N[1][2];
    N[2][2] = -Sxx + Syy - Szz;
    N[2][3] = Syz + Szy;

    N[3][0] = N[0][3];
    N[3][1] = N[1][3];
    N[3][2] = N[2][3];
    N[3][3] = -Sxx - Syy + Szz;

	// Compute the quaternion rotation
//	Rebels::Quaternion qRot = findLargestEigenvector(N);
	Rebels::Quaternion qRot = SolveMatrix(N);
	printf( "%.4f %.4f  %.4f  %.4f\n", qRot.q0(), qRot.x(), qRot.y(), qRot.z() );

	// Now compute the translation
#if 1
	Rebels::Point3d p3XfmdRef = qRot.RotatePoint( p3AvgRef );

	Rebels::Point3d p3Pos = p3AvgMeas - p3XfmdRef;
	xfm.vSetTranslation( p3Pos );
	xfm.vSetRotation( qRot );
#else
	Rebels::Point3d p3XfmdMeas = qRot.RotatePoint( p3AvgMeas );

	Rebels::Point3d p3Pos = p3AvgRef - p3XfmdMeas;
	xfm.vSetTranslation( p3Pos );
	xfm.vSetRotation( qRot );
#endif
	double fMax;
	double fRMS = fCalcRMS( xfm, vp3Meas, vp3Ref, fMax );
	printf( "--> %.4f %.4f %.4f %.4f : %.3f  %.3f  %3.f  -- Max: %.3f  RMS: %.3f\n", qRot.q0(), qRot.x(), qRot.y(), qRot.z(), p3Pos.x(), p3Pos.y(), p3Pos.z(), fMax, fRMS );
	return true;	
}
# if 0
Quaternion absoluteOrientation(const std::vector<Vector3>& A, const std::vector<Vector3>& B)
{
    size_t n = A.size();
    Vector3 centroidA = {0, 0, 0}, centroidB = {0, 0, 0};

    for (size_t i = 0; i < n; ++i) {
        centroidA = centroidA + A[i];
        centroidB = centroidB + B[i];
    }
    centroidA /= (double)n;
    centroidB /= (double)n;

    // Compute cross-covariance elements
    double Sxx = 0, Sxy = 0, Sxz = 0;
    double Syx = 0, Syy = 0, Syz = 0;
    double Szx = 0, Szy = 0, Szz = 0;

    for (size_t i = 0; i < n; ++i) {
        Vector3 a = A[i] - centroidA;
        Vector3 b = B[i] - centroidB;
        Sxx += a.x * b.x; Sxy += a.x * b.y; Sxz += a.x * b.z;
        Syx += a.y * b.x; Syy += a.y * b.y; Syz += a.y * b.z;
        Szx += a.z * b.x; Szy += a.z * b.y; Szz += a.z * b.z;
    }

    // Construct the M matrix (4x4)
    double M[4][4];
    
    // Trace elements
    double tr = Sxx + Syy + Szz;
    
    // Constructing symmetric matrix M based on Horn's paper
    M[0][0] = tr;
    M[0][1] = Syz - Szy;
    M[0][2] = Szx - Sxz;
    M[0][3] = Sxy - Syx;

    M[1][0] = M[0][1];
    M[1][1] = Sxx - (Syy + Szz);
    M[1][2] = Sxy + Syx;
    M[1][3] = Szx + Sxz;

    M[2][0] = M[0][2];
    M[2][1] = M[1][2];
    M[2][2] = Syy - (Sxx + Szz);
    M[2][3] = Syz + Szy;

    M[3][0] = M[0][3];
    M[3][1] = M[1][3];
    M[3][2] = M[2][3];
    M[3][3] = Szz - (Sxx + Syy);

    return findLargestEigenvector(M);
}

int main() {
    // Example usage: Two sets of points (B is A rotated 90 deg around Z)
    std::vector<Vector3> A = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    std::vector<Vector3> B = {{0, 1, 0}, {-1, 0, 0}, {0, 0, 1}};

    Quaternion q = absoluteOrientation(A, B);

    std::cout << "Rotation Quaternion (w, x, y, z): " 
              << q.q0 << ", " << q.qx << ", " << q.qy << ", " << q.qz << std::endl;

    return 0;
}
#endif
#if 0
#include <stdio.h>
#include <math.h>

#define N 4
#define MAX_ITER 100

// Helper to print matrix
void print_matrix(double mat[N][N]) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++)
            printf("%10.4f ", mat[i][j]);
        printf("\n");
    }
}

// Perform QR Decomposition using Householder transformation
void qr_decomposition(double A[N][N], double Q[N][N], double R[N][N]) {
    double v[N], u[N];
    double tempQ[N][N], I[N][N];

    // Initialize Q as Identity
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            Q[i][j] = (i == j) ? 1.0 : 0.0;
            R[i][j] = A[i][j];
        }
    }

    for (int k = 0; k < N - 1; k++) {
        double x_norm = 0;
        for (int i = k; i < N; i++) x_norm += R[i][k] * R[i][k];
        x_norm = sqrt(x_norm);

        double alpha = (R[k][k] > 0) ? -x_norm : x_norm;
        
        // Construct Householder vector v
        for (int i = 0; i < N; i++) v[i] = 0;
        v[k] = R[k][k] - alpha;
        for (int i = k + 1; i < N; i++) v[i] = R[i][k];

        double v_norm = 0;
        for (int i = k; i < N; i++) v_norm += v[i] * v[i];
        v_norm = sqrt(v_norm);

        if (v_norm > 1e-10) {
            for (int i = k; i < N; i++) v[i] /= v_norm;

            // Apply transformation to R: R = (I - 2vv^T)R
            for (int j = k; j < N; j++) {
                double dot = 0;
                for (int i = k; i < N; i++) dot += v[i] * R[i][j];
                for (int i = k; i < N; i++) R[i][j] -= 2.0 * v[i] * dot;
            }

            // Accumulate Q: Q = Q(I - 2vv^T)
            for (int i = 0; i < N; i++) {
                double dot = 0;
                for (int j = k; j < N; j++) dot += Q[i][j] * v[j];
                for (int j = k; j < N; j++) Q[i][j] -= 2.0 * dot * v[j];
            }
        }
    }
}

void qr_algorithm(double A[N][N]) {
    double Q[N][N], R[N][N], nextA[N][N];

    for (int iter = 0; iter < MAX_ITER; iter++) {
        qr_decomposition(A, Q, R);

        // A_next = R * Q
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                nextA[i][j] = 0;
                for (int k = 0; k < N; k++)
                    nextA[i][j] += R[i][k] * Q[k][j];
            }
        }

        // Copy nextA back to A
        for (int i = 0; i < N; i++)
            for (int j = 0; j < N; j++)
                A[i][j] = nextA[i][j];
    }
}

int main() {
    double A[N][N] = {
        {4, 1, -2, 2},
        {1, 2, 0, 1},
        {-2, 0, 3, -2},
        {2, 1, -2, -1}
    };

    printf("Original Matrix:\n");
    print_matrix(A);

    qr_algorithm(A);

    printf("\nMatrix after QR Algorithm (Eigenvalues on diagonal):\n");
    print_matrix(A);

    return 0;
}
#endif
#if 0
#include <stdio.h>
#include <math.h>

#define N 4
#define MAX_ITER 500
#define TOLERANCE 1e-10

// Function to calculate the dot product of two vectors
double dot_product(double v1[N], double v2[N]) {
    double result = 0.0;
    for (int i = 0; i < N; i++) result += v1[i] * v2[i];
    return result;
}

// Function to normalize a vector to unit length
void normalize(double v[N]) {
    double norm = sqrt(dot_product(v, v));
    for (int i = 0; i < N; i++) v[i] /= norm;
}

void power_iteration(double A[N][N]) {
    double b[N] = {1.0, 0.5, 0.2, 0.1}; // Initial guess vector
    double next_b[N];
    double eigenvalue = 0.0;
    double prev_eigenvalue = 0.0;

    printf("Starting Power Iteration...\n\n");

    for (int iter = 0; iter < MAX_ITER; iter++) {
        // 1. Multiply: next_b = A * b
        for (int i = 0; i < N; i++) {
            next_b[i] = 0.0;
            for (int j = 0; j < N; j++) {
                next_b[i] += A[i][j] * b[j];
            }
        }

        // 2. Estimate Eigenvalue (Rayleigh Quotient)
        // Since we normalize b at the end of each loop, b_dot_b is 1.0
        // So lambda = (b * Ab) / (b * b) simplifies to b * next_b
        eigenvalue = dot_product(b, next_b);

        // 3. Normalize next_b and update b
        double norm = sqrt(dot_product(next_b, next_b));
        for (int i = 0; i < N; i++) {
            b[i] = next_b[i] / norm;
        }

        // 4. Check for convergence
        if (fabs(eigenvalue - prev_eigenvalue) < TOLERANCE) {
            printf("Converged in %d iterations.\n", iter + 1);
            break;
        }
        prev_eigenvalue = eigenvalue;
    }

    // Output Results
    printf("Largest Eigenvalue:  %f\n", eigenvalue);
    printf("Associated Eigenvector: [ ");
    for (int i = 0; i < N; i++) {
        printf("%.6f ", b[i]);
    }
    printf("]\n");
}

int main() {
    // Example 4x4 Matrix
    double A[N][N] = {
        {6, 2, 0, 1},
        {2, 5, 1, 0},
        {0, 1, 4, 2},
        {1, 0, 2, 3}
    };

    power_iteration(A);

    return 0;
}
#endif

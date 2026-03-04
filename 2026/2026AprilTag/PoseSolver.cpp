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
	Euler e( px[0], px[1], px[2] );
	Quaternion q( e );
	Point3d p( px[3], px[4], px[5] );

	QXfrm qxF2R( q, p );  // 
	QXfrm qxF2C = m_mc.qxmR2C()* qxF2R;
	for (auto f : *m_pvatf ) {
		size_t sTags = f->sTags();
		int nCam = f->nCam();
		CamParm const &cp = m_mc[nCam];

		for (size_t sTag = 0; sTag < sTags; sTag++) {
			ATDetection const &atd = (*f)[sTag];
			unsigned uTagId = atd.uId();

			FieldTag const &ft = m_Field[uTagId - 1];
			Point3d const *pCorners = ft.Corners();
			for (int i = 0; i < 4; i++ ) {
				// Transform the corner point into the "Multi-Camera" frame of reference.
				Point3d p3dCam = qxF2C.TransformPoint( pCorners[i] );
#if 0
				// Transform the corner point into the "Sensor's" frame of reference
				// and compute the undistorted point on the sensor
				Point3d p3dSen = cp.getXfrm().TransformPoint( p3dCam );
				float fU = (p3dSen.x() / p3dSen.z()) * m_fU + m_fU0;
				float fV = (p3dSen.y() / p3dSen.z()) * m_fV + m_fV0;
#else
				Point2d p2d = cp.ProjectPoint( p3dCam );
#endif

				// Get the U/V position of this corner point.  Note that the
				// Apriltag Library "UnDistorts" the corners
				auto p2f = atd[i];
				
				*pv++ = p2d.u() - p2f.x;
				*pv++ = p2d.v() - p2f.y;
			}
		}
	}
	return 1;
}

bool PoseSolver::bSolve( QXfrm &xfm, std::vector<ATFrame *> &vatf )
{
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
	grfX[4] = qxInv.Translation().y();
	grfX[5] = qxInv.Translation().z();

	// count the number of unknowns...
	int m = 0;
	for (auto f : vatf ) {
		m += f->sTags() * 8;  // there are 4 corners per tag and a u,v coordinate pair per corner
	}

	float *pfVals = new float[n];
	int *pniwa = new int[n];
	int wa = m * n + 5 * n + m;
	float *plwa = new float[wa];

	slmdif1( eval, static_cast<void *>(this), m, n, grfX, pfVals, .1, pniwa, plwa, wa  );

	delete plwa;
	delete pniwa;
	delete pfVals;
	Euler euNew( grfX[0], grfX[1], grfX[2] );
	Point3d p3( grfX[3], grfX[4], grfX[5] );
	QXfrm qxNew( euNew, p3 );

	xfm = qxNew.Inverse();

	return true;
}

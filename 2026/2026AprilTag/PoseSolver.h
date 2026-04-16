#include <vector>

#include "2702math.h"
#include "2702camera.h"

#include "ATFrame.h"
#include "field.h"

using namespace Rebels;

class PoseSolver
{
	Field					m_Field;
	MultiCam				m_mc;
	std::vector<CamParm>	m_vcp;
	std::vector<ATFrame *>	*m_pvatf;

	uint32_t				m_u32MinTagPixels = 1;
#if FIELD_UNIT_INCHES
	float					m_fMaxDistSqd = (27.F * 12) * (27.F * 12);
#else
	float					m_fMaxDistSqd = (8.F * 8.F);
#endif
	bool					m_bSolveUseAll = false;
	float					m_fMaxRMS2 = 100.F;
	float					m_fMax3DErr = 10.F;

static int eval( void *p, int m, int n, float const *x, float *fv, int iflag );

	int eval( int m, int n, float const *x, float *fv, int iflag );

	bool bTagPNP( unsigned uCam, ATDetection &d );

	bool bCalcHorn( QXfrm &xfm, std::vector<Rebels::Point3d> const &vp3Ref, std::vector<Rebels::Point3d> const &vpMeas );

public:
	PoseSolver( Field const &fld, MultiCam const &mc );
	~PoseSolver();

	bool bFilterTags( std::vector<ATFrame *>&vatf );
	bool bEstimate( QXfrm &xfm, std::vector<ATFrame *> &vatf );
	bool bSolve( QXfrm &xfm, std::vector<ATFrame *> &vatf );

	bool bGuessPose( QXfrm &xfm, std::vector<ATFrame *>&vatf );

	bool bPose2D( QXfrm &xfm, std::vector<ATFrame *>&vatf );

	// Uses Horn Algorithm from "Closed-Form solution to absolute orientation using unit quaternions"
	bool bPoseHorn( QXfrm &xfm, std::vector<ATFrame *>&vatf );

	// returns number of residuals for this frame...  if pfRes == null, simply counts the
	// number of residuals for this frame.
	int nCalcResidualsUV( QXfrm &xfm, std::vector<ATFrame *> &vatf, float *pfRes );
	int nCalcResiduals3D( QXfrm &xfm, std::vector<ATFrame *> &vatf, float *pfRes );
	void vSetMinTagPixels( uint32_t u32MinPixels );
	void vSetMaxDistance( uint32_t fMaxDist );
	void vSetSolveMode( bool bUseAll );
	void vSetMaxRMS( float fRMS );

	int nEnumerateEqnsUV( std::vector<ATFrame *> const &vatf, int &nCams, uint64_t &u64TagMask ) const;
	int nEnumerateEqns3D( std::vector<ATFrame *> const &vatf, int &nCams, uint64_t &u64TagMask ) const;
};

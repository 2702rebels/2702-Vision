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
static int eval( void *p, int m, int n, float const *x, float *fv, int iflag );

	int eval( int m, int n, float const *x, float *fv, int iflag );
public:
	PoseSolver( Field const &fld, MultiCam const &mc );
	~PoseSolver();

	bool bSolve( QXfrm &xfm, std::vector<ATFrame *> &vatf );
};

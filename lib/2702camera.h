#pragma once

#include "2702math.h"

namespace Rebels {

class CamParm {
  float m_grfMat[3][3];
  float m_grfDist[5];	 // distortion paramters
  float m_fU;
  float m_fV;
  float m_fU0;
  float m_fV0;
  int m_nURes;
  int m_nVRes;

  QXfrm m_qxm;

 public:
  CamParm();
  ~CamParm() {}

  inline float const (*(intrinsics)() const)[3] {
  	return m_grfMat;
  }
  inline float const *distortion() const {
  	return m_grfDist;
  }
  float pinhole(int r, int c) const
  {
  	return m_grfMat[r][c];
  }
  float fFU() const
  {
  	return m_fU;
  }
  float fFV() const
  {
  	return m_fV;
  }
  float fU0() const
  {
  	return m_fU0;
  }
  float fV0() const
  {
  	return m_fV0;
  }
  
  inline float fk1() const {
  	return m_grfDist[0];
  }
  
  inline float fk2() const {
  	return m_grfDist[1];
  }
  
  inline float fp1() const {
  	return m_grfDist[2];
  }
  
  inline float fp2() const {
  	return m_grfDist[3];
  }
  
  inline float fk3() const {
  	return m_grfDist[4];
  }

  float dist(int i) const {
  	return m_grfDist[i];
  }

  bool Load(char const *psz);
  bool Save(char const *psz);
  void SetRotation(Quaternion const &q) { m_qxm.vSetRotation( q ); }
  void SetOrigin(Point3d const &p) { m_qxm.vSetTranslation( p ); }
  Quaternion const &getRotation() const { return m_qxm.Rotation(); }
  Point3d const &getOrigin() const { return m_qxm.Translation(); }
  QXfrm const &getXfrm() const { return m_qxm; }

  void UpdateResolution(int nURes, int nVRes);
  void Undistort(float fU, float fV, float &fUndistU, float &fUnistV) const;			// normalized by focal length
  void UndistortPixels(float fU, float fV, float &fUndistU, float &fUnistV) const;		// in Pixel units
  Point3d LOS(float fU, float fV) const;

  Point2d ProjectPoint( Point3d const &p3d ) const;

  Point3d CalcTargetPosFromHeight(float fU, float fV, float fZ);

  CamParm &operator=(CamParm const &src);
};
#if 1
class StereoCam {
  CamParm m_grCams[2];
  Quaternion m_CamToRobotRot;
  Point3d m_CamToRobotXlat;

 public:
  StereoCam() {}
  StereoCam(CamParm const &c1, CamParm const &c2) {
    m_grCams[0] = c1;
    m_grCams[1] = c2;
  }
  ~StereoCam() {}

  void SetCam(int nCam, CamParm const &c) { m_grCams[nCam] = c; }
  void SetCamToRobot(Quaternion const &q, Point3d const &x) {
    m_CamToRobotRot = q;
    m_CamToRobotXlat = x;
  }
  bool bLoadCam(int nCam, char const *psz);
  void SetCamXfrms(Quaternion const &q1, Point3d const &x1,
                   Quaternion const &q2, Point3d const &x2);
  bool bLoad(char const *psz);
  bool Save(char const *psz);
  void UpdateResolution(int nURes, int nVRes);
  float fCalc3D(Point2d const &p1, Point2d const &p2,
                Point3d &res);  // returns line separation
};
#endif
// MultiCam stores the camera paramters and transformation for collection of
// cameras.
class MultiCam {
	std::vector<CamParm>	m_vcp;
	QXfrm					m_qxmR2C;		// Transformation from Robot Coordinates to Camera Coordinates
public:
	MultiCam() {}
	MultiCam( std::vector<CamParm> vcp, QXfrm qxmR2C );
	MultiCam( char const *pszFile );
	~MultiCam();

	bool bLoad( char const *psz );
	bool bSave( char const *psz );

	inline size_t nCam() const
	{
		return m_vcp.size();
	}
	inline CamParm const &operator[]( size_t n ) const
	{
		return m_vcp[n];
	}
	inline CamParm &operator[]( size_t n )
	{
		return m_vcp[n];
	}

	inline QXfrm const &qxmR2C() const
	{
		return m_qxmR2C;
	}
};
};  // namespace Rebels

#include <stdio.h>

#include "2702math.h"
#include "2702PackUnpack.h"

using namespace Rebels;

Point3d::Point3d(Point3d const &src)
{
	*this = src;
}

Point3d::Point3d(uint8_t const *pu8, size_t stLen)
{
	UnPack( pu8, stLen );
}

Point3d &Point3d::operator=(Point3d const &other)
{
  m_fX = other.m_fX;
  m_fY = other.m_fY;
  m_fZ = other.m_fZ;

  return *this;
}

Point3d Point3d::operator+(Point3d const &other) const
{
  float x = m_fX + other.x();
  float y = m_fY + other.y();
  float z = m_fZ + other.z();
  return Point3d(x, y, z);
}

Point3d &Point3d::operator+=(Point3d const &other)
{
  m_fX += other.x();
  m_fY += other.y();
  m_fZ += other.z();

  return *this;
}

Point3d &Point3d::operator-=(Point3d const &other)
{
  m_fX -= other.x();
  m_fY -= other.y();
  m_fZ -= other.z();

  return *this;
}

Point3d Point3d::operator-(Point3d const &other) const
{
  float x = m_fX - other.x();
  float y = m_fY - other.y();
  float z = m_fZ - other.z();
  return Point3d(x, y, z);
}

Point3d Point3d::operator*(float const scalar) const
{
  Point3d ret(m_fX * scalar, m_fY * scalar, m_fZ * scalar);
  return ret;
}

Point3d &Point3d::operator*=(float const scalar)
{
  m_fX *= scalar;
  m_fY *= scalar;
  m_fZ *= scalar;
  return *this;
}

Point3d Point3d::operator/(float const scalar) const
{
  // todo: catch divide by 0
  Point3d ret(m_fX / scalar, m_fY / scalar, m_fZ / scalar);
  return ret;
}

Point3d &Point3d::operator/=(float const scalar)
{
  // todo: catch divide by 0
  m_fX /= scalar;
  m_fY /= scalar;
  m_fZ /= scalar;
  return *this;
}

float Point3d::length() const
{
  return sqrt(m_fX * m_fX + m_fY * m_fY + m_fZ * m_fZ);
}

Point3d Point3d::CrossProduct(Point3d const &v2) const
{
  float fI = m_fY * v2.m_fZ - v2.m_fY * m_fZ;
  float fJ = m_fZ * v2.m_fX - v2.m_fZ * m_fX;
  float fK = m_fX * v2.m_fY - v2.m_fX * m_fY;

  return Point3d(fI, fJ, fK);
}

float Point3d::DotProduct(Point3d const &v2) const
{
  return m_fX * v2.m_fX + m_fY * v2.m_fY + m_fZ * v2.m_fZ;
}

void Point3d::Normalize()
{
  float fMag = sqrt(m_fX * m_fX + m_fY * m_fY + m_fZ * m_fZ);
  if (fMag == 0.F) {
    printf("Error - Point normalize->length = 0.\n");
    m_fX = 1.F;
    m_fY = 0.F;
    m_fZ = 0.F;
  } else {
    m_fX /= fMag;
    m_fY /= fMag;
    m_fZ /= fMag;
  }
}

size_t Point3d::UnPack(uint8_t const *pu8, size_t stLen)
{
	size_t stIdx = 0;

	stIdx += ::UnPack(m_fX, pu8, stLen);
	stIdx += ::UnPack(m_fY, pu8 + stIdx, stLen - stIdx);
	stIdx += ::UnPack(m_fZ, pu8 + stIdx, stLen - stIdx);
	
	return stIdx;
}

#if 0
#if 1
RotationMatrix &RotationMatrix::operator=( Euler const &src )
{
	float fCosYaw = cos( src.m_fYaw );
	float fSinYaw = sin( src.m_fYaw );
	float fCosPitch = cos( src.m_fPitch );
	float fSinPitch = sin( src.m_fPitch );
	float fCosRoll = cos( src.m_fRoll );
	float fSinRoll = sin( src.m_fRoll );
	float fSYSP = fSinYaw * fSinPitch;
	float fCYSP = fCosYaw * fSinPitch;
		
	m_grfMat[0][0] = fCosPitch * fCosRoll;
	m_grfMat[0][1] = -fCosPitch * fSinRoll;
	m_grfMat[0][2] = -fSinPitch;
	m_grfMat[1][0] = -fSYSP * fCosRoll + fCosYaw * fSinRoll;
	m_grfMat[1][1] =  fSYSP * fSinRoll + fCosYaw * fCosRoll;
	m_grfMat[1][2] = -fSinYaw * fCosPitch;
	m_grfMat[2][0] = fCYSP * fCosRoll + fSinYaw * fSinRoll;
	m_grfMat[2][1] = -fCYSP * fSinRoll + fSinYaw * fCosRoll;
	m_grfMat[2][2] = fCosYaw * fCosPitch;

	return *this;
}
#else
// Rx = yaw
// Ry = pitch
// Rz = roll
//       | 1     0     0 |      | cosy 0 -siny |      | cosz -sinz 0 |
//  Rx = | 0  cosx -sinx | Ry = |    0 1     0 | Rz = | sinz  cosz 0 |
//       | 0  sinx  cosx |      | siny 0  cosy |      |    0     0 1 |

RotationMatrix &RotationMatrix::operator=( Euler const &src )
{
	float fCosYaw = cos( src.m_fYaw );
	float fSinYaw = sin( src.m_fYaw );
	float fCosPitch = cos( src.m_fPitch );
	float fSinPitch = sin( src.m_fPitch );
	float fCosRoll = cos( src.m_fRoll );
	float fSinRoll = sin( src.m_fRoll );
	float fSYSP = fSinYaw * fSinPitch;
	float fCYSP = fCosYaw * fSinPitch;
		
	m_grfMat[0][0] = fCosPitch * fCosRoll;
	m_grfMat[0][1] = fSYSP * fCosRoll - fSinRoll * fCosYaw;
	m_grfMat[0][2] = fCYSP * fCosRoll + fSinRoll * fSinYaw;
	m_grfMat[1][0] = fCosPitch * fSinRoll;
	m_grfMat[1][1] = fSYSP * fSinRoll + fCosYaw * fCosRoll;
	m_grfMat[1][2] = fCYSP * fSinRoll - fSinYaw * fCosRoll;
	m_grfMat[2][0] = -fSinPitch;
	m_grfMat[2][1] = fSinYaw * fCosPitch;
	m_grfMat[2][2] = fCosYaw * fCosPitch;

	return *this;
}
#endif
#endif

RotationMatrix &RotationMatrix::operator=(RotationMatrix const &src)
{
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      m_grfMat[i][j] = src.m_grfMat[i][j];
    }
  }
  return *this;
}

RotationMatrix RotationMatrix::operator*(RotationMatrix const &right)
{
  RotationMatrix res;

  for (int c = 0; c < 3; c++) {
    for (int r = 0; r < 3; r++) {
      res.m_grfMat[r][c] = m_grfMat[r][0] * right.m_grfMat[0][c] +
                           m_grfMat[r][1] * right.m_grfMat[1][c] +
                           m_grfMat[r][2] * right.m_grfMat[2][c];
    }
  }
  return res;
}

RotationMatrix &RotationMatrix::operator=(Quaternion const &quat)
{
  float fQ0X = quat.m_fQ0 * quat.m_fX;
  float fQ0Y = quat.m_fQ0 * quat.m_fY;
  float fQ0Z = quat.m_fQ0 * quat.m_fZ;

  float fXX = quat.m_fX * quat.m_fX;
  float fXY = quat.m_fX * quat.m_fY;
  float fXZ = quat.m_fX * quat.m_fZ;

  float fYY = quat.m_fY * quat.m_fY;
  float fYZ = quat.m_fY * quat.m_fZ;

  float fZZ = quat.m_fZ * quat.m_fZ;

  m_grfMat[0][0] = 1.F - 2.F * (fYY + fZZ);
  m_grfMat[0][1] = 2.F * (fXY - fQ0Z);
  m_grfMat[0][2] = 2.F * (fXZ + fQ0Y);

  m_grfMat[1][0] = 2.F * (fXY + fQ0Z);
  m_grfMat[1][1] = 1.F - 2.F * (fXX + fZZ);
  m_grfMat[1][2] = 2.F * (fYZ - fQ0X);

  m_grfMat[2][0] = 2.F * (fXZ - fQ0Y);
  m_grfMat[2][1] = 2.F * (fYZ + fQ0X);
  m_grfMat[2][2] = 1.F - 2.F * (fXX + fYY);

  return *this;
}

Point3d RotationMatrix::operator*(Point3d const &pt) const
{
  float fX = m_grfMat[0][0] * pt.x() + m_grfMat[0][1] * pt.y() +
             m_grfMat[0][2] * pt.z();
  float fY = m_grfMat[1][0] * pt.x() + m_grfMat[1][1] * pt.y() +
             m_grfMat[1][2] * pt.z();
  float fZ = m_grfMat[2][0] * pt.x() + m_grfMat[2][1] * pt.y() +
             m_grfMat[2][2] * pt.z();

  return Point3d(fX, fY, fZ);
}

Euler::Euler( RotationMatrix const &r )
{
	// from matrix quaternion faq
	float m_fRy = -asin( r.m_grfMat[0][2]);        /* Calculate Y-axis angle */
    float C     =  cos( m_fRy );

    if ( fabs( C ) > 0.005 )             /* Gimball lock? */
      {
      float ftrx      =  r.m_grfMat[2][2] / C;           /* No, so get X-axis angle */
      float ftry      = -r.m_grfMat[1][1] / C;

      m_fRx  = atan2( ftry, ftrx );

      ftrx      =  r.m_grfMat[0][0] / C;            /* Get Z-axis angle */
      ftry      = -r.m_grfMat[0][1] / C;

      m_fRz  = atan2( ftry, ftrx );
      }
    else                                 /* Gimball lock has occurred */
      {
      m_fRx  = 0;                      /* Set X-axis angle to zero */

      float ftrx      = r.m_grfMat[1][1];                 /* And calculate Z-axis angle */
      float ftry      = r.m_grfMat[1][0];

      m_fRz  = atan2( ftry, ftrx );
      }

}

Euler::Euler( Quaternion const &q )
{
    // roll (x-axis rotation)
    double sinr_cosp = 2 * (q.m_fQ0 * q.m_fX + q.m_fY * q.m_fZ);
    double cosr_cosp = 1 - 2 * (q.m_fX * q.m_fX + q.m_fY * q.m_fY);
    m_fRx = std::atan2(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    double sinp = 2 * (q.m_fQ0 * q.m_fY - q.m_fZ * q.m_fX);
    if (std::abs(sinp) >= 1) {
        m_fRy = std::copysign(M_PI / 2, sinp); // use 90 degrees if out of range
	}
    else
        m_fRy = std::asin(sinp);

    // yaw (z-axis rotation)
    double siny_cosp = 2 * (q.m_fQ0 * q.m_fZ + q.m_fX * q.m_fY);
    double cosy_cosp = 1 - 2 * (q.m_fY * q.m_fY + q.m_fZ * q.m_fZ);
    m_fRz = std::atan2(siny_cosp, cosy_cosp);
}


Quaternion &Quaternion::operator=(Quaternion const &src)
{
  m_fQ0 = src.m_fQ0;
  m_fX = src.m_fX;
  m_fY = src.m_fY;
  m_fZ = src.m_fZ;

  return *this;
}

Quaternion::Quaternion(float fTheta, Point3d const &axis) {
  Point3d normalizedAxis(axis);
  normalizedAxis.Normalize();

  float fTheta2 = fTheta / 2.F;
  float fSinTheta2 = sin(fTheta2);
  m_fQ0 = cos(fTheta2);
  m_fX = normalizedAxis.x() * fSinTheta2;
  m_fY = normalizedAxis.y() * fSinTheta2;
  m_fZ = normalizedAxis.z() * fSinTheta2;
  Normalize();
}

Quaternion::Quaternion(Euler const &er)
{
    double cy = cos(er.m_fRz * 0.5);
    double sy = sin(er.m_fRz * 0.5);
    double cp = cos(er.m_fRy * 0.5);
    double sp = sin(er.m_fRy * 0.5);
    double cr = cos(er.m_fRx * 0.5);
    double sr = sin(er.m_fRx * 0.5);

    // Apply the ZYX conversion formula
    m_fQ0 = cr * cp * cy + sr * sp * sy;
    m_fX = sr * cp * cy - cr * sp * sy;
    m_fY = cr * sp * cy + sr * cp * sy;
    m_fZ = cr * cp * sy - sr * sp * cy;

	Normalize();
}

// Quaternion from Rodrigues
Quaternion::Quaternion(Point3d const &rvec) {
	double dX = rvec.x();
	double dY = rvec.y();
	double dZ = rvec.z();

	// fMag = theta
	double fMag = sqrt( dX * dX + dY * dY + dZ * dZ );

	if (fMag == 0.) {
		m_fQ0 = 1.0F;
		m_fX = 0.F;
		m_fY = 0.F;
		m_fZ = 0.F;
	}
	else {
		double fTheta2 = fMag / 2.f;
		double fSinTheta2 = sin(fTheta2);
		double fFac = fSinTheta2 / fMag;

		m_fQ0 = cos(fTheta2);
		m_fX = dX * fFac;
		m_fY = dY * fFac;
		m_fZ = dZ * fFac;
	}

	Normalize();
}

Quaternion::Quaternion(uint8_t const *pu8, size_t stLen)
{
	UnPack(pu8, stLen);
}

size_t Quaternion::UnPack(uint8_t const *pu8, size_t stLen)
{
	size_t stIdx = 0;

	stIdx += ::UnPack(m_fQ0, pu8, stLen);
	stIdx += ::UnPack(m_fX, pu8 + stIdx, stLen - stIdx);
	stIdx += ::UnPack(m_fY, pu8 + stIdx, stLen - stIdx);
	stIdx += ::UnPack(m_fZ, pu8 + stIdx, stLen - stIdx);

	return stIdx;
}

Quaternion &Quaternion::operator=(RotationMatrix const &rm) {
  float fT = rm.m_grfMat[0][0] + rm.m_grfMat[1][1] + rm.m_grfMat[2][2] + 1;
  if (fT > 0.00000001F) {
    float fS = sqrt(fT) * 2.F;
    m_fQ0 = 0.25F * fS;
    m_fX = (rm.m_grfMat[2][1] - rm.m_grfMat[1][2]) / fS;
    m_fY = (rm.m_grfMat[0][2] - rm.m_grfMat[2][0]) / fS;
    m_fZ = (rm.m_grfMat[1][0] - rm.m_grfMat[0][1]) / fS;
  } else if (rm.m_grfMat[0][0] > rm.m_grfMat[1][1] &&
             rm.m_grfMat[0][0] > rm.m_grfMat[2][2]) {
    float fS =
        sqrt(1.0F + rm.m_grfMat[0][0] - rm.m_grfMat[1][1] - rm.m_grfMat[2][2]) *
        2.F;
    m_fX = 0.25F * fS;
    m_fY = (rm.m_grfMat[1][0] + rm.m_grfMat[0][1]) / fS;
    m_fZ = (rm.m_grfMat[0][2] + rm.m_grfMat[2][0]) / fS;
    m_fQ0 = (rm.m_grfMat[2][1] - rm.m_grfMat[1][2]) / fS;
  } else if (rm.m_grfMat[1][1] > rm.m_grfMat[2][2]) {
    float fS =
        sqrt(1.0F + rm.m_grfMat[1][1] - rm.m_grfMat[0][0] - rm.m_grfMat[2][2]) *
        2.F;
    m_fX = (rm.m_grfMat[1][0] + rm.m_grfMat[0][1]) / fS;
    m_fY = 0.25F * fS;
    m_fZ = (rm.m_grfMat[2][1] + rm.m_grfMat[1][2]) / fS;
    m_fQ0 = (rm.m_grfMat[0][2] - rm.m_grfMat[2][0]) / fS;
  } else {
    float fS =
        sqrt(1.0F + rm.m_grfMat[2][2] - rm.m_grfMat[0][0] - rm.m_grfMat[1][1]) *
        2.F;
    m_fX = (rm.m_grfMat[0][2] + rm.m_grfMat[2][0]) / fS;
    m_fY = (rm.m_grfMat[2][1] + rm.m_grfMat[1][2]) / fS;
    m_fZ = 0.25F * fS;
    m_fQ0 = (rm.m_grfMat[1][0] - rm.m_grfMat[0][1]) / fS;
  }
  return *this;
}

Quaternion Quaternion::operator*(Quaternion const &other) const
{
  float fQ0 = m_fQ0 * other.m_fQ0 - m_fX * other.m_fX - m_fY * other.m_fY -
              m_fZ * other.m_fZ;
  float fX = m_fQ0 * other.m_fX + m_fX * other.m_fQ0 + m_fY * other.m_fZ -
             m_fZ * other.m_fY;
  float fY = m_fQ0 * other.m_fY + m_fY * other.m_fQ0 + m_fZ * other.m_fX -
             m_fX * other.m_fZ;
  float fZ = m_fQ0 * other.m_fZ + m_fZ * other.m_fQ0 + m_fX * other.m_fY -
             m_fY * other.m_fX;
  Quaternion res(fQ0, fX, fY, fZ);
  res.Normalize();
  return res;
}

Quaternion &Quaternion::operator*=(Quaternion const &other) {
  float fQ0 = m_fQ0 * other.m_fQ0 - m_fX * other.m_fX - m_fY * other.m_fY -
              m_fZ * other.m_fZ;
  float fX = m_fQ0 * other.m_fX + m_fX * other.m_fQ0 + m_fY * other.m_fZ -
             m_fZ * other.m_fY;
  float fY = m_fQ0 * other.m_fY + m_fY * other.m_fQ0 + m_fZ * other.m_fX -
             m_fX * other.m_fZ;
  float fZ = m_fQ0 * other.m_fZ + m_fZ * other.m_fQ0 + m_fX * other.m_fY -
             m_fY * other.m_fX;
  m_fQ0 = fQ0;
  m_fX  = fX;
  m_fY  = fY;
  m_fZ  = fZ;
  Normalize();
  return *this;
}

void Quaternion::Normalize() {
  float fMag = sqrt(m_fQ0 * m_fQ0 + m_fX * m_fX + m_fY * m_fY + m_fZ * m_fZ);
  if (fMag == 0.F) {
    m_fQ0 = 1.0F;
    m_fX = 0.F;
    m_fY = 0.F;
    m_fZ = 0.F;
  } else {
    m_fQ0 /= fMag;
    m_fX /= fMag;
    m_fY /= fMag;
    m_fZ /= fMag;
  }
}

Point3d Quaternion::RotatePoint(Point3d const &pt) const
{
  float UxV_x = m_fY * pt.z() - m_fZ * pt.y();
  float UxV_y = m_fZ * pt.x() - m_fX * pt.z();
  float UxV_z = m_fX * pt.y() - m_fY * pt.x();

  Point3d res;
  res.m_fX = pt.x() + 2.F * (m_fQ0 * UxV_x + m_fY * UxV_z - m_fZ * UxV_y);
  res.m_fY = pt.y() + 2.F * (m_fQ0 * UxV_y + m_fZ * UxV_x - m_fX * UxV_z);
  res.m_fZ = pt.z() + 2.F * (m_fQ0 * UxV_z + m_fX * UxV_y - m_fY * UxV_x);

  return res;
}

void Quaternion::Invert()
{
	m_fQ0 = -m_fQ0;
}

Quaternion Quaternion::Inverse() const
{
	return Quaternion( m_fQ0, -m_fX, -m_fY, -m_fZ );
}

Point2d::Point2d() :
	m_fU(0.F),
	m_fV(0.F)
{
}

Point2d::Point2d(float fU, float fV) :
	m_fU(fU),
	m_fV(fV)
{
}

Point2d::Point2d(Point2d const &src) :
	m_fU(src.m_fU),
	m_fV(src.m_fV)
{
}

Point2d::Point2d(uint8_t const *pu8, size_t stLen) 
{
	UnPack(pu8, stLen);
}

size_t Point2d::UnPack(uint8_t const *pu8, size_t stLen)
{
	size_t stIdx = 0;
	stIdx += ::UnPack( m_fU, pu8, stLen );
	stIdx += ::UnPack( m_fV, pu8 + stIdx, stLen - stIdx );
	return stIdx;
}

Point2d &Point2d::operator=(Point2d const &src) {
  m_fU = src.m_fU;
  m_fV = src.m_fV;
  return *this;
}

QXfrm::QXfrm(uint8_t const *pu8, size_t stLen)
{
	UnPack(pu8, stLen);
}

size_t QXfrm::UnPack(uint8_t const *pu8, size_t stLen)
{
	size_t stIdx = m_q.UnPack(pu8, stLen);
	stIdx = m_p.UnPack(pu8 + stIdx, stLen - stIdx);

	return stIdx;
}

QXfrm QXfrm::operator*( QXfrm const &rhs ) const
{
	Quaternion q( m_q );
	q *= rhs.m_q;
	Point3d p = m_q.RotatePoint( rhs.m_p );
	p += m_p;
	return QXfrm( q, p );
}

QXfrm &QXfrm::operator*=( QXfrm const &rhs )
{
	Quaternion q = m_q * rhs.m_q;
	Point3d p = m_q.RotatePoint( rhs.m_p );
	p += rhs.m_p;
	m_q = q;
	m_p = p;

	return *this;
}

Point3d QXfrm::TransformPoint( Point3d const &p ) const
{
	Point3d ret = m_q.RotatePoint( p );
	ret += m_p;

	return ret;
}

void QXfrm::vSetTranslation( Point3d const &p )
{
	m_p = p;
}

void QXfrm::vSetRotation( Quaternion const &q )
{
	m_q = q;
}

QXfrm::operator Point3d const &() const
{
	return m_p;
}

Point3d const &QXfrm::Translation() const
{
	return m_p;
}

QXfrm::operator Quaternion const &() const
{
	return m_q;
}

Quaternion const &QXfrm::Rotation() const
{
	return m_q;
}

void QXfrm::Invert()
{
	m_q = m_q.Inverse();
	Point3d p = m_q.RotatePoint( m_p );
	m_p = p * -1;
}

QXfrm QXfrm::Inverse() const
{
	Quaternion q = m_q.Inverse();
	Point3d p = q.RotatePoint( m_p );

	return QXfrm( q, p * -1 );
}

#if 0
Rebels::QXfrm operator*( Rebels::QXfrm const &lhs, Rebels::QXfrm const &rhs )
{
	Quaternion q = lhs.m_q * rhs.m_q;
	Point3d p = lhs.m_q * rhs.m_p;
	p += lhs.m_p;

	QXfrm res( q, p );
	return res;
}
#endif

#ifdef TESTING
#define RADIANS (180 / PI)
void PrintEuler( Euler const &e )
{
	printf( "e: %6.4f %6.4f %6.4f\n", e.fX(), e.fY(), e.fZ() );
}

void PrintQuat( Quaternion const &q )
{
	printf( "q: %6.4f %6.4f %6.4f %6.4f\n", q.q0(), q.x(), q.y(), q.z() );
}

int main( int argc, char **argv )
{
	Euler eu( 0., 0., 90. / RADIANS );
	Quaternion q( eu );

	for( float r = 0; r < 180; r += 1.0F ) {
		for( float p = 0; p < 180; p += 1.0F ) {
			for( float y = 0; y < 180; y += 1.0F ) {
				Euler e( y / RADIANS, p / RADIANS, r / RADIANS );
				Quaternion q( e );
				Euler e1( q );
#if 1
				Quaternion q1( e1 );
				if( fabs(q1.q0() - q.q0()) > 0.0001 ||
					fabs(q1.x() - q.x()) > 0.0001 ||
					fabs(q1.y() - q.y()) > 0.0001 ||
					fabs(q1.z() - q.z()) > 0.0001 )
				{
					PrintQuat( q );
					PrintQuat( q1 );
				}
#else
				if (fabs(e.fX() - e1.fX()) > 0.0001 ||
					fabs(e.fY() - e1.fY()) > 0.0001 ||
					fabs(e.fZ() - e1.fZ()) > 0.0001) {
					PrintEuler( e );
					PrintEuler( e1 );
				}
#endif
			}
		}
	}
	PrintEuler( eu );
	printf( "q: %6.4f %6.4f %6.4f %6.4f\n", q.q0(), q.x(), q.y(), q.z() );

	Euler e1( q );

	PrintEuler( e1 );
	return 0;
}
#endif

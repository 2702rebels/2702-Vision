#pragma once
// add 2702 header
//
//
#include <math.h>

namespace Rebels {
#ifndef PI
#define PI 3.1415926535897932384626433832795F
#endif

class Point2d {
  /*! \brief Horizontal component of point. */
  float m_fU;
  /*! \brief Vertical component of point. */
  float m_fV;

 public:
  Point2d();
  Point2d(float fU, float fV);
  Point2d(Point2d const &src);

  Point2d &operator=(Point2d const &src);

  /*! \brief Assign new values to point coordinates. 
  * \param fU - new horizontal component of point.
  * \param fV - new vertical component of point.
  */
  inline void set(float fU, float fV) {
    m_fU = fU;
    m_fV = fV;
  }
  /*! \brief Returns horizontal component of point. */
  inline float u() const { return m_fU; }
  /*! \brief Returns vertical component of point. */
  inline float v() const { return m_fV; };
  double length();
};

class Point3d;
class Quaternion;
class Euler;

class Point3d {
  friend class Quaternion;
  friend class RotationMatrix;
  /*! \brief X component of point. */
  float m_fX;
  /*! \brief Y component of point. */
  float m_fY;
  /*! \brief Z component of point. */
  float m_fZ;

 public:
  Point3d() : m_fX(0.F), m_fY(0.F), m_fZ(0.F) {}
  Point3d(float fX, float fY, float fZ) : m_fX(fX), m_fY(fY), m_fZ(fZ) {}
  Point3d(Point3d const &src);

  Point3d &operator=(Point3d const &other);

  Point3d operator+(Point3d const &other) const;
  Point3d operator-(Point3d const &other) const;
  Point3d operator*(float const scalar) const;
  Point3d operator/(float const scalar) const;
  Point3d &operator*=(float const scalar);
  Point3d &operator/=(float const scalar);
  Point3d &operator+=(Point3d const &other);
  Point3d &operator-=(Point3d const &other);

  Point3d CrossProduct(Point3d const &v2) const;
  float DotProduct(Point3d const &v2) const;
  void Normalize();
  float length() const;

  /*! \brief Returns x component of point. */
  inline float x() const { return m_fX; }
  /*! \brief Returns y component of point. */
  inline float y() const { return m_fY; }
  /*! \brief Returns z component of point. */
  inline float z() const { return m_fZ; }
};

class RotationMatrix {
  friend class Euler;
  friend class Quaternion;
  float m_grfMat[3][3];

 public:
  RotationMatrix() {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        m_grfMat[i][j] = (i == j) ? 1.0F : 0.0F;
      }
    }
  }
  RotationMatrix(RotationMatrix const &src) { *this = src; }
#if 0
	RotationMatrix( Euler const &src )
	{
		*this = src;
	}
#endif
  RotationMatrix(Quaternion const &src) { *this = src; }
  ~RotationMatrix() {}
  RotationMatrix &operator=(RotationMatrix const &src);
  //	RotationMatrix &operator=( Euler const &src );
  RotationMatrix &operator=(Quaternion const &src);
  RotationMatrix operator*(RotationMatrix const &right);

  Point3d operator*(Point3d const &pt) const;
};
#if 0
class Euler
{
	friend class RotationMatrix;
	
	float	m_fRx;
	float	m_fRy;
	float	m_fRz;
public:
	Euler() :
		m_fRx( 0.F ),
		m_fRy( 0.F ),
		m_fRz( 0.F )
	{
	}
	Euler( float fRx, float fRy, float fRz ) :
		m_fRx( fRx ),
		m_fRx( fRy ),
		m_fRx( fRz )
	{
	}
	~Euler()
	{
	}
};
#endif

class Quaternion {
  friend class RotationMatrix;
  /*! \brief Q component of rotation. */
  float m_fQ0;
  /*! \brief X component of rotation. */
  float m_fX;
  /*! \brief Y component of rotation. */
  float m_fY;
  /*! \brief Z component of rotation. */
  float m_fZ;

 public:
  Quaternion() : m_fQ0(1.0F), m_fX(0.F), m_fY(0.F), m_fZ(0.F) {}
  Quaternion(float fQ0, float fX, float fY, float fZ)
      : m_fQ0(fQ0), m_fX(fX), m_fY(fY), m_fZ(fZ) {}
  Quaternion(Quaternion const &src) { *this = src; }
  Quaternion(RotationMatrix const &rm);
  ~Quaternion() {}
  Quaternion(float fTheta, Point3d const &axis);
  void Normalize();
  Quaternion &operator=(RotationMatrix const &rm);
  Quaternion &operator=(Quaternion const &src);

  Quaternion operator*(const Quaternion &other);
  Quaternion operator*=(const Quaternion &other);

  Point3d operator*(const Point3d &other) const;

  /*! \brief Returns q component of rotation. */
  inline float q0() const { return m_fQ0; }
  /*! \brief Returns x component of rotation. */
  inline float x() const { return m_fX; }
  /*! \brief Retursn y component of rotation. */
  inline float y() const { return m_fY; }
  /*! \brief Returns z component of rotation. */
  inline float z() const { return m_fZ; }
};

class LinearSlope {
  double m_slope;
  double m_yIntercept;

  public:
    LinearSlope();
    LinearSlope(double slope, double yIntercept);
    LinearSlope(double x1, double y1, double x2, double y2);
    double getSlope();
    double getYIntercept();
    void setSlope(double slope);
    void setYIntercept(double yIntercept);
    bool IsPointAboveLine(double x, double y);
    bool IsPointBelowLine(double x, double y);
    double CalculateY(double x);
    double CalculateX(double y);
}; 

};// namespace Rebels

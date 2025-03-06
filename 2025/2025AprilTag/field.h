#pragma once

#include <vector>
#include "2702math.h"

class Xfrm
{
	int 				m_nId;
	Rebels::Quaternion	m_rot;
	Rebels::Point3d		m_p3d;
public:
	Xfrm();
	Xfrm( int nId, Rebels::Quaternion const &q, Rebels::Point3d const &p );
	Xfrm( Xfrm const &src );
	~Xfrm();

	Xfrm &operator=( Xfrm const &src );
	operator int() const;
	operator Rebels::Point3d const &() const;
	operator Rebels::Quaternion const &() const;
};

class Field
{
	std::vector<Xfrm> m_vxTags;
public:
	
	Field();
	Field( char const *pszJson );
	Field( Field const &src );
	virtual ~Field();

	Field &operator=( Field const &src );
	size_t nTags() const;
	Xfrm const &operator[]( int n ) const;
};

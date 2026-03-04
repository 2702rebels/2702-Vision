#pragma once

#include <vector>
#include "2702math.h"

class FieldTag
{
	int					m_nId;
	bool				m_bIgnore;
	Rebels::QXfrm		m_qfm;
	Rebels::Point3d		m_grCorners[4];	// 4 corners of the Tag in Field Coordinate System
public:
	FieldTag( int nId, Rebels::QXfrm const &q );
	FieldTag( FieldTag const &ft );
	~FieldTag();

	bool bIgnore() const;
	void vSetIgnore( bool bIgnore );

	Rebels::Point3d const *Corners() const;

	operator int() const;
	operator Rebels::QXfrm const &() const;
};

class Field
{
	std::vector<FieldTag>	 m_vxTags;
public:
	
	Field();
	Field( char const *pszJson );
	Field( Field const &src );
	virtual ~Field();

	Field &operator=( Field const &src );
	size_t nTags() const;
	FieldTag const &operator[]( int n ) const;
};

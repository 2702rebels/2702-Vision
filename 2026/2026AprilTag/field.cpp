#include "2702util.h"

#include "field.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"	

using namespace rapidjson;

#define FIELD_UNITS_INCHES 1
#if FIELD_UNITS_INCHES
static const float s_fTagSize = 6.5;
#else
static const float s_fTagSize = 6.5 * .0254;
#endif

#if 0
Rebels::Point3d FieldTag::m_grBaseCorners[4] = {
		Rebels::Point3d( s_fTagSize/2,  s_fTagSize/2, 0 ),
		Rebels::Point3d(-s_fTagSize/2,  s_fTagSize/2, 0 ),
		Rebels::Point3d(-s_fTagSize/2, -s_fTagSize/2, 0 ),
		Rebels::Point3d( s_fTagSize/2, -s_fTagSize/2, 0 )
	};
#else
Rebels::Point3d FieldTag::m_grBaseCorners[4] = {
		Rebels::Point3d( 0., -s_fTagSize/2, -s_fTagSize/2 ),
		Rebels::Point3d( 0.,  s_fTagSize/2, -s_fTagSize/2 ),
		Rebels::Point3d( 0.,  s_fTagSize/2,  s_fTagSize/2 ),
		Rebels::Point3d( 0., -s_fTagSize/2,  s_fTagSize/2 )
	};
#endif

FieldTag::FieldTag( int nId, Rebels::QXfrm const &q ) :
	m_nId( nId ),
	m_bIgnore( false ),
	m_qfm( q )
{
	// compute corners
	for( int i = 0; i < 4; i++ ) {
		m_grCorners[i] = m_qfm.TransformPoint( m_grBaseCorners[i] );
	}
	
}

FieldTag::FieldTag( FieldTag const &src ) :
	m_nId( src.m_nId ),
	m_bIgnore( src.m_bIgnore ),
	m_qfm( src.m_qfm )
{
	for( int i = 0; i < 4; i++ ) {
		m_grCorners[i] = src.m_grCorners[i];
	}
}

FieldTag::~FieldTag()
{
}

bool FieldTag::bIgnore() const 
{
	return m_bIgnore;
}

void FieldTag::vSetIgnore( bool bIgnore )
{
	m_bIgnore = bIgnore;
}

FieldTag::operator Rebels::QXfrm const &() const
{
	return m_qfm;
}

Rebels::Point3d const *FieldTag::Corners() const
{
	return m_grCorners;
}

FieldTag::operator int() const
{
	return m_nId;
}

Rebels::Point3d const *FieldTag::RawCorners()
{
	return m_grBaseCorners;
}

Rebels::Point3d const &FieldTag::RawCorner(int i)
{
	return m_grBaseCorners[i];
}

Field::Field()
{
}

Field::Field( char const *pszJson )
{
	FILE *fp = fopen( pszJson, "r" );
	if (!fp) {
		throw std::runtime_error( strPrintf( "Field: Unable to open %s", pszJson ) );
	}

	char grcBuff[32768];
	rapidjson::FileReadStream is(fp, grcBuff, sizeof(grcBuff));

	rapidjson::Document rjd;
	rjd.ParseStream(is);

	const Value& tags = rjd["tags"];

	for (SizeType i = 0; i < tags.Size(); i++) {
		const Value& pose = tags[i]["pose"];
		const Value& trans = pose["translation"];
		const Value& quat = pose["rotation"]["quaternion"];

		int n = tags[i]["ID"].GetInt();
#if FIELD_UNITS_INCHES
		double fX = trans["x"].GetDouble() * 1000. / 25.4;
		double fY = trans["y"].GetDouble() * 1000. / 25.4;
		double fZ = trans["z"].GetDouble() * 1000. / 25.4;
#else
		double fX = trans["x"].GetDouble();
		double fY = trans["y"].GetDouble();
		double fZ = trans["z"].GetDouble();
#endif
		double q0 = quat["W"].GetDouble();
		double qx = quat["X"].GetDouble();
		double qy = quat["Y"].GetDouble();
		double qz = quat["Z"].GetDouble();

		FieldTag ft( n, Rebels::QXfrm( Rebels::Quaternion( q0, qx, qy, qz ), Rebels::Point3d( fX, fY, fZ ) ) );
		m_vxTags.push_back( ft );
	}
	fclose( fp );

}

Field::Field( Field const &src )
{
	m_vxTags = src.m_vxTags;
}

Field::~Field()
{
}

Field &Field::operator=( Field const &src )
{
	m_vxTags = src.m_vxTags;
	return *this;
}

size_t Field::nTags() const 
{
	return m_vxTags.size();
}

FieldTag const &Field::operator[]( int n ) const
{
	return m_vxTags[n];
}


#ifdef TESTING
int main( int argc, char **argv)
{
	Field f( argv[1] );
}
#endif

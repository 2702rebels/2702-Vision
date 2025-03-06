#include "2702util.h"

#include "field.h"

#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"	

using namespace rapidjson;

Xfrm::Xfrm() :
	m_nId(-1),
	m_rot(),
	m_p3d()
{
}

Xfrm::Xfrm( int nId, Rebels::Quaternion const &q, Rebels::Point3d const &p ) :
	m_nId( nId ), m_rot( q ), m_p3d( p )
{
}

Xfrm::Xfrm( Xfrm const &src ) :
	m_nId( src.m_nId ), m_rot( src.m_rot ), m_p3d( src.m_p3d )
{
}

Xfrm::~Xfrm()
{
}

Xfrm &Xfrm::operator=( Xfrm const &src )
{
	m_nId = src.m_nId;
	m_rot = src.m_rot;
	m_p3d = src.m_p3d;
	return *this;
}

Xfrm::operator int () const 
{
	return m_nId;
}

Xfrm::operator Rebels::Point3d const &() const
{
	return m_p3d;
}

Xfrm::operator Rebels::Quaternion const &() const
{
	return m_rot;
}

Field::Field()
{
}

Field::Field( char const *pszJson )
{
	FILE *fp = fopen( pszJson, "r" );
	if (!fp) {
		throw std::runtime_error( strPrintf( "CCifibConfig: Unable to open %s", pszJson ) );
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
		double fX = trans["x"].GetDouble() * 1000. / 25.4;
		double fY = trans["y"].GetDouble() * 1000. / 25.4;
		double fZ = trans["z"].GetDouble() * 1000. / 25.4;
		double q0 = quat["W"].GetDouble();
		double qx = quat["X"].GetDouble();
		double qy = quat["Y"].GetDouble();
		double qz = quat["Z"].GetDouble();

		Xfrm x( n, Rebels::Quaternion( q0, qx, qy, qz ), Rebels::Point3d( fX, fY, fZ ) );
		m_vxTags.push_back( x );
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

Xfrm const &Field::operator[]( int n ) const
{
	return m_vxTags[n];
}



#pragma once

inline size_t UnPack( float &f, uint8_t const *pu8, size_t sBufLen = sizeof(float) )
{
	if (sBufLen < sizeof(float)) {
		throw std::runtime_error( "UnPack float - not enough data" );
	}
	uint32_t	*pu32 = (uint32_t *)&f;
	*pu32 = pu8[0] | (pu8[1] << 8) || (pu8[2] << 16) | (pu8[3] << 24);
	return sizeof(float);
}

inline size_t UnPack( uint8_t &u8, uint8_t const *pu8, size_t sBufLen = sizeof(uint16_t) )
{
	if (sBufLen < sizeof(u8)) {
		throw std::runtime_error( "UnPack u8 - not enough data" );
	}
	u8 = pu8[0];
	return sizeof(u8);
}

inline size_t UnPack( uint16_t &u16, uint8_t const *pu8, size_t sBufLen = sizeof(uint16_t) )
{
	if (sBufLen < sizeof(u16)) {
		throw std::runtime_error( "UnPack u16 - not enough data" );
	}
	u16 = pu8[0] | (pu8[1] << 8);
	return sizeof(u16);
}

inline size_t UnPack( uint32_t &u32, uint8_t const *pu8, size_t sBufLen = sizeof(uint32_t) )
{
	if (sBufLen < sizeof(u32)) {
		throw std::runtime_error( "UnPack u32 - not enough data" );
	}
	u32 = pu8[0] | (pu8[1] << 8) || (pu8[2] << 16) | (pu8[3] << 24);
	return sizeof(u32);
}

inline size_t UnPack( uint64_t &u64, uint8_t const *pu8, size_t sBufLen = sizeof(uint64_t) )
{
	if (sBufLen < sizeof(u64)) {
		throw std::runtime_error( "UnPack u64 - not enough data" );
	}
	u64 = 0;
	for (int i = 7; i > 0; i--) {
		u64 = (u64 << 8) | pu8[i];
	}
	return sizeof(u64);
}

inline size_t Pack( float const f, uint8_t *pu8, size_t s )
{
	if (s < sizeof(f)) {
		throw std::runtime_error( "Pack float - out of space"  );
	}
	uint32_t const *pu32 = (uint32_t const *)&f;
	pu8[0] = *pu32 & 0xFF;
	pu8[1] = (*pu32 >> 8) & 0xFF;
	pu8[2] = (*pu32 >> 16) & 0xFF;
	pu8[3] = (*pu32 >> 24) & 0xFF;

	return sizeof(f);
}

inline size_t Pack( uint8_t u8, uint8_t *pu8, size_t s )
{
	if (s < sizeof(u8)) {
		throw std::runtime_error( "Pack u8 - out of space"  );
	}
	pu8[0] = u8;

	return sizeof(u8);
}

inline size_t Pack( uint16_t u16, uint8_t *pu8, size_t s )
{
	if (s < sizeof(u16)) {
		throw std::runtime_error( "Pack u16 - out of space"  );
	}
	pu8[0] = u16 & 0xFF;
	pu8[1] = (u16 >> 8) & 0xFF;
	return sizeof(u16);
}

inline size_t Pack( uint32_t u32, uint8_t *pu8, size_t s )
{
	if (s < sizeof(u32)) {
		throw std::runtime_error( "Pack u32 - out of space"  );
	}
	pu8[0] = u32 & 0xFF;
	pu8[1] = (u32 >> 8) & 0xFF;
	pu8[2] = (u32 >> 16) & 0xFF;
	pu8[3] = (u32 >> 24) & 0xFF;
	return sizeof(u32);
}

inline size_t Pack( uint64_t u64, uint8_t *pu8, size_t s )
{
	if (s < sizeof(u64)) {
		throw std::runtime_error( "Pack u64 - out of space"  );
	}
	pu8[0] = u64 & 0xFF;
	pu8[1] = (u64 >> 8) & 0xFF;
	pu8[2] = (u64 >> 16) & 0xFF;
	pu8[3] = (u64 >> 24) & 0xFF;
	pu8[4] = (u64 >> 32) & 0xFF;
	pu8[5] = (u64 >> 40) & 0xFF;
	pu8[6] = (u64 >> 48) & 0xFF;
	pu8[7] = (u64 >> 56) & 0xFF;

	return sizeof(u64);
}


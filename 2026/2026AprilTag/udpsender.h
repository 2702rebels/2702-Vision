#pragma once
#include <span>

class CUDPSender
{
	int	m_nSock = 0;

public:
	CUDPSender();
	~CUDPSender();

	bool bSend( std::span<uint8_t> const &spBuf );
};

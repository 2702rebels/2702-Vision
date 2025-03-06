#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <span>
#include <stdexcept>

#include "udpsender.h"

CUDPSender::CUDPSender()
{
	m_nSock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_nSock < 0) {
		throw std::runtime_error( "Unable to create Socket" );
	}
}

CUDPSender::~CUDPSender()
{
	if (m_nSock >= 0) {
		close( m_nSock );
	}
}

bool CUDPSender::bSend( std::span<uint8_t> const &spBuf )
{
	struct sockaddr_in saddr;

	memset( &saddr, 0, sizeof(saddr) );
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(27022);
	saddr.sin_addr.s_addr = htonl( 0x0A1B0202 );

	sendto(m_nSock, spBuf.data(), spBuf.size(), 0, (const struct sockaddr *)&saddr, sizeof(saddr));

	return true;
}


#pragma once
#include <span>
#include <stdexcept>
#include <asio.hpp>
 
class UDPSender
{
	asio::io_context io;
    asio::ip::udp::socket sock;
    asio::ip::udp::endpoint endpoint;

 
public:
	CUDPSender(const std::string& ipStr, const unsigned short port);

    bool bSend( std::span<uint8_t> const &spBuf );
};
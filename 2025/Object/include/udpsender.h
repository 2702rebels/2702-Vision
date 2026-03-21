#pragma once
#include <asio.hpp>
#include <span>
#include <stdexcept>

class UDPSender {
    asio::io_context io;
    asio::ip::udp::socket sock;
    asio::ip::udp::endpoint endpoint;

  public:
    UDPSender(const std::string& ipStr, const unsigned short port);

    bool bSend(const std::span<uint8_t>& spBuf);
};
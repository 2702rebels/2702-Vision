#include <udpsender.h>

UDPSender::UDPSender(const std::string& ipStr, const unsigned short port)
    : sock(io), endpoint(asio::ip::make_address(ipStr), port) {
    sock.open(asio::ip::udp::v4());
}

bool UDPSender::bSend(const std::span<uint8_t>& spBuf) {
    try {
        sock.send_to(asio::buffer(spBuf.data(), spBuf.size()), endpoint);
    } catch (std::exception& e) {
        return false;
    }
    return true;
}
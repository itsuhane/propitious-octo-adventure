#pragma once

// From Gaffer on Games, it teaches many useful knowledge on
// game physics and networking:
// http://gafferongames.com/networking-for-game-programmers/

#include <sstream>

#define PLATFORM_WINDOWS 1
#define PLATFORM_MAC     2
#define PLATFORM_LINUX   3

#if defined(_WIN32)
#   define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#   define PLATFORM PLATFORM_MAC
#else
#   define PLATFORM PLATFORM_LINUX
#endif

#if PLATFORM == PLATFORM_WINDOWS
#   define _WINSOCK_DEPRECATED_NO_WARNINGS
#   include <winsock2.h>
#   include <ws2tcpip.h>
#   pragma comment(lib, "Ws2_32.lib")
#else
#   include <cerrno>
#   include <arpa/inet.h>
#   include <sys/socket.h>
#   include <sys/types.h>
#   include <netinet/in.h>
#   include <fcntl.h>
#   include <netdb.h>
#   include <unistd.h>
#   define SOCKET int
#   define SOCKET_ERROR   -1
#   define INVALID_SOCKET -1
#endif

namespace udp {

    typedef unsigned long ipaddr;

    class address {
        friend class socket;
    public:

        // 0.0.0.0:0
        address() : address(0, 0) {}

        // 0.0.0.0:port
        address(unsigned short port) : address(0, port) {}

        // a.b.c.d:port
        address(unsigned char a, unsigned char b, unsigned char c, unsigned char d, unsigned short port) : address((a << 24) | (b << 16) | (c << 8) | d, port) {}

        // a.b.c.d:port
        address(const std::string &ip, unsigned short port) : address(::htonl(::inet_addr(ip.c_str())), port) {}

        // use together with resolv()
        address(ipaddr host, unsigned short port) {
            m_address.sin_family = AF_INET;
            m_address.sin_addr.s_addr = ::htonl(host);
            m_address.sin_port = ::htons(port);
        }

        // resolv("github.com") => address
        // resolv("domain.not.exist") => INADDR_NONE
        // use after socket::startup() and before socket::cleanup()
        // otherwise INADDR_NONE will be returned always
        static ipaddr resolv(const std::string &host) {
            addrinfo *result = nullptr, hints;
            ZeroMemory(&hints, sizeof(addrinfo));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_protocol = IPPROTO_UDP;
            int ret = ::getaddrinfo(host.c_str(), nullptr, &hints, &result);
            if (ret != 0) {
                return INADDR_NONE;
            }
            ipaddr add = ::ntohl(((sockaddr_in*)result->ai_addr)->sin_addr.s_addr);
            ::freeaddrinfo(result);
            return add;
        }

        std::string to_string() const {
            std::stringstream ss;
            ss << ::inet_ntoa(m_address.sin_addr) << ':' << ::ntohs(m_address.sin_port);
            return ss.str();
        }

    private:
        sockaddr_in m_address;
    };

    // use after socket::startup() and before socket::cleanup()
    class socket {
    public:
        static bool startup() {
#if PLATFORM == PLATFORM_WINDOWS
            WSADATA wsaData;
            int ret = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (ret != 0) {
                return false;
            }
#endif
            return true;
        }

        static void cleanup() {
#if PLATFORM == PLATFORM_WINDOWS
            ::WSACleanup();
#endif
        }

        socket() {
            m_socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (m_socket == INVALID_SOCKET) {
                throw "Cannot create non-blocking socket!";
            }
#if PLATFORM == PLATFORM_WINDOWS
            DWORD non_blocking = 1;
            int ret = ::ioctlsocket(m_socket, FIONBIO, &non_blocking);
            if (ret != 0) {
                throw "Cannot create non-blocking socket!";
            }
#else
            int non_blocking = 1;
            int ret = ::fcntl(m_socket, F_SETFL, O_NONBLOCK, non_blocking);
            if (ret == -1) {
                throw "Cannot create non-blocking socket!";
            }
#endif
        }

        ~socket() {
#if PLATFORM == PLATFORM_WINDOWS
            ::closesocket(m_socket);
#else
            ::close(m_socket);
#endif
        }

        socket(const socket&) = delete;

        bool bind(const address &host) {
            int ret = ::bind(m_socket, (const sockaddr*)&host.m_address, sizeof(sockaddr_in));
            if (ret != 0) {
                return false;
            }
            return true;
        }

        bool send(const address &dest, const void *data, size_t len) {
            int sendlen = ::sendto(m_socket, (char*)data, (int)len, 0, (const sockaddr*)&dest.m_address, sizeof(sockaddr_in));
            return sendlen == (int)len;
        }

        size_t recv(address &from, void *data, size_t maxlen) {
            socklen_t addlen = sizeof(sockaddr_in);
            int recvlen = ::recvfrom(m_socket, (char*)data, (int)maxlen, 0, (sockaddr*)&from.m_address, &addlen);
            if (recvlen == SOCKET_ERROR) {
#if PLATFORM == PLATFORM_WINDOWS
                if (::WSAGetLastError() == WSAEWOULDBLOCK) {
#else
                if(errno == EWOULDBLOCK || errno == EAGAIN) {
#endif
                    recvlen = 0;
                }
                else {
                    recvlen = 0;
                    throw "Error receiving data.";
                }
            }
            return (size_t)recvlen;
        }

        void add_multicast(const address &addr) {
            ip_mreq ipmr;
            ipmr.imr_multiaddr.s_addr = addr.m_address.sin_addr.s_addr;
            ipmr.imr_interface.s_addr = htonl(INADDR_ANY);
            int ret = ::setsockopt(m_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&ipmr, sizeof(ip_mreq));
            if (ret == SOCKET_ERROR) {
                throw "Add membership failed.";
            }
        }

        void drop_multicast(const address &addr) {
            ip_mreq ipmr;
            ipmr.imr_multiaddr.s_addr = addr.m_address.sin_addr.s_addr;
            ipmr.imr_interface.s_addr = htonl(INADDR_ANY);
            int ret = ::setsockopt(m_socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&ipmr, sizeof(ip_mreq));
            if (ret == SOCKET_ERROR) {
                throw "Drop membership failed.";
            }
        }

        SOCKET raw_socket() const {
            return m_socket;
        }

    private:
        SOCKET m_socket = INVALID_SOCKET;
    };
}

/*
    // Server example
    #include <array>
    #include <string>
    #include <iostream>
    #include <UDPSocket.h>

    int main() {
        udp::socket::startup();
        udp::socket socket;
        bool ret = socket.bind(udp::address(5911));
        while (ret) {
            udp::address address;
            std::array<char, 256> buffer;
            size_t recv_len = socket.recv(address, buffer.data(), buffer.size());
            if (recv_len) {
                std::string str(buffer.begin(), buffer.begin() + recv_len);
                if (str == "<SHUTDOWN>") {
                    break;
                }
                std::cout << "from " << address.to_string() << " => " << str << std::endl;
            }
        }
        udp::socket::cleanup();
        return 0;
    }
*/

/*
    // Client example
    #include <string>
    #include <UDPSocket.h>

    int main() {
        std::string msg = "Hello, world!";

        udp::socket::startup();
        udp::socket socket;
        udp::address dest(127, 0, 0, 1, 5911);
        socket.send(dest, msg.data(), msg.size());
        socket.send(dest, "<SHUTDOWN>", 10);
        udp::socket::cleanup();
        return 0;
    }
*/

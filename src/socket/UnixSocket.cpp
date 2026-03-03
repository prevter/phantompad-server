#include "UnixSocket.hpp"

#include <unistd.h>
#include <arpa/inet.h>
#include <fmt/ostream.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

#include <Geode/Result.hpp>

namespace phant {
    UnixSocket::UnixSocket(uint16_t port) : m_port(port) {}

    UnixSocket::~UnixSocket() { this->close(); }

    geode::Result<> UnixSocket::open() {
        m_socketFd = ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
        if (m_socketFd < 0) return geode::Err(fmt::format("Failed to create socket: {}", ::strerror(errno)));

        int opt = 1;
        if (::setsockopt(m_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            ::close(m_socketFd);
            return geode::Err(fmt::format("Failed to set SO_REUSEADDR: {}", ::strerror(errno)));
        }

        int rcvBufSize = 1024 * 1024; // 1 MB
        if (::setsockopt(m_socketFd, SOL_SOCKET, SO_RCVBUF, &rcvBufSize, sizeof(rcvBufSize)) < 0) {
            ::close(m_socketFd);
            return geode::Err(fmt::format("Failed to set SO_RCVBUF: {}", ::strerror(errno)));
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(m_port);
        if (bind(m_socketFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            ::close(m_socketFd);
            return geode::Err(fmt::format("Failed to bind socket: {}", ::strerror(errno)));
        }

        m_epollFd = ::epoll_create1(EPOLL_CLOEXEC);
        if (m_epollFd < 0) {
            ::close(m_socketFd);
            return geode::Err(fmt::format("Failed to create epoll instance: {}", ::strerror(errno)));
        }

        epoll_event event{
            .events = EPOLLIN,
            .data = {.fd = m_socketFd}
        };

        if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_socketFd, &event) < 0) {
            ::close(m_socketFd);
            ::close(m_epollFd);
            return geode::Err(fmt::format("Failed to add socket to epoll: {}", ::strerror(errno)));
        }

        m_eventFd = ::eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        if (m_eventFd < 0) {
            ::close(m_socketFd);
            ::close(m_epollFd);
            return geode::Err(fmt::format("Failed to create eventfd: {}", ::strerror(errno)));
        }

        event.events = EPOLLIN;
        event.data.fd = m_eventFd;

        if (::epoll_ctl(m_epollFd, EPOLL_CTL_ADD, m_eventFd, &event) < 0) {
            ::close(m_socketFd);
            ::close(m_epollFd);
            ::close(m_eventFd);
            return geode::Err(fmt::format("Failed to add eventfd to epoll: {}", ::strerror(errno)));
        }

        return geode::Ok();
    }

    void UnixSocket::close() {
        if (m_eventFd != -1) ::close(std::exchange(m_eventFd, -1));
        if (m_epollFd != -1) ::close(std::exchange(m_epollFd, -1));
        if (m_socketFd != -1) ::close(std::exchange(m_socketFd, -1));
    }

    geode::Result<bool> UnixSocket::poll(int timeoutMs) const {
        epoll_event ev;
        int n = ::epoll_wait(m_epollFd, &ev, 1, timeoutMs);

        if (n < 0) {
            if (errno == EINTR) return geode::Ok(false);
            return geode::Err(fmt::format("epoll_wait failed: {}", ::strerror(errno)));
        }

        return geode::Ok(n > 0);
    }

    geode::Result<> UnixSocket::sendTo(ClientEndpoint const& to, std::span<uint8_t const> data) const {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = to.ip;
        addr.sin_port = to.port;

        ssize_t n = ::sendto(m_socketFd, data.data(), data.size(), 0, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        if (n < 0) return geode::Err(fmt::format("Failed to send packet to {}: {}", to.toString(), ::strerror(errno)));
        if (static_cast<size_t>(n) != data.size()) {
            return geode::Err(fmt::format("Partial packet sent to {}: {}/{}", to.toString(), n, data.size()));
        }

        return geode::Ok();
    }

    geode::Result<std::optional<ReceiveResult>> UnixSocket::receiveTo(std::span<uint8_t> buffer) const {
        sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        ssize_t n = ::recvfrom(
            m_socketFd,
            buffer.data(),
            buffer.size(),
            0,
            reinterpret_cast<sockaddr*>(&from),
            &fromLen
        );

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return geode::Ok(std::nullopt);
            return geode::Err(fmt::format("recvfrom failed: {}", ::strerror(errno)));
        }

        return geode::Ok(ReceiveResult{
            {from.sin_addr.s_addr, from.sin_port},
            static_cast<size_t>(n)
        });
    }

    void UnixSocket::wakeup() const {
        uint64_t val = 1;
        ::write(m_eventFd, &val, sizeof(val));
    }
}

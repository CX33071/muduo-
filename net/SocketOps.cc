#include "SocketOps.h"
#include "../base/logger.h"
#include <fcntl.h>
#include <assert.h>
using namespace muduo::net;
int socket::createNonblockingOrDie(){
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
    if(sockfd<0){
        LOG_FATAL << "SocketOps:Create sockket failed!";
        exit(EXIT_FAILURE);
    }
    return sockfd;
}
void socket::bindOrDie(int sockfd,const sockaddr_in&addr){
    int ret =
        ::bind(sockfd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
        if(ret<0){
            LOG_FATAL << "SocketOps:Bind failed";
        }
}
void socket::listenOrDie(int sockfd){
    int stat = ::listen(sockfd, SOMAXCONN);
    if(stat<0){
        LOG_FATAL << "SocketOps:Listen failed!";
    }
}
int socket::accept(int sockfd,sockaddr_in*addr){
    socklen_t len = sizeof(*addr);
    int ret = ::accept4(sockfd, reinterpret_cast<sockaddr*>(&addr), &len,
                        SOCK_NONBLOCK | SOCK_CLOEXEC);
                        if(ret<0){
                            LOG_FATAL << "SocketOps:Accept failed";
                        }
                        return ret;
}
int socket::accept1(int sockfd,sockaddr_in *addr){
    socklen_t len = sizeof(*addr);
    int ret = ::accept4(sockfd, reinterpret_cast<sockaddr*>(&addr), &len,
                        SOCK_CLOEXEC);
                        if(ret<0){
                            LOG_FATAL << "SocketOps:Accept failed";
                        }
                        return ret;
}
void socket::close(int sockfd){
    int stat = ::close(sockfd);
    if(stat<0){
        LOG_FATAL << "SocketOps:Close failed";
    }
}
int socket::connect(int sockfd,const sockaddr_in &addr){
    return ::connect(sockfd, reinterpret_cast<const sockaddr*>(&addr),
                     sizeof(addr));
}
void socket::setNonBlockAndCloseOnExec(int sockfd){
    int flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int ret = ::fcntl(sockfd, F_SETFL, flags);
    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    ret = ::fcntl(sockfd, F_SETFD, flags);
}
void socket::shutdownWrite(int sockfd){
    int ret = ::shutdown(sockfd, SHUT_WR);
    if(ret<0){
        LOG_ERROR << "SocketOps:shutdownWrite failed";
    }
}
sockaddr_in socket::getLocalAddr(int sockfd){
    sockaddr_in addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    socklen_t addr_len = sizeof(addr);
    if(::getsockname(sockfd,reinterpret_cast<sockaddr*>(&addr),&addr_len)<0){
        LOG_SYSERR << "SocketOps:getLocalAddr failed";
    }
    return addr;
}
sockaddr_in socket::getPeerAddr(int sockfd){
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sockaddr_in));
    socklen_t sin_len = sizeof(sin);
    if(::getpeername(sockfd,reinterpret_cast<sockaddr*>(&sin),&sin_len)<0){
        LOG_SYSERR << "SocketOps:getPeerAddr failed";
    }
    return sin;
}
int socket::getSocketError(int sockfd){
    int optval;
    socklen_t optlen = static_cast<socklen_t>(sizeof optval);
    if(::getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&optval,&optlen)<0){
        return errno;
    }else{
        return optval;
    }
}
bool socket::isSelfConnect(int sockfd) {
    sockaddr_in local = getLocalAddr(sockfd);
    sockaddr_in peer = getPeerAddr(sockfd);
    return (local.sin_port == peer.sin_port) &&
           (local.sin_addr.s_addr == peer.sin_addr.s_addr);
}
void socket::fromHostPort(const char*ip,uint16_t port,struct sockaddr_in *addr){
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if(::inet_pton(AF_INET,ip,&addr->sin_addr)<=0){
        LOG_SYSERR << "sockets:fromHostPort";
    }
}
void socket::toHostPort(char*buf,size_t size,const struct sockaddr_in &addr){
    char host[INET_ADDRSTRLEN] = "INVALID";
    ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof(host));
    uint16_t port = ntohs(addr.sin_port);
    snprintf(buf, size, "%s:%u", host, port);
}
void socket::toIpPort(char*buf,size_t size,const struct sockaddr*addr){
    toIp(buf, size, addr);
    size_t end = strlen(buf);
    const struct sockaddr_in* addr4 =
        reinterpret_cast<const struct sockaddr_in*>(addr);
    uint16_t port = ntohs(addr4->sin_port);
    assert(size > end);
    snprintf(buf + end, size - end, ":%u", port);
}
void socket::toIp(char*buf,size_t size,const struct sockaddr*addr){
    assert(size >= INET_ADDRSTRLEN);
    const struct sockaddr_in* addr4 =
        reinterpret_cast<const struct sockaddr_in*>(addr);
    ::inet_ntop(AF_INET, &addr4->sin_addr, buf,
                    static_cast<socklen_t>(size));
}
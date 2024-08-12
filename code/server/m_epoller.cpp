#include "m_epoller.h"

Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)),events_(maxEvent){
    //cepoll_create用于创建一个 epoll 实例
    assert(epollFd_>=0 && events_.size()>0);
}

Epoller::~Epoller(){
    close(epollFd_);
}

bool Epoller::AddFd(int fd,uint32_t events){
    if(fd<0 )return false;
    epoll_event ev={0};
    ev.data.fd=fd;
    ev.events=events;
    //epoll_ctl 是 Linux 操作系统中 epoll 接口的一个关键函数
    //用于对 epoll 实例进行控制操作
    //比如添加、修改或删除感兴趣的文件描述符
    return 0==epoll_ctl(epollFd_,EPOLL_CTL_ADD,fd,&ev);
}

bool Epoller::ModFd(int fd,uint32_t events){
    if(fd<0) return false;
    epoll_event ev={0};
    ev.data.fd=fd;
    ev.events=events;
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}
bool Epoller::DelFd(int fd) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &ev);
}

int Epoller::Wait(int timeoutMs) {
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

int Epoller::GetEventFd(size_t i)const{
    assert(i<events_.size()&& i>=0);
    return events_[i].data.fd;
}

uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}
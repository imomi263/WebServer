#ifndef M_EPOLLER_H
#define M_EPOLLER_H

#include<sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include<vector>
#include<errno.h>

class Epoller{
public:
    //防止隐式类型转换和复制初始化
    explicit Epoller(int maxEvent=1024);
    ~Epoller();

    bool AddFd(int fd,uint32_t events);

    bool ModFd(int fd,uint32_t events);

    bool DelFd(int fd);

    int Wait(int timeoutMs=-1);

    int GetEventFd(size_t i)const;

    uint32_t GetEvents(size_t i)const;

private:
    int epollFd_;
    std::vector<struct epoll_event>events_;

};
#endif //M_EPOLLER_H
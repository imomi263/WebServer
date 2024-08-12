#include "m_webserver.h"
using namespace std;

M_WebServer::M_WebServer(
    int port, int trigMode, int timeoutMS, bool OptLinger,
            int sqlPort, const char* sqlUser, const  char* sqlPwd,
            const char* dbName, int connPoolNum, int threadNum,
            bool openLog, int logLevel, int logQueSize):
            port_(port), openLinger_(OptLinger), timeoutMS_(timeoutMS), isClose_(false),
            timer_(new HeapTimer()), threadpool_(new ThreadPool(threadNum)), epoller_(new Epoller())
{
        // 获取当前工作目录
        srcDir_=getcwd(nullptr,256);
        assert(srcDir_);
        strncat(srcDir_,"/resources",16);
        HttpConn::userCount=0;
        HttpConn::srcDir=srcDir_;
        SqlConnPool::Instance()->Init("localhost", sqlPort, sqlUser, sqlPwd, dbName, connPoolNum);
        InitEventMode_(trigMode);
        if(!InitSocket_()){
            isClose_=true;
        }
        if(openLog){
            Log::Instance()->init(logLevel,"./log",".log",logQueSize);
            if(isClose_){
                LOG_ERROR("========== Server init error!==========");
            }else{
                LOG_INFO("========== Server init ==========");
                LOG_INFO("Port:%d, OpenLinger: %s", port_, OptLinger? "true":"false");
                LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                                (listenEvent_ & EPOLLET ? "ET": "LT"),
                                (connEvent_ & EPOLLET ? "ET": "LT"));
                LOG_INFO("LogSys level: %d", logLevel);
                LOG_INFO("srcDir: %s", HttpConn::srcDir);
                LOG_INFO("SqlConnPool num: %d, ThreadPool num: %d", connPoolNum, threadNum);
            }

        }
}

M_WebServer::~M_WebServer(){
    close(listenFd_);
    isClose_=false;
    free(srcDir_);
    SqlConnPool::Instance()->ClosePool();
}

void M_WebServer::InitEventMode_(int trigMode){
    // EPOLLRDHUP 被添加到监听的事件中，当对端关闭其写入端时，该事件会被触发。
    listenEvent_=EPOLLRDHUP;
    //EPOLLONESHOT 确保了即使多个事件连续发生
    //每个文件描述符也只会触发一次事件。
    connEvent_=EPOLLONESHOT | EPOLLRDHUP;
    switch(trigMode){
        case 0:
            break;
        case 1:
            //“边缘触发”,发生变化会触发
            connEvent_|=EPOLLET;
        case 2:
            listenEvent_|=EPOLLET;
        case 3:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
        default:
            listenEvent_ |= EPOLLET;
            connEvent_ |= EPOLLET;
            break;
    }   
    HttpConn::isET=(connEvent_ & EPOLLET);
}

void M_WebServer::start(){
    int timeMS=-1;
    if(!isClose_){
        LOG_INFO("========== Server start ==========");
    }
    while(!isClose_){
        if(timeoutMS_>0){
            timeMS=timer_->GetNextTick();

        }
        int eventCnt=epoller_->Wait(timeMS);
        for(int i=0;i<eventCnt;i++){
            int fd=epoller_->GetEventFd(i);
            uint32_t events=epoller_->GetEvents(i);
            if(fd==listenFd_){
                DealListen_();
            }
            // EPOLLHUP 当这个事件发生时，它表示对端已经关闭了连接
            // 并且本端也已经完成读入了
            else if(events & (EPOLLRDHUP | EPOLLHUP |  EPOLLERR)){
                assert(users_.count(fd)>0);
                CloseConn_(&users_[fd]);
            }else if(events & EPOLLIN){
                assert(users_.count(fd) > 0);
                DealRead_(&users_[fd]);
            }else if(events & EPOLLOUT) {
                assert(users_.count(fd) > 0);
                DealWrite_(&users_[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void M_WebServer::sendError_(int fd,const char* info){
    assert(fd>0);
    int ret=send(fd,info,strlen(info),0);
    if(ret < 0) {
        LOG_WARN("send error to client[%d] error!", fd);
    }
    close(fd);
}

void M_WebServer::CloseConn_(HttpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->GetFd());
    epoller_->DelFd(client->GetFd());
    client->Close();
}

void M_WebServer::AddClient_(int fd,sockaddr_in addr){
    assert(fd>0);
    users_[fd].init(fd,addr);
    if(timeoutMS_>0){
        timer_->add(fd,timeoutMS_,std::bind(&M_WebServer::CloseConn_,this,&users_[fd]));

    }
    epoller_->AddFd(fd,EPOLLIN|connEvent_);
    SetFdNonBlock(fd);
    LOG_INFO("Client[%d] in!", users_[fd].GetFd());
}

void M_WebServer::DealListen_(){
    struct sockaddr_in addr;
    socklen_t len=sizeof(addr);
    do{
        int fd=accept(listenFd_,(struct sockaddr*)&addr,&len);
        if(fd<=0){return;}
        else if(HttpConn::userCount>=MAX_FD){
            sendError_(fd,"Server busy!");
            LOG_WARN("Clients is full!");
            return;
        }
        AddClient_(fd,addr);
    }while(listenEvent_ & EPOLLET);
}

void M_WebServer::DealRead_(HttpConn* client){
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&M_WebServer::OnRead_,this,client));

}


void M_WebServer::DealWrite_(HttpConn* client) {
    assert(client);
    ExtentTime_(client);
    threadpool_->AddTask(std::bind(&M_WebServer::OnWrite_, this, client));
}

void M_WebServer::ExtentTime_(HttpConn* client) {
    assert(client);
    if(timeoutMS_ > 0) { timer_->adjust(client->GetFd(), timeoutMS_); }
}

void M_WebServer::OnRead_(HttpConn* client){
    assert(client);
    int ret=-1;
    int readErrno=0;
    ret=client->read(&readErrno);
    if(ret<=0 && readErrno !=EAGAIN){
        CloseConn_(client);
        return;
    }
    OnProcess(client);
}

void M_WebServer::OnProcess(HttpConn* client) {
    if(client->process()) {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
    } else {
        epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLIN);
    }
}

void M_WebServer::OnWrite_(HttpConn* client){
    assert(client);
    int ret=-1;
    int writeErrno=0;
    ret=client->write(&writeErrno);
    if(client->ToWriteBytes()==0){
        if(client->IsKeepAlive()){
            OnProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        //当一个操作（如读取或写入）不能立即完成时，返回 EAGAIN 表示“资源暂时不可用”
        //这通常发生在非阻塞模式的 I/O 操作中。
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller_->ModFd(client->GetFd(), connEvent_ | EPOLLOUT);
            return;
        }
    }
    CloseConn_(client);
}

bool M_WebServer::InitSocket_(){
    int ret;
    struct sockaddr_in addr;
    if(port_>65535 || port_<1024){
        LOG_ERROR("Port:%d error!",  port_);
        return false;
    }

    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port_);

    //当一个应用程序关闭其 TCP 连接时
    //它可以选择是否使用 linger 选项
    //控制套接字在关闭连接前的行为。
    struct linger optLinger={0};
    if(openLinger_){
        optLinger.l_onoff=1;
        //最长等待时间
        optLinger.l_linger = 1;
    }

    listenFd_=socket(AF_INET,SOCK_STREAM,0);

    if(listenFd_<0){
        LOG_ERROR("Create socket error!", port_);
        return false;
    }

    ret=setsockopt(listenFd_,SOL_SOCKET,SO_LINGER,&optLinger, sizeof(optLinger));
    if(ret < 0) {
        close(listenFd_);
        LOG_ERROR("Init linger error!", port_);
        return false;
    }

    int optval=1;
    ret=setsockopt(listenFd_,SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

    if(ret==-1){
        LOG_ERROR("set socket setsockopt error!");
        close(listenFd_);
        return false;
    }

    ret = bind(listenFd_, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port_);
        close(listenFd_);
        return false;
    }

    ret = listen(listenFd_, 6);
    if(ret < 0) {
        LOG_ERROR("Listen port:%d error!", port_);
        close(listenFd_);
        return false;
    }
    ret = epoller_->AddFd(listenFd_,  listenEvent_ | EPOLLIN);
    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd_);
        return false;
    }
    SetFdNonBlock(listenFd_);
    LOG_INFO("Server port:%d", port_);
    return true;
}

int M_WebServer::SetFdNonBlock(int fd){
    assert(fd>0);
    return fcntl(fd,F_SETFL,fcntl(fd,F_GETFD,0)| O_NONBLOCK);
}
#ifndef M_WEBSERVER
#define M_WEBSERVER
#include <unordered_map>
#include<fcntl.h>
#include<unistd.h>
#include<assert.h>
#include <errno.h>
#include <sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>

#include "m_epoller.h"
#include "../log/m_log.h"
#include "../timer/m_heaptimer.h"
#include "../pool/m_sqlconnpool.h"
#include "../pool/m_threadpool.h"
#include "../pool/m_sqlconnRAII.h"
#include "../http/m_httpconn.h"

class M_WebServer{
public:
    M_WebServer(
        int port,int trigMode,int timeoutMS,bool OptLinger,
        int sqlPort,const char* sqlUser,const char* sqlPwd,
        const char* dbName,int connPoolNum,int threadNum,
        bool openLog,int logLevel,int logQueSize);
    
    ~M_WebServer();
    void start();

private:
    bool InitSocket_();
    void InitEventMode_(int trigMode);
    void AddClient_(int fd,sockaddr_in addr);

    void DealListen_();
    void DealWrite_(HttpConn* client);
    void DealRead_(HttpConn* client);
    
    void sendError_(int fd,const char* info);
    void ExtentTime_(HttpConn* client);
    void CloseConn_(HttpConn* client);

    void OnRead_(HttpConn* client);
    void OnWrite_(HttpConn* client);
    void OnProcess(HttpConn* client);

    static const int MAX_FD =65536;
    static int SetFdNonBlock(int fd);
    int port_;
    bool openLinger_;
    int timeoutMS_;
    bool isClose_;
    int listenFd_;
    char *srcDir_;

    uint32_t listenEvent_;
    uint32_t connEvent_;

    std::unique_ptr<HeapTimer> timer_;
    std::unique_ptr<ThreadPool> threadpool_;
    std::unique_ptr<Epoller> epoller_;
    std::unordered_map<int,HttpConn>users_;

};


#endif //M_WEBSERVER
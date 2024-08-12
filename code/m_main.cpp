#include <unistd.h>
#include "server/m_webserver.h"


int main(){
    M_WebServer server(
        1316,3,60000,false,
        3306,"root","root","webserver",
        12,6,true,1,1024);
    
    server.start();
}
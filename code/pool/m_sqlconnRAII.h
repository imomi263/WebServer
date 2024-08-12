#ifndef M_SQLCONNRAII_H
#define M_SQLCONNRAII_H
#include "m_sqlconnpool.h"
class SqlConnRAII{
public:
    SqlConnRAII(MYSQL** sql,SqlConnPool *connpool){
        assert(connpool);
        *sql=connpool->GetConn();
        sql_=*sql;
        connpool_=connpool;
    }

    ~SqlConnRAII(){
        if(sql_){
            connpool_->FreeConn(sql_);
        }
    }
private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};
#endif
#if 0
#ifndef _TEST_REDIS_H_
#define _TEST_REDIS_H_
#include "redis/hiredis.h"
#include "redis/async.h"
#include "uvnet/tcpclient.h"

using namespace UVNet;
class test_redis
{
public:
    test_redis();
    ~test_redis();
    bool Connect(char* host, unsigned short port);
    static void CloseCB(int clientid, void* userdata);
    static void ReadCB(const unsigned char* buf,unsigned int lenght, void* userdata);
    static void AfterConnect(void* userdata);

    void RunRedis(char* host, unsigned short port);

    void  redisSetCallBack(redisAsyncContext* predisAsyncContext, redisReply* predisReply, void* pData);
    void  redisGetCallBack(redisAsyncContext* predisAsyncContext, redisReply* predisReply, void* pData);

    void HMSETTest();
    void HMGetTest();
private:    
    TCPClient*                pClient;
};
#endif // !_TEST_REDIS_H_
#endif

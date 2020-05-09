#ifndef _TEST_REDIS_H_
#define _TEST_REDIS_H_
#include "hiredis/hiredis.h"
#include "hiredis/async.h"
#include "hiredis/adapters/libuv.h"

class test_redis
{
public:
    test_redis();
    ~test_redis();
    bool Connect(char* host, unsigned short port);
    //void AfterConnect(const redisAsyncContext *c, int status);
    //void AfterDisConnect(const redisAsyncContext *c, int status);

    void RunRedis(char* host, unsigned short port);

    /*void  redisSetCallBack(redisAsyncContext* predisAsyncContext, redisReply* predisReply, void* pData);
    void  redisGetCallBack(redisAsyncContext* predisAsyncContext, redisReply* predisReply, void* pData);*/

    void HMSETTest();
    void HMGetTest();
private:    
    redisAsyncContext* pRedisAsyncContext;
    uv_loop_t*         uv_loop;
};
#endif // !_TEST_REDIS_H_

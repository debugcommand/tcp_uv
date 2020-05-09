#include "test_redis.h"
#include <vector>
#include <map>
#include <signal.h>
#include <functional>
#include "uvnet\thread_uv.h"

using namespace std;
#define RedisCallBack(realCallBack, holder) std::bind(realCallBack, holder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)

struct SRedisTest
{
    SRedisTest() {
    }
    void _init() {
        _m = 10;
        for (size_t i = 0; i < 10; i++)
        {
            _array[i] = i;
        }        
        sprintf(_str, "SRedisTest");
    }
    int _m;
    int _array[10];    
    char _str[32];

    //ps:no string ,no vector,map,list...
};

void AfterConnect(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
    test_redis* pTestRedis = (test_redis*)c->data;
    pTestRedis->HMSETTest();
}

void AfterDisConnect(const redisAsyncContext *c, int status)
{
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
    delete c->data;
}

void  redisGetCallBack(redisAsyncContext *c, void *r, void *privdata)
{
    printf("redis Get CallBack! \n");
    redisReply *reply = (redisReply *)r;
    if (!c || reply->elements <= 0) {
        printf("redis Get CallBack Error! \n");
        return;
    }
    SRedisTest _RT;
    fprintf(stdout, "Get SRedisTest:(%p)\n", &_RT);
    memcpy(&_RT, reply->element[0]->str, reply->element[0]->len);
}

void  redisSetCallBack(redisAsyncContext *c, void *r, void *privdata)
{
    test_redis* pTR = (test_redis*)c->data;
    printf("redis Set CallBack! \n");
    SRedisTest* pRT = (SRedisTest*)privdata;
    fprintf(stdout, "delete SRedisTest:(%p)\n", pRT);
    delete pRT;

    uv_thread_sleep(100);
    pTR->HMGetTest();
}


test_redis::test_redis()
{
    pRedisAsyncContext = nullptr;
    uv_loop = nullptr;
}

test_redis::~test_redis()
{
    if (pRedisAsyncContext)
    { 
        redisAsyncFree(pRedisAsyncContext);
    }
}

bool test_redis::Connect(char* host, unsigned short port) {
#ifndef _MSC_VER
    signal(SIGPIPE, SIG_IGN);
#endif // !MSVC    
    redisAsyncContext *c = redisAsyncConnect("127.0.0.1", 6379);
    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return false;
    }
    if (pRedisAsyncContext)
    {
        redisAsyncFree(pRedisAsyncContext);
    }
    pRedisAsyncContext = c;
    pRedisAsyncContext->data = this;

    redisLibuvAttach(pRedisAsyncContext, uv_loop);
    redisAsyncSetConnectCallback(pRedisAsyncContext, AfterConnect);
    redisAsyncSetDisconnectCallback(pRedisAsyncContext, AfterDisConnect);
    /*redisAsyncCommand(c, NULL, NULL, "SET key %b", argv[argc - 1], strlen(argv[argc - 1]));
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");*/
    return true;
}

void test_redis::RunRedis(char* host, unsigned short port) {
    
    uv_loop = uv_default_loop();
    if (!Connect(host, port))
    {
        printf("Error: %s\n");
        return;
    }
    uv_run(uv_loop, UV_RUN_DEFAULT);
}

void test_redis::HMSETTest()
{
    if (!pRedisAsyncContext) {
        return;
    }
    SRedisTest* pRT = new SRedisTest();
    fprintf(stdout, "new SRedisTest:(%p)\n", pRT);
    pRT->_init();
    redisAsyncCommand(pRedisAsyncContext, redisSetCallBack, pRT,
        "HMSET test "
        "value %b"//
    , pRT,sizeof(SRedisTest));
}

void test_redis::HMGetTest() {

    if (!pRedisAsyncContext) {
        return;
    }
    redisAsyncCommand(pRedisAsyncContext, redisGetCallBack, nullptr,
        "HMGET test "
        "value ");
}

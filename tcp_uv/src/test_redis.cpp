#include "test_redis.h"
#include <vector>
#include <map>

using namespace std;
#define RedisCallBack(realCallBack, holder) std::bind(realCallBack, holder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
static redisAsyncContextWrapper* pRedisAsyncContext = nullptr;

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

test_redis::test_redis()
{
    
    pClient = nullptr;
}

test_redis::~test_redis()
{
}

bool test_redis::Connect(char* host, unsigned short port) {
    if (!pClient)
    {
        pClient = new TCPClient();
        pClient->SetRecvBufCB(ReadCB, this);
        pClient->SetClosedCB(CloseCB, this);
        pClient->SetConnectedCB(AfterConnect, this);
        if (!pClient->Connect(host, port,false,false)) {
            printf("connect error:%s\n", pClient->GetLastErrMsg());
            return false;
        }
        else {
            printf("client(%p) connect succeed.\n", pClient);
            return true;
        }
    }
    return false;
}

void test_redis::RunRedis(char* host, unsigned short port) {
    
    if (Connect(host, port))
    {

    }
    while (true)
    {
        Sleep(1);
    }
}

void test_redis::AfterConnect(void* userdata) {
    test_redis* pTestRedis = (test_redis*)userdata;
    if (!pRedisAsyncContext) {
        pRedisAsyncContext = new redisAsyncContextWrapper();
        pRedisAsyncContext->Init(std::bind(&TCPClient::Send, pTestRedis->pClient, std::placeholders::_1, std::placeholders::_2)
            , std::bind(&TCPClient::Close, pTestRedis->pClient, std::placeholders::_1));
    }
    pRedisAsyncContext->SetConnected(true);

    pTestRedis->HMSETTest();
   
}

void test_redis::CloseCB(int clientid, void* userdata)
{
    test_redis* pTestRedis = (test_redis*)userdata;
    if (!pRedisAsyncContext) {
        pRedisAsyncContext->SetConnected(false);
    }
}

void test_redis::ReadCB(const unsigned char* buf, unsigned int lenght, void* userdata)
{
    test_redis* pTestRedis = (test_redis*)userdata;
    if (pRedisAsyncContext) {
        int res = pRedisAsyncContext->redisAsyncReaderFeed((const char*)buf, lenght);
        if (res != REDIS_OK) {
            return;
        }
        pRedisAsyncContext->redisProcessCallbacks();
    }
}

void  test_redis::redisGetCallBack(redisAsyncContext* predisAsyncContext, redisReply* predisReply, void* pData)
{
    printf("redis Get CallBack! \n");
    if (!predisReply || predisReply->elements <= 0) {
        printf("redis Get CallBack Error! \n");
        return;
    }
    SRedisTest _RT;    
    fprintf(stdout, "Get SRedisTest:(%p)\n", &_RT);
    memcpy(&_RT, predisReply->element[0]->str, predisReply->element[0]->len);
}

void  test_redis::redisSetCallBack(redisAsyncContext* predisAsyncContext, redisReply* predisReply, void* pData)
{
    printf("redis Set CallBack! \n");
    SRedisTest* pRT = (SRedisTest*)pData;
    fprintf(stdout, "delete SRedisTest:(%p)\n", pRT);
    delete pRT;

    Sleep(1000);
    HMGetTest();
}

void test_redis::HMSETTest()
{
    if (!pRedisAsyncContext) {
        return;
    }
    if (!pRedisAsyncContext->CanbeUsed()) {
        return;
    }
    SRedisTest* pRT = new SRedisTest();
    fprintf(stdout, "new SRedisTest:(%p)\n", pRT);
    pRT->_init();
    pRedisAsyncContext->redisAsyncCommand(RedisCallBack(&test_redis::redisSetCallBack, this), pRT,
        "HMSET test "
        "value %b"//
    , pRT,sizeof(SRedisTest));    
}

void test_redis::HMGetTest() {

    if (!pRedisAsyncContext) {
        return;
    }
    if (!pRedisAsyncContext->CanbeUsed()) {
        return;
    }
    pRedisAsyncContext->redisAsyncCommand(RedisCallBack(&test_redis::redisGetCallBack, this), nullptr,
        "HMGET test "
        "value ");
}

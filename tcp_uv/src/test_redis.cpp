#include "test_redis.h"

#define RedisCallBack(realCallBack, holder) std::bind(realCallBack, holder, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)
static redisAsyncContextWrapper* pRedisAsyncContext = nullptr;
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
        pClient->SetRecvCB(ReadCB, pClient);
        pClient->SetClosedCB(CloseCB, pClient);
        pClient->SetConnectedCB(AfterConnect, pClient);
        if (!pClient->Connect(host, port)) {
            printf("connect error:%s\n", pClient->GetLastErrMsg());
            return false;
        }
        else {
            printf("client(%p) connect succeed.\n", pClient);
            return true;
        }
    }
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
}

void test_redis::CloseCB(int clientid, void* userdata)
{
    test_redis* pTestRedis = (test_redis*)userdata;
    if (!pRedisAsyncContext) {
        pRedisAsyncContext->SetConnected(false);
    }
}

void test_redis::ReadCB(const NetPacket& packet, const unsigned char* buf, void* userdata)
{
    test_redis* pTestRedis = (test_redis*)userdata;
    if (pRedisAsyncContext) {
        int res = pRedisAsyncContext->redisAsyncReaderFeed((const char*)buf, packet.datalen);
        if (res != REDIS_OK) {
            return;
        }
        pRedisAsyncContext->redisProcessCallbacks();
    }
}

void  redisGetCallBack(redisAsyncContext* predisAsyncContext, redisReply* predisReply, void* pData)
{
#if 0
    GameAndCS::SwitchServer* pSwitchServer = (GameAndCS::SwitchServer*)pData;
    if (!pSwitchServer)
    {
        ELOG(LOG_WARNNING, "");
        return;
    }
    bool ifQueryFromRedis = false;
    SPlayerDBData sPlayerDBData;
    SRuntimeKeyData sRTKeyData;
    do {
        if (!predisReply || predisReply->elements != 2) {
            break;
        }
        memcpy(&sPlayerDBData, predisReply->element[0]->str, predisReply->element[0]->len);
        memcpy(&sRTKeyData, predisReply->element[1]->str, predisReply->element[1]->len);
        ifQueryFromRedis = true;
    } while (false);
#endif
}

void OnTest()
{
#if 0
    if (!pReids) {
        break;
    }
    if (!pReids->CanbeUsed()) {
        break;
    }
    ifQueryFromRedis = true;
    pReids->redisAsyncCommand(RedisCallBack(&redisGetCallBack, nullptr), nullptr,
        "HMGET guid:%lld "
        "playerdata rtkey"
        , pSwitchServer->guid);
#endif
}


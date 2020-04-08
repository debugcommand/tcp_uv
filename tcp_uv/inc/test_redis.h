#ifndef _TEST_REDIS_H_
#define _TEST_REDIS_H_
#include "Redis/hiredis.h"
#include "Redis\async.h"
#include "UVNet\tcpclient.h"

using namespace UVNet;
class test_redis
{
public:
    test_redis();
    ~test_redis();
    bool Connect(char* host, unsigned short port);
    static void CloseCB(int clientid, void* userdata);
    static void ReadCB(const NetPacket& packet, const unsigned char* buf, void* userdata);
    static void AfterConnect(void* userdata);

    void RunRedis(char* host, unsigned short port);
private:    
    TCPClient*                pClient;
};
#endif // !_TEST_REDIS_H_


#pragma once
#ifndef _TEST_TCP_H_
#define _TEST_TCP_H_
#include <iostream>
#include <string>
#include "uvnet/tcpclient.h"
#include "uvnet/tcpserver.h"

using namespace std;
using namespace UVNet;
class test_tcpclient
{
public:
    test_tcpclient();
    ~test_tcpclient();

    static void CloseCB(int clientid, void* userdata);
    static void ReadCB(const NetPacket& packet, const unsigned char* buf, void* userdata);    
    int  RunClient(std::string ip, int port, int cli_count);
    void ProfileReport(int64_t tCurrTime);
private:
    char serverip[128];
    bool is_exist = false;    
};

class test_tcpserver
{
public:
    test_tcpserver();
    ~test_tcpserver();

    static void CloseCB(int clientid, void* userdata);
    static void NewConnect(int clientid, void* userdata);
    int RunServer(int port);
    void ProfileReport(int64_t tCurrTime);
private:
    bool is_eist;
    int call_time;
    TCPServer* pServer;
};
#endif // !_TEST_TCP_H_

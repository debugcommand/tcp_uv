#include "test_tcp.h"
#include <sstream>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#include <MMsystem.h>
#pragma comment(lib,"winmm.lib")
#else
#include <unistd.h>
#endif

static __int64_t GetTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

test_tcpserver::test_tcpserver()
{
    is_eist = false;
    call_time = 0;
    pServer = new TCPServer();
}

test_tcpserver::~test_tcpserver()
{
}

void test_tcpserver::CloseCB(int clientid, void* userdata)
{
    printf("client %d close\n",clientid);
    TCPServer *theclass = (TCPServer *)userdata;
}

void test_tcpserver::NewConnect(int clientid, void* userdata)
{
    fprintf(stdout,"new connect:%d\n",clientid);
    ((TCPServer *)userdata)->SetRecvCB(clientid,nullptr,nullptr);
}

static __int64_t start_time = 0;
void test_tcpserver::ProfileReport(__int64_t tCurrTime)
{
    if (tCurrTime - start_time < 5000)
    {
        return;
    }
    /*
    start_time = tCurrTime;
    wstringstream strTitle;
    strTitle << "client=" << pServer->client_count() << " avia=" << pServer->avai_count() << " wparam=" << pServer->wparam_count();
    SetConsoleTitle(strTitle.str().c_str());*/
}

int test_tcpserver::RunServer(int port)
{
	TestTCPProtocol protocol;
    pServer->SetNewConnectCB(NewConnect,pServer);
    pServer->SetPortocol(&protocol);
    if(!pServer->Start("0.0.0.0", port)) {
        fprintf(stdout,"Start Server error:%s\n", pServer->GetLastErrMsg());
    }
    pServer->SetKeepAlive(1,60);//enable Keepalive, 60s
    fprintf(stdout,"server running.\n");
    while(!is_eist) {
        ProfileReport(GetTime());
        usleep(1);
    }
    return 0;
}


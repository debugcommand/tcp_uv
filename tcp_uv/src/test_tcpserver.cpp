#include "test_tcp.h"
#include <sstream>
#ifdef _WIN32
#include <windows.h>
#include <MMsystem.h>
#pragma comment(lib,"winmm.lib")
#endif

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

static int64_t start_time = 0;
void test_tcpserver::ProfileReport(int64_t tCurrTime)
{
    if (tCurrTime - start_time < 5000)
    {
        return;
    }
    start_time = tCurrTime;
    wstringstream strTitle;
    strTitle << "client=" << pServer->client_count() << " avia=" << pServer->avai_count() << " wparam=" << pServer->wparam_count();
    //strTitle << "game" << " id=" << theConfig.GetKernelConfig()->un32ServerId << " fps=" << fps << " online=" << theRTCtlr.GetOnlineCount()
    //    << " offline=" << theRTCtlr.GetOfflineMapSize() << " account=" << theRTCtlr.GetAccountMapSize();
    SetConsoleTitle(strTitle.str().c_str());
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
        ProfileReport(timeGetTime());
        Sleep(1);
    }
    return 0;
}


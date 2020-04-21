#include "test_tcp.h"
#include <sstream>
#include <chrono>

static INT64 GetTime()
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

static INT64 start_time = 0;
void test_tcpserver::ProfileReport(INT64 tCurrTime)
{
    if (tCurrTime - start_time < 5000)
    {
        return;
    }
#ifdef WIN32
    start_time = tCurrTime;
    wstringstream strTitle;
    strTitle << "client=" << pServer->client_count() << " avia=" << pServer->avai_count() << " wparam=" << pServer->wparam_count();
    SetConsoleTitle(strTitle.str().c_str());
#endif // WIN32
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
        uv_thread_sleep(1);
    }
    return 0;
}


#include "test_tcp.h"
#include <sstream>
#include <chrono>
#ifdef WIN32
#else
#include <unistd.h>
#endif

static __int64_t GetTime()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

static int call_time = 0;
test_tcpclient::test_tcpclient()
{
    is_exist = false;
}

test_tcpclient::~test_tcpclient()
{
}

void test_tcpclient::CloseCB(int clientid, void* userdata)
{
    printf("client %d close\n", clientid);
    TCPClient* client = (TCPClient*)userdata;
    client->Close();
}

static unsigned long maxPing = 0;
void test_tcpclient::ReadCB(const MessageHeader& header, const unsigned char* buf, void* userdata)
{
#if 0
    printf("ReadCB:call time %d\n", ++call_time);
    if (call_time > 5000) {
        return;
    }
#endif // 0
    char senddata[256] = {0};
	TCPClient* client = (TCPClient*)userdata;
    int64_t curT = GetTime();
    int64_t sendtime = atoll((const char*)buf);
    int64_t  pingtime = curT - sendtime;
    fprintf(stdout, "recv server:client(%p) ping(%lld-%lld-%lld)\n", client, curT,sendtime, pingtime);
#if 0
    if (rand() % 2) {
        NetPacket tmppack = packet;
        tmppack.datalen = (std::min)(strlen(senddata), sizeof(senddata) - 1);
        std::string retstr = PacketData(tmppack, (const unsigned char*)senddata);
        if (client->Send(&retstr[0], retstr.length()) <= 0) {
            printf("ReadCB:(%p)send error.%s\n", client, client->GetLastErrMsg());
        }
    }
#endif
    client->SetHangeUp(false);
    if (pingtime > maxPing)
    {
        maxPing = pingtime;
    }
}

int test_tcpclient::RunClient(std::string ip,int port,int cli_count)
{
    strcpy(serverip, ip.c_str());
    const int clientsize = cli_count;
    TCPClient** pClients = new TCPClient*[clientsize];
    char senddata[32];
    for (int i = 0; i < clientsize; ++i) {
        pClients[i] = new TCPClient();
        pClients[i]->SetRecvCB(ReadCB, pClients[i]);
        pClients[i]->SetClosedCB(CloseCB, pClients[i]);
        if (!pClients[i]->Connect(serverip, port,true,true)) {
            printf("connect error:%s\n", pClients[i]->GetLastErrMsg());
        } else {
            printf("client(%p) connect succeed.\n", pClients[i]);
        }   
    }    
    
    while (!is_exist) {
        int64_t curTime = GetTime();
        MessageHeader header;
        header.type = 1;
        sprintf(senddata, "%lld", curTime);
        header.datalen = header.datalen = (std::min)(strlen(senddata), sizeof(senddata) - 1);
        for (int i = 0; i < clientsize; ++i) {
            if (pClients[i]&& !pClients[i]->IsClosed()&& !pClients[i]->IsHangUp())
            {
                //memset(senddata, 0, sizeof(senddata));
                //sprintf(senddata, "main loop: client(%p) call %d", pClients[i], call_time);
                //packet.datalen = (std::min)(strlen(senddata), sizeof(senddata) - 1);
                std::string str = PackData(header, (const unsigned char*)senddata);
                if (!pClients[i]->Send(&str[0], str.length())) {
                    printf("main loop:(%p)send error.%s\n", pClients[i], pClients[i]->GetLastErrMsg());
                }
                else {
                    pClients[i]->SetHangeUp(true);
                    //fprintf(stdout,"发送的数据为:\n");
                    //         for (int i=0; i<str.length(); ++i) {
                    //             fprintf(stdout,"%02X",(unsigned char)str[i]);
                    //         }
                    //         fprintf(stdout,"\n");
                    //fprintf(stdout, "main loop: send succeed:%s-%d\n", senddata, packet.datalen);
                }           
            }
        }
        call_time++;
        ProfileReport(curTime);
        usleep(1000);
    }
    return 0;
}

static int64_t start_time = 0;
void test_tcpclient::ProfileReport(int64_t tCurrTime) {
    if (tCurrTime - start_time < 5000)
    {
        return;
    }
    /*
    start_time = tCurrTime;
    wstringstream strTitle;
    strTitle << "ping=" << maxPing;
    SetConsoleTitle(strTitle.str().c_str());*/
}

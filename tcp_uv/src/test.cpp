#include "test_tcp.h"
#include <Windows.h>
#include <sstream>
#include <stdio.h>
#include ".\ELoggingHeader.h"
#include <TCHAR.H>
#include "test_redis.h"

using namespace std;
#define NEXT_ARG ((((*Argv)[2])==TEXT('\0'))?(--Argc,*++Argv):(*Argv)+2)

//将TCHAR转为char   
//*tchar是TCHAR类型指针，*_char是char类型指针   
void TCharToChar(const TCHAR * tchar, char * _char)
{
    int iLength;
    //获取字节长度   
    iLength = WideCharToMultiByte(CP_ACP, 0, tchar, -1, NULL, 0, NULL, NULL);
    //将tchar值赋给_char    
    WideCharToMultiByte(CP_ACP, 0, tchar, -1, _char, iLength, NULL, NULL);
}
void CharToTChar(const char * _char, TCHAR * tchar)
{
    int iLength;
    iLength = MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, NULL, 0);
    MultiByteToWideChar(CP_ACP, 0, _char, strlen(_char) + 1, tchar, iLength);
}

int main(int argc, char** argv)
{
#ifdef DEBUG
    system("pause");
#endif // DEBUG    
    char logname[16] = "test_tcp";
	ELOGINIT(logname);
    //commandline///////////////////
    auto GetCmdParam = [&](int idx) {
        return idx < argc ? argv[idx] : NULL;
    };
    const char* platform = GetCmdParam(1);
    if (strcmp(platform, "-c") == 0)
    {//cleint
        test_tcpclient* pTClient = new test_tcpclient();
        pTClient->RunClient(GetCmdParam(2),atoi(GetCmdParam(3)),atoi(GetCmdParam(4)));
    }
    else if (strcmp(platform, "-s") == 0) {//sever
        test_tcpserver* pTServer = new test_tcpserver();
        pTServer->RunServer(atoi(GetCmdParam(2)));
    }
    else if (strcmp(platform, "-r") == 0) {//redis
        test_redis* pTRedis = new test_redis();
        pTRedis->RunRedis(GetCmdParam(2), atoi(GetCmdParam(3)));
    }
    return 0;
}
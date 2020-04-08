/***************************************
* @file     packet_sync.h
* @brief    TCP 数据包封装.依赖libuv,openssl.功能：接收数据，解析得到一帧后回调给用户。同步处理，接收到马上解析
* @details  根据net_base.h中NetPacket的定义，对数据包进行封装。
			md5校验码使用openssl函数
			同一线程中实时解码
			长度为0的md5为：d41d8cd98f00b204e9800998ecf8427e，改为全0. 编解码时修改。
//调用方法
Packet packet;
packet.SetPacketCB(GetPacket,&serpac);
socket有数据到达时，调用packet.recvdata((const unsigned char*)buf,bufsize); 只要足够一帧它会触发GetFullPacket
****************************************/
#ifndef _PACKET_SYNC_H_
#define _PACKET_SYNC_H_
#include <algorithm>
#include "openssl/md5.h"
#include "net_packet.h"
#include "thread_uv.h"//for GetUVError
#if defined (WIN32) || defined(_WIN32)
#include <windows.h>
#define ThreadSleep(ms) Sleep(ms);//睡眠ms毫秒
#elif defined __linux__
#include <unistd.h>
#define ThreadSleep(ms) usleep((ms) * 1000)//睡眠ms毫秒
#endif

typedef void (*GetFullPacket)(const NetPacket& packethead, const unsigned char* packetdata, void* userdata);
typedef void (*CBClose)(void* userdata);
#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE (1024*10)
#endif

class PacketSync
{
public:
    PacketSync(): packet_cb_(NULL), packetcb_userdata_(NULL) {
        thread_readdata = uv_buf_init((char*)malloc(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE); //负责从circulebuffer_读取数据
        thread_packetdata = uv_buf_init((char*)malloc(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE); //负责从circulebuffer_读取packet 中data部分
        truepacketlen = 0;//readdata有效数据长度
        parsetype = PARSE_NOTHING;
    }
    virtual ~PacketSync() {
        free(thread_readdata.base);
        free(thread_packetdata.base);
    }

public:
#if 0
    void recvmsg(const unsigned char* data, int len) {
        truepacketlen += len;
        if (truepacketlen < sizeof(int))
        {
            return;
        }
        char* pData = thread_readdata.base;
        while (truepacketlen > 0)
        {
            int nUsed = 0;
            //解析，可以引入函数指针
            {
                int totLength = *(int*)pData;
                if (totLength <= truepacketlen) {
                    nUsed = totLength;
                }
                else
                {
                    break;
                }

                memcpy(&theMsg, pData, nUsed);
                if (this->msg_cb_) {
                    this->msg_cb_(theMsg, theMsg.GetLenght(), this->packetcb_userdata_);
                }

                pData += nUsed;
                truepacketlen -= nUsed;
            }
        }

        if (truepacketlen > 0 && (pData != thread_readdata.base))
        {
            memmove(thread_readdata.base, pData, truepacketlen);
        }
}
#endif // 0
    void resetdata(char* dest,char* src ,int len){
        if (dest != src && len > 0) {
            memmove(dest, src, len);
        }
    }
    void recvdata(const unsigned char* data, int len) { //接收到数据，把数据保存在circulebuffer_ 
        char* pReadData = nullptr;
        if (data == nullptr||len <= 0|| truepacketlen+len > MAX_BUFFER_SIZE)
        {
            goto _CLOSE;
        }
        memcpy(thread_readdata.base + truepacketlen, data, len);
        truepacketlen += len;
        if (truepacketlen < NET_PACKAGE_HEADLEN&&PARSE_NOTHING == parsetype)
        {
            return;
        }
        pReadData = thread_readdata.base;
        while (truepacketlen > 0)
        {
            if (PARSE_NOTHING == parsetype) {//先解析头
                memcpy(&theNexPacket, pReadData, NET_PACKAGE_HEADLEN);
                if (theNexPacket.datalen > MAX_BUFFER_SIZE - NET_PACKAGE_HEADLEN)
                {
                    goto _CLOSE;
                }
                pReadData += NET_PACKAGE_HEADLEN;
                truepacketlen -= NET_PACKAGE_HEADLEN;
                parsetype = PARSE_HEAD;
                //缓冲位置重置
                resetdata(thread_readdata.base, pReadData, truepacketlen);
                //得帧数据
                if (thread_packetdata.len < (size_t)theNexPacket.datalen) {//检测正式数据buff大小
                    thread_packetdata.base = (char*)realloc(thread_packetdata.base, theNexPacket.datalen);
                    thread_packetdata.len = theNexPacket.datalen;
                }
                continue;
            }
            else
            {
                if (truepacketlen >= theNexPacket.datalen)
                {
                    memcpy(thread_packetdata.base, thread_readdata.base, theNexPacket.datalen);

#ifdef  NETPACKET_CHECK//检测需要重新写
                    if (0 == theNexPacket.datalen) { //长度为0的md5为：d41d8cd98f00b204e9800998ecf8427e，改为全0
                        memset(md5str, 0, sizeof(md5str));
                    }
                    else {
                        MD5_CTX md5;
                        MD5_Init(&md5);
                        MD5_Update(&md5, thread_packetdata.base, theNexPacket.datalen); //包数据的校验值
                        MD5_Final(md5str, &md5);
                    }
                    if (memcmp(theNexPacket.check, md5str, MD5_DIGEST_LENGTH) != 0) {
                        fprintf(stdout, "读取%d数据, 校验码不合法\n", NET_PACKAGE_HEADLEN + theNexPacket.datalen + 2);
                        if (truepacketlen - headpos - 1 - NET_PACKAGE_HEADLEN >= theNexPacket.datalen + 1) {//thread_readdata数据足够
                            memmove(thread_readdata.base, thread_readdata.base + headpos + 1, truepacketlen - headpos - 1); //2.4
                            truepacketlen -= headpos + 1;
                        }
                        else {//thread_readdata数据不足
                            if (thread_readdata.len < NET_PACKAGE_HEADLEN + theNexPacket.datalen + 1) {//包含最后的tail
                                thread_readdata.base = (char*)realloc(thread_readdata.base, NET_PACKAGE_HEADLEN + theNexPacket.datalen + 1);
                                thread_readdata.len = NET_PACKAGE_HEADLEN + theNexPacket.datalen + 1;
                            }
                            memmove(thread_readdata.base, thread_readdata.base + headpos + 1, NET_PACKAGE_HEADLEN); //2.4
                            truepacketlen = NET_PACKAGE_HEADLEN;
                            memcpy(thread_readdata.base + truepacketlen, thread_packetdata.base, theNexPacket.datalen + 1);
                            truepacketlen += theNexPacket.datalen + 1;
                        }
                        parsetype = PARSE_NOTHING;//重头再来
                        goto _CLOSE;
                        continue;
                    }
#endif //  NETPACKET_CHECK      
                    pReadData += theNexPacket.datalen;
                    truepacketlen -= theNexPacket.datalen;
                    //缓冲位置重置
                    resetdata(thread_readdata.base, pReadData, truepacketlen);
                    //回调帧数据给用户
                    if (this->packet_cb_) {
                        this->packet_cb_(theNexPacket, (const unsigned char*)thread_packetdata.base, this->packetcb_userdata_);
                    }
                    parsetype = PARSE_NOTHING;//重头再来
                    continue;
                }
                break;
            }
        }
        return;
    _CLOSE:
        call_close_(this->packetcb_userdata_);
    }
    void SetPacketCB(GetFullPacket pfun, CBClose pclose, void* userdata) {
        packet_cb_ = pfun;
        call_close_ = pclose;
        packetcb_userdata_ = userdata;
    }
private:
    GetFullPacket packet_cb_;//回调函数
    CBClose       call_close_;//回调关闭
    void*         packetcb_userdata_;//回调函数所带的自定义数据

    enum {
        PARSE_HEAD,
        PARSE_NOTHING,
    };
    int parsetype;
    int getdatalen;
    uv_buf_t  thread_readdata;//负责从circulebuffer_读取数据
    uv_buf_t  thread_packetdata;//负责从circulebuffer_读取packet 中data部分
    int truepacketlen;//readdata有效数据长度
    NetPacket theNexPacket;
    unsigned char md5str[MD5_DIGEST_LENGTH];
private:// no copy
    PacketSync(const PacketSync&);
    PacketSync& operator = (const PacketSync&);
};


/***********************************************辅助函数***************************************************/
/*****************************
* @brief   把数据组合成NetPacket格式的二进制流，可直接发送。
* @param   packet --NetPacket包，里面的version,header,tail,type,datalen,reserve必须提前赋值，该函数会计算check的值。然后组合成二进制流返回
	       data   --要发送的实际数据
* @return  std::string --返回的二进制流。地址：&string[0],长度：string.length()
******************************/
inline std::string PacketData(NetPacket& packet, const unsigned char* data)
{
#ifdef NETPACKET_CHECK
    if (packet.datalen == 0 || data == NULL) {//长度为0的md5为：d41d8cd98f00b204e9800998ecf8427e，改为全0
        memset(packet.check, 0, sizeof(packet.check));
}
    else {
        MD5_CTX md5;
        MD5_Init(&md5);
        MD5_Update(&md5, data, packet.datalen);
        MD5_Final(packet.check, &md5);
    }
#endif //     
    unsigned char packchar[NET_PACKAGE_HEADLEN];
    NetPacketToChar(packet, packchar);

    std::string retstr;
    retstr.append((const char*)packchar, NET_PACKAGE_HEADLEN);
    retstr.append((const char*)data, packet.datalen);
    return std::move(retstr);
}

//客户端或服务器关闭的回调函数
//服务器：当clientid为-1时，表现服务器的关闭事件
//客户端：clientid无效，永远为-1
typedef void (*TcpCloseCB)(int clientid, void* userdata);

//TCPServer接收到新客户端回调给用户
typedef void (*NewConnectCB)(int clientid, void* userdata);

//TCPServer接收到客户端数据回调给用户
typedef void (*ServerRecvCB)(int clientid, const NetPacket& packethead, const unsigned char* buf, void* userdata);

//TCPClient接收到服务器数据回调给用户
typedef void (*ClientRecvCB)(const NetPacket& packethead, const unsigned char* buf, void* userdata);

//连接成功的外部回调
typedef void(*ConnectedCB)(void* userdata);
//网络事件类型
typedef enum {
    NET_EVENT_TYPE_RECONNECT = 0,  //与服务器自动重连成功事件
    NET_EVENT_TYPE_DISCONNECT      //与服务器断开事件
} NET_EVENT_TYPE;
//TCPClient断线重连函数
typedef void (*ReconnectCB)(NET_EVENT_TYPE eventtype, void* userdata);

#endif//_PACKET_SYNC_H_
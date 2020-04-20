/***************************************
* @file     message_parse.h
* @brief    TCP 数据包封装.依赖libuv,openssl.功能：接收数据，解析得到一帧后回调给用户。同步处理，接收到马上解析
* @details  根据net_msg.h中message header的定义，对数据包进行封装。
			md5校验码使用openssl函数
			同一线程中实时解码
			长度为0的md5为：d41d8cd98f00b204e9800998ecf8427e，改为全0. 编解码时修改。
****************************************/
#ifndef _MESSAGE_PARSE_H_
#define _MESSAGE_PARSE_H_
#include <algorithm>
#include "openssl/md5.h"
#include "net_msg.h"
#include "thread_uv.h"//for GetUVError
#include <string.h>
#if defined (WIN32) || defined(_WIN32)
#include <windows.h>
#define ThreadSleep(ms) Sleep(ms);//睡眠ms毫秒
#elif defined __linux__
#include <unistd.h>
#define ThreadSleep(ms) usleep((ms) * 1000)//睡眠ms毫秒
#endif

typedef void (*GetMsg) (const MessageHeader& header, const unsigned char* realdata, void* userdata);
typedef void (*GetData)(const unsigned char* data, unsigned int lenght, void* userdata);
typedef void (*CBClose)(void* userdata);
#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE (1024*10)
#endif

class MessageParse
{
public:
    MessageParse(): msg_cb_(nullptr), cb_userdata_(nullptr),data_cb_(nullptr) {
        thread_readdata = uv_buf_init((char*)malloc(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE); //负责从circulebuffer_读取数据
        thread_realdata = uv_buf_init((char*)malloc(MAX_BUFFER_SIZE), MAX_BUFFER_SIZE); //负责从circulebuffer_读取有效data部分
        usedlen = 0;//readdata有效数据长度
        parsetype = PARSE_NOTHING;
        useheader = true;
        needparse = true;
    }
    virtual ~MessageParse() {
        free(thread_readdata.base);
        free(thread_realdata.base);
    }

public:
    void resetdata(char* dest,char* src ,int len){
        if (dest != src && len > 0) {
            memmove(dest, src, len);
        }
    }

    void recvdata(const unsigned char* data, int len) { //接收到数据，把数据保存在circulebuffer_ 
        if (useheader)
        {
            header_parse(data, len);
        }
        else
        {
            length_parse(data, len);
        }
    }

    void length_parse(const unsigned char* data, int len) {
        char* pReadData = nullptr;
        int totLength = 0;
        if (data == nullptr || len <= 0 || usedlen + len > MAX_BUFFER_SIZE)
        {
            goto _CLOSE;
        }
        memcpy(thread_readdata.base + usedlen, data, len);
        usedlen += len;
        pReadData = thread_readdata.base;        
        while (usedlen > 0)
        {
            totLength = usedlen;
            if (needparse)
            { 
                if (totLength < LENGTH_HEADLEN)
                {
                    return;
                }
                totLength = *(int*)pReadData;
            }

            if (totLength < 0|| totLength > MAX_BUFFER_SIZE)
            {
                goto _CLOSE;
            }
            else if (totLength <= usedlen) {
                if (thread_realdata.len < totLength) {//检测正式数据buff大小
                    thread_realdata.base = (char*)realloc(thread_realdata.base, totLength);
                    thread_realdata.len = totLength;
                }

                memcpy(thread_realdata.base, thread_readdata.base, totLength);

                pReadData += totLength;
                usedlen -= totLength;
                //缓冲位置重置
                resetdata(thread_readdata.base, pReadData, usedlen);
                //回调帧数据给用户
                if (this->data_cb_) {
                    this->data_cb_((const unsigned char*)thread_realdata.base, totLength, this->cb_userdata_);
                }
                continue;
            }
            break;
        }
        return;
    _CLOSE:
        call_close_(this->cb_userdata_);
    }

    void header_parse(const unsigned char* data, int len) {
        char* pReadData = nullptr;
        if (data == nullptr || len <= 0 || usedlen + len > MAX_BUFFER_SIZE)
        {
            goto _CLOSE;
        }
        memcpy(thread_readdata.base + usedlen, data, len);
        usedlen += len;
        if (usedlen < MESSAGE_HEADER_LEN&&PARSE_NOTHING == parsetype)
        {
            return;
        }
        pReadData = thread_readdata.base;
        while (usedlen > 0)
        {
            if (PARSE_NOTHING == parsetype) {//先解析头
                memcpy(&theHeader, pReadData, MESSAGE_HEADER_LEN);
                if (theHeader.datalen > MAX_BUFFER_SIZE - MESSAGE_HEADER_LEN)
                {
                    goto _CLOSE;
                }
                pReadData += MESSAGE_HEADER_LEN;
                usedlen -= MESSAGE_HEADER_LEN;
                parsetype = PARSE_HEAD;
                //缓冲位置重置
                resetdata(thread_readdata.base, pReadData, usedlen);
                //得帧数据
                if (thread_realdata.len < (size_t)theHeader.datalen) {//检测正式数据buff大小
                    thread_realdata.base = (char*)realloc(thread_realdata.base, theHeader.datalen);
                    thread_realdata.len = theHeader.datalen;
                }
                continue;
            }
            else
            {
                if (usedlen >= theHeader.datalen)
                {
                    memcpy(thread_realdata.base, thread_readdata.base, theHeader.datalen);

#ifdef  MESSAGE_CHECK//检测需要重新写
                    if (0 == theHeader.datalen) { //长度为0的md5为：d41d8cd98f00b204e9800998ecf8427e，改为全0
                        memset(md5str, 0, sizeof(md5str));
                    }
                    else {
                        MD5_CTX md5;
                        MD5_Init(&md5);
                        MD5_Update(&md5, thread_realdata.base, theHeader.datalen); //包数据的校验值
                        MD5_Final(md5str, &md5);
                    }
                    if (memcmp(theHeader.check, md5str, MD5_DIGEST_LENGTH) != 0) {
                        fprintf(stdout, "读取%d数据, 校验码不合法\n", MESSAGE_HEADER_LEN + theHeader.datalen + 2);
                        if (usedlen - headpos - 1 - MESSAGE_HEADER_LEN >= theHeader.datalen + 1) {//thread_readdata数据足够
                            memmove(thread_readdata.base, thread_readdata.base + headpos + 1, usedlen - headpos - 1); //2.4
                            usedlen -= headpos + 1;
                        }
                        else {//thread_readdata数据不足
                            if (thread_readdata.len < MESSAGE_HEADER_LEN + theHeader.datalen + 1) {//包含最后的tail
                                thread_readdata.base = (char*)realloc(thread_readdata.base, MESSAGE_HEADER_LEN + theHeader.datalen + 1);
                                thread_readdata.len = MESSAGE_HEADER_LEN + theHeader.datalen + 1;
                            }
                            memmove(thread_readdata.base, thread_readdata.base + headpos + 1, MESSAGE_HEADER_LEN); //2.4
                            usedlen = MESSAGE_HEADER_LEN;
                            memcpy(thread_readdata.base + usedlen, thread_realdata.base, theHeader.datalen + 1);
                            usedlen += theHeader.datalen + 1;
                        }
                        parsetype = PARSE_NOTHING;//重头再来
                        goto _CLOSE;
                        continue;
                    }
#endif //  MESSAGE_CHECK      
                    pReadData += theHeader.datalen;
                    usedlen -= theHeader.datalen;
                    //缓冲位置重置
                    resetdata(thread_readdata.base, pReadData, usedlen);
                    //回调帧数据给用户
                    if (this->msg_cb_) {
                        this->msg_cb_(theHeader, (const unsigned char*)thread_realdata.base, this->cb_userdata_);
                    }
                    parsetype = PARSE_NOTHING;//重头再来
                    continue;
                }
                break;
            }
        }
        return;
    _CLOSE:
        call_close_(this->cb_userdata_);
    }

    void SetMsgCB(GetMsg pfun, CBClose pclose, void* userdata) {
        msg_cb_ = pfun;
        call_close_ = pclose;
        cb_userdata_ = userdata;
        useheader = true;
    }

    void SetDataCB(GetData pFun, CBClose pclose, void* userdata, bool _prase) {
        data_cb_ = pFun;
        call_close_ = pclose;
        cb_userdata_ = userdata;
        useheader = false;
        needparse = _prase;
    }
private:
    GetMsg        msg_cb_;//回调函数
    GetData       data_cb_;//回调函数
    CBClose       call_close_;//回调关闭
    void*         cb_userdata_;//回调函数所带的自定义数据
    enum {
        PARSE_HEAD  = 0,
        PARSE_NOTHING,
    };
    int parsetype;
    bool useheader; //是否使用message header作为协议头
    bool needparse;
    uv_buf_t  thread_readdata;//负责从circulebuffer_读取数据
    uv_buf_t  thread_realdata;//负责从circulebuffer_读取有效的data部分
    int usedlen;//readdata有效数据长度
    MessageHeader theHeader;
    unsigned char md5str[MD5_DIGEST_LENGTH];
private:// no copy
    MessageParse(const MessageParse&);
    MessageParse& operator = (const MessageParse&);
};


/***********************************************辅助函数***************************************************/
/*****************************
* @brief   把数据组合成Messageheader格式的二进制流，可直接发送。
* @param   message --header包，里面的type,datalen必须提前赋值，该函数会计算check的值。然后组合成二进制流返回
	       data   --要发送的实际数据
* @return  std::string --返回的二进制流。地址：&string[0],长度：string.length()
******************************/
inline std::string PackData(MessageHeader& header, const unsigned char* data)
{
#ifdef MESSAGE_CHECK
    if (header.datalen == 0 || data == NULL) {//长度为0的md5为：d41d8cd98f00b204e9800998ecf8427e，改为全0
        memset(packet.check, 0, sizeof(header.check));
}
    else {
        MD5_CTX md5;
        MD5_Init(&md5);
        MD5_Update(&md5, data, header.datalen);
        MD5_Final(header.check, &md5);
    }
#endif //     
    unsigned char packchar[MESSAGE_HEADER_LEN];
    HeaderToChar(header, packchar);

    std::string retstr;
    retstr.append((const char*)packchar, MESSAGE_HEADER_LEN);
    retstr.append((const char*)data, header.datalen);
    return std::move(retstr);
}

//客户端或服务器关闭的回调函数
//服务器：当clientid为-1时，表现服务器的关闭事件
//客户端：clientid无效，永远为-1
typedef void (*TcpCloseCB)(int clientid, void* userdata);

//TCPServer接收到新客户端回调给用户
typedef void (*NewConnectCB)(int clientid, void* userdata);

//TCPServer接收到客户端数据回调给用户
typedef void (*ServerRecvCB)(int clientid, const MessageHeader& header, const unsigned char* buf, void* userdata);

//TCPClient接收到服务器数据回调给用户
typedef void (*ClientRecvCB)(const MessageHeader& header, const unsigned char* buf, void* userdata);
typedef void (*ClientRecvBufCB)(const unsigned char* buf, unsigned int lenght,void* userdata);
//连接成功的外部回调
typedef void(*ConnectedCB)(void* userdata);
//网络事件类型
typedef enum {
    NET_EVENT_TYPE_RECONNECT = 0,  //与服务器自动重连成功事件
    NET_EVENT_TYPE_DISCONNECT      //与服务器断开事件
} NET_EVENT_TYPE;
//TCPClient断线重连函数
typedef void (*ReconnectCB)(NET_EVENT_TYPE eventtype, void* userdata);

#endif//_MESSAGE_PARSE_H_

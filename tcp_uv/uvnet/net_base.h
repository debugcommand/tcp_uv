/***************************************
* @file     net_base.h
* @brief    网络库基本功能函数
* @details  大小端判断
            32&64位系统判断
            ntohll与htonl的实现
			int32与int64序列/反序列化为char[4],char[8]
			数据包头结构定义
			在win平台测试过，linux平台未测试
****************************************/
#ifndef _NET_BASE_H_
#define _NET_BASE_H_
#include <stdint.h>
#if defined (WIN32) || defined (_WIN32)
#include <WinSock2.h>
#include <stdlib.h>
#elif defined(__linux__)
#include <sys/types.h>
#include <netinet/in.h>
#else
#error "no supported os"
#endif
#ifdef _MSC_VER
#pragma comment(lib,"Ws2_32.lib")
#endif //_MSC_VER


//判断是否小端字节序，是返回true
inline bool  IsLittleendian()
{
    int i = 0x1;
    return *(char *)&i == 0x1;
}

//判断是否32位系统，是返回true
inline bool IsSystem32()
{
    unsigned short x64 = 0;
#if defined(_MSC_VER)
    __asm mov x64,gs  // vs
#else
    asm("mov %%gs, %0" : "=r"(x64)); // gcc
#endif
    return x64 == 0;
}

namespace UVNet
{
//htonll与ntohll win8才支持，linux支持也不全，所以自己实现
inline uint64_t htonll(uint64_t num)
{
    int i = 0x1;
    uint64_t retval;
    if(*(char *)&i == 0x1) {
#if defined (WIN32) || defined (_WIN32)
        retval = _byteswap_uint64(num);//比htonl快
#elif defined(__linux__ )
        retval = __builtin_bswap64(num);
#else
        retval = ((uint64_t)htonl(num & 0x00000000ffffffffULL)) << 32;
        retval |= htonl((num & 0xffffffff00000000ULL) >> 32);
#endif
    } else {
        return num;
    }
    return retval;
}

inline uint64_t ntohll(uint64_t num)
{
    int i = 0x1;
    uint64_t retval;
    if(*(char *)&i == 0x1) {
#if defined (WIN32) || defined (_WIN32)
        retval = _byteswap_uint64(num);
#elif defined(__linux__ )
        retval = __builtin_bswap64(num);
#else
        retval = ((uint64_t)ntohl(num & 0x00000000ffffffffULL)) << 32;
        retval |= ntohl((num & 0xffffffff00000000ULL) >> 32);
#endif
    } else {
        return num;
    }
    return retval;
}
}
#endif//_NET_BASE_H_
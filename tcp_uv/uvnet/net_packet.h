#ifndef _NET_PACKET_H_
#define _NET_PACKET_H_
#include <stdint.h>
#include "net_base.h"

//把32位的int保存在char[4]中.先转为网络字节序，然后int的最高位保存为char[0],最低位保存于char[3]
inline bool Int32ToChar(const uint32_t intnum, unsigned char* charnum)
{
    unsigned long network_byteorder = htonl(intnum);//转换为网络字节序
    charnum[0] = (unsigned char)((network_byteorder & 0xff000000) >> 24);//int的最高位
    charnum[1] = (unsigned char)((network_byteorder & 0x00ff0000) >> 16);//int的次高位
    charnum[2] = (unsigned char)((network_byteorder & 0x0000ff00) >> 8); //int的次低位;
    charnum[3] = (unsigned char)((network_byteorder & 0x000000ff));    //int的最低位;
    return true;
}

//把char[4]转换为32位的int。int的最高位保存为char[0],最低位保存于char[3]，然后转为主机字节序
inline bool CharToInt32(const unsigned char* charnum, uint32_t& intnum)
{
    intnum = (charnum[0] << 24) + (charnum[1] << 16) + (charnum[2] << 8) + charnum[3];
    intnum = ntohl(intnum);//转换为网络字节序
    return true;
}

//把64位的int保存在char[8]中.先转为网络字节序，然后int的最高位保存为char[0],最低位保存于char[7]
inline bool Int64ToChar(const uint64_t intnum, unsigned char* charnum)
{
    uint64_t network_byteorder = UVNet::htonll(intnum);//转换为网络字节序
    charnum[0] = (unsigned char)((network_byteorder & 0xff00000000000000ULL) >> 56);//int的最高位
    charnum[1] = (unsigned char)((network_byteorder & 0x00ff000000000000ULL) >> 48);//int的次高位
    charnum[2] = (unsigned char)((network_byteorder & 0x0000ff0000000000ULL) >> 40);
    charnum[3] = (unsigned char)((network_byteorder & 0x000000ff00000000ULL) >> 32);
    charnum[4] = (unsigned char)((network_byteorder & 0x00000000ff000000ULL) >> 24);
    charnum[5] = (unsigned char)((network_byteorder & 0x0000000000ff0000ULL) >> 16);
    charnum[6] = (unsigned char)((network_byteorder & 0x000000000000ff00ULL) >> 8); //int的次低位;
    charnum[7] = (unsigned char)((network_byteorder & 0x00000000000000ffULL));    //int的最低位;
    return true;
}

//把char[8]转换为64位的int。int的最高位保存为char[0],最低位保存于char[7]，然后转为主机字节序
inline bool CharToInt64(const unsigned char* charnum, uint64_t& intnum)
{
    intnum = ((uint64_t)charnum[0] << 56) + ((uint64_t)charnum[1] << 48) + ((uint64_t)charnum[2] << 40) + ((uint64_t)charnum[3] << 32) +
        (charnum[4] << 24) + (charnum[5] << 16) + (charnum[6] << 8) + charnum[7];
    intnum = UVNet::ntohll(intnum);//转换为网络字节序
    return true;
}
// 一个数据包的内存结构
//增加包头与包尾数据，用于检测包的完整性。检验值用于检测包的完全性。
//|-----head----|-----pack header---------|--------------------pack data----------|-----tail----|
//|--包头1字节--|--[datalen][type][check]--|--datalen长度的内存数据(根据type去解析)--|--包尾1字节--|
#pragma pack(1)//将当前字节对齐值设为1
typedef struct _NetPacket {//传输自定义数据包头结构
    int32_t datalen;        //包数据的内容长度-不包括此包结构和包头尾  :0-3    
    int32_t type;           //包数据的类型                           :4-7
#ifdef NETPACKET_CHECK
    unsigned char check[16];//pack data校验值-16字节的md5二进制数据   :8-23
#endif // NETPACKET_CHECK
}NetPacket;
#define NET_PACKAGE_HEADLEN sizeof(NetPacket)//包头长度
#pragma pack()//将当前字节对齐值设为默认值(通常是4)

//NetPackage转为char*数据
inline bool NetPacketToChar(const NetPacket& package, unsigned char* chardata) {
    if (!Int32ToChar((uint32_t)package.datalen, chardata)) {
        return false;
    }
    if (!Int32ToChar((uint32_t)package.type, chardata + 4)) {
        return false;
    }
#ifdef NETPACKET_CHECK
    memcpy(chardata + 8, package.check, sizeof(package.check));
#endif // NETPACKET_CHECK
    return true;
}

//char*转为NetPackage数据，chardata必须有38字节的空间
inline bool CharToNetPacket(const unsigned char* chardata, NetPacket& package) {
    uint32_t tmp32;
    if (!CharToInt32(chardata, tmp32)) {
        return false;
    }    
    package.datalen = tmp32;

    if (!CharToInt32(chardata + 4, tmp32)) {
        return false;
    }
    package.type = tmp32;

#ifdef NETPACKET_CHECK
    memcpy(package.check, chardata + 8, sizeof(package.check));
#endif // NETPACKET_CHECK
    return true;
}
#endif

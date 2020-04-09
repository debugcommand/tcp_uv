#ifndef _NET_PACKET_H_
#define _NET_PACKET_H_
#include <stdint.h>
#include "net_base.h"

//��32λ��int������char[4]��.��תΪ�����ֽ���Ȼ��int�����λ����Ϊchar[0],���λ������char[3]
inline bool Int32ToChar(const uint32_t intnum, unsigned char* charnum)
{
    unsigned long network_byteorder = htonl(intnum);//ת��Ϊ�����ֽ���
    charnum[0] = (unsigned char)((network_byteorder & 0xff000000) >> 24);//int�����λ
    charnum[1] = (unsigned char)((network_byteorder & 0x00ff0000) >> 16);//int�Ĵθ�λ
    charnum[2] = (unsigned char)((network_byteorder & 0x0000ff00) >> 8); //int�Ĵε�λ;
    charnum[3] = (unsigned char)((network_byteorder & 0x000000ff));    //int�����λ;
    return true;
}

//��char[4]ת��Ϊ32λ��int��int�����λ����Ϊchar[0],���λ������char[3]��Ȼ��תΪ�����ֽ���
inline bool CharToInt32(const unsigned char* charnum, uint32_t& intnum)
{
    intnum = (charnum[0] << 24) + (charnum[1] << 16) + (charnum[2] << 8) + charnum[3];
    intnum = ntohl(intnum);//ת��Ϊ�����ֽ���
    return true;
}

//��64λ��int������char[8]��.��תΪ�����ֽ���Ȼ��int�����λ����Ϊchar[0],���λ������char[7]
inline bool Int64ToChar(const uint64_t intnum, unsigned char* charnum)
{
    uint64_t network_byteorder = UVNet::htonll(intnum);//ת��Ϊ�����ֽ���
    charnum[0] = (unsigned char)((network_byteorder & 0xff00000000000000ULL) >> 56);//int�����λ
    charnum[1] = (unsigned char)((network_byteorder & 0x00ff000000000000ULL) >> 48);//int�Ĵθ�λ
    charnum[2] = (unsigned char)((network_byteorder & 0x0000ff0000000000ULL) >> 40);
    charnum[3] = (unsigned char)((network_byteorder & 0x000000ff00000000ULL) >> 32);
    charnum[4] = (unsigned char)((network_byteorder & 0x00000000ff000000ULL) >> 24);
    charnum[5] = (unsigned char)((network_byteorder & 0x0000000000ff0000ULL) >> 16);
    charnum[6] = (unsigned char)((network_byteorder & 0x000000000000ff00ULL) >> 8); //int�Ĵε�λ;
    charnum[7] = (unsigned char)((network_byteorder & 0x00000000000000ffULL));    //int�����λ;
    return true;
}

//��char[8]ת��Ϊ64λ��int��int�����λ����Ϊchar[0],���λ������char[7]��Ȼ��תΪ�����ֽ���
inline bool CharToInt64(const unsigned char* charnum, uint64_t& intnum)
{
    intnum = ((uint64_t)charnum[0] << 56) + ((uint64_t)charnum[1] << 48) + ((uint64_t)charnum[2] << 40) + ((uint64_t)charnum[3] << 32) +
        (charnum[4] << 24) + (charnum[5] << 16) + (charnum[6] << 8) + charnum[7];
    intnum = UVNet::ntohll(intnum);//ת��Ϊ�����ֽ���
    return true;
}
// һ�����ݰ����ڴ�ṹ
//���Ӱ�ͷ���β���ݣ����ڼ����������ԡ�����ֵ���ڼ�������ȫ�ԡ�
//|-----head----|-----pack header---------|--------------------pack data----------|-----tail----|
//|--��ͷ1�ֽ�--|--[datalen][type][check]--|--datalen���ȵ��ڴ�����(����typeȥ����)--|--��β1�ֽ�--|
#pragma pack(1)//����ǰ�ֽڶ���ֵ��Ϊ1
typedef struct _NetPacket {//�����Զ������ݰ�ͷ�ṹ
    int32_t datalen;        //�����ݵ����ݳ���-�������˰��ṹ�Ͱ�ͷβ  :0-3    
    int32_t type;           //�����ݵ�����                           :4-7
#ifdef NETPACKET_CHECK
    unsigned char check[16];//pack dataУ��ֵ-16�ֽڵ�md5����������   :8-23
#endif // NETPACKET_CHECK
}NetPacket;
#define NET_PACKAGE_HEADLEN sizeof(NetPacket)//��ͷ����
#define LENGTH_HEADLEN sizeof(int32_t)//ͷ����
#pragma pack()//����ǰ�ֽڶ���ֵ��ΪĬ��ֵ(ͨ����4)

//NetPackageתΪchar*����
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

//char*תΪNetPackage���ݣ�chardata������38�ֽڵĿռ�
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

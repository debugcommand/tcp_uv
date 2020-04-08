/***************************************
* @file     tcp_protocol.h
* @brief    TCP Server protocol pure base class,need to subclass.
* @details  
****************************************/
#ifndef _TCP_PROTOCOL_H_
#define _TCP_PROTOCOL_H_
#include <string>
#include "net_base.h"
#include "packet_sync.h"

class tcp_protocol
{
public:
    tcp_protocol(){}
    virtual ~tcp_protocol(){}

    //parse the recv packet, and make the response packet return. see test_tcpserver.cpp for example
	//packet     : the recv packet
	//buf        : the packet data
	//std::string: the response packet. no response can't return empty string.
    virtual const std::string& ParsePacket(const NetPacket& packet, const unsigned char* buf) = 0;
};

class TestTCPProtocol : public tcp_protocol
{
public:
    TestTCPProtocol() {}
    virtual ~TestTCPProtocol() {}
    virtual const std::string& ParsePacket(const NetPacket& packet, const unsigned char* buf) {        
#if 0        
        static char senddata[256];
        sprintf(senddata, "****recv datalen %d", packet.datalen);
        fprintf(stdout, "%s\n", senddata);
#endif
        NetPacket tmppack = packet;        
        pro_packet_ = PacketData(tmppack, buf);
        return pro_packet_;
    }
private:
    std::string pro_packet_;
};

#endif//_TCP_PROTOCOL_H_
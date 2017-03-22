#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include <time.h>
#include <chrono>
#include <vector>
#include <cassert>
#include <chrono>
#include <string>


#include <iostream>
#include <fstream>
#include <deque>
#include "crc32.h"




#include "PacketHeader.h"

const int BODY_BYTE = 1400;

struct SendLogEntry {
	PacketHeader packetHeader;     // 0: START; 1: END; 2: DATA; 3: ACK
    char* data;   // Described below
    std::chrono::high_resolution_clock::time_point send_time;
};

struct RecvLogEntry {
	PacketHeader packetHeader; 
	char* data;  
};


PacketHeader getPacketHeader(int type, int seqNum = 0, int length = 0,
	 int checksum = 0) {
	PacketHeader result;
	result.type = type;
	result.seqNum = seqNum;
	result.length = length;
	result.checksum = checksum;
	return result;
}

// msghdr getMsg(void* hdr, int hdr_size, void* body, int body_size, sockaddr_in* server, int server_size) {

// 	iovec iov[2];
// 	iov[0].iov_base = hdr;
// 	iov[0].iov_len = hdr_size;
// 	iov[1].iov_base = body;
// 	iov[1].iov_len = body_size;
// 	msghdr msg;
// 	msg.msg_iov = iov;
// 	msg.msg_iovlen = 2;
// 	msg.msg_control = NULL;
// 	msg.msg_controllen = 0;

// 	return msg;

// }

#include "../helper_function/helper.h"

using namespace std;

int main(int argc, char* argv[]) {
// ./wReceiver <port-num> <log> <window-size>

	if(argc != 4){
		cerr << "wrong number of arguments" << endl;
		return -1;
	}

	int port = atoi(argv[1]);
	char* log_path = argv[2];
	int window_size = atoi(argv[3]);

	//create for logging

	ofstream output(string(log_path), std::ofstream::out);


	int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd < 1) {
		std::cout << "Couldn't create a socket\n";
		return 1;
	}

	struct sockaddr_in self;
	memset((char *) &self, 0, sizeof(struct sockaddr_in));
	self.sin_family = AF_INET;
	self.sin_addr.s_addr = INADDR_ANY;
	self.sin_port = htons(port);

	bind(sd, (struct sockaddr*) &self, sizeof(struct sockaddr_in));
	

	char buffer[BODY_BYTE];
	PacketHeader hdr;
	struct sockaddr_in clientAddr;

	iovec iov[2];
	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = buffer;
	iov[1].iov_len = BODY_BYTE;
	msghdr msg;
	msg.msg_name = &clientAddr;
	msg.msg_namelen = sizeof(clientAddr);
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	int file_num = 1;

	int byteSent, bytesRecvd;
	
	while(1) {

		bool end = false, start_data = false;

		unsigned int sliding_start = 0;
		deque<RecvLogEntry> sliding_window;
		
		string filename = "FILE-" + to_string(file_num);

		ofstream os;

		while(!end) {
			bytesRecvd = recvmsg(sd, &msg, 0);
			//create for logging
			output << hdr.type << " " << hdr.seqNum << " " << hdr.length << " " << hdr.checksum << endl;
			if(bytesRecvd <= 0) continue;
			//not curr client continue

			if(!start_data && hdr.type == 0) {//start
				PacketHeader start_ack = getPacketHeader(3, hdr.seqNum);

				byteSent = sendto(sd, &start_ack, sizeof(start_ack), 0, (struct sockaddr *)&clientAddr, sizeof(clientAddr));
				output << start_ack.type << " " << start_ack.seqNum << " " << start_ack.length << " " << start_ack.checksum << endl;



			} else if(hdr.type == 2) {//data
				if(!start_data) {
					start_data = true;
					os.open(filename.c_str(), ofstream::binary);
				}

				//out of sliding window
				if(hdr.seqNum < sliding_start) {
					PacketHeader ack = getPacketHeader(3, hdr.seqNum);
					//cout << "ack seqNum:" << hdr.seqNum << endl;

					byteSent = sendto(sd, &ack, sizeof(ack), 0, (struct sockaddr *) &clientAddr,
						 sizeof(clientAddr));

					output << ack.type << " " << ack.seqNum << " " << ack.length << " " << ack.checksum << endl;
					if(byteSent <= 0) {
						cerr <<"send ack fail" << endl;
					}

					continue;

				} else if(hdr.seqNum >= sliding_start + window_size) continue;

				bool found = false;
				for(auto entry: sliding_window) {
					if(entry.packetHeader.seqNum == hdr.seqNum) {
						found = true;
						break;
					}
				}
				if(found) {
					PacketHeader ack = getPacketHeader(3, hdr.seqNum);

					byteSent = sendto(sd, &ack, sizeof(ack), 0, (struct sockaddr *) &clientAddr,
						 sizeof(clientAddr));

					output << ack.type << " " << ack.seqNum << " " << ack.length << " " << ack.checksum << endl;
					if(byteSent <= 0) {
						cerr <<"send ack fail" << endl;
					}

					continue;
				}

				char* data = new char[sizeof(buffer)];
				memcpy(data, buffer, hdr.length);
				memset(buffer, 0, sizeof(buffer));
				unsigned int checksum = crc32((void*) data, hdr.length);

				//corrupted data
				if(checksum != hdr.checksum){
					cout << "corrupted" << endl;
					delete[] data;
					continue;
				} 

				RecvLogEntry logEntry;
				logEntry.packetHeader = hdr;
				logEntry.data = data;
				sliding_window.push_back(logEntry);

				if(hdr.seqNum == sliding_start) {
					int i = 0;
					for(; i < window_size; i++) {
						bool found = false;
						for(int j = 0; j < sliding_window.size(); j++) {
							if(sliding_window[j].packetHeader.seqNum
								 == sliding_start + i) {
								found = true;
								char* towrite = sliding_window[j].data;
								u_int write_size = sliding_window[j].packetHeader.length;
								//cout << "write "<< sliding_start + i << endl;
								os.write(towrite, write_size);
								delete[] towrite;
								sliding_window.erase(sliding_window.begin() + j);

								break;
							}
						}
						if(!found) break;
					}
					sliding_start += i;
				}

				PacketHeader ack = getPacketHeader(3, hdr.seqNum);
				
				byteSent = sendto(sd, &ack, sizeof(ack), 0, (struct sockaddr *) &clientAddr,
					 sizeof(clientAddr));

				//create for logging

				output << ack.type << " " << ack.seqNum << " " << ack.length << " " << ack.checksum << endl;
				if(byteSent <= 0) {
					cerr <<"send ack fail" << endl;
				}

			} else if(hdr.type == 1) {//end
				PacketHeader end_ack = getPacketHeader(3, hdr.seqNum);

				byteSent = sendto(sd, &end_ack, sizeof(end_ack), 0, (struct sockaddr *) &clientAddr,
					 sizeof(clientAddr));
				//create for logging

				output << end_ack.type << " " << end_ack.seqNum << " " << end_ack.length << " " << end_ack.checksum << endl;

				if(byteSent <= 0) {
					cerr <<"send ack fail" << endl;
				}
				end = true;
				sliding_start = 0;
				sliding_window.clear();

			} else continue;

		}
		os.close();

		file_num++;

	}

	return 0;
}

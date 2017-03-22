#include "../helper_function/helper.h"

using namespace std;

int main(int argc, char* argv[]) {
// ./wSender <input-file> <window-size> <log> <receiver-IP> <receiver-port>

	if(argc != 6){
		cerr << "wrong number of arguments" << endl;
		return -1;
	}
	char* input_file = argv[1];
	int window_size = atoi(argv[2]);
	char* log_path = argv[3];
	char* receiver_ip = argv[4];
	int receiver_port = atoi(argv[5]);
	
	//create for logging

	ofstream output(string(log_path), std::ofstream::out);

	ifstream is(input_file, std::ifstream::binary);

	int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd < 1) {
		cout << "Couldn't create a socket\n";
		return 1;
	}

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons((u_short) receiver_port);
	server.sin_addr.s_addr = inet_addr(receiver_ip);

	int err = connect(sd, (sockaddr*) &server, sizeof(server));
	if(err == -1) {
		cout << "Error on connect\n";
		return 1;
	}

	PacketHeader hdr = getPacketHeader(0);

	iovec iov[2];
	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;
	msghdr msg;
	msg.msg_name = &server;
	msg.msg_namelen = sizeof(server);
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;

	auto start_send_time = chrono::high_resolution_clock::now();
	bool start_send_first = true;
	while(1) {
		auto start_curr_time = chrono::high_resolution_clock::now();
		auto dur = start_curr_time - start_send_time;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (dur).count();
		if(ms > 500 || start_send_first) {
			//send start message
			int bytesSent = sendmsg(sd, &msg, 0);
			start_send_time = chrono::high_resolution_clock::now();
			//create for logging
			output << hdr.type << " " << hdr.seqNum << " " << hdr.length << " " << hdr.checksum << endl;

			if(bytesSent <= 0) {
				cout << "Error using send" << endl;
				return 1;
			}
			start_send_first = false;
		}
		//recv start acknowledge
		PacketHeader ack;
		int bytesRecvd = recv(sd, &ack, sizeof(ack), MSG_DONTWAIT);
		if(bytesRecvd > 0) {
			//create for logging
			output << ack.type << " " << ack.seqNum << " " << ack.length << " " << ack.checksum << endl;
			assert(ack.seqNum == hdr.seqNum);
			break;
		}
	}

	int seq = 0;

	deque<SendLogEntry> sliding_window;

	chrono::high_resolution_clock::time_point send_time;

	while(is || (!sliding_window.empty())) {
		auto curr_time = chrono::high_resolution_clock::now();
		if(!sliding_window.empty()) {
			auto dur = curr_time - send_time;
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (dur).count();
			if(ms > 500) {
				//resend
				char* body = sliding_window[0].data;
				hdr = sliding_window[0].packetHeader;
				iov[0].iov_base = &hdr;
				iov[0].iov_len = sizeof(hdr);
				iov[1].iov_base = body;
				iov[1].iov_len = hdr.length;
				msg.msg_iov = iov;
				msg.msg_iovlen = 2;

				sliding_window[0].send_time = chrono::high_resolution_clock::now();

				err = sendmsg(sd, &msg, 0);

				//create logging

				output << hdr.type << " " << hdr.seqNum << " " << hdr.length << " " << hdr.checksum << endl;

				if(err <= 0) {
					cout << "Error using sendmsg" << endl;
					return 1;
				}
			}

		}

		if(is && sliding_window.size() < window_size) {

			int length = BODY_BYTE;
			char* body = new char [length];
			is.read(body, length);

			//unsigned int sendsize = min(length, (int)strlen(body));
			u_int sendsize = length;
			if(!is) {
				unsigned int readsize = is.gcount();

				sendsize = min(sendsize, readsize);
			}
			
			unsigned int checksum = crc32((void *)body, sendsize);
			hdr = getPacketHeader(2, seq, sendsize, checksum);

			iov[0].iov_base = &hdr;
			iov[0].iov_len = sizeof(hdr);
			iov[1].iov_base = body;
			iov[1].iov_len = hdr.length;
			msg.msg_iov = iov;
			msg.msg_iovlen = 2;

			SendLogEntry logEntry;
			logEntry.packetHeader = hdr;
			logEntry.data = body;
			logEntry.send_time = chrono::high_resolution_clock::now();

			err = sendmsg(sd, &msg, 0);
			//create for logging

			output << hdr.type << " " << hdr.seqNum << " " << hdr.length << " " << hdr.checksum << endl;

			if(err <= 0) {
				cout << "Error using sendmsg" << endl;
				return 1;
			}

			if(sliding_window.empty()) {
				send_time = logEntry.send_time;
			}

			sliding_window.push_back(logEntry);

			seq++;

		}

		if(!sliding_window.empty()) {
			PacketHeader ack;
			int bytesRecvd = recv(sd, &ack, sizeof(ack), MSG_DONTWAIT);
			if(bytesRecvd > 0){
				//create for logging
				output << ack.type << " " << ack.seqNum << " " << ack.length << " " << ack.checksum << endl;				
				while(!sliding_window.empty() && 
						ack.seqNum > sliding_window[0].packetHeader.seqNum) {
					delete[] sliding_window.front().data;
					sliding_window.pop_front();

					if(!sliding_window.empty()) {
						send_time = sliding_window.front().send_time;
					}

				}
			}
		}

	}

	hdr = getPacketHeader(1);

	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = 2;

	auto end_send_time = chrono::high_resolution_clock::now();
	bool end_send_first = true;
	while(1) {
		auto end_curr_time = chrono::high_resolution_clock::now();
		auto dur = end_curr_time - end_send_time;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds> (dur).count();
		if(ms > 500 || end_send_first) {
			//send start message
			int bytesSent = sendmsg(sd, &msg, 0);	
			end_send_time = chrono::high_resolution_clock::now();
			//create for logging

			output << hdr.type << " " << hdr.seqNum << " " << hdr.length << " " << hdr.checksum << endl;
			if(bytesSent <= 0) {
				cout << "Error using send" << endl;
				return 1;
			}
			end_send_first = false;
		}

		//recv start acknowledge
		PacketHeader ack;
		int bytesRecvd = recv(sd, &ack, sizeof(ack), MSG_DONTWAIT);

		if(bytesRecvd > 0) {

			//create for logging

			output << ack.type << " " << ack.seqNum << " " << ack.length << " " << ack.checksum << endl;
			if(ack.seqNum == hdr.seqNum) {
				break;
			}

		}
	}

	is.close();

	return 0;

}

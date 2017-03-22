CC = g++
CFLAGS = -std=c++11

all: wSender wReceiver wSender_opt wReceiver_opt

wSender: WTP-base/wSender.cpp
	$(CC) WTP-base/wSender.cpp -o WTP-base/wSender $(CFLAGS)

wReceiver: WTP-base/wReceiver.cpp
	$(CC) WTP-base/wReceiver.cpp -o WTP-base/wReceiver $(CFLAGS) 

wSender_opt: WTP-opt/wSender_opt.cpp
	$(CC) WTP-opt/wSender_opt.cpp -o WTP-opt/wSender_opt $(CFLAGS) 

wReceiver_opt: WTP-opt/wReceiver_opt.cpp
	$(CC) WTP-opt/wReceiver_opt.cpp -o WTP-opt/wReceiver_opt $(CFLAGS) 
	
clean:
	-rm -f WTP-base/wReceiver WTP-base/wSender WTP-opt/wSender_opt WTP-opt/wReceiver_opt FILE-* log_*
	
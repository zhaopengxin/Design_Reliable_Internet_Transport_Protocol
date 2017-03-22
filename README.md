# Design Reliable Internet Transport Protocol

1. Our transport protocol is at the forth layer of internet, which has the ability of providing in­order, reliable delivery in the presence of events like packet loss, delay, corruption, duplication, and re­ordering.

2. User can appoint the `sliding window` size of receiver of sender side.

3. Optimal sender and receiver will optimize the send rate of the transportation.

### Running `wSender`
`wSender` should be invoked as follows:

`./wSender <input-file> <window-size> <log> <receiver-IP> <receiver-port>`

* `input-file` Path to the file that has to be transferred. It can be a text as well as a binary file (e.g., image or video).
* `window-size` Maximum number of outstanding packets.
* `log` The file path to which you should log the messages as described below.
* `receiver-IP` The IP address of the host that `wReceiver` is running on.
* `receiver-port` The port number on which `wReceiver` is listening.

### Logging
`wSender` should create a log of its activity. After sending or receiving each packet, it should append the following line to the log (i.e., everything except the `data` of the `packet` structure described earlier):

`<type> <seqNum> <length> <checksum>`

### Running `wReceiver`
`wReceiver` should be invoked as follows:
`./wReceiver <port-num> <log> <window-size>`

* `port-num` The port number on which `wReceiver` is listening for data.
* `log` The file path to which you should log the messages as described above.
* `window-size` Maximum number of outstanding packets.

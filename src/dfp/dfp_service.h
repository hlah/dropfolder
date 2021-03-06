#include <netinet/in.h>
#include <queue>          // std::queue
#include <vector>
#include <string.h>
#include <sys/types.h>

#define NUM_BUFFERS 64

#define MAX_PAYLOAD_LEN 16

#define UDP_PACKET_MAX_SIZE (64*1024)

#define PACKET_TYPE_DATA 0
#define PACKET_TYPE_ACK 1

#define STATE_IDLE 0
#define STATE_RECEIVING 1

using namespace std;

typedef struct packet{
	uint16_t type;          // ack or data
	uint16_t msgID;         //current message ID
	uint32_t seqn;              //current message seq number offset
	uint32_t total_size;        //packet total size
	uint16_t payload_length;    //payload length
	const char *_payload;
} packet;

typedef struct st_message{
	vector<uint8_t> data;
	sockaddr_in src_addr;
} message;

typedef struct st_ackPacket{
    packet header;
	sockaddr_in src_addr;
} ackPacket;

#if 0
class st_buffer{
	int state;
	message msg;
	public:
	void process_packet(packet pckt);
};
#endif

using namespace std;
class dfp_service
{
	pthread_t th_recv;
	pthread_mutex_t msgQueue_mutex;
	queue<message> messageQueue;

    queue<ackPacket> ackQueue;
    int state;
	const sockaddr_in* transmitting_addr;
    uint16_t msgID;
	int port;
	int sockfd, n;
	struct sockaddr_in* dest_addr;
	uint8_t buf[UDP_PACKET_MAX_SIZE];


	void send_ACK();
	void send_ACK(struct sockaddr_in* dest_addr);
	vector<uint8_t>*transmitting_packet;
    static vector<vector<uint8_t>> split_message(uint16_t msgID, uint8_t* message, uint32_t len);
	void send_segment(vector<uint8_t>*segment, const sockaddr_in* dest_addr);
    void WAIT_ACK();
	bool not_binded(void);
	bool accepting_client(sockaddr_in *dest_addr);
	static void* retransmit_thread(void*arg);
	void retransmit();


	public:

	~dfp_service(void);
	dfp_service(int port);
	dfp_service(int port, struct sockaddr *dest_addr);
	
	void receive_thread(void);

	int sendmessage(const void* buf, size_t len);
	int sendmessage(const void* buf, size_t len, const struct sockaddr_in *dest_addr);
	
	vector<uint8_t> recvmessage(struct sockaddr_in *src_addr = NULL);

	int numMessages(void);
};

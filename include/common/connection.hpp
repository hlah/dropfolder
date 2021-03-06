#ifndef _DROPFOLDER_CONNECTION_HPP_
#define _DROPFOLDER_CONNECTION_HPP_

#include <netdb.h>
#include <memory>
#include <queue>

struct ReceivedData {
    std::unique_ptr<uint8_t[]> data;
    size_t length;

    ReceivedData()
        : data{}, length{0} {}
    ReceivedData( std::unique_ptr<uint8_t[]>&& data, size_t length )
        : data{std::move(data)}, length{length} {}
};

enum class ConnMode{
    Normal,
    Promiscuous,
};

class Connection {
    public:
        // try to connect to a server
        static std::shared_ptr<Connection> connect(ConnMode mode, const std::string&, int port, int timelimit=1000);
        // wait for a connection
        static std::shared_ptr<Connection> listen(int port, int min_range=10000, int max_range=40000, int timelimit=5000);

        //reconstruct a connection
        static std::shared_ptr<Connection> reconnect(ConnMode mode, uint32_t peerIP, uint16_t peerPort, 
                                                            int min_range=10000, int max_range=40000, int timelimit=5000);
		

		void receive_thread();

        // receive data (non blocking);
        ReceivedData receive(int receive_timelimit = 0);
        // send data (blocking)
        void send(uint8_t* data, size_t size);

        // return remote ip/port
		uint32_t getPeerIP() const { return ntohl(_other.sin_addr.s_addr); }
        uint16_t getPeerPort() const { return ntohs(_other.sin_port); }

		// set remote ip/port
		void setPeerIP(uint32_t ip) { _other.sin_addr.s_addr= htonl(ip); }
        void setPeerPort(uint16_t port) { _other.sin_port = htons(port); }

		// return local Ip/port
		std::vector<uint32_t>getIP();
        uint16_t getPort();

        bool hasNewMessage(){ return !recvQueue.empty(); }

        // Destructor
        ~Connection();
        // move constructor
        Connection( Connection&& conn ) noexcept;
        Connection& operator=( Connection&& conn );

    private:
        // packet type enum
        enum class PacketType : uint8_t {
            Null,
            Data,
            Ack,
            NewPort,
            Conn,
			ChangeServerAddr
        };

		enum class recvState : uint8_t {
			Waiting,
			RetrievingData
		};

        // packet struct
        struct Packet {
            PacketType type;
            uint16_t msg_id;         // for PacketType::NewPort is the value of the new port
            uint32_t seqn;       
            uint32_t total;
            uint16_t payload_length;
            char* data;

            Packet( PacketType type=PacketType::Null, uint16_t msg_id=0, uint32_t seqn=0, uint32_t total=1, uint16_t payload_length=0 ) 
                : type{type}, msg_id{msg_id}, seqn{seqn}, total{total}, payload_length{payload_length} {}
        };

        Connection(ConnMode mode, int socket_fd, sockaddr_in other, int timelimit,  int trylimit=5);
        int _socket_fd = 0;
        ConnMode mode;

		void sendAck(Packet* packet, sockaddr_in *sender_addr);

		pthread_cond_t ackQueueCond;
		pthread_mutex_t ackQueueMutex;
		pthread_mutex_t recvQueueMutex;
		pthread_t _recv_thread;
        bool _quit_thread=false;
		std::queue<ReceivedData> recvQueue;
		std::queue<Packet*> ackQueue;

        uint16_t _next_msg_id = 0;
        sockaddr_in _other;
        int _timelimit;
        int _trylimit;
        Connection( const Connection& other ) = delete;
        Connection& operator=( const Connection& other ) = delete;

		template<class T>
		int poll_queue(std::queue<T>* a_queue, pthread_cond_t* cond, pthread_mutex_t* mutex, int _timelimit);

        // helper funciton to throw errors
        static void throw_errno( const std::string& str );
        // socket builder
        static int build_socket();
        // get host by name
        static hostent* get_host(const std::string& hostname);
        // poll socket for messages
        static bool poll_socket( int socketfd, int timelimit );
        

};

class ConnectionException : public std::exception {
    public:
        ConnectionException(std::string msg) : _msg{msg} {}
        virtual const char* what() const noexcept override { return _msg.c_str(); }
    private:
        std::string _msg;
};

#endif // _DROPFOLDER_CONNECTION_HPP_





#ifndef _DROPFOLDER_CONNECTION_HPP_
#define _DROPFOLDER_CONNECTION_HPP_

#include <netdb.h>
#include <memory>

struct ReceivedData {
    std::unique_ptr<uint8_t[]> data;
    size_t length;

    ReceivedData()
        : data{}, length{0} {}
    ReceivedData( std::unique_ptr<uint8_t[]>&& data, size_t length )
        : data{std::move(data)}, length{length} {}
};

class Connection {
    public:
        // try to connect to a server
        static Connection connect(const std::string&, int port, int timelimit=5000);
        // wait for a connection
        static Connection listen(int port, int min_range=10000, int max_range=40000, int timelimit=5000);

        // receive data (non blocking);
        ReceivedData receive();
        // send data (blocking)
        void send(uint8_t* data, size_t size);

        // return remote port
        int port() const { return ntohs(_other.sin_port); }


        // Destrouctor
        ~Connection();
    private:
        Connection(int socket_fd, sockaddr_in other) 
            : _socket_fd{socket_fd}, _other{other} {}
        int _socket_fd;
        sockaddr_in _other;

        // packet type enum
        enum class PacketType : uint8_t {
            Null,
            Data,
            Ack,
            NewPort,
            Conn
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





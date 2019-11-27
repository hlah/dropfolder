#ifndef _DROPFOLDER_PEERS_REGISTER_HPP_
#define _DROPFOLDER_PEERS_REGISTER_HPP_

#include <string>
//#include <iostream>
#include <sstream>

struct PeersReg{
		std::string type;
		std::string username;
		uint32_t ip_addr;
		uint16_t ip_port;
        bool operator < (PeersReg j){
            if(ip_addr == j.ip_addr){
                return (ip_port < j.ip_port);
            }
            return ip_addr < j.ip_addr;

        }
        bool operator == (PeersReg j){
            return (ip_addr==j.ip_addr && ip_port==j.ip_port);
        }
};


std::ostream& operator<<(std::ostream& out, const PeersReg& reg)
{
     return out << reg.type << " " << reg.username << " " << reg.ip_addr << " " << reg.ip_port;
}

std::istream& operator>>(std::istream& in, PeersReg& reg) // non-const h
{
    PeersReg values; // use extra instance, for setting result transactionally
    in >> values.type >> values.username >> values.ip_addr >> values.ip_port;
	reg = std::move(values);
    return in;
}

#endif

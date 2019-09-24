#include "common.hpp"
#include <sstream>
#include <iterator>

std::vector<std::string> split( const std::string& str ) {
    std::istringstream iss{ str };
    std::vector<std::string> words{ 
        std::istream_iterator<std::string>{iss},
        std::istream_iterator<std::string>{}
    };

    return words;
}

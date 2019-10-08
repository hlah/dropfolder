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

std::string basename( const std::string& str ) {
    auto last = str.rfind( "/" );
    if( last != std::string::npos ) {
        return std::string{ str, last+1 };
    } else {
        return std::string{ str };
    }
}

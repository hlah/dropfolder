#include "common.hpp"
#include <sstream>
#include <iterator>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>

#include <iostream>

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

std::string printdir( const std::string& dirname ) {
    std::stringstream strs;

    for( auto filename : listdir( dirname ) ) {
        struct stat stat_result;
        std::string filepath = dirname + std::string{"/"} + filename;
        stat( filepath.c_str(), &stat_result );
        auto atime = localtime( &stat_result.st_atim.tv_sec );
        auto mtime = localtime( &stat_result.st_mtim.tv_sec );
        auto ctime = localtime( &stat_result.st_ctim.tv_sec );

        strs << filename << "\n" 
            << "atime:\t" << asctime(atime)
            << "mtime:\t" << asctime(mtime) 
            << "ctime:\t" << asctime(ctime) << std::endl;
    }

    return strs.str();
}

std::vector<std::string> listdir( const std::string& dirname ) {
    std::vector<std::string> files;
    auto dir = opendir( dirname.c_str() );
    dirent* entry;
    while( (entry = readdir(dir)) != NULL ) {
        if(entry->d_type == DT_REG) {
            files.push_back( entry->d_name );
        }
    }
    return files;
}

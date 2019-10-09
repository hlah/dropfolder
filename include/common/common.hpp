#include <string>
#include <vector>

// Split string by white spaces.
std::vector<std::string> split( const std::string& str );

// get basename
std::string basename( const std::string& str );

// list files in a directory
std::vector<std::string> listdir( const std::string& dirname );

// print files in a directory with information
std::string printdir( const std::string& dirname );

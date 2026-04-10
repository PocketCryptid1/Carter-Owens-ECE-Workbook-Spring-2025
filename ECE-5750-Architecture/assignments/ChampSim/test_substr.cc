#include <iostream>
#include <string>

int main() {
    std::string full_name = "602.gcc_s-734B.champsimtrace.xz";
    size_t pos = full_name.find_last_of(".");
    std::cout << "Position of last dot: " << pos << std::endl;
    std::cout << "String length: " << full_name.length() << std::endl;
    
    if (pos != std::string::npos) {
        std::string last_dot = full_name.substr(pos);
        std::cout << "Substring from last dot: " << last_dot << std::endl;
        std::cout << "Character at pos+1: " << last_dot[1] << std::endl;
    } else {
        std::cout << "No dot found!" << std::endl;
    }
    
    return 0;
}

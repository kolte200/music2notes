#include "error.h"


void err(std::string str, int code) {
    std::cerr << "Error : " << str << std::endl;
    exit(code);
}

#include <iostream>
#include <stdexcept>
#include <memory>               // shared_ptr
#include <errno.h>
#include <string.h>
#include "../common/socket.hpp"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "usage: " << argv[0] << " host port" << std::endl;
        return 0;
    }

    try
    {
        std::string host(argv[1]);
        int port = std::stoi(argv[2]);

        Socket s;
        s.setNonBlocked(true);
        s.connect(host, port);

        std::cerr << "errno: " << errno << " [" << strerror(errno) << "]\n";

    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

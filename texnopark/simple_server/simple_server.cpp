#include <iostream>
#include <stdexcept>
#include <thread>
#include "../common/socket.hpp"

void client_work(std::shared_ptr<Socket> client)
{
    client->setRcvTimeout(/*sec*/30, /*microsec*/0);
    while (true) try
    {
        std::string line = client->recv();
        client->send("echo: " + line + "\n");
    }
    catch(const std::exception &e)
    {
        std::cerr << "exception: " << e.what() << std::endl;
        return;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "usage: " << argv[0] << " port" << std::endl;
        return 0;
    }
    int port = std::stoi(std::string(argv[1]));

    try
    {
        Socket s;
        s.createServerSocket(port, 1);

        while(true)
        {
            std::shared_ptr<Socket> client = s.accept();
            client_work(client);
        }
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

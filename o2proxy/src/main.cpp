#include <iostream>

#include "server.hpp"
#include "logger.hpp"

int main(int argc, char *argv[])
{
    try
    {
        Options opt(argc, argv);
        opt.dump();
        Config conf("");

        Server serv(opt, conf);
        serv.run();
        return 0;
    }
    catch (const std::exception &e)
    {
        logi("Terminated: {0}", e.what());
    }
    return 0;
}

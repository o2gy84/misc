#include <iostream>

#include "server.hpp"
#include "logger.hpp"
#include "config.hpp"

std::string g_Version = "0.0.2";

int main(int argc, char *argv[])
{
    logi("o2proxy version: ", g_Version);
    try
    {
        Options opt;
        opt.add("help", "h", "print help and exit", false);
        opt.add("port", "p", "port to listen to (1025..65536)", 7788);
        opt.add("loglevel", "l", "loglevel (1..5)", 0);
        opt.add("config", "c", "path to config", "");
        opt.parse(argc, argv);

        Logger::get().logLevel(opt.get<int>("loglevel"));
        opt.dump();

        Config::get()->load(opt.get<std::string>("config"));

        Server s(opt);
        s.run();
        return 0;
    }
    catch (const std::exception &e)
    {
        loge("terminated: ", e.what());
    }
    return 0;
}

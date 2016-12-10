#include <iostream>

#include "server.hpp"
#include "logger.hpp"
#include "config.hpp"

static const std::string g_Version = "0.0.2";

int main(int argc, char *argv[])
{
    logi("o2proxy version: ", g_Version);
    try
    {
        Options opt(argc, argv);
        Logger::get().logLevel(opt._log_level);

        opt.dump();
        Config::get()->load(opt._config);

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

#include <iostream>

#include "server.hpp"
#include "logger.hpp"

int main(int argc, char *argv[])
{
    try
    {
        Options opt(argc, argv);
        Logger::get().logLevel(opt._log_level);

        opt.dump();
        Config conf(opt._config);

        Server s(opt, conf);
        s.run();
        return 0;
    }
    catch (const std::exception &e)
    {
        loge("terminated: ", e.what());
    }
    return 0;
}

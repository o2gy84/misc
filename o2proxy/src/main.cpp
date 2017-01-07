#include <iostream>

#include "server.hpp"
#include "logger.hpp"
#include "config.hpp"

std::string g_Version = "0.1.1";

void init_config(const std::string &path, Config *conf)
{
    conf->add("port", "port to listen to", 7788);
    conf->add("engine", "input/output engine: select, poll or epoll", "epoll");
    conf->add("local_address", "local address", "");
    //conf->add("test1", "test 1", std::vector<int>());
    //conf->add("test2", "test 2", std::vector<std::string>());

    conf->load(path);
    conf->dump();
    logd5("", conf->usage());

    /*
    std::vector<int> test1 = conf->get<std::vector<int>>("test1");
    for (size_t i = 0; i < test1.size(); ++i)
    {
        logi("test[{0}] = {1}", i, test1[i]);
    }
    std::vector<std::string> test2 = conf->get<std::vector<std::string>>("test2");
    for (size_t i = 0; i < test2.size(); ++i)
    {
        logi("test[{0}] = {1}", i, test2[i]);
    }
    */
}

int main(int argc, char *argv[])
{
    try
    {
        Options opt;
        opt.add("help", "h", "print help and exit", false);
        opt.add("port", "p", "port to listen to (1025..65536)", 0);
        opt.add("loglevel", "l", "loglevel (1..5)", 0);
        opt.add("config", "c", "path to config", "");
        opt.add("syslog", "", "write logs into syslog", false);

        opt.parse(argc, argv);

        Logger::get().setOptionLogLevel(opt.get<int>("loglevel"));
        Logger::get().setOptionSyslog(argv[0], opt.get<bool>("syslog"));

        logi("o2proxy version: ", g_Version);

        opt.dump();

        init_config(opt.get<std::string>("config"), Config::impl());

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

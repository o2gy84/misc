#include <stdexcept>
#include <sstream>
#include <iostream>

#include "options.hpp"
#include "utils.hpp"



namespace 
{
    const std::string default_conf = "/etc/o2proxy.conf";


    const std::string opt_port      = "-p";
    const std::string opt_cfg       = "-c";
    const std::string opt_log       = "-l";

    void usage(const std::string &progname)
    {
        std::stringstream ss;
        ss << std::endl << "\tUsage: " 
            << progname << " "
            << opt_port << " port "
            << opt_cfg << " path"
            << std::endl << std::endl;

        ss  << "\t" << opt_port << ": 1025..65536" << std::endl
            << "\t" << opt_cfg << ": path to config" << std::endl;
        
        std::cerr << ss.str() << std::endl;
   }
}


Options::Options(int count, const char *const *args)
{
    _port = 0;
    _config = default_conf;
    _log_level = 0;

    if (count == 1)
    {
        // use only config
        return;
    }

    if (count%2 == 0)
    {
        usage(args[0]);
        throw std::runtime_error("bad options");
    }
    
    for (int i = 1; i < count; i += 2)
    {
        if (args[i] == opt_port)
        {
            _port = std::stoi(args[i+1]);
        }
        else if (args[i] == opt_cfg)
        {
            _config = args[i+1];
        }
        else if (args[i] == opt_log)
        {
            _log_level = std::stoi(args[i+1]);
        }
        else
        {
            usage(args[0]);
            throw std::runtime_error("bad options");
        }
    }
}

void Options::dump() const noexcept
{
    if (_log_level == 0)
        return;

    std::cerr << "[LOG1] Options:\n"
        << "\tlog_level: " << _log_level << "\n"
        << "\tport: " << _port << "\n"
        << "\tconfig: " << _config << "\n"
        ;
}


#include <stdexcept>
#include <sstream>
#include <iostream>

#include "options.hpp"
#include "utils.hpp"
#include "logger.hpp"


namespace options
{
    const std::string default_conf = "/etc/o2proxy.conf";

    const std::string port      = "-p";
    const std::string cfg       = "-c";
    const std::string loglevel  = "-l";

    void usage(const std::string &progname)
    {
        std::stringstream ss;
        ss << std::endl
            << "\tUsage: " 
            << progname
            << " " << port << " port"
            << " " << cfg << " path"
            << " " << loglevel << " loglevel"
            << std::endl << std::endl;

        ss  << "\t" << port << ": 1025..65536" << std::endl
            << "\t" << cfg << ": path to config" << std::endl
            << "\t" << loglevel << ": 1..5" << std::endl;

        std::cerr << ss.str() << std::endl;
   }
}


void Options::init() noexcept
{
    _port = 0;
    _config = options::default_conf;
    _log_level = 0;
}

Options::Options(int count, const char *const *args)
{
    init();

    if (count == 1)
    {
        // use only config
        return;
    }

    if (count%2 == 0)
    {
        options::usage(args[0]);
        throw std::runtime_error("bad options");
    }
    
    for (int i = 1; i < count; i += 2)
    {
        if (args[i] == options::port)
        {
            _port = std::stoi(args[i+1]);
        }
        else if (args[i] == options::cfg)
        {
            _config = args[i+1];
        }
        else if (args[i] == options::loglevel)
        {
            _log_level = std::stoi(args[i+1]);
        }
        else
        {
            options::usage(args[0]);
            throw std::runtime_error("bad options");
        }
    }
}

void Options::dump() const noexcept
{
    if (_log_level == 0)
        return;

    std::stringbuf str;
    std::ostream os(&str);

    os << "Options dump:\n"
        << "\tlog_level: " << _log_level << "\n"
        << "\tport: " << _port << "\n"
        << "\tconfig: " << _config << "\n"
        ;
   logd1(str.str());
}


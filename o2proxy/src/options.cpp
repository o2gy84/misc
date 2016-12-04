#include <stdexcept>
#include <sstream>
#include <iostream>

#include "options.hpp"
#include "utils.hpp"
#include "logger.hpp"


namespace options
{
    const std::string port      = "-p";
    const std::string cfg       = "-c";
    const std::string loglevel  = "-l";
    const std::string help      = "-h";

    void usage(const std::string &progname)
    {
        std::stringstream ss;
        ss << std::endl
            << "\tUsage: " 
            << progname
            << " " << port << " port"
            << " " << cfg << " config"
            << " " << loglevel << " loglevel"
            << std::endl << std::endl;

        ss  << "\t" << port << ": 1025..65536" << std::endl
            << "\t" << cfg << ": path to config" << std::endl
            << "\t" << loglevel << ": 1..5" << std::endl
            << "\t" << help << ": print this help and exit" << std::endl;

        logi(ss.str());
   }
}


void Options::init() noexcept
{
    _port = 0;
    _log_level = 0;
    _config = "";
}

const char* nextOpt(int &counter, int total_opts, const char *const *args)
{
    if (counter + 1 >= total_opts)
    {
        options::usage(args[0]);
        throw std::runtime_error("bad options");
    }

    return args[counter++ + 1];
}

Options::Options(int count, const char *const *args)
{
    init();

    if (count == 1)
    {
        // use only config
        return;
    }

    int counter = 1;
    while (true)
    {
        if (counter >= count) break;

        if (args[counter] == options::port)
        {
            _port = std::stoi(nextOpt(counter, count, args));
        }
        else if (args[counter] == options::cfg)
        {
            _config = nextOpt(counter, count, args);
        }
        else if (args[counter] == options::loglevel)
        {
            _log_level = std::stoi(nextOpt(counter, count, args));
        }
        else if (args[counter] == options::help)
        {
            options::usage(args[0]);
            exit(0);
        }
        else
        {
            options::usage(args[0]);
            throw std::runtime_error("bad options");
        }
        ++counter;
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


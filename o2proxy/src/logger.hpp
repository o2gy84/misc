#pragma once

#include <iostream>
#include <sstream>
#include <initializer_list>
#include <utility>
#include <tuple>
#include <string>
#include <assert.h>


/*
    logi - [i] - info
    logw - [w] - warning
    loge - [e] - error
    logd - [d] - debug [logd1, logd2, logd3, logd4, logd5]
*/


// can log into stdout, file, stream, syslog...
class Logger
{
public:
    static Logger& get();

    // TODO: file, stream, syslog, loglevel :)
    template <typename T>
    void log(char prefix, int log_level, T val) const
    {
        if (_log_level < log_level) return;
        std::cout << "[" << prefix << "] " << val << std::endl;
    }
    template <typename K, typename V>
    void log(char prefix, int log_level, K &key, V &val) const
    {
        if (_log_level < log_level) return;
        std::cout << "[" << prefix << "] " << key << val << std::endl;
    }

    void setOptionLogLevel(uint16_t level);
    void setOptionSyslog(bool syslog);

private:
    Logger();
    Logger(const Logger &);
    Logger& operator=(const Logger &);
private:
    uint16_t _log_level;
    bool _syslog;
};


namespace logger
{

// get number from braced string like "{123}"
int braced_number(const std::string &s);

template <std::size_t N, typename... Tp>
typename std::enable_if<(N < sizeof...(Tp)), void>::type print_tuple_at(const std::tuple<Tp...> &t, std::ostream &os)
{
    os << std::get<N>(t);
}

template <std::size_t N, typename... Tp>
typename std::enable_if< (N >= sizeof...(Tp)), void>::type print_tuple_at(const std::tuple<Tp...> &, std::ostream &os)
{
    os << "nil";
}

// logging without args, string, char* e.t.c.
template<typename T>
void logi_impl(char prefix, int log_level, T &text)
{
    const Logger &l = Logger::get();
    l.log(prefix, log_level, text);
}

// logging in "key: value" style
template<typename T>
void logi_impl2(char prefix, int log_level, const std::string &key, T val)
{
    Logger::get().log(prefix, log_level, key, val);
}

// logging in style like: string.format("key: {0}, value: {1}", key, val)
template <typename ...Args>
void log_impl3(char prefix, int log_level, const std::string &format, Args&&... args)
{
    std::tuple<Args...> list(args...);

    std::stringbuf stringbuf;
    std::ostream os(&stringbuf);

    std::string::size_type last_pos = 0;
    std::string::size_type open_brace = format.find('{', 0);
    while (open_brace != std::string::npos)
    {
        std::string::size_type close_brace = format.find('}', open_brace + 1);
        if (close_brace == std::string::npos)
        {
            open_brace = format.find('{', open_brace + 1);
            continue;
        }

        int tuple_arg = logger::braced_number(std::string(format.begin() + open_brace + 1, format.begin() + close_brace));
        if (tuple_arg == -1)
        {
            open_brace = format.find('{', open_brace + 1);
            continue;
        }

        os << std::string(format.begin() + last_pos, format.begin() + open_brace);
        switch (tuple_arg)
        {    
            case 0: logger::print_tuple_at<0>(list, os); break;
            case 1: logger::print_tuple_at<1>(list, os); break;
            case 2: logger::print_tuple_at<2>(list, os); break;
            case 3: logger::print_tuple_at<3>(list, os); break;
            case 4: logger::print_tuple_at<4>(list, os); break;
            case 5: logger::print_tuple_at<5>(list, os); break;
            case 6: logger::print_tuple_at<6>(list, os); break;
            case 7: logger::print_tuple_at<7>(list, os); break;
            case 8: logger::print_tuple_at<8>(list, os); break;
            case 9: logger::print_tuple_at<9>(list, os); break;
            default: throw std::runtime_error("logger: too many arguments (max = 9)");
        }

        last_pos = close_brace + 1;
        open_brace = format.find('{', close_brace + 1);
    }
    os << std::string(format.begin() + last_pos, format.end());
    Logger::get().log(prefix, log_level, stringbuf.str());
}


}   // namespace

template<typename T> void logi(T &text)
{
    logger::logi_impl('i', 0, text);
}
template<typename T> void loge(T &text)
{
    logger::logi_impl('e', 0, text);
}
template<typename T> void logw(T &text)
{
    logger::logi_impl('w', 0, text);
}
template<typename T> void logd(T &text)
{
    logger::logi_impl('d', 0, text);
}
template<typename T> void logd1(T &text)
{
    logger::logi_impl('d', 1, text);
}
template<typename T> void logd2(T &text)
{
    logger::logi_impl('d', 2, text);
}
template<typename T> void logd3(T &text)
{
    logger::logi_impl('d', 3, text);
}
template<typename T> void logd4(T &text)
{
    logger::logi_impl('d', 4, text);
}
template<typename T> void logd5(T &text)
{
    logger::logi_impl('d', 5, text);
}


template<typename T> void logi(const std::string &text, T val)
{
    logger::logi_impl2('i', 0, text, val);
}
template<typename T> void loge(const std::string &text, T val)
{
    logger::logi_impl2('e', 0, text, val);
}
template<typename T> void logw(const std::string &text, T val)
{
    logger::logi_impl2('w', 0, text, val);
}
template<typename T> void logd(const std::string &text, T val)
{
    logger::logi_impl2('d', 0, text, val);
}
template<typename T> void logd1(const std::string &text, T val)
{
    logger::logi_impl2('d', 1, text, val);
}
template<typename T> void logd2(const std::string &text, T val)
{
    logger::logi_impl2('d', 2, text, val);
}
template<typename T> void logd3(const std::string &text, T val)
{
    logger::logi_impl2('d', 3, text, val);
}
template<typename T> void logd4(const std::string &text, T val)
{
    logger::logi_impl2('d', 4, text, val);
}
template<typename T> void logd5(const std::string &text, T val)
{
    logger::logi_impl2('d', 5, text, val);
}


template<typename ...Args> void logi(const std::string &format, Args&&... args)
{
    logger::log_impl3('i', 0, format, args...);
}
template<typename ...Args> void loge(const std::string &format, Args&&... args)
{
    logger::log_impl3('e', 0, format, args...);
}
template<typename ...Args> void logw(const std::string &format, Args&&... args)
{
    logger::log_impl3('w', 0, format, args...);
}
template<typename ...Args> void logd(const std::string &format, Args&&... args)
{
    logger::log_impl3('d', 0, format, args...);
}
template<typename ...Args> void logd1(const std::string &format, Args&&... args)
{
    logger::log_impl3('d', 1, format, args...);
}
template<typename ...Args> void logd2(const std::string &format, Args&&... args)
{
    logger::log_impl3('d', 2, format, args...);
}
template<typename ...Args> void logd3(const std::string &format, Args&&... args)
{
    logger::log_impl3('d', 3, format, args...);
}
template<typename ...Args> void logd4(const std::string &format, Args&&... args)
{
    logger::log_impl3('d', 4, format, args...);
}
template<typename ...Args> void logd5(const std::string &format, Args&&... args)
{
    logger::log_impl3('d', 5, format, args...);
}

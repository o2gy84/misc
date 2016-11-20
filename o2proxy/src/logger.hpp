#pragma once

#include <iostream>
#include <sstream>
#include <initializer_list>
#include <utility>
#include <tuple>
#include <string>
#include <assert.h>



// can log into stdout, file, stream, syslog...
class Logger
{
public:
    static const Logger& get();

    template <typename T>
    void log(T val) const
    {
        // TODO: file, stream, syslog, loglevel :)
        std::cout << val << std::endl;
    }

private:
    Logger();
    Logger(const Logger &);
    Logger& operator=(const Logger &);
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

}   // namespace

// logging without args
void logi(const std::string &text);

// logging in "key: value" style
template<typename T>
void logi(const std::string &text, T val)
{
    std::stringbuf str;
    std::ostream os(&str);
    os << text << val;
    Logger::get().log(str.str());
}

// logging in style like: string.format("key: {0}, value: {1}", key, val)
template <typename ...Args>
void logi(const std::string &format, Args&&... args)
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
    Logger::get().log(stringbuf.str());
}

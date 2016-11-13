#pragma once

#include <iostream>
#include <initializer_list>
#include <utility>
#include <tuple>
#include <string>
#include <assert.h>

class Logger
{
public:
    static const Logger& get();

private:
    Logger();
};


namespace logger
{

// get number from braced string like "{123}"
int braced_number(const std::string &s);

template <std::size_t N, typename... Tp>
typename std::enable_if<(N < sizeof...(Tp)), void>::type tuple_at(const std::tuple<Tp...> &t)
{
    std::cout << std::get<N>(t);
}

template <std::size_t N, typename... Tp>
typename std::enable_if< (N >= sizeof...(Tp)), void>::type tuple_at(const std::tuple<Tp...> &)
{
    std::cout << "nil";
}

}   // namespace

// log without args
void logi(const std::string &text);

// logging in style like: string.format("key: {0}, value: {1}", key, val)
template <typename ...Args>
void logi(const std::string &format, Args&&... args)
{
    std::tuple<Args...> list(args...);

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

        std::cout << std::string(format.begin() + last_pos, format.begin() + open_brace);
        switch (tuple_arg)
        {    
            case 0: logger::tuple_at<0>(list); break;
            case 1: logger::tuple_at<1>(list); break;
            case 2: logger::tuple_at<2>(list); break;
            case 3: logger::tuple_at<3>(list); break;
            case 4: logger::tuple_at<4>(list); break;
            case 5: logger::tuple_at<5>(list); break;
            case 6: logger::tuple_at<6>(list); break;
            case 7: logger::tuple_at<7>(list); break;
            case 8: logger::tuple_at<8>(list); break;
            case 9: logger::tuple_at<9>(list); break;
            default: throw std::runtime_error("logger: too many arguments (max = 9)");
        }

        last_pos = close_brace + 1;
        open_brace = format.find('{', close_brace + 1);
    }
    std::cout << std::string(format.begin() + last_pos, format.end()) << std::endl;
}

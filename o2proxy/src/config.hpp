#include <string>


enum class engine_t: uint8_t 
{
    UNKNOWN,
    SELECT,
    POLL,
    EPOLL
};


/*
    Programm config class.
*/
struct Config
{
        Config(const std::string &path);

        uint16_t port;
        engine_t engine;
};

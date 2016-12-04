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
class Config
{
public:
    Config(const std::string &path);

    uint16_t _port;
    engine_t _engine;

private:
    void init();
};

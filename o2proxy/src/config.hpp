#pragma once

#include <string>

namespace config
{

enum class engine_t: uint8_t 
{
    UNKNOWN,
    SELECT,
    POLL,
    EPOLL
};

engine_t string2engine(const std::string &str);
std::string engine2string(engine_t engine);

}

/*
    Programm config class.
*/
class Config
{
public:
    static Config* get();
    void load(const std::string &path);

public:
    uint16_t _port;
    config::engine_t _engine;
    std::string _local_address;


private:
    static Config *_self;

private:
    Config() {};
    Config(const Config &);
    const Config& operator=(const Config &);

private:
    void init();
    void read(const std::string &path);
    void parse(const std::string &config);
};

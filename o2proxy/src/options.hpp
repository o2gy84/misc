#include <string>


/*
    Programm option class.
*/
class Options
{
public:
    Options(int count, const char *const *args);
    void dump() const noexcept;

public:
    uint16_t        _log_level;
    uint16_t        _port;
    std::string     _config;

private:
    void init() noexcept;
};

#include <string>

/*
    Programm option class.
*/
struct Options
{
        Options(int count, const char *const *args);
        void dump() const noexcept;

        uint16_t _log_level;
        uint16_t _port;
        std::string _config;

};

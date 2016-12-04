#ifndef _CLIENT_HPP
#define _CLIENT_HPP

#include <inttypes.h>   // uint8_t
#include <unistd.h>
#include <string>

enum class client_state_t: uint8_t
{
    WANT_READ,
    WANT_WRITE
};


/*
    Client - is just an sd and state
*/
struct Client
{
    explicit Client(int sd)              : _sd(sd), _state(client_state_t::WANT_READ) {}
    Client(int sd, client_state_t state) : _sd(sd), _state(state) {}
    virtual ~Client() { if (_sd > 0) ::close (_sd); }

    virtual void onRead(const std::string &str) {}
    virtual void onWrite()                      {}
    virtual void onDead()                       {}

    int _sd;
    client_state_t _state;
};

#endif

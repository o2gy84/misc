#pragma once

#include "http.hpp"
#include "client.hpp"
#include "engine.hpp"


class AbstractStream
{
public:
    void append(const std::string &str)     { _buf.append(str); }
    void clear()                            { _buf.clear(); }
    std::string stream()                    { return _buf; }

public:
    std::string _buf;
};

/*
 * Реализация собственно логики проксирования
 */

class ProxyClient: public Client
{
public:
    enum class state: uint8_t
    {
        UNKNOWN_STATE,
        INIT_STATE,
        WANT_WRITE_TO_TARGET,
        WANT_READ_FROM_TARGET,
        WANT_CONNECT,

        WANT_WRITE_TO_CLI,
        WANT_READ_FROM_CLI,
    };

    friend void bind(ProxyClient *to_browser, ProxyClient *to_target);
    friend void unbind(ProxyClient *to_browser, ProxyClient *to_target);

public:
    ProxyClient(int sd, Engine *ev);
    virtual ~ProxyClient();

    void nextState(ProxyClient::state state);

    // getters
    ProxyClient::state getState() const { return _state; }
    int sd() const { return _sd; }

    // setters
    void setClientIP(int ip) { _client_ip = ip; };

    // callbacks
    virtual void onRead(const std::string &str) override;
    virtual void onWrite() override;
    virtual void onDead() override;

public:
    // TODO: abstract protocol
    HttpRequest _req;
    AbstractStream _stream;
    int _client_ip;

private:

    Engine                      *_ev;
    ProxyClient                 *_partner;
    state                       _state;
};

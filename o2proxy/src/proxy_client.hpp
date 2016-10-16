#ifndef _PROXY_CLIENT_HPP
#define _PROXY_CLIENT_HPP

#include "http.hpp"
#include "client.hpp"
#include "engine.hpp"


class AbstractStream
{
public:
    void dump(const std::string &prefix, const std::string &direction);
    void append(const std::string &str)     { buf.append(str); }

    void clear()                            { buf.clear(); }
    std::string stream()                    { return buf; }

public:
    std::string buf;
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

public:
    ProxyClient(int sd, Engine *ev);
    virtual ~ProxyClient();

    void nextState(ProxyClient::state state);

    virtual void onRead(const std::string &str) override;
    virtual void onWrite() override;
    virtual void onDead() override
    {
        if (_partner != NULL)
        {
            if (_partner->_partner != NULL)
            {
                _partner->_partner = NULL;
            }
        }
    }

public:
    // TODO: abstract protocol
    HttpRequest _req;
    HttpRequest _resp;

    AbstractStream _stream;


private:

    Engine                      *_ev;
    ProxyClient                 *_partner;
    state                       _state;
};





#endif

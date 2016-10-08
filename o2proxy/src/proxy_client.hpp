#ifndef _PROXY_CLIENT_HPP
#define _PROXY_CLIENT_HPP

#include "http.hpp"
#include "client.hpp"
#include "engine.hpp"


/*
 * Реализация собственно логики проксирования
 */

// here sd - socket for client (browser)
class ProxyClient: public Client
{
public:
    enum class state: uint8_t
    {
        WANT_READ_FROM_CLI,
        WANT_WRITE_TO_CLI,
    };

public:
    ProxyClient(int sd, Engine *ev): Client(sd), _ev(ev) 
    {
        _state = state::WANT_READ_FROM_CLI;
    }
    virtual ~ProxyClient() {}

    void setState(ProxyClient::state state)
    {
        _state = state;
        _ev->changeEvents(this, engine::event_t::EV_WRITE);
    }
    void setResponse(const HttpRequest resp) { _resp = resp; }

    virtual void onRead(const std::string &str) override;
    virtual void onWrite() override;

private:
    // TODO: abstract protocol
    HttpRequest _req;
    HttpRequest _resp;

    Engine                      *_ev;
    state                       _state;
};


// here sd - socket for remote server
class ProxyClientToRemoteServer: public Client
{
public:
    enum class state: uint8_t
    {
        WANT_WRITE_FROM_CLI_TO_TARGET,
        WANT_READ_FROM_TARGET
    };

public:
    ProxyClientToRemoteServer(int sd, Engine *ev, ProxyClient *proxy, const HttpRequest &req) :
        Client(sd),
         _ev(ev),
         m_cli(proxy)
    {
        _req = req;
        _state = state::WANT_WRITE_FROM_CLI_TO_TARGET;
    }

    virtual void onRead(const std::string &str) override;
    virtual void onWrite() override;
private:
    // TODO: abstract protocol
    HttpRequest                 _req;
    HttpRequest                 _resp;

    Engine                      *_ev;
    state                       _state;
    ProxyClient                 *m_cli;
};

#endif

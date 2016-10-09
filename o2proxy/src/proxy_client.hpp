#ifndef _PROXY_CLIENT_HPP
#define _PROXY_CLIENT_HPP

#include "http.hpp"
#include "client.hpp"
#include "engine.hpp"


/*
 * Реализация собственно логики проксирования
 */


class ProxyClientToRemoteServer;

// here sd - socket for client (browser)
class ProxyClient: public Client
{
public:
    enum class state: uint8_t
    {
        WANT_READ_FROM_CLI,
        WANT_WRITE_TO_CLI,
        WANT_READ_FROM_CLI_BINARY,
        WANT_WRITE_TO_CLI_BINARY,
        WANT_WRITE_TO_CLI_AFTER_CONNECT,
        WANT_READ_FROM_CLI_AFTER_CONNECT,
        WAIT_FOR_CONNECT,
        WAIT_FOR_TARGET,
        WAIT_BOTH_TARGET_OR_CLIENT,
    };

public:
    ProxyClient(int sd, Engine *ev): Client(sd), _ev(ev) 
    {
        _state = state::WANT_READ_FROM_CLI;
    }
    virtual ~ProxyClient() {}

    void setState(ProxyClient::state state);
    void setResponse(const HttpRequest resp) { _resp = resp; }

    virtual void onRead(const std::string &str) override;
    virtual void onWrite() override;

private:
    // TODO: abstract protocol
    HttpRequest _req;
    HttpRequest _resp;

    Engine                      *_ev;
    state                       _state;
    ProxyClientToRemoteServer   *m_proxy;
};


// here sd - socket for remote server
class ProxyClientToRemoteServer: public Client
{
public:
    enum class state: uint8_t
    {
        WANT_WRITE_FROM_CLI_TO_TARGET,
        WANT_WRITE_FROM_CLI_TO_TARGET_BINARY,
        WANT_READ_FROM_TARGET,
        WANT_READ_FROM_TARGET_BINARY,
        WANT_CONNECT,
    };

public:
    ProxyClientToRemoteServer(int sd, Engine *ev, ProxyClient *proxy, const HttpRequest &req, ProxyClientToRemoteServer::state state) :
        Client(sd),
         _ev(ev),
         _state(state),
         m_cli(proxy)
    {
        _req = req;
    }

    void setRequest(const HttpRequest req) { _req = req; }
    void addRequest(const HttpRequest req) { _req.append(req.asIs()); }

    void setState(ProxyClientToRemoteServer::state state) { _state = state; }

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

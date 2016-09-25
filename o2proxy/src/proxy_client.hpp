#ifndef _PROXY_CLIENT_HPP
#define _PROXY_CLIENT_HPP

#include "http.hpp"
#include "client.hpp"
#include "engine.hpp"


/*
 * Реализация собственно логики проксирования
 */
class ProxyClient: public Client
{
public:
    ProxyClient(int _sd, Engine *ev): Client(_sd), _ev(ev) {}
    virtual ~ProxyClient() {}

    virtual void onRead(const std::string &str) override;
    virtual void onWrite() override;

    // TODO: abstract protocol
    HttpRequest _req;
    HttpRequest _resp;

protected:
    Engine *_ev;

};

class ProxyClientToRemoteServer: public ProxyClient
{
public:
    enum class state: uint8_t
    {
        WANT_WRITE_FROM_CLI_TO_TARGET,
        WANT_READ_FROM_TARGET,
        WATN_WRITE_TO_CLI
    };

public:
    ProxyClientToRemoteServer(int sd, Engine *ev, int browser_sd, const HttpRequest &req):
        ProxyClient(sd, ev),
        _browser_sd(browser_sd)
    {
        _req = req;
        _state = state::WANT_WRITE_FROM_CLI_TO_TARGET;
    }

    virtual void onRead(const std::string &str) override;
    virtual void onWrite() override;

    state _state;
    int _browser_sd;

private:

};

#endif

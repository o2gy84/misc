#include <vector>

#include <sys/socket.h> // socket(), AF_INET/PF_INET
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_aton()
#include <netdb.h>      // gethostbyname
#include <fcntl.h>      // F_GETFL, O_NONBLOCK e.t.c.
#include <errno.h>
#include <string.h>
#include <unistd.h>     // close

#include "proxy_client.hpp"
#include "utils.hpp"



namespace 
{
    struct sockaddr_in resolve(const char* host, int port)
    {
        struct hostent* hp = gethostbyname(host);
        if (NULL == hp)
            throw std::runtime_error("resolve error: " + std::string(strerror(errno)));

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = /*Address Family*/AF_INET;        // only AF_INET !
        addr.sin_port = htons(port);
        memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);

        return addr;
    }

    void set_non_blocked(int sd, bool opt) throw (std::exception)
    {
        int flags = fcntl(sd, F_GETFL, 0);
        int new_flags = (opt)? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
        if (fcntl(sd, F_SETFL, new_flags) == -1)
            throw std::runtime_error("make nonblocked: " + std::string(strerror(errno)));
    }

    int non_blocked_connect(const std::string &host, int port)
    {
        struct sockaddr_in addr = resolve(host.data(), port);

        int sd = socket(/*Protocol Family*/PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sd <= 0)
            throw std::runtime_error("error to create socket: " + std::string(strerror(errno)));

        set_non_blocked(sd, true);

        int connected = ::connect(sd, (struct sockaddr*)&addr, sizeof(addr));
        if (connected == -1 && errno != EINPROGRESS)
        {
            ::close(sd);
            std::cerr << "connect error: " << strerror(errno) << "\n";
            return -1;
        }
        return sd;
    }

    int send(int sd, const std::string &str) noexcept
    {
        size_t left = str.size();
        ssize_t sent = 0;
        //int flags = MSG_DONTWAIT | MSG_NOSIGNAL;
        int flags = 0;

        while (left > 0)
        {
            sent = ::send(sd, str.data() + sent, str.size() - sent, flags);
            if (-1 == sent)
            {
                std::cerr << "write failed: " << strerror(errno) << "\n";
                return -1;
            }

            left -= sent;
        }
        
        return str.size();
    }

}   // namespace


std::string state2string(ProxyClient::state state)
{
    if (state == ProxyClient::state::WAIT_BOTH_TARGET_OR_CLIENT)        return "wait_both_target_or_client";
    if (state == ProxyClient::state::WANT_READ_FROM_CLI)                return "read_from_cli";
    if (state == ProxyClient::state::WANT_WRITE_TO_CLI)                 return "write_to_cli";
    if (state == ProxyClient::state::WANT_READ_FROM_CLI_BINARY)         return "read_from_cli_binary";
    if (state == ProxyClient::state::WANT_WRITE_TO_CLI_BINARY)          return "write_to_cli_binary";
    if (state == ProxyClient::state::WANT_WRITE_TO_CLI_AFTER_CONNECT)   return "write_to_cli_after_connect";
    if (state == ProxyClient::state::WANT_READ_FROM_CLI_AFTER_CONNECT)  return "read_from_cli_after_connect";
    if (state == ProxyClient::state::WAIT_FOR_CONNECT)                  return "wait_for_connect";
    if (state == ProxyClient::state::WAIT_FOR_TARGET)                   return "wait_for_target";
    throw std::runtime_error("ProxyClient: unknown state");
}


void ProxyClient::setState(ProxyClient::state state)
{ 
    std::cerr << "[" << _sd << "] " <<"SET STATE: \"" << state2string(_state) << "\" --> \"" << state2string(state) << "\"\n";

    // TODO: clean this table

    if (_state == ProxyClient::state::WAIT_FOR_CONNECT)
    {
        if (state == ProxyClient::state::WANT_WRITE_TO_CLI)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI_AFTER_CONNECT;
            return;
        }
    }
    else if (_state == ProxyClient::state::WAIT_FOR_TARGET)
    {
        if (state == ProxyClient::state::WANT_WRITE_TO_CLI)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI;
            return;
        }
        else if (state == ProxyClient::state::WANT_WRITE_TO_CLI_BINARY)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI_BINARY;
            return;
        }
        else if (state == ProxyClient::state::WAIT_BOTH_TARGET_OR_CLIENT)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI_BINARY;
            return;
        }
    }
    else if (_state == ProxyClient::state::WAIT_BOTH_TARGET_OR_CLIENT)
    {
        if (state == ProxyClient::state::WANT_WRITE_TO_CLI_BINARY)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI_BINARY;
            return;
        }
        if (state == ProxyClient::state::WAIT_BOTH_TARGET_OR_CLIENT)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI_BINARY;
            return;
        }
    }
    else if (_state == ProxyClient::state::WANT_WRITE_TO_CLI_BINARY)
    {
        if (state == ProxyClient::state::WANT_WRITE_TO_CLI_BINARY)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI_BINARY;
            return;
        }
        if (state == ProxyClient::state::WAIT_BOTH_TARGET_OR_CLIENT)
        {
            _state = ProxyClient::state::WANT_WRITE_TO_CLI_BINARY;
            return;
        }
    }

    throw std::runtime_error("state machine error");
}

void ProxyClient::onRead(const std::string &str)
{
    std::cerr << "ProxyClient on read: " << str.size() << " bytes [sd: " << _sd << ", state: " << state2string(_state) << "]\n";

    _req.append(str);

    if (_state == state::WANT_READ_FROM_CLI)
    {
        if (!_req.valid())
        {
            // need read more
            _ev->changeEvents(this, engine::event_t::EV_READ);
            return;
        }

        _req.dump("ProxyClient: onRead req", "<<< ");

        std::vector<std::string> host_port = utils::split(_req._headers.header("Host"), ":");
        std::string host;
        std::string port;
        if (host_port.size() != 2)
        {
            host = _req._headers.header("Host");
            port = "80";
        }
        else
        {
            host = host_port[0];
            port = host_port[1];
        }

        if (_ev->isMyHost(host))
        {
            _resp.append("HTTP/1.1 200 OK\r\nContent-Length: 7\r\n\r\no2proxy");
            _state = ProxyClient::state::WANT_WRITE_TO_CLI;
            _ev->changeEvents(this, engine::event_t::EV_WRITE);
            return;
        }

        if (_req._headers._method == "CONNECT")
        {
            _state = state::WAIT_FOR_CONNECT;

            int nbsd = non_blocked_connect(host, std::stoi(port));
            set_non_blocked(nbsd, false);

            m_proxy = new ProxyClientToRemoteServer(nbsd, _ev, this, _req, ProxyClientToRemoteServer::state::WANT_CONNECT);
            _ev->addToEventLoop(m_proxy, engine::event_t::EV_WRITE);
            _req.clear();
            return;
        }

        if (_req._headers._method == "GET"
            || _req._headers._method == "POST"
            )
        {
            _state = state::WAIT_FOR_TARGET;

            int nbsd = non_blocked_connect(host, std::stoi(port));

            // nonblocked socket never blocks, so epoll always returns immediately
            set_non_blocked(nbsd, false);
            
            Client *c = new ProxyClientToRemoteServer(nbsd, _ev, this, _req, ProxyClientToRemoteServer::state::WANT_WRITE_FROM_CLI_TO_TARGET);
            _ev->addToEventLoop(c, engine::event_t::EV_WRITE);
            _req.clear();
            return;
        }
            
        throw std::runtime_error("UNKNOWN HTTP REQUEST!");
    }

    if (_state == state::WANT_READ_FROM_CLI_AFTER_CONNECT)
    {
        _state = state::WAIT_FOR_TARGET;

        m_proxy->setRequest(_req);
        m_proxy->setState(ProxyClientToRemoteServer::state::WANT_WRITE_FROM_CLI_TO_TARGET_BINARY);
        _req.clear();

        _ev->addToEventLoop(m_proxy, engine::event_t::EV_WRITE);
        return;
    }

    if (_state == state::WAIT_FOR_TARGET)
    {
        std::cerr << "TEST\n";
        sleep(1);

        // client sends requset in many buckets
        m_proxy->addRequest(_req);
        //m_proxy->setState(ProxyClientToRemoteServer::state::WANT_WRITE_FROM_CLI_TO_TARGET_BINARY);
        _req.clear();
        return;
    }

    if (_state == state::WAIT_BOTH_TARGET_OR_CLIENT)
    {
        // if this state onRead, we need to do request on target
        m_proxy->setRequest(_req);
        m_proxy->setState(ProxyClientToRemoteServer::state::WANT_WRITE_FROM_CLI_TO_TARGET_BINARY);
        _req.clear();

        _ev->changeEvents(m_proxy, engine::event_t::EV_WRITE);
        return;
    }

    throw std::runtime_error("onRead: unknown state");
}


void ProxyClient::onWrite()
{
    std::cerr << "ProxyClient onWrite [sd: " << _sd << ", state: " << state2string(_state) << "]\n";

    if (_state == state::WANT_WRITE_TO_CLI || _state == state::WANT_WRITE_TO_CLI_AFTER_CONNECT)
    {
        _resp.dump("ProxyToBrowser: onWrite req", ">>> ");
        int rc = send(_sd, _resp.toString());
        if (rc <= 0)
        {
            std::cerr << "send error :(\n";
        }
        else
        {
            std::cerr << "send rc: " << rc << " bytes\n";
        }

        if (_state == state::WANT_WRITE_TO_CLI)
        {
            _state = state::WANT_READ_FROM_CLI;
        }
        else if (_state == state::WANT_WRITE_TO_CLI_AFTER_CONNECT)
        {
            _state = state::WANT_READ_FROM_CLI_AFTER_CONNECT;
        }

        _ev->changeEvents(this, engine::event_t::EV_READ);
        return;
    }


    if (_state == state::WANT_WRITE_TO_CLI_BINARY)
    {
        int rc = send(_sd, _resp.asIs());
        if (rc <= 0)
        {
            std::cerr << "send error :(\n";
        }
        else
        {
            std::cerr << "send rc: " << rc << " bytes\n";
        }

        // can read_from_cli_binary or wait_for_target
        _state = state::WAIT_BOTH_TARGET_OR_CLIENT;

        _ev->changeEvents(this, engine::event_t::EV_READ);
        _ev->changeEvents(m_proxy, engine::event_t::EV_READ);

        return;
    }

    throw std::runtime_error("onWrite: unknown state");
}


std::string state2string(ProxyClientToRemoteServer::state state)
{
    if (state == ProxyClientToRemoteServer::state::WANT_WRITE_FROM_CLI_TO_TARGET)           return "write_to_target";
    if (state == ProxyClientToRemoteServer::state::WANT_WRITE_FROM_CLI_TO_TARGET_BINARY)    return "write_to_target_binary";
    if (state == ProxyClientToRemoteServer::state::WANT_READ_FROM_TARGET)                   return "read_from_target";
    if (state == ProxyClientToRemoteServer::state::WANT_CONNECT)                            return "connect_to_target";
    if (state == ProxyClientToRemoteServer::state::WANT_READ_FROM_TARGET_BINARY)            return "read_from_target_binary";
    //if (state == ProxyClientToRemoteServer::state::)
    //if (state == ProxyClientToRemoteServer::state::)
    throw std::runtime_error("ProxyClientToRemoteServer: unknown state");
}

void ProxyClientToRemoteServer::onRead(const std::string &str)
{
    std::cerr << "ProxyToRemoteServer onRead: " << str.size() << " bytes [sd: " << _sd << ", state: " << state2string(_state) << "]\n";

    if (_state == state::WANT_READ_FROM_TARGET)
    {
        //std::cerr << str << "\n";

        _resp.append(str);

        if (_resp.valid())
        {
            if (_resp._chunked)
            {
                _resp._headers._headers["Content-Length"] = std::to_string(_resp._body.size());
                _resp._headers._headers.erase("Transfer-Encoding");
            }

            m_cli->setResponse(_resp);
            m_cli->setState(ProxyClient::state::WANT_WRITE_TO_CLI);
            _ev->changeEvents(m_cli, engine::event_t::EV_WRITE);
            _resp.clear();
            return;
        }
        else
        {
            // need read more
            _ev->changeEvents(this, engine::event_t::EV_READ);
            return;
        }
        return;
    }

    if (_state == state::WANT_READ_FROM_TARGET_BINARY)
    {
        _resp.append(str);

        m_cli->setResponse(_resp);
        m_cli->setState(ProxyClient::state::WAIT_BOTH_TARGET_OR_CLIENT);
        _ev->changeEvents(m_cli, engine::event_t::EV_WRITE);
        //_ev->changeEvents(this, engine::event_t::EV_NONE);
        _resp.clear();
        return;
    }

    throw std::runtime_error("onRead: unknown state");
}

void ProxyClientToRemoteServer::onWrite()
{
    std::cerr << "ProxyToRemoteServer onWrite: " << _sd << ", state: " << state2string(_state) << "\n";

    if (_state == state::WANT_WRITE_FROM_CLI_TO_TARGET)
    {
        _req.dump("ProxyClientToRemoteServer: onWrite req", ">>> ");
        int rc = send(_sd, _req.toString());
        if (rc <= 0)
        {
            std::cerr << "send error :(\n";
        }
        else
        {
            std::cerr << "proxy send: " << rc << " bytes\n";
        }

        _state = state::WANT_READ_FROM_TARGET;
        _ev->changeEvents(this, engine::event_t::EV_READ);
        return;
    }

    if (_state == state::WANT_WRITE_FROM_CLI_TO_TARGET_BINARY)
    {
        int rc = send(_sd, _req.asIs());
        if (rc <= 0)
        {
            std::cerr << "send error :(\n";
        }
        else
        {
            std::cerr << "proxy send: " << rc << " bytes\n";
        }

        _state = state::WANT_READ_FROM_TARGET_BINARY;
        _ev->changeEvents(this, engine::event_t::EV_READ);
        return;
    }


    if (_state == state::WANT_CONNECT)
    {
        _resp.append("HTTP/1.1 200 OK\r\n\r\n");
        m_cli->setResponse(_resp);
        _resp.clear();
        m_cli->setState(ProxyClient::state::WANT_WRITE_TO_CLI);
        _ev->changeEvents(m_cli, engine::event_t::EV_WRITE);
        _ev->changeEvents(this, engine::event_t::EV_NONE);
        return;
    }

    throw std::runtime_error("onWrite: unknown state");
}





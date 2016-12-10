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
#include "logger.hpp"


extern std::string g_Version;


namespace 
{
    struct sockaddr_in resolve(const char* host, int port)
    {
        struct hostent* hp = gethostbyname(host);
        if (NULL == hp)
            throw std::runtime_error("proxy [" + std::string(host) + "] resolve error: " + std::string(strerror(errno)));

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

        if (0)
        {
            struct hostent *hp = gethostbyname("192.168.70.129");
            if (!hp) throw std::runtime_error("gethostbyname failed");
            struct sockaddr_in serv_addr;
            memset(&serv_addr, 0, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = *((unsigned long *)hp->h_addr);
            serv_addr.sin_port = 0;

            if (::bind(sd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            {
                throw std::runtime_error(std::string("bind proxy error: ") + strerror(errno));
            }
        }


        set_non_blocked(sd, true);

        int connected = ::connect(sd, (struct sockaddr*)&addr, sizeof(addr));
        if (connected == -1 && errno != EINPROGRESS)
        {
            ::close(sd);
            loge("connect error: ", strerror(errno));
            return -1;
        }
        return sd;
    }

    int send(int sd, const std::string &str) noexcept
    {
        size_t left = str.size();
        ssize_t sent = 0;
        //int flags = MSG_DONTWAIT | MSG_NOSIGNAL;
        int flags = MSG_NOSIGNAL;

        while (left > 0)
        {
            sent = ::send(sd, str.data() + sent, str.size() - sent, flags);
            if (-1 == sent)
            {
                return -1;
            }

            left -= sent;
        }
        
        return str.size();
    }

}   // namespace


std::string state2string(ProxyClient::state state)
{
    if (state == ProxyClient::state::UNKNOWN_STATE)                     return "unknown_state";
    if (state == ProxyClient::state::INIT_STATE)                        return "init_state";
    if (state == ProxyClient::state::WANT_CONNECT)                      return "want_connect";
    if (state == ProxyClient::state::WANT_READ_FROM_TARGET)             return "want_read_from_target";
    if (state == ProxyClient::state::WANT_WRITE_TO_TARGET)              return "want_write_to_target";
    if (state == ProxyClient::state::WANT_READ_FROM_CLI)                return "read_from_cli";
    if (state == ProxyClient::state::WANT_WRITE_TO_CLI)                 return "write_to_cli";
    throw std::runtime_error("ProxyClient: unknown state");
}

// friend
void bind(ProxyClient *to_browser, ProxyClient *to_target)
{
    to_browser->_partner = to_target;
    to_target->_partner = to_browser;
}

// friend
void unbind(ProxyClient *to_browser, ProxyClient *to_target)
{
    to_browser->_partner = nullptr;
    to_target->_partner = nullptr;
}


ProxyClient::ProxyClient(int sd, Engine *ev): Client(sd), _ev(ev)
{
    _state = state::INIT_STATE;
    _partner = nullptr;
}

ProxyClient::~ProxyClient()
{
    logd4("~DESTROY: ", _sd);
    _sd = -1;
}


void ProxyClient::nextState(ProxyClient::state new_state)
{ 
    _state = new_state;
}

void ProxyClient::onRead(const std::string &str)
{
    logd3("onRead [sd: {0}, partner: {1}, state: {2}, bytes: {3}]", _sd, ((_partner)? _partner->sd() : -1),
            state2string(_state), str.size());

    if (_state == state::INIT_STATE)
    {
        _req.clear();
        _req.append(str);

        if (!_req.valid())
        {
            // need read more
            _ev->changeEvents(this, engine::event_t::EV_READ);
            return;
        }

        logi("client: {0}, sd: {1}, request: {2} {3}", utils::int2ipv4(_client_ip), _sd, _req.headers()._method,
                _req.headers()._resource);
        
        //logd4("new request dump: ", _req.dump());

        std::string destination = _req.headers().header("Host");
        if (_req.headers()._method == "CONNECT")
        {
            destination = _req.headers()._resource;
        }

        std::vector<std::string> host_port = utils::split(destination, ":");
        std::string host;
        std::string port;

        if (host_port.size() != 2)
        {
            host = _req.headers().header("Host");
            port = "80";
        }
        else
        {
            host = host_port[0];
            port = host_port[1];
        }

        if (host.empty())
        {
            loge("host is empty. close connection. request dump: ", _req.dump());

            // shutdown with RDWR allow epoll to trigger event and delete this client
            shutdown(_sd, SHUT_RDWR);
            return;
        }

        if (_ev->isMyHost(host))
        {
             _partner = new ProxyClient(-1, _ev);
             bind(this, _partner);

            std::string message = "o2proxy ready (ver. " + g_Version + ")";
 
            _partner->_stream.append("HTTP/1.1 200 OK\r\nContent-Length: ");
            _partner->_stream.append(std::to_string(message.size()));
            _partner->_stream.append("\r\n\r\n" + message);
            nextState(ProxyClient::state::WANT_WRITE_TO_CLI);
            _ev->changeEvents(this, engine::event_t::EV_WRITE);
            _req.clear();
            shutdown(_sd, SHUT_RD);
            return;
        }

        if (_req.headers()._method == "CONNECT")
        {
            int proxy_sd = -1;
            try
            {
                proxy_sd = non_blocked_connect(host, std::stoi(port));
            }
            catch(const std::exception &e)
            {
                loge("connect: ", e.what());
                close(_sd);
                return;
            }
            set_non_blocked(proxy_sd, false);

            _partner = new ProxyClient(proxy_sd, _ev);
             bind(this, _partner);

            if (false == _ev->addToEventLoop(_partner, engine::event_t::EV_WRITE))
            {
                loge("error add to event loop on connect: ", _req.dump());

                // shutdown with RDWR allow epoll to trigger event and delete this client
                shutdown(_sd, SHUT_RDWR);
                return;
            }

            _req.clear();
            _partner->nextState(ProxyClient::state::WANT_CONNECT);
            return;
        }

        if (_req.headers()._method == "GET"
            || _req.headers()._method == "POST"
            || _req.headers()._method == "OPTIONS"
            || _req.headers()._method == "HEAD"
            )
        {
            int proxy_sd = -1;
            try
            {
                proxy_sd = non_blocked_connect(host, std::stoi(port));
            }
            catch(const std::exception &e)
            {
                loge("connect: ", e.what());
                close(_sd);
                return;
            }

            // nonblocked socket never blocks, so epoll always returns immediately
            set_non_blocked(proxy_sd, false);

             _partner = new ProxyClient(proxy_sd, _ev);
             bind(this, _partner);
           
            if (false == _ev->addToEventLoop(_partner, engine::event_t::EV_WRITE))
            {
                loge("error add to event loop on method: {0}. requset dump: {1}", _req.headers()._method, _req.dump());

                // shutdown with RDWR allow epoll to trigger event and delete this client
                shutdown(_sd, SHUT_RDWR);
                return;
            }

            _partner->nextState(ProxyClient::state::WANT_WRITE_TO_TARGET);
            _stream.clear();
            _stream.append(_req.toString());
            _req.clear();

            return;
        }

        loge("unknown http request. close connection. request dump: ", _req.dump());
        shutdown(_sd, SHUT_RDWR);
        return;
    }

    _req.clear();


    if (_state == state::WANT_READ_FROM_TARGET)
    {
        _stream.append(str);
        if (_partner == nullptr)
        {
            loge("PARTNER1 IS NULL !");
            return;
        }

        if (_partner->getState() != ProxyClient::state::WANT_WRITE_TO_TARGET)
        {
            _ev->changeEvents(_partner, engine::event_t::EV_WRITE);
            _partner->nextState(ProxyClient::state::WANT_WRITE_TO_CLI);
        }
        return;
    }
    
    if (_state == state::WANT_READ_FROM_CLI)
    {
        _stream.append(str);
        if (_partner == nullptr)
        {
            loge("PARTNER2 IS NULL !");
            return;
        }

        if (_partner->getState() != ProxyClient::state::WANT_WRITE_TO_TARGET)
        {
            _ev->changeEvents(_partner, engine::event_t::EV_WRITE);
            _partner->nextState(ProxyClient::state::WANT_WRITE_TO_TARGET);
        }
        return;
    }


    throw std::runtime_error("onRead: unknown state");
}


void ProxyClient::onWrite()
{
    logd3("onWrite [sd: {0}, partner: {1}, state: {2}]", _sd, ((_partner)? _partner->sd() : -1), state2string(_state));

    Client *cs = static_cast<Client*>(_partner);
    //logd5("partner: ", (void*)_partner);
    if (cs == nullptr)
    {
        loge("PARTNER IS NULL ON WRITE");
        _ev->changeEvents(this, engine::event_t::EV_NONE);
        return;
    }

    if (_state == state::WANT_CONNECT)
    {
        _stream.append("HTTP/1.1 200 OK\r\n\r\n");
        _ev->changeEvents(_partner, engine::event_t::EV_WRITE);
        _ev->changeEvents(this, engine::event_t::EV_NONE);
        _partner->nextState(ProxyClient::state::WANT_WRITE_TO_CLI);
        return;
    }

    if (_state == state::WANT_WRITE_TO_TARGET)
    {
        int rc = send(_sd, _partner->_stream.stream());
        if (rc <= 0)
        {
            loge("[{0}] proxy->target send error: {1}, {2}, wasnt sent: {3} bytes", _sd, rc,
                    strerror(errno), _partner->_stream.stream().size());
        }
        else
        {
            logd4("proxy->target send [sd: {0}, bytes: {1}]", _sd, rc);
        }

        _partner->_stream.clear();

        nextState(state::WANT_READ_FROM_TARGET);
        _ev->changeEvents(this, engine::event_t::EV_READ);
        return;
    }

    if (_state == state::WANT_WRITE_TO_CLI)
    {
        int rc = send(_sd, _partner->_stream.stream());
        if (rc <= 0)
        {
            loge("[{0}] proxy->browser send error: {1}, {2}, wasnt sent: {3} bytes", _sd, rc,
                    strerror(errno), _partner->_stream.stream().size());
        }
        else
        {
            logd4("proxy->browser send [sd: {0}, bytes: {1}]", _sd, rc);
        }

        _partner->_stream.clear();

        nextState(ProxyClient::state::WANT_READ_FROM_CLI);
        _ev->changeEvents(this, engine::event_t::EV_READ);
        return;
    }

    throw std::runtime_error("onWrite: unknown state");
}

void ProxyClient::onDead()
{
    if (_partner == nullptr)
    {
        return;
    }

    // теоретически, после закрытия сокета event-loop это обнаружит
    // и объект _partner должен быть удален
 
    logd3("shutdown partner socket: ", _partner->sd());
    if (_partner->sd() != -1)
    {
        shutdown(_partner->sd(), SHUT_RDWR);
    }
    unbind(this, _partner);
}

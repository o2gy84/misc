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
    if (state == ProxyClient::state::UNKNOWN_STATE)                     return "unknown_state";
    if (state == ProxyClient::state::INIT_STATE)                        return "init_state";
    if (state == ProxyClient::state::WANT_CONNECT)                      return "want_connect";
    if (state == ProxyClient::state::WANT_READ_FROM_TARGET)             return "want_read_from_target";
    if (state == ProxyClient::state::WANT_WRITE_TO_TARGET)              return "want_write_to_target";
    if (state == ProxyClient::state::WANT_READ_FROM_CLI)                return "read_from_cli";
    if (state == ProxyClient::state::WANT_WRITE_TO_CLI)                 return "write_to_cli";
    throw std::runtime_error("ProxyClient: unknown state");
}

void AbstractStream::dump(const std::string &prefix, const std::string &direction)
{
    //std::cerr << direction << prefix << " " << _buf.substr(0,  1024) << " [" << _buf.size() << " bytes]\n";
    //return;


    std::string::size_type pos = _buf.find("\r\n");
    if (pos == std::string::npos)
    {
        size_t max = std::min((size_t)_buf.size(), (size_t)10);
        std::string tmp = _buf.substr(0, max);

        if (max == 0)
        {
            std::cerr << direction << prefix << " stream empty\n";
        }
        else
        {
            bool is_readable = true;
            for (size_t i = 0; i < max; ++i)
            {
                if (isalpha(tmp[i]) || isspace(tmp[i]))
                    continue;

                is_readable = false;
            }

            if (is_readable)
            {
                std::cerr << direction << prefix << " " << tmp << " [" << max << " bytes]\n";
            }
            else
            {
                std::cerr << direction << prefix << " binary stream\n";
            }
        }
        return;
    }
    
    size_t max = std::min((size_t)pos, (size_t)100);
    std::string tmp = _buf.substr(0, max);


    bool is_readable = true;
    for (size_t i = 0; i < max; ++i)
    {
        if (isprint(tmp[i]))
            continue;

        is_readable = false;
    }

    if (is_readable)
    {
        std::cerr << direction << prefix << " " << tmp << " [" << max << " bytes]\n";
    }
    else
    {
        std::cerr << direction << prefix << " binary stream\n";
    }
}


void bind(ProxyClient *to_browser, ProxyClient *to_target)
{
    to_browser->_partner = to_target;
    to_target->_partner = to_browser;
}

ProxyClient::ProxyClient(int sd, Engine *ev): Client(sd), _ev(ev)
{
    _state = state::INIT_STATE;
    _partner = NULL;
}

ProxyClient::~ProxyClient()
{
    std::cerr << "~DESTROY: " << _sd << "\n";
}


void ProxyClient::nextState(ProxyClient::state new_state)
{ 
    //std::cerr << "[" << _sd << "] " <<"STATE: \"" << state2string(_state) << "\" --> \"" << state2string(new_state) << "\"\n";

    _state = new_state;
}

void ProxyClient::onRead(const std::string &str)
{
    std::cerr << "[" << _sd << " (" << (_partner? _partner->sd() : -1) << ")] onRead: " << str.size() << " bytes [state: " << state2string(_state) << "]\n";

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
             _partner = new ProxyClient(-1, _ev);
             bind(this, _partner);
 
            _partner->_stream.append("HTTP/1.1 200 OK\r\nContent-Length: 7\r\n\r\no2proxy");
            nextState(ProxyClient::state::WANT_WRITE_TO_CLI);
            _ev->changeEvents(this, engine::event_t::EV_WRITE);
            _req.clear();
            return;
        }

        std::cerr << "[I] connect to: " << host << ":" << port << "\n";

        if (_req._headers._method == "CONNECT")
        {
            int proxy_sd = non_blocked_connect(host, std::stoi(port));
            set_non_blocked(proxy_sd, false);

            _partner = new ProxyClient(proxy_sd, _ev);
             bind(this, _partner);

            _ev->addToEventLoop(_partner, engine::event_t::EV_WRITE);
            _req.clear();

            _partner->nextState(ProxyClient::state::WANT_CONNECT);
            return;
        }

        if (_req._headers._method == "GET"
            || _req._headers._method == "POST"
            )
        {
            int proxy_sd = non_blocked_connect(host, std::stoi(port));

            // nonblocked socket never blocks, so epoll always returns immediately
            set_non_blocked(proxy_sd, false);

             _partner = new ProxyClient(proxy_sd, _ev);
             bind(this, _partner);
           
            _ev->addToEventLoop(_partner, engine::event_t::EV_WRITE);
            _partner->nextState(ProxyClient::state::WANT_WRITE_TO_TARGET);

            _stream.clear();
            _stream.append(_req.toString());
            _req.clear();

            return;
        }
            
        throw std::runtime_error("UNKNOWN HTTP REQUEST!");
    }

    _req.clear();


    if (_state == state::WANT_READ_FROM_TARGET)
    {
        _stream.append(str);
        //_stream.dump("ReadFromTarget: ", "<<< ");

        if (_partner == NULL)
        {
            std::cerr << "PARTNER1 IS NULL !\n";
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
        //_stream.dump("ReadFromCli: ", "<<< ");

        if (_partner == NULL)
        {
            std::cerr << "PARTNER2 IS NULL !\n";
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
    std::cerr << "[" << _sd << " (" << ((_partner)? _partner->sd() : -1) << ")] ProxyClient onWrite [state: " << state2string(_state) << "]\n";

    Client *cs = static_cast<Client*>(_partner);
    //std::cerr << "partner: " << (void*)_partner << "\n";
    if (cs == NULL)
    {
        std::cerr << "\t\t\tPARTNER IS NULL ON WRITE!\n";
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
        _partner->_stream.dump("ProxyToRemoteServer onWrite: ", ">>> ");
        int rc = send(_sd, _partner->_stream.stream());

        if (rc <= 0)
        {
            std::cerr << "send error :(\n";
        }
        else
        {
            std::cerr << "[" << _sd << "] proxy->target send: " << rc << " bytes\n";
        }

        _partner->_stream.clear();

        nextState(state::WANT_READ_FROM_TARGET);
        _ev->changeEvents(this, engine::event_t::EV_READ);
        return;
    }

    if (_state == state::WANT_WRITE_TO_CLI)
    {
        _partner->_stream.dump("ProxyToBrowser onWrite: ", ">>> ");
        int rc = send(_sd, _partner->_stream.stream());

        if (rc <= 0)
        {
            std::cerr << "send error :(\n";
        }
        else
        {
            std::cerr << "[" << _sd << "] proxy->browser send: " << rc << " bytes\n";
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
    if (_partner != NULL)
    {
        std::cerr << "~delete partner: " << _partner->sd() << "\n";
        delete _partner;

        //::close(_partner->sd());
        //if (_partner->_partner != NULL)
        //{
            //_partner->_partner = NULL;
        //}
    }
}


#include <string>
#include <unistd.h>     // close()
#include <memory>


class Socket
{
    public:
        Socket()       : m_Sd(-1) {}
        Socket(int sd) : m_Sd(sd) {}
        ~Socket()                 { if (m_Sd > 0) ::close(m_Sd); }

    public:
        int  sd() const noexcept { return m_Sd; }
        void connect(const std::string &host, int port)                 throw (std::exception);
        void connect(const std::string &host, int port, int timeout)    throw (std::exception);
        void send(const std::string &s)                                 throw (std::exception);
        bool hasData()                                                  throw (std::exception);
        std::string recv()                                              throw (std::exception);
        std::string recv(size_t bytes)                                  throw (std::exception);
        void setRcvTimeout(int sec, int microsec)                       throw (std::exception);
        void setNonBlocked(bool opt)                                    throw (std::exception);
        void setReuseAddr(int sd)                                       throw (std::exception);
        void createServerSocket(uint32_t port, uint32_t queue_size)     throw (std::exception);
        std::shared_ptr<Socket> accept()                                throw (std::exception);
        void close()                     { ::close(m_Sd); }

        void httpQuery(const std::string &query, std::function<void(const std::string &s)> cb) throw (std::exception)
        {
            send(query);
            std::string res = recv();
            cb(res);
        }

    private:
        //Socket(const Socket &s);
        //const Socket& operator=(const Socket &s);
    private:
        int m_Sd;
};


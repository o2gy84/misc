
#include <vector>
#include <mutex>
#include <queue>

#include "engine.hpp"

class Connection;                           // pre declaration

typedef std::pair<std::shared_ptr<Connection>, std::function<void(const std::string &s)>> event_cb_t;

class EventLoop
{
    public:
        explicit EventLoop() {}
        void run();

        void asyncRead(std::shared_ptr<Connection>, std::function<void(const std::string &s)> cb);
        void asyncWrite(std::shared_ptr<Connection>, const std::string &str, std::function<void(const std::string &s)> cb);

    private:
        int manageConnections();            // push int queue READ or WRITE events
        void delClient(int sd);

    private:
        std::mutex _mutex;
        //std::queue<Client> _queue;                  // queue of events
        
        std::vector<event_cb_t> m_ClientsWantWork;  // clients who wants read or write
        std::queue<event_cb_t> m_ClientsHaveWork;   // clients who has data to read or write
};

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        explicit Connection(int sd, EventLoop *loop);
        ~Connection();
        
        void read();
        void write(const std::string &s);

        void readHandler(const std::string &s);
        void writeHandler(const std::string &s);
        //void writeHandler();

        std::string m_WriteBuf;
        Client m_Client;
        EventLoop *m_EventLoop;
};


/*
typedef int (Connection::*callback)(const std::string);
class ConnectionCallback
{
    public:
        std::function<void(const std::string &s)> m_Cb;
};
*/

class AsyncPollEngine: public Engine
{
    public:
        explicit AsyncPollEngine(int port): Engine(port) {}
        virtual void run() override;

    private:
        void delClient(int sd);

        //void acceptNewConnections();
        //void manageConnections();

    private:
};

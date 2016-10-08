#include <vector>
#include <mutex>
#include <queue>
#include <string>

#include "engine.hpp"
#include "client.hpp"

class EventLoop
{
    public:
        static EventLoop& eventLoop() { static EventLoop ev; return ev; }
        void run();

        void asyncRead(int sd, std::string &str, std::function<void(int)> cb);
        void asyncWrite(int sd, const std::string &str, std::function<void(int)> cb);

    private:
        EventLoop() {};
        EventLoop(const EventLoop&);
        int manageConnections();            // push into queue READ or WRITE events
        void deleteClient(int sd);

        struct Event
        {
            Event(Client c, std::string &data): _client(c), _data(data)   {}
            Event& operator=(const Event &h)
            {
                _client = h._client;
                _data = std::ref(h._data);
                _callback = h._callback;
                return *this;
            }

            Client _client;
            std::function<void(int)> _callback;
            std::reference_wrapper<std::string> _data;  // read or write buffer
        };

    private:
        std::mutex m_HaveWorkQueueMutex;
        std::mutex m_WantWorkQueueMutex;

        std::vector<Event> m_ClientsWantWork;  // clients who wants read or write
        std::queue<Event> m_ClientsHaveWork;   // clients who has data to read or write
};

class Connection: public std::enable_shared_from_this<Connection>
{
    public:
        explicit Connection(int sd, EventLoop *loop);
        ~Connection();
        
        void read();
        void write(const std::string &s);

        void readHandler(int error);
        void writeHandler(int error);

    private:
        std::string m_WriteBuf;
        std::string m_ReadBuf;

        int m_Sd;
        EventLoop *m_EventLoop;
};

class AsyncPollEngine: public Engine
{
    public:
        explicit AsyncPollEngine(int port): Engine(port) {}
        virtual void run() override;
        virtual void addToEventLoop(Client *c, engine::event_t events) override {};
        virtual void changeEvents(Client *c, engine::event_t events) override {};

};

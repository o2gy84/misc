#include <vector>

#include "engine.hpp"
#include "client.hpp"

class PollEngine: public Engine
{
    public:
        explicit PollEngine(int port): Engine(port) {}
        virtual void run() override;
        virtual bool addToEventLoop(Client *c, engine::event_t events) override { return false; };
        virtual void changeEvents(Client *c, engine::event_t events) override {};

    private:
        void acceptNewConnections();
        void manageConnections();

    private:
        std::vector<Client> m_Clients;
};

#include <vector>

#include "engine.hpp"
#include "client.hpp"

class PollEngine: public Engine
{
    public:
        explicit PollEngine(int port): Engine(port) {}
        virtual void run() override;
        virtual void addToEventLoop(Client *c, event_t events) override {};
        virtual void changeEvents(Client *c, event_t events) override {};

    private:
        void acceptNewConnections();
        void manageConnections();

    private:
        std::vector<Client> m_Clients;
};

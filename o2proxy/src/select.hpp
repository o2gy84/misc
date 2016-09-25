#include <vector>

#include "engine.hpp"
#include "client.hpp"

class SelectEngine: public Engine
{
    public:
        explicit SelectEngine(int port): Engine(port) {}
        virtual void run() override;
        virtual void addToEventLoop(Client *c, event_t events) override {};
        virtual void changeEvents(Client *c, event_t events) override {};

    private:
        void manageConnections();

    private:
        std::vector<Client> m_Clients;
};

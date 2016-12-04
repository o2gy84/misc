#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h> 
#include <stdint.h>
#include <string.h>

#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>

#include "select.hpp"
#include "logger.hpp"

void SelectEngine::manageConnections()
{
    fd_set read_fds;
    m_Clients.push_back(Client(listener()));        // hack!!!

    while (1)
    {
        std::vector<int> disconnected_clients;
        FD_ZERO(&read_fds);
        int fdmax = listener();

        for (const Client &c: m_Clients)
        {
            if (c._sd > fdmax) fdmax = c._sd;
            FD_SET(c._sd, &read_fds);
        }

        int sel = select(fdmax + 1, /*read*/&read_fds, /*write*/NULL, /*exceptions*/NULL, /*timeout*/NULL);
        if (sel == -1)
        {
            throw std::runtime_error(std::string("select: ") + strerror(errno));
        }

        if (sel == 0)
            continue;

        for (size_t i = 0; i < m_Clients.size(); ++i)
        {
            if (!FD_ISSET(m_Clients[i]._sd, &read_fds))
                continue;

            if (m_Clients[i]._sd == listener())
            {
                struct sockaddr_in client;
                memset(&client, 0, sizeof(client));
                socklen_t cli_len = sizeof(client);
                int cli_sd = accept(listener(), (struct sockaddr*)&client, &cli_len);
                std::cerr << "new client: " << cli_sd << "\n";

                // TODO: need lock
                m_Clients.push_back(Client(cli_sd));

                // т.к. m_Clients увеличился, лучше цикл закончить
                break;
            }
            else
            {
                char buf[256];
                int r = read(m_Clients[i]._sd, buf, sizeof(buf));

                if (r < 0)
                {
                    std::cerr << "some read error!\n";
                    disconnected_clients.push_back(m_Clients[i]._sd);
                    continue;
                }
                buf[r] = '\0';

                std::string tmp(buf);
                while (tmp[tmp.size() - 1] == '\r' || tmp[tmp.size() - 1] == '\n')
                    tmp.pop_back();

                if (r > 0)
                {
                    std::cerr << "read: " << r << " bytes [" << tmp.c_str() << "]\n";
                    std::string ans = "echo: " + tmp + "\n";
                    write(m_Clients[i]._sd, ans.data(), ans.size());
                }

                if (r == 0)
                {
                    std::cerr << "client disconnected: " << m_Clients[i]._sd << "\n";
                    disconnected_clients.push_back(m_Clients[i]._sd);
                }
            }
        }

        // remove disconnected clients
        for (size_t i = 0; i< disconnected_clients.size(); ++i)
        {
            int sd = disconnected_clients[i];
            auto f = [&sd](const Client &state) { return state._sd == sd; };
            auto iterator = std::find_if(m_Clients.begin(), m_Clients.end(), f);
            assert(iterator != m_Clients.end());

            close(sd);
            FD_CLR(sd, &read_fds);
            m_Clients.erase(iterator);
        }
    }
}

void SelectEngine::run()
{
    logi("start even loop");
    manageConnections();
    /*
    try
    {
        std::thread t(std::bind(&SelectEngine::manageConnections, this));
        t.join();
    }
    catch (const std::exception &e)
    {
        throw;
    }
    */
}

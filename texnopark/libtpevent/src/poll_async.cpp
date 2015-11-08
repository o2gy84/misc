/*
    event-loop на epoll().
    Особенности:
        - 
*/

#include <sys/poll.h>
#include <sys/socket.h> // socket()
#include <netinet/in.h> // htons(), INADDR_ANY
#include <errno.h>
#include <assert.h>

#include <unistd.h>     // close()

#include <stdint.h>     // uint32_t
#include <string.h>

#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>
#include <map>

#include "poll_async.hpp"

using namespace std::placeholders;

// CONNECTION

Connection::Connection(int sd, EventLoop *loop): m_Client(sd), m_EventLoop(loop)
{
    std::cerr << "+Connection: " << m_Client.sd << "\n";
}
Connection::~Connection()
{
    std::cerr << "~Connection: " << m_Client.sd << "\n"; close(m_Client.sd);
}

void Connection::read()
{
    // TODO: need lock

    m_Client.state = client_state_t::WANT_READ;

    m_EventLoop->asyncRead(shared_from_this(), std::bind(&Connection::readHandler, this, _1));
}

void Connection::readHandler(const std::string &s)
{
    if (s.empty())
    {
        std::cerr << "thread: " << pthread_self() << ". readHandler: abort connection!\n";
        return;
    }

    std::string tmp = s;
    while (tmp[tmp.size() - 1] == '\r' || tmp[tmp.size() - 1] == '\n')
        tmp.pop_back();

    std::cerr << "thread: " << pthread_self() << ". readHandler: " << s.size() << " bytes [" << tmp << "]\n";
    write("server: " + tmp + "\n");
}

void Connection::write(const std::string &s)
{
    // TODO: need lock
    m_WriteBuf = s;

    m_Client.state = client_state_t::WANT_WRITE;

    m_EventLoop->asyncWrite(shared_from_this(), s, std::bind(&Connection::writeHandler, this, _1));
}

//void Connection::writeHandler()
//{
    //read();
//}

void Connection::writeHandler(const std::string &s)
{
    read();
}


// EVENT LOOP

void EventLoop::delClient(int sd)
{
    auto f = [&sd](const event_cb_t &state) { return state.first->m_Client.sd == sd; };
    auto iterator = std::find_if(m_ClientsWantWork.begin(), m_ClientsWantWork.end(), f);
    assert(iterator != m_ClientsWantWork.end());
    m_ClientsWantWork.erase(iterator);
}

void EventLoop::asyncRead(std::shared_ptr<Connection> conn, std::function<void(const std::string &s)> cb)
{
    m_ClientsWantWork.push_back(std::pair<std::shared_ptr<Connection>, std::function<void(const std::string &s)>>(conn, cb));
}

void EventLoop::asyncWrite(std::shared_ptr<Connection> conn, const std::string &str, std::function<void(const std::string &s)> cb)
{
    m_ClientsWantWork.push_back(std::pair<std::shared_ptr<Connection>, std::function<void(const std::string &s)>>(conn, cb));
}

int EventLoop::manageConnections()
/*
    push into _queue READ or WRITE events
*/
{
    std::cerr << "start connection manager thread: " << pthread_self() << std::endl;
    struct pollfd fds[32768];       // 2^15

    while (1)
    {
        std::vector<int> disconnected_clients;

        if (m_ClientsWantWork.empty())
        {
            // no clients
            usleep(1000);
            continue;
        }

        for (size_t i = 0; i < m_ClientsWantWork.size(); ++i)
        {
            fds[i].fd = m_ClientsWantWork[i].first->m_Client.sd;

            if (m_ClientsWantWork[i].first->m_Client.state == client_state_t::WANT_READ)
                fds[i].events = POLLIN;
            else
                fds[i].events = POLLOUT;
            fds[i].revents = 0;
        }

        int poll_ret = poll(fds, m_ClientsWantWork.size(), /* timeout in msec */ 100);

        if (poll_ret == 0)
        {
            // nothing activity from any clients
            continue;
        }
        else if (poll_ret == -1)
        {
            std::cerr << "poll error!\n";
            return -1;
        }

        for (size_t i = 0; i < m_ClientsWantWork.size(); ++i)
        {
            if (fds[i].revents == 0)
                continue;

            if (fds[i].revents & POLLHUP)
            {
                // e.g. previous write() was in a already closed sd
                std::cerr << "client hup\n";
                disconnected_clients.push_back(fds[i].fd);
            }
            else if (fds[i].revents & POLLIN)
            {
                if (m_ClientsWantWork[i].first->m_Client.state != client_state_t::WANT_READ)
                    continue;
               
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    m_ClientsHaveWork.push(m_ClientsWantWork[i]);
                }
                delClient(fds[i].fd);
            }
            else if (fds[i].revents & POLLOUT)
            {
                if (m_ClientsWantWork[i].first->m_Client.state != client_state_t::WANT_WRITE)
                    continue;

                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    m_ClientsHaveWork.push(m_ClientsWantWork[i]);
                }
                delClient(fds[i].fd);
            }
            else if (fds[i].revents & POLLNVAL)
            {
                // e.g. if set clos'ed descriptor in poll
                std::cerr << "POLLNVAL !!! need remove this descriptor: " << fds[i].fd << "\n";
                disconnected_clients.push_back(fds[i].fd);
            }
            else
            {
                if (fds[i].revents & POLLERR)
                    std::cerr<<"WARNING> revents = POLLERR. [SD = " << fds[i].fd <<"]\n";
                else
                    std::cerr<<"WARNIG> revent = UNKNOWN_EVENT: "<< fds[i].revents<<" [SD = " << fds[i].fd<<"]\n";
            }
        }

        // remove disconnected clients
        for (size_t i = 0; i< disconnected_clients.size(); ++i)
            delClient(disconnected_clients[i]);

    }   // while (1)

    return 0;
}

namespace {
    std::mutex conn_manager_mutex;
}

void EventLoop::run()
/*
    pop() from _queue, doing real read() or write()
*/

{
    std::unique_ptr<std::thread> conn_manager;
    {
        std::lock_guard<std::mutex> lock(conn_manager_mutex);
        static int xyz = 0;
        if (xyz == 0)
        {
            conn_manager.reset(new std::thread(&EventLoop::manageConnections, this));
            usleep(1000);
            ++xyz;
        }
    }

    std::cerr << "start worker thread: " << pthread_self() << "\n";
    while (1)
    {
        event_cb_t event;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            if (m_ClientsHaveWork.empty())
            {
                lock.unlock();
                usleep(1000);
                continue;
            }

            event = m_ClientsHaveWork.front();
            m_ClientsHaveWork.pop();
        }

        if (event.first->m_Client.state == client_state_t::WANT_READ)
        {
            char buf[256];
            int r = read(event.first->m_Client.sd, buf, sizeof(buf));

            if (r < 0)
            {
                std::cerr << "some read error!\n";
                sleep(1);
                close(event.first->m_Client.sd);
                continue;
            }
            buf[r] = '\0';

            if (r > 0)
            {
                event.second(std::string(buf, buf + r));
            }
            else if (r == 0)
            {
                event.second("");
            }
        }
        else if (event.first->m_Client.state == client_state_t::WANT_WRITE)
        {
            write(event.first->m_Client.sd, event.first->m_WriteBuf.data(), event.first->m_WriteBuf.size());
            event.second("govno");
        }
        else
        {
            throw std::runtime_error("unknown client state");
        }
    }

    conn_manager->join();
}

std::shared_ptr<Connection> accept_work(int sd, EventLoop *ev)
{
    int timeout = 3000;     // msec

    std::shared_ptr<Connection> conn;

    struct pollfd fd;
    fd.fd = sd;
    fd.events = POLLIN;
    fd.revents = 0;

    while (1)
    {
        int poll_ret = poll(&fd, 1, /* timeout in msec */ timeout);

        if (poll_ret == 0)
        {
            continue;
        }
        else if (poll_ret == -1)
        {
            throw std::runtime_error("poll error");
        }
   
        struct sockaddr_in client;
        memset(&client, 0, sizeof(client));
        socklen_t cli_len = sizeof(client);

        int cli_sd = accept(sd, (struct sockaddr*)&client, &cli_len);
        conn.reset(new Connection(cli_sd, ev));
        return conn;
    }
    return conn;
}

void AsyncPollEngine::run()
{
    std::cerr << "async poll server starts" << std::endl;

    EventLoop ev;
    std::vector<std::thread> event_loop_threads;

    for (int i = 0; i < 4; ++i)
    {
        event_loop_threads.push_back(std::thread(std::bind(&EventLoop::run, &ev)));
        usleep(1000);
    }

    while(1)
    {
        std::shared_ptr<Connection> conn = accept_work(listener(), &ev);
        if (conn)
        {
            conn->read();
        }
    }
}

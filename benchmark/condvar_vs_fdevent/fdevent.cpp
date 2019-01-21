#include <sys/eventfd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <iostream>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>
#include <atomic>


// Documentation:
// http://man7.org/linux/man-pages/man2/eventfd.2.html

namespace
{

class Timer
{
public:
    Timer () { start(); }
    void start() { m_Start = now(); }
    uint64_t ms() const { return std::chrono::duration_cast<std::chrono::milliseconds>(now() - m_Start).count(); }
    uint64_t ns() const { return std::chrono::duration_cast<std::chrono::nanoseconds>(now() - m_Start).count(); }

private:
    std::chrono::time_point<std::chrono::steady_clock> now() const { return std::chrono::steady_clock::now(); }

private:
    std::chrono::time_point<std::chrono::steady_clock> m_Start;
};


template <typename T>
class Channel
{
public:
    Channel()
    {
        static_assert(sizeof(T) <= sizeof(uint64_t), "this toy works with uint64_t");
        m_Efd = eventfd(0, EFD_SEMAPHORE|EFD_NONBLOCK);
        if (m_Efd == -1)
        {
            throw std::runtime_error("create eventfd error");
        }
        m_Waiters = 0;

        FD_ZERO(&m_ReadFds);
        FD_SET(m_Efd, &m_ReadFds);
    }
public:
    int put(T &&val)
    {
        if (m_Closed)
        {
            return -1;
        }

        uint64_t u = static_cast<uint64_t>(val);
        ssize_t bytes = ::write(m_Efd, &u, sizeof(uint64_t));
        if (bytes != sizeof(uint64_t))
        {
            throw std::runtime_error("eventfd write failed");
        }
        return 0;
    }
    T get()
    {
        uint64_t u = 0;
        ssize_t bytes = ::read(m_Efd, &u, sizeof(uint64_t));
        if (bytes == -1 && errno == EAGAIN) 
        {
            ++m_Waiters;
            // fixme: timeout
            int sel = select(m_Efd + 1, /*read*/&m_ReadFds, /*write*/NULL, /*exceptions*/NULL, /*timeout*/NULL);
            if (sel != 1)
            {
                throw std::runtime_error("select error");
            }
            --m_Waiters;
            bytes = ::read(m_Efd, &u, sizeof(uint64_t));
        }

        if (bytes != sizeof(uint64_t))
        {
            throw std::runtime_error(std::string("eventfd read failed: ") + strerror(errno));
        }
        return static_cast<T>(u);
    }
    size_t waiters()
    {
        return m_Waiters;
    }
    void close()
    {
        std::unique_lock<std::mutex> lock(m_Mtx);
        m_Closed = true;
    }
private:
    int m_Efd = -1;
    fd_set m_ReadFds;
    bool m_Closed = false;
    std::atomic<size_t> m_Waiters;
    std::mutex m_Mtx;
};

uint64_t g_Sum = 0;                // to prevent compiler optimizations

}   // namespace


void consumer(Channel<int> &chan, uint64_t max_messages)
{
    uint64_t msg_received = 0;
    while (true)
    {
        g_Sum += chan.get();
        ++msg_received;
        if (msg_received >= max_messages)
        {
            chan.close();
            break;
        }
    }
}

int run(Channel<int> &chan, uint64_t max_messages)
{
    std::thread t(std::bind(consumer, std::ref(chan), max_messages));
    while (chan.waiters() != 1) {std::this_thread::sleep_for(std::chrono::milliseconds(1));}

    Timer timer;
    uint64_t msg_sended = 0;
    while (true)
    {
        if (   -1 == chan.put(1)
            || ++msg_sended >= max_messages)
        {
            std::cerr << "cycle time: " << timer.ms() << "ms\n";
            break;
        }
    }

    t.join();
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: ./" << argv[0] << " sum_threshold[uint64_t]\n";
        return -1;
    }

    try
    {
        uint64_t max_messages = std::stoul(argv[1]);
        std::cerr << "Test will transmit: " << max_messages << " messages through channel\n";

        Channel<int> chan;
        run(chan, max_messages);
        std::cerr << "sum: " << g_Sum << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception: " << e.what() << "\n";
        exit(-1);
    }
    return 0;
}

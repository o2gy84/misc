#include <semaphore.h>
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


// No real data transfer,
// just notify.

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
        m_Waiters = 0;

        int ret = sem_init(&m_Sema, /*shared_between_threads*/0, /*value*/0);
        if (ret == -1)
        {
            throw std::runtime_error(std::string("sem_init: ") + strerror(errno));
        }
    }
    ~Channel()
    {
        sem_destroy(&m_Sema); 
    }
public:
    int put(T &&val)
    {
        if (m_Closed)
        {
            return -1;
        }

        int ret = sem_post(&m_Sema);
        if (ret != 0)
        {
            throw std::runtime_error("sem_post: ");
        }

        return 0;
    }
    int getval()
    {
        int sval = 0;
        int ret = sem_getvalue(&m_Sema, &sval);
        if (ret != 0)
        {
            throw std::runtime_error("sem_getvalue: ");
        }
        return sval;
    }
    T get()
    {
        ++m_Waiters;
        int ret = sem_wait(&m_Sema);
        if (ret != 0)
        {
            throw std::runtime_error("sem_wait: ");
        }
        --m_Waiters;
        return static_cast<T>(1);
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
    sem_t m_Sema;
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

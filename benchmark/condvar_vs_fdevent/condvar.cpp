#include <iostream>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <queue>

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
    int put(T &&val)
    {
        std::unique_lock<std::mutex> lock(m_Mtx);
        if (m_Closed)
        {
            return -1;
        }

        m_Queue.emplace(std::move(val));
        m_Condvar.notify_one();
        return 0;
    }
    T get()
    {
        std::unique_lock<std::mutex> lock(m_Mtx);
        ++m_Waiters;
        m_Condvar.wait(lock, [this]() { return !m_Queue.empty(); } );
        --m_Waiters;
        T tmp = m_Queue.front();
        m_Queue.pop();
        return tmp;
    }
    size_t waiters()
    {
        std::unique_lock<std::mutex> lock(m_Mtx);
        return m_Waiters;
    }
    void close()
    {
        std::unique_lock<std::mutex> lock(m_Mtx);
        m_Closed = true;
    }
private:
    bool m_Closed = false;
    std::queue<T> m_Queue;
    size_t m_Waiters = 0;
    std::condition_variable m_Condvar;
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
    uint64_t ms = 0;
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

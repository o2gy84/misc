#include <mqueue.h>
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
// https://linux.die.net/man/3/mq_open
// https://linux.die.net/man/3/mq_send
// https://linux.die.net/man/3/mq_receive
// ...

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

#define Q_NAME "/test_queue"
#define Q_MAX_MSGS_COUNT 10

template <typename T>
class Channel
{
public:
    Channel()
    {
        static_assert(sizeof(T) <= sizeof(uint64_t), "this toy works with uint64_t");

        m_Waiters = 0;

        struct mq_attr attr;
        attr.mq_flags = 0;
        attr.mq_maxmsg = Q_MAX_MSGS_COUNT;
        attr.mq_msgsize = sizeof(uint64_t);
        attr.mq_curmsgs = 0;

        m_Mq = mq_open(Q_NAME, O_CREAT | O_EXCL | O_RDWR, 0644, &attr);
        if (-1 == m_Mq)
        {
            if (EEXIST == errno)
            {
                std::cerr << "queue will be delete, try again\n";
                mq_unlink(Q_NAME);
            }
            throw std::runtime_error(std::string("error create queue: ") + strerror(errno));
        }
    }
    ~Channel()
    {
        mq_close(m_Mq);
        mq_unlink(Q_NAME);
    }
public:
    int put(T &&val)
    {
        if (m_Closed)
        {
            return -1;
        }

        uint64_t v = static_cast<uint64_t>(val);
        char *p = reinterpret_cast<char*>(&v);
        int ret = mq_send(m_Mq, p, sizeof(v), 0);
        if (ret != 0)
        {
            throw std::runtime_error(std::string("mq_send: ") + strerror(errno));
        }
        return 0;
    }
    T get()
    {
        char buffer[sizeof(uint64_t)];
        ++m_Waiters;
        ssize_t bytes = mq_receive(m_Mq, buffer, sizeof(buffer), 0);
        --m_Waiters;
        if (bytes != sizeof(uint64_t))
        {
            throw std::runtime_error(std::string("mq_receive: ") + strerror(errno));
        }

        uint64_t *v = reinterpret_cast<uint64_t*>(&buffer);
        return static_cast<T>(*v);
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
    mqd_t m_Mq;
    bool m_Closed = false;
    std::queue<T> m_Queue;
    std::atomic<size_t> m_Waiters;
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

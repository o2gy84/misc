#include <semaphore.h>
#include <mqueue.h>
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
#include <memory>


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

template<typename T>
class BaseChannel
{
public:
    virtual ~BaseChannel() {}
    virtual int put(T &&val) = 0;
    virtual T get()          = 0;
    virtual size_t waiters() = 0;
    virtual void close()     = 0;
};

template <typename T>
class ChannelCondVar : public BaseChannel<T>
{
public:
    virtual int put(T &&val) override
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


// Documentation:
// http://man7.org/linux/man-pages/man2/eventfd.2.html
template <typename T>
class ChannelEventFd : public BaseChannel<T>
{
public:
    ChannelEventFd()
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


// Documentation:
// https://linux.die.net/man/3/mq_open
// https://linux.die.net/man/3/mq_send
// https://linux.die.net/man/3/mq_receive
// ...
#define Q_NAME "/test_queue"
#define Q_MAX_MSGS_COUNT 10

template <typename T>
class ChannelMq : public BaseChannel<T>
{
public:
    ChannelMq()
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
    ~ChannelMq()
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
    std::atomic<size_t> m_Waiters;
    std::mutex m_Mtx;
};

// No real data transfer,
// just notify.
template <typename T>
class ChannelSemaphore : public BaseChannel<T>
{
public:
    ChannelSemaphore()
    {
        m_Waiters = 0;

        int ret = sem_init(&m_Sema, /*shared_between_threads*/0, /*value*/0);
        if (ret == -1)
        {
            throw std::runtime_error(std::string("sem_init: ") + strerror(errno));
        }
    }
    ~ChannelSemaphore()
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


void consumer(BaseChannel<int> &chan, uint64_t max_messages)
{
    Timer timer;
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
    std::cerr << "receive cycle time: " << timer.ms() << "ms\n";
}

int run(BaseChannel<int> &chan, uint64_t max_messages)
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
            std::cerr << "send cycle time: " << timer.ms() << "ms\n";
            break;
        }
    }

    t.join();
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "usage: " << argv[0] << " sum_threshold[uint64_t] type[cond|efd|mq|sema]\n";
        return -1;
    }

    try
    {
        uint64_t max_messages = std::stoul(argv[1]);
        std::cerr << "Test will transmit: " << max_messages << " messages through channel\n";

        std::unique_ptr<BaseChannel<int>> chan;
        if      ("cond" == std::string(argv[2])) chan.reset(new ChannelCondVar<int>());
        else if ("efd"  == std::string(argv[2])) chan.reset(new ChannelEventFd<int>());
        else if ("mq"   == std::string(argv[2])) chan.reset(new ChannelMq<int>());
        else if ("sema" == std::string(argv[2])) chan.reset(new ChannelSemaphore<int>());
        else    throw std::runtime_error("not supported channel type");
        run(*chan, max_messages);
        std::cerr << "sum: " << g_Sum << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception: " << e.what() << "\n";
        exit(-1);
    }
    return 0;
}

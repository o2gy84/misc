#include <iostream>
#include <sstream>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>


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
    Channel(bool unlock_mtx_before_notify = false) : m_ForceUnlockMutexBeforeNotify(unlock_mtx_before_notify) {}
public:
    int put(T &&val) {
        Timer t;
        std::unique_lock<std::mutex> lock(m_Mtx);
        m_MutexLockTime_put += t.ns();

        if (m_Closed)
        {
            return -1;
        }

        m_Val = std::move(val);

        if (m_ForceUnlockMutexBeforeNotify)
        {
            lock.unlock();            // <-- that is sooo muuuuuuuch slower o0, who knows why?
        }

        t.start();
        m_Condvar.notify_one();
        m_NotifyOneTime_put += t.ns();
        return 0;
    }
    T get() {
        Timer t;
        std::unique_lock<std::mutex> lock(m_Mtx);
        m_MutexLockTime_get += t.ns();
        ++m_Waiters;
        m_Condvar.wait(lock, [this]() { return m_Val == 1; } );
        --m_Waiters;
        T tmp = m_Val;
        m_Val = T();
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
    std::string report()
    {
        std::unique_lock<std::mutex> lock(m_Mtx);
        std::stringstream ss;
        ss << "Channel::get()->mutex_lock: " << m_MutexLockTime_get/1000000 << "ms" << std::endl;
        ss << "Channel::put()->mutex_lock: " << m_MutexLockTime_put/1000000 << "ms" << std::endl;
        ss << "Channel::put()->notify_one: " << m_NotifyOneTime_put/1000000 << "ms";
        return ss.str();
    }
private:
    bool m_ForceUnlockMutexBeforeNotify = false;
    bool m_Closed = false;
    T m_Val = T();
    size_t m_MutexLockTime_get = 0;
    size_t m_MutexLockTime_put = 0;
    size_t m_NotifyOneTime_put = 0;
    size_t m_Waiters = 0;
    std::condition_variable m_Condvar;
    std::mutex m_Mtx;
};

uint64_t g_Sum = 0;                // to prevent compiler optimizations
uint64_t g_SumThresh = 0;

}   // namespace


void consumer(Channel<int> &chan)
{
    while (true)
    {
        g_Sum += chan.get();
        if (g_Sum >= g_SumThresh)
        {
            chan.close();
            break;
        }
    }
}

int run(Channel<int> &chan)
{
    std::thread t(std::bind(consumer, std::ref(chan)));
    while (chan.waiters() != 1) {std::this_thread::sleep_for(std::chrono::milliseconds(1));}

    Timer timer;
    uint64_t ms = 0;
    while (true)
    {
        if (-1 == chan.put(1))
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
    if (argc < 3)
    {
        std::cerr << "usage: ./" << argv[0] << " sum_threshold[uint64_t] force_mutex_unlock[bool]\n";
        return -1;
    }

    try
    {

        g_SumThresh = std::stoul(argv[1]);
        bool force_unlock_mutex = static_cast<bool>(std::stoi(argv[2]));
        std::cerr << "Sum threshold: " << g_SumThresh
                  << ", force_mutex_unlock: " << std::boolalpha << force_unlock_mutex << "\n";

        Channel<int> chan(force_unlock_mutex);

        run(chan);
        
        std::cerr << "sum: " << g_Sum << "\n";
        std::cerr << chan.report() << "\n";
    }
    catch (const std::exception &e)
    {
        std::cerr << "exception: " << e.what() << "\n";
        exit(-1);
    }
    return 0;
}

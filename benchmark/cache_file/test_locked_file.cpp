#include "common.hpp"

#define WH_KEY_PATH "/etc/resolv.conf"
#define WH_KEY_ID 'R'


int main(int argc, char *argv[])
{
    if (argc < 3
        || (argc >= 3 && L_NONE == str2lock(argv[2])))
    {
        std::cerr << "Usage: " << argv[0] << " path_to_file shmem|cache" << std::endl;
        return 0;
    }

    try
    {
        const std::string path(argv[1]);

        if (L_SHARED_MEM == str2lock(argv[2]))
        {
            WrappedShmem ws;
            ws.attach(WH_KEY_PATH, WH_KEY_ID);
            ssize_t index = ws.find_file_by_name(path.c_str());
            if (-1 == index)
            {
                std::cerr << "file not found into shared memory: " << path << std::endl;
                return -1;
            }
    
            const shmem_data_t &sd = ws[index];

            PerfTimer t;
            std::string data(reinterpret_cast<const char*>(sd.data), sd.data_size);
            int ms = t.ms();

            std::cerr << "read from shared memory at: " << ms << " msec" << std::endl;
            std::cerr << "size: " << data.size() << " bytes" << std::endl;
            std::cerr << "crc: " << my_crc(data.data(), data.size()) << std::endl;
        }
        else
        {
            PerfTimer t;
            std::string data = read_file_with_mmap(path.c_str());
            int read_ms = t.ms();
            std::cerr << "read from disk at: " << read_ms << " msec" << std::endl;
            std::cerr << "size: " << data.size() << " bytes" << std::endl;
            std::cerr << "crc: " << my_crc(data.data(), data.size()) << std::endl;

            char *buf = new char[data.size() + 1];
            t.reset();
            size_t len = data.copy(buf, data.size(), 0);
            int copy_ms = t.ms();
            buf[len] = '\0';
            std::cerr << "read from heap at: " << copy_ms << " msec" << std::endl;
            std::cerr << "size: " << len << " bytes" << std::endl;
            std::cerr << "crc: " << my_crc(buf, len) << std::endl;

            if ((float)read_ms/(float)copy_ms <= 2.0)
            {
                std::cerr << "file is probably CACHED" << std::endl;
            }
            else
            {
                std::cerr << "file is probably NOT CACHED" << std::endl;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[e] " << e.what() << "\n";
    }

    return 0;
}

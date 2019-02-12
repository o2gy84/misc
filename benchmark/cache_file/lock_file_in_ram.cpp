#include "common.hpp"

#define WH_KEY_PATH "/etc/resolv.conf"
#define WH_KEY_ID 'R'

void my_wait()
{
    while (true)
    {
        std::string s;
        std::cin >> s;
        if (s == "quit" || s == "exit" || s == "q")
        {
            break;
        }
    }
}

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
        FILE *f = fopen(path.c_str(), "r");
        if (NULL == f)
        {
            std::cerr << "fopen: " << strerror(errno) << std::endl;
            return -1;
        }
        fseek(f, 0, SEEK_END);
        size_t f_size = ftell(f);

        void *data = mmap(NULL, f_size, PROT_READ, MAP_PRIVATE, fileno(f), 0);
        if (data == MAP_FAILED)
        {
            std::cerr << "mmap failed: " << strerror(errno) << std::endl;
            return -1;
        }

        if (L_PAGE_CACHE == str2lock(argv[2]))
        {
            if (mlock(data, f_size) != 0)
            {
                std::cerr << "mlock failed: " << strerror(errno) << std::endl;
                return -1;
            }

            std::cerr << "file: " << path << " is locked into RAM" << std::endl;
            std::cerr << "size: " << f_size << " bytes" << std::endl;
            std::cerr << "crc: " << my_crc(static_cast<char*>(data), f_size) << std::endl;

            my_wait();
            if (munlock(data, f_size) != 0)
            {
                std::cerr << "munlock failed: " << strerror(errno) << std::endl;
            }
        }
        else
        {
            WrappedShmem ws;
            ws.attach_or_create(2, WH_KEY_PATH, WH_KEY_ID);

            ws.lock();  // prevent from swap

            shmem_data_t *sd = ws.add_item();
            if (NULL == sd)
            {
                std::cerr << "can't add one more item into shared memory" << std::endl;
                return -1;
            }

            if (f_size >= sizeof(sd->data))
            {
                std::cerr << "too large file" << std::endl;
                return -1;
            }
            memcpy(sd->data, data, f_size);
            sd->data_size = f_size;

            int sz = snprintf(sd->fname, sizeof(sd->fname), "%s", path.c_str());
            if (sz >= sizeof(sd->fname))
            {
                std::cerr << "too long name" << std::endl;
                return -1;
            }

            std::cerr << "file: " << path << " is stored into shared memory" << std::endl;
            std::cerr << "size: " << f_size << " bytes" << std::endl;
            std::cerr << "crc: " << my_crc(static_cast<char*>(data), f_size) << std::endl;

            if (0 != munmap(data, f_size))
            {
                std::cerr << "munmap failed: " << strerror(errno) << std::endl;
                return -1;
            }

            my_wait();
            ws.destroy();
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[e] " << e.what() << "\n";
    }

    return 0;
}

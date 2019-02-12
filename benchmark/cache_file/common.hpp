#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include <fstream>
#include <string>
#include <exception>
#include <stdexcept>
#include <iostream>


enum lock_type_t
{
    L_NONE,
    L_SHARED_MEM,
    L_PAGE_CACHE
};

lock_type_t str2lock(const char *str)
{
    if (strcmp(str, "shmem") == 0) return L_SHARED_MEM;
    if (strcmp(str, "cache") == 0) return L_PAGE_CACHE;
    return L_NONE;
}

class PerfTimer
{
public:
    PerfTimer()
    {
        reset();
    }
    void reset()
    {
        _timer = 0;
        gettimeofday(&_start, 0);
    }
    uint64_t ms()
    {
        timeval tmp;
        gettimeofday(&tmp, 0);
        _timer += int64_t(tmp.tv_sec - _start.tv_sec) * 1000000 + int64_t(tmp.tv_usec - _start.tv_usec);
        return _timer / 1000;
    }
private:
    int64_t _timer;  // microseconds
    timeval _start;
};

std::string read_file_with_mmap(const char *path)
{
    FILE *f = fopen(path, "r");
    if (NULL == f)
    {
        throw std::runtime_error("fopen: " + std::string(strerror(errno)));
    }

    struct stat s;
    if (0 != stat(path, &s))
    {
        throw std::runtime_error("stat: " + std::string(strerror(errno)));
    }

    size_t size = s.st_size;

    // todo: munmup
    const char *data = static_cast<const char*>(mmap(NULL, size, PROT_READ, MAP_PRIVATE, fileno(f), 0));
    if (data == MAP_FAILED)
    {
        throw std::runtime_error("mmap: " + std::string(strerror(errno)));
    }
    std::string big_file(data, size);
    return big_file;
}

int32_t my_crc(const char *data, size_t size)
{
    int ret = 42;
    for (size_t i = 0; i < size; ++i)
    {
        ret ^= data[i];
    }
    return ret;
}


//#define MAX_DATA_SIZE 1073741824      // 1Gb
#define MAX_DATA_SIZE 1610612736        // 1.5Gb


// FIXME: need mutex to protect shared data
struct shmem_data_t
{
    size_t data_size;
    uint8_t data[MAX_DATA_SIZE];
    char fname[256];
};

void attach_shmem(int shmid, shmem_data_t **wh)
{
     *wh = (shmem_data_t*)shmat(shmid, 0, 0); 
     if (*wh == static_cast<void*>(0))
     {
         throw std::runtime_error("can't get pointer of shared memory pool");
     }
     std::cerr << "attached shared memory [id: " << shmid << ", pid: " << getpid() << "]\n";
}

int create_shmem(key_t key, size_t size)
{
    int shmid = shmget(key, sizeof(shmem_data_t) * size, 0666 | IPC_CREAT);
    if (shmid < 0)
    {
        std::cerr << "error shmget: " << strerror(errno) << ", size: " << size << "\n";
        throw std::runtime_error("shmget");
    }
    std::cerr << "create shared memory [id: " << shmid << ", size: " << sizeof(shmem_data_t)*size << " bytes, pid: " << getpid() << "]\n"; 
    return shmid;
}

int find_shmem_by_key(key_t key)
{
     std::cerr << "check for existing shared memory [key: " << key << "]\n";
     int shmid = shmget(key, 0, 0);
     if (shmid < 0)
     {
         std::cerr << "shared memory not exist [key: " << key << "]\n";
         return -1;
     }
     std::cerr << "shared memory exist [key: " << key << "]\n";
     return shmid;
}

size_t segment_bytes(int shmid)
{
    shmid_ds shmem_info;
    memset(&shmem_info, 0, sizeof(shmem_info));
    if (-1 == shmctl(shmid, IPC_STAT, &shmem_info))
    {
        throw std::runtime_error("shmctl error: " + std::string(strerror(errno)));
    }
    return shmem_info.shm_segsz;
}

bool check_sizeof_shmem(int shmid, size_t new_items_count)
{
    size_t old_items_count = segment_bytes(shmid) / sizeof(shmem_data_t);
    if (old_items_count != new_items_count)
    {
        std::cerr << "existed shared memory size missmatch."
                  << " this segment will be destroyed. please restart the program\n"; 

        if (shmctl(shmid, IPC_RMID, NULL) < 0)
        {
            std::cerr << "can't mark shared memory pool for destroying: " << strerror(errno) << "\n";
        }
        return false;
    }
    return true;
}

class WrappedShmem
{
public:
    WrappedShmem()
        : _data(NULL)
        , _capacity(0)
        , _shmid(-1)
    {
    }

    void destroy()
    {
        if (_data)
        {
            std::cerr << "delete shmem\n";
            shmdt(_data);
        }
        _data = NULL;

        if (_shmid > 0)
        {
            if (shmctl(_shmid, IPC_RMID, NULL) < 0)
            {
                std::cerr << "can't mark shared memory pool for destroying: " << strerror(errno) << "\n";
            }
        }
    }

    void lock()
    {
        if (shmctl(_shmid, SHM_LOCK, NULL) < 0)
        {
            std::cerr << "can't lock shared memory: " << strerror(errno) << "\n";
            return;
        }
        std::cerr << "shared memory locked\n";
    }

    void attach_or_create(size_t capacity, const char *k_path, char k_id)
    {
        if (capacity == 0)
        {
            throw std::runtime_error("capacity should be a positive number");
        }
        _shmid = attach_or_create_impl(capacity, k_path, k_id);
        _capacity = capacity;
    }

    void attach(const char *k_path, char k_id)
    {
        _shmid = get_shmid(k_path, k_id);
        if (_shmid > 0)
        {
            attach_shmem(_shmid, &_data);
            _capacity = segment_bytes(_shmid) / sizeof(shmem_data_t);
            std::cerr << "capacity of shmem is: " << _capacity << std::endl;
            return;
        }
        throw std::runtime_error("not found shared memory segment with path: " + std::string(k_path));
    }

    const shmem_data_t& operator[](size_t i)
    {
        if (NULL == _data) throw std::runtime_error("not inited"); 
        if (0 == _capacity) throw std::runtime_error("shmem not inited or does not contain items");
        if (i >= _capacity) throw std::runtime_error("bad range");
        return _data[i];
    }

    shmem_data_t* add_item()
    {
        for (size_t i = 0; i < _capacity; ++i)
        {
            if (strlen((_data + i)->fname) == 0 && (_data + i)->data_size == 0)
            {
                //snprintf((_data + i)->fname, sizeof((_data + i)->fname), "%s", item.fname);
                //snprintf((_data + i)->data, sizeof((_data + i)->data), "%s", item.data);
                //(_data + i)->data_size = item.data_size;
                return &_data[i];
            }
        }
        return NULL;
    }

    ssize_t find_file_by_name(const char *name)
    {
        for (size_t i = 0; i < _capacity; ++i)
        {
            if (strcmp(name, (_data + i)->fname) == 0)
            {
                return i;
            }
        }
        return -1;
    }

private:
    int attach_or_create_impl(size_t capacity, const char *k_path, char k_id)
    {
        int shmid = get_shmid(k_path, k_id);
        if (shmid > 0)
        {
            if (!check_sizeof_shmem(shmid, capacity))
            {
                throw std::runtime_error("need restart");
            }

            attach_shmem(shmid, &_data);
            return shmid;
        }

        key_t key = get_shmem_key(k_path, k_id);
        shmid = create_shmem(key, capacity);
        attach_shmem(shmid, &_data);
        return shmid;
    }

    key_t get_shmem_key(const char *k_path, char k_id) const
    {
        key_t key = ftok(k_path, k_id); 
        if (key < 0)
        {
            throw std::runtime_error("can't get key for shared memory pool");
        }
        return key;
    }

    int get_shmid(const char *k_path, char k_id) const
    {
        key_t key = get_shmem_key(k_path, k_id);
        return find_shmem_by_key(key);
    }

private:
    shmem_data_t *_data;
    size_t _capacity;
    int _shmid;
};


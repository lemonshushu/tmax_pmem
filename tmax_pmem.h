#include <stddef.h>
#include <stdio.h>

enum
{
    /**
     * Operation success.
     */
    SUCCESS = 0,

    /**
     * Error: Memory kind is not available.
     */
    ERROR_UNAVAILABLE = -1,

    /**
     * Error: Call to mmap() failed.
     */
    ERROR_MMAP = -3,

    /**
     * Error: Call to malloc() failed.
     */
    ERROR_MALLOC = -6,

    /**
     * Error: Unable to parse environment variable.
     */
    ERROR_ENVIRON = -12,

    /**
     * Error: Invalid argument.
     */
    ERROR_INVALID = -13,

    /**
     * Error: Unspecified run-time error.
     */
    ERROR_RUNTIME = -255
};

struct pmem_file
{
    int fd;              // file descriptor
    off_t offset;        // offset of the file
    size_t current_size; // current size of the file
    char *dir;           // directory of the file
};

void *request_pmem(const char *dir, void *addr, size_t size, struct pmem_file *pfile);
int pmem_create_tmpfile(const char *dir, int *fd);
int pmem_cleanup(void *addr, struct pmem_file *pfile);
static int pmem_recreate_file(struct pmem_file *pfile, size_t size);

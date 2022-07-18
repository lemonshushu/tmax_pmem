/**
 * @brief APIs to use PMEM like a volatile memory. Whenever the function is called, a temporary file is created on the PMEM and mapped to the virtual memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <signal.h>
#include <pthread.h>

/* The error code set by various library functions.  */
extern int *__errno_location(void) __THROW __attribute_const__;
#define errno (*__errno_location())

enum
{
    /**
     * Operation success.
     */
    MEMKIND_SUCCESS = 0,

    /**
     * Error: Memory kind is not available.
     */
    MEMKIND_ERROR_UNAVAILABLE = -1,

    /**
     * Error: Call to mbind() failed.
     */
    MEMKIND_ERROR_MBIND = -2,

    /**
     * Error: Call to mmap() failed.
     */
    MEMKIND_ERROR_MMAP = -3,

    /**
     * Error: Call to malloc() failed.
     */
    MEMKIND_ERROR_MALLOC = -6,

    /**
     * Error: Unable to parse environment variable.
     */
    MEMKIND_ERROR_ENVIRON = -12,

    /**
     * Error: Invalid argument.
     */
    MEMKIND_ERROR_INVALID = -13,

    /**
     * Error: Attempt to initialize more than MEMKIND_MAX_KIND number of kinds.
     */
    MEMKIND_ERROR_TOOMANY = -15,

    /**
     * Error: Invalid memkind_ops structure.
     */
    MEMKIND_ERROR_BADOPS = -17,

    /**
     * Error: Unable to allocate huge pages.
     */
    MEMKIND_ERROR_HUGETLB = -18,

    /**
     * Error: Requested memory type is not available.
     */
    MEMKIND_ERROR_MEMTYPE_NOT_AVAILABLE = -20,

    /**
     * Error: Operation failed.
     */
    MEMKIND_ERROR_OPERATION_FAILED = -21,

    /**
     * Error: Call to jemalloc's arenas.create failed.
     */
    MEMKIND_ERROR_ARENAS_CREATE = -22,

    /**
     * Error: Unspecified run-time error.
     */
    MEMKIND_ERROR_RUNTIME = -255
};

struct pmem_file
{
    int fd;                    // file descriptor
    off_t offset;              // offset of the file
    size_t max_size;           // maximum size of the file
    pthread_mutex_t pmem_lock; // lock for the file
    size_t current_size;       // current size of the file
    char *dir;                 // directory of the file
};

int memkind_create_pmem(const char *dir, size_t max_size, struct pmem_file **kind_pmem)
{
    int oerrno;

    int fd = -1;
    char name[16];

    int err = memkind_pmem_create_tmpfile(dir, &fd);
    if (err)
        goto exit;

    snprintf(name, sizeof(name), "pmem%08x", fd);

    err = memkind_create(name, kind_pmem);
    if (err)
        goto exit;

    (*kind_pmem)->fd = fd;
    (*kind_pmem)->offset = 0;
    (*kind_pmem)->current_size = 0;
    (*kind_pmem)->max_size = max_size;
    (*kind_pmem)->dir = (char *)malloc(strlen(dir) + 1);
    if (!((*kind_pmem)->dir))
        goto exit;
    memcpy((*kind_pmem)->dir, dir, strlen(dir));

    return err;
exit:
    oerrno = errno;
    if (fd != -1)
        (void)close(fd);
    errno = oerrno;
    return err;
}

int memkind_pmem_create_tmpfile(const char *dir, int *fd)
{
    static char template[] = "/memkind.XXXXXX";
    int err = MEMKIND_SUCCESS;
    int oerrno;
    int dir_len = strlen(dir);

    if (dir_len > PATH_MAX)
    {
        printf("Could not create temporary file: too long path.");
        return MEMKIND_ERROR_INVALID;
    }

    char fullname[dir_len + sizeof(template)];
    (void)strcpy(fullname, dir);
    (void)strcat(fullname, template);

    sigset_t set, oldset;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, &oldset);

    if ((*fd = mkstemp(fullname)) < 0) // Create a temporary file
    {
        log_err("Could not create temporary file: errno=%d.", errno);
        err = MEMKIND_ERROR_INVALID;
        goto exit;
    }

    (void)unlink(fullname);
    (void)sigprocmask(SIG_SETMASK, &oldset, NULL);

    return err;

exit:
    oerrno = errno;
    (void)sigprocmask(SIG_SETMASK, &oldset, NULL);
    if (*fd != -1)
        (void)close(*fd);
    fd = -1;
    errno = oerrno;
    return err;
}

int memkind_pmem_create(struct pmem_file *kind_pmem, const char *name)
{
    int err;

    kind_pmem = (struct pmem_file *)malloc(sizeof(struct pmem_file));
    if (!kind_pmem)
    {
        printf("[%s] malloc failed\n", __func__);
        return MEMKIND_ERROR_MALLOC;
    }

    if (pthread_mutex_init(&kind_pmem->pmem_lock, NULL) != 0)
    {
        err = MEMKIND_ERROR_RUNTIME;
        goto exit;
    }

    return 0;
exit:
    pthread_mutex_destroy(&kind_pmem->pmem_lock);
    free(kind_pmem);
    return err;
}

static int memkind_create(const char *name, struct pmem_file **kind_pmem)
{
    int err;
    *kind_pmem = (struct pmem_file *)calloc(1, sizeof(struct pmem_file));
    if (!*kind_pmem)
    {
        err = MEMKIND_ERROR_MALLOC;
        printf("[%s] malloc failed\n", __func__);
        return err;
    }
    err = memkind_pmem_create(*kind_pmem, name);
    if (err)
    {
        free(*kind_pmem);
        return err;
    }
}

int memkind_pmem_destroy(struct pmem_file *kind_pmem)
{
    pthread_mutex_destroy(&kind_pmem->pmem_lock);
    (void)close(kind_pmem->fd);
    free(kind_pmem->dir);
    free(kind_pmem);

    return 0;
}
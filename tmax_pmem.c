/**
 * @brief APIs to use PMEM like a volatile memory. Whenever the function is called, a temporary file is created on the PMEM and mapped to the virtual memory.
 */

#include <tmax_pmem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <signal.h>
#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>


/**
 * @brief Request pmem allocation. The function creates a temporary file on the PMEM and maps it to the virtual memory.
 *
 * @param dir Directory of the file.
 * @param size The size of the memory to be allocated.
 * @param addr Address of the memory to be mapped to the temporary file. If addr is NULL, the function allocates memory.
 * @param pfile Pointer to the pmem_file structure.
 * @return void * The pointer to the memory allocated.
 */
void *request_pmem(const char *dir, void *addr, size_t size, struct pmem_file *pfile)
{
    int oerrno;

    int fd = -1;

    int err = pmem_create_tmpfile(dir, &fd);
    if (err)
        goto exit;

    if (ftruncate(fd, size)) // set the size of the file
        goto exit;

    if (!(pfile = (struct pmem_file *)malloc(sizeof(struct pmem_file))))
    {
        printf("[%s] malloc failed\n", __func__);
        goto exit;
    }

    pfile->fd = fd;
    pfile->offset = 0;
    pfile->current_size = size;
    pfile->dir = strdup(dir);
    if (!pfile->dir)
    {
        err = ERROR_MALLOC;
        goto exit;
    }

    // Map the file to the virtual memory
    addr = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED)
    {
        err = ERROR_MMAP;
        goto exit;
    }

    return addr;

exit:
    oerrno = errno;
    if (fd != -1)
        (void)close(fd);
    errno = oerrno;
    return NULL;
}

/**
 * @brief Create a temporary file on the PMEM and adjust size of the file.
 *
 * @param dir Directory of the file.
 * @param size Size of the file.
 * @param fd File descriptor of the file.
 * @return int
 */
int pmem_create_tmpfile(const char *dir, int *fd)
{
    static char template[] = "/memkind.XXXXXX";
    int err = SUCCESS;
    int oerrno;
    int dir_len = strlen(dir);

    // Check if the directory exists
    if (access(dir, F_OK))
    {
        printf("[%s] directory %s does not exist\n", __func__, dir);
        return ERROR_INVALID;
    }

    if (dir_len > PATH_MAX)
    {
        printf("Could not create temporary file: too long path.");
        return ERROR_INVALID;
    }

    char fullname[dir_len + sizeof(template)];
    (void)strcpy(fullname, dir);
    (void)strcat(fullname, template);

    sigset_t set, oldset;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, &oldset);

    if ((*fd = mkstemp(fullname)) < 0) // Create a temporary file
    {
        printf("Could not create temporary file: errno=%d.", errno);
        err = ERROR_INVALID;
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
    fd = (int *)-1;
    errno = oerrno;
    return err;
}

/**
 * @brief Close the file and free the memory.
 * @param addr Memory address mapped to the file.
 * @param pfile Pointer to the pmem_file struct.
 *
 * @return int
 */
int pmem_cleanup(void *addr, struct pmem_file *pfile)
{
    if (munmap(addr, pfile->current_size) != 0)
    {
        printf("[%s] munmap failed\n", __func__);
        return ERROR_MMAP;
    }
    (void)close(pfile->fd);
    free(pfile->dir);
    free(pfile);

    return SUCCESS;
}

/**
 * @brief Resize the file.
 *
 * @param pfile Pointer to the pmem_file struct.
 * @param size New size of the file.
 * @return int
 */
static int pmem_recreate_file(struct pmem_file *pfile, size_t size)
{
    int status = -1;
    int fd = -1;
    int err = pmem_create_tmpfile(pfile->dir, &fd);
    if (err)
        goto exit;
    if ((errno = posix_fallocate(fd, 0, (off_t)size)) != 0)
        goto exit;
    close(pfile->fd);
    pfile->fd = fd;
    pfile->offset = 0;
    pfile->current_size = size;
    status = 0;
exit:
    return status;
}

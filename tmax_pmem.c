/**
 * @brief APIs to use PMEM like a volatile memory. Whenever the function is called, a temporary file is created on the PMEM and mapped to the virtual memory.
 */

#include <tmax_pmem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <dirent.h>

/**
 * @brief Request pmem allocation. The function creates a temporary file on the PMEM and maps it to the virtual memory.
 *
 * @param dir Directory of the file.
 * @param size The size of the memory to be allocated.
 * @param addr Address of the memory to be mapped to the temporary file. If addr is NULL, the function allocates memory.
 * @param pfile Pointer to the pmem_file structure.
 * @return void * The pointer to the memory allocated.
 */
void *pmem_malloc(const char *dir, void *addr, size_t size, struct pmem_file **pfile_ptr)
{
    int oerrno;
    int err;

    *pfile_ptr = (struct pmem_file *)malloc(sizeof(struct pmem_file));
    if (*pfile_ptr == NULL)
    {
        err = ERROR_MALLOC;
        goto exit;
    }

    err = pmem_create_tmpfile(dir, pfile_ptr);
    if (err)
        goto exit;

    if (ftruncate((*pfile_ptr)->fd, size)) // set the size of the file
        goto exit;

    // Map the file to the virtual memory
    addr = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_SHARED, (*pfile_ptr)->fd, 0);
    if (addr == MAP_FAILED)
    {
        err = ERROR_MMAP;
        goto exit;
    }

    (*pfile_ptr)->current_size = size;

    return addr;

exit:
    oerrno = errno;
    if ((*pfile_ptr)->fd != -1)
        (void)close((*pfile_ptr)->fd);
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
int pmem_create_tmpfile(const char *dir, struct pmem_file **pfile_ptr)
{
    static char template[] = "/pmem.XXXXXX";
    int err = SUCCESS;
    int oerrno;
    int dir_len = strlen(dir);

    // Check if the directory exists
    if (access(dir, F_OK))
    {
        err = ERROR_INVALID;
        return err;
    }

    if (dir_len > PATH_MAX)
    {
        printf("Could not create temporary file: too long path.");
        return ERROR_INVALID;
    }

    char *fullname = (char *)malloc(dir_len + sizeof(template));
    (void)strcpy(fullname, dir);
    (void)strcat(fullname, template);

    sigset_t set, oldset;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, &oldset);

    if (((*pfile_ptr)->fd = mkstemp(fullname)) < 0) // Create a temporary file
    {
        printf("Could not create temporary file: errno=%d.", errno);
        err = ERROR_INVALID;
        goto exit;
    }
    (*pfile_ptr)->fullpath = fullname;

    (void)sigprocmask(SIG_SETMASK, &oldset, NULL);

    return err;

exit:
    oerrno = errno;
    (void)sigprocmask(SIG_SETMASK, &oldset, NULL);
    if ((*pfile_ptr)->fd != -1)
        (void)close((*pfile_ptr)->fd);
    (*pfile_ptr)->fd = (int *)-1;
    errno = oerrno;
    free(fullname);
    return err;
}

/**
 * @brief Close the file and free the memory.
 * @param addr Memory address mapped to the file.
 * @param pfile Pointer to the pmem_file struct.
 *
 * @return int
 */
int pmem_free(void *addr, struct pmem_file **pfile_ptr)
{
    if (munmap(addr, (*pfile_ptr)->current_size) != 0)
    {
        printf("[%s] munmap failed\n", __func__);
        return ERROR_MMAP;
    }
    (void)close((*pfile_ptr)->fd);
    // Remove the file
    if (unlink((*pfile_ptr)->fullpath) != 0)
    {
        printf("[%s] unlink failed\n", __func__);
        return ERROR_RUNTIME;
    }
    free((*pfile_ptr)->fullpath);
    free(*pfile_ptr);

    return SUCCESS;
}

/**
 * @brief Delete all files in the directory.
 *
 * @param dir
 * @return int
 */
int pmem_cleanup_all(const char *dir)
{
    int err = SUCCESS;
    DIR *dirp = opendir(dir);
    if (dirp == NULL)
    {
        err = ERROR_INVALID;
        goto exit;
    }
    struct dirent *dp;
    while ((dp = readdir(dirp)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
            continue;
        char *fullpath = (char *)malloc(strlen(dir) + strlen(dp->d_name) + 2);
        (void)strcpy(fullpath, dir);
        (void)strcat(fullpath, "/");
        (void)strcat(fullpath, dp->d_name);
        if (unlink(fullpath) != 0)
        {
            printf("[%s] unlink failed\n", __func__);
            err = ERROR_RUNTIME;
            goto exit;
        }
        free(fullpath);
    }
    closedir(dirp);

    return err;

exit:
    return err;
}
/**
 * @file tmax_pmem.c
 * @author 송수빈 (subin_song@tmax.co.kr)
 * @brief Persistent Memory 디바이스를 Volatile한 방식으로 사용하기 위한 함수들을 제공한다.
 * @version 0.1
 * @date 2022-07-14
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <signal.h>

enum
{
    INIT_SUCCESS = 1,
    CLEANUP_SUCCESS = 2,
    INIT_ERROR_DIRPATH_TOOLONG = -1,
    INIT_ERROR_MKSTEMP_FAILED = -2,
}; // Enums for error codes and success code

struct memkind_pmem
{
    int fd;
    off_t offset;
    size_t max_size;
    pthread_mutex_t pmem_lock;
    size_t current_size;
    char *dir;
};

/* globals */
struct memkind_pmem *pmem_info;

/**
 * @brief 임시 파일을 생성하고 파일 정보를 pmem_info에 저장한다.
 *
 * @param dir 임시 파일을 생성할 디렉토리 경로
 * @param filename 임시 파일의 이름
 * @param max_size 임시 파일의 최대 크기
 * @return int
 */
int pmem_init(const char *dir, const char *filename, size_t max_size)
{
    int fd = -1;
    int dir_len = strlen(dir);

    if (dir_len > PATH_MAX)
    {
        printf("[%s] dir path is too long\n", __func__);
        return INIT_ERROR_DIRPATH_TOOLONG;
    }

    char *fullname[dir_len + strlen(filename) + 2];
    sprintf(fullname, "%s/%s", dir, filename);

    sigset_t set, oldset;
    sigfillset(&set);
    sigprocmask(SIG_BLOCK, &set, &oldset);

    if ((fd = mkstemp(fullname)) == -1) // Create a temporary file
    {
        printf("[%s] mkstemp failed\n", __func__);
        sigprocmask(SIG_SETMASK, &oldset, NULL);
        if (fd != -1)
            close(fd);
        return INIT_ERROR_MKSTEMP_FAILED;
    }

    unlink(fullname);
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    pmem_info = (struct memkind_pmem *)malloc(sizeof(struct memkind_pmem));
    pmem_info->fd = fd;
    pmem_info->offset = 0;
    pmem_info->current_size = 0;
    pmem_info->max_size = max_size;
    pmem_info->dir = (char *)malloc(dir_len + 1);
    strcpy(pmem_info->dir, dir);

    return INIT_SUCCESS;
}

/**
 * @brief 임시 파일을 제거한다.
 * @return int
 */
int pmem_cleanup()
{
    pthread_mutex_destroy(&pmem_info->pmem_lock);
    close(pmem_info->fd);
    free(pmem_info->dir);
    free(pmem_info);
    return CLEANUP_SUCCESS;
}
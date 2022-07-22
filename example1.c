/**
 * @brief Example code using APIs in tmax_pmem.c
 */

#include <tmax_pmem.h>
#include <stdio.h>
#include <string.h>

int main()
{
    const char *dir = "/pmem/tmp/";
    const char *str1 = "Hello World";
    const char *str2 = "Changed String";

    pmem_cleanup_all(dir);

    struct pmem_file *pfile;
    volatile void *addr;
    addr = request_pmem(dir, NULL, 1024000000 * sizeof(char), &pfile);
    if (addr == NULL)
    {
        printf("[%s] request_pmem failed\n", __func__);
        goto exit;
    }
    for (int i = 0; i < 1024000000; i++)
    {
        ((char *)addr)[i] = 'a';
    }

    strcpy(addr, str1);
    printf("[%s] addr: %s\n", __func__, (char *)addr);

    // Change string inside addr
    strcpy(addr, str2);
    printf("[%s] addr: %s\n", __func__, (char *)addr);

    // Check content inside file and print it
    FILE *fp = fopen(pfile->fullpath, "r");
    if (fp == NULL)
    {
        printf("[%s] fopen failed\n", __func__);
        goto exit;
    }
    char buf[100];
    fgets(buf, 100, fp);
    printf("[%s] buf: %s\n", __func__, buf);
    fclose(fp);

    pmem_cleanup(addr, &pfile);
    return 0;
exit:
    return -1;
}
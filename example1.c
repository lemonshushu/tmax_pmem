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
    struct pmem_file *pfile;
    void *addr;
    addr = request_pmem(dir, NULL, strlen(str1) + 1, &pfile);
    if (addr == NULL)
    {
        printf("[%s] request_pmem failed\n", __func__);
        goto exit;
    }
    strcpy(addr, str1);

    // Print string inside addr
    printf("[%s] addr: %s\n", __func__, (char *)addr);
    pmem_cleanup(addr, &pfile);
    return 0;
exit:
    return -1;
}
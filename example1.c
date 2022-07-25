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
    int i;

    pmem_cleanup_all(dir);

    struct pmem_file *pfile;
    char *addr;
    addr = (char *)pmem_malloc(dir, NULL, 1024000000 * sizeof(char), &pfile);
    if (addr == NULL)
    {
        printf("[%s] pmem_malloc failed\n", __func__);
        goto exit;
    }
    for (i = 0; i < 1024000000; i++)
    {
        addr[i] = 'a';
    }

    strcpy(addr, str1);
    printf("[%s] addr: %s\n", __func__, (char *)addr);

    // Change string inside addr
    strcpy(addr, str2);
    printf("[%s] addr: %s\n", __func__, (char *)addr);

    pmem_free(addr, &pfile);
    return 0;
exit:
    return -1;
}
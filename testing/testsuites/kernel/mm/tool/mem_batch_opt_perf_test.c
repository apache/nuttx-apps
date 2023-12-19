/****************************************************************************
 * Included Files
 ****************************************************************************/
#include <nuttx/config.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>
#include "MmTest.h"

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/
static void show_usage(void)
{
    printf("\nUsage: mm_stress_test  <min_size>  <max_size>  <list_length> <test_num>\n");
    printf("\nWhere:\n");
    printf("  <min_size>    Minimum number of memory requests.\n");
    printf("  <max_size>    Maximum number of memory requests.\n");
    printf("  <list_length> The length of the memory request list.\n");
    printf("  <test_num>    Number of tests.\n");
}

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
    int malloc_size = 0;
    int mallc_min_size; /* The minimum memory length requested in the test */
    int mallc_max_size; /* The maximum memory length requested in the test */
    int list_length;
    int test_num = 1;
    struct timespec t_start;
    struct timespec t_end;
    long timedif;
    char *address_ptr = NULL;
    char **mem_list = NULL;

    if (argc < 5)
    {
        syslog(LOG_WARNING, "Missing required arguments\n");
        show_usage();
        exit(1);
    }

    mallc_min_size = atoi(argv[1]);
    mallc_max_size = atoi(argv[2]);
    list_length = atoi(argv[3]);
    test_num = atoi(argv[4]);

    for (int n = 1; n <= test_num; n++)
    {
        mem_list = (char **)malloc(sizeof(char *) * list_length);
        if (mem_list == NULL)
        {
            syslog(LOG_ERR, "Failed to apply for the memory list !\n");
            exit(1);
        }

        for (int i = 0; i < list_length; i++)
        {
            srand(i + gettid());
            malloc_size = mmtest_get_rand_size(mallc_min_size, mallc_max_size);

            clock_gettime(CLOCK_MONOTONIC, &t_start);
            address_ptr = (char *)malloc(malloc_size * sizeof(char));
            clock_gettime(CLOCK_MONOTONIC, &t_end);
            timedif = 1000000 * (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1000;
            if (address_ptr != NULL)
            {
                syslog(LOG_INFO, "[Test %d Round] Allocate success -> mem_list[%d] (address:%p size:%d) takes:%ld microseconds\n", n, i, address_ptr, malloc_size, timedif);
                memset(address_ptr, 0x67, malloc_size);
                /* Add to list */
                mem_list[i] = address_ptr;
            }
            else
            {
                syslog(LOG_ERR, "Malloc failed ! The remaining memory may be insufficient\n");
                syslog(LOG_ERR, "Continue to test !!\n");
                /* Memory allocation failure */
                mem_list[i] = NULL;
                continue;
            }
        }

        for (int k = 0; k < list_length; k++)
        {
            address_ptr = mem_list[k];
            if (address_ptr != NULL)
            {
                clock_gettime(CLOCK_MONOTONIC, &t_start);
                free(address_ptr);
                clock_gettime(CLOCK_MONOTONIC, &t_end);
                timedif = 1000000 * (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1000;
                /* Use mem_list[k] directly instead of address_ptr to avoid warning about use-after-free */
                syslog(LOG_INFO, "[Test %d Round] Free -> mem_list[%d] (address:%p size:%d) takes:%ld microseconds\n", n, k, mem_list[k], malloc_size, timedif);
                mem_list[k] = NULL;
            }
        }

        free(mem_list);
    }

    return 0;
}
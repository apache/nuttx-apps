

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdio.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/



int main(int argc, FAR char *argv[])
{
  printf("Essa é a primeira aplicacao!!\n");
  struct timespec start;
  struct timespec end;
  int elapsed_secs;
  int N = 1000;
  int i = 1;
  clock_gettime(CLOCK_MONOTONIC, &start);

  while (i <= N) {
    printf("%d \n", i);
    
    if(i==1000){
       
      printf("I counted %d numbers \n", i);
    }
    i = i + 1;
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  return 0;
}

double TimeSpecToSeconds(struct timespec* ts){
    return (double)ts -> tv_sec + (double)ts->tv_sec / 1000000000.0;
}
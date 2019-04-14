#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main() {
  //--------------1.print  process ID to the screen-----------------------
  printf("sneaky_process pid = %d\n", getpid());
  //--------------2.

  return EXIT_SUCCESS;
}

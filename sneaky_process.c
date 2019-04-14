#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>


int main() {
  //--------------1.print  process ID to the screen-----------------------
  printf("sneaky_process pid = %d\n", getpid());

  
  //--------------2.copy /etc/passwd to /tmp/passwd----------------------
  char source_file[20] = "/etc/passwd";
  char target_file[20] = "/tmp/passwd";
  FILE *source, *target;
  source = fopen(source_file, "r");
  if (source == NULL) {
    fprintf(stderr, "fail to open file %s", source_file);
    exit(EXIT_FAILURE);
  }
  target = fopen(target_file, "w");
  if (target == NULL) {
    fprintf(stderr, "fail to open file %s", target_file);
    exit(EXIT_FAILURE);
  }


  char ch;
  while ((ch = fgetc(source)) != EOF) {
    fputc(ch, target);
  }


  fclose(source);
  fclose(target);





  
  return EXIT_SUCCESS;
}

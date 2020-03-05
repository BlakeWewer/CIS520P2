#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include <user/syscall.h>
#include "devices/input.h"
#include "devices/shutdown.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler (struct intr_frame *);
int add_file (struct file *file_name);
void get_args (struct intr_frame *f, int *arg, int num_of_args);
void syscall_halt (void);
pid_t syscall_exec(const char* cmdline);
int syscall_wait(pid_t pid);
bool syscall_create(const char* file_name, unsigned starting_size);
bool syscall_remove(const char* file_name);
int syscall_open(const char * file_name);
int syscall_filesize(int filedes);
int syscall_read(int filedes, void *buffer, unsigned length);
int syscall_write (int filedes, const void * buffer, unsigned byte_size);
void syscall_seek (int filedes, unsigned new_position);
unsigned syscall_tell(int fildes);
void syscall_close(int filedes);
void validate_ptr (const void* vaddr);
void validate_str (const void* str);
void validate_buffer (const void* buf, unsigned byte_size);

bool file_lock_init = false;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if(!file_lock_init)
  {
    lock_init(&file_system_lock);
    file_lock_init = true;
  }

  int arg[3];
  int esp = getpage_ptr((const void *) f->esp);

  switch(* (int *) esp)
  {
    case SYS_HALT:
      syscall_halt();
      break;
    
    case SYS_EXIT:
      get_args(f, &arg[0], 1);
      syscall_exit(arg[0]);
      break;

    case SYS_EXEC:
      get_args(f, &arg[0], 1);
      validate_str((const void*) arg[0]);   //Check if cmdline is valid

      arg[0] = getpage_ptr((const void*) arg[0]);
      f->eax = syscall_exec((const char*)arg[0]);
      break;

    case SYS_WAIT:
      get_args(f, &arg[0], 1);
      f->eax = syscall_wait((const char*)arg[0]);
      break;

    case SYS_CREATE:
      get_args(f, &arg[0], 2);
      validate_str((const void*)arg[0]);
      arg[0] = getpage_ptr((const void*)arg[0]);
      f->eax = syscall_create((const char *)arg[0], (unsigned)arg[1]);
      break;

    case SYS_REMOVE:
      get_args(f, &arg[0], 1);
      validate_str((const void*)arg[0]);
      arg[0] = getpage_ptr((const void *)arg[0]);
      f->eax = syscall_remove((const char *)arg[0]);
      break;

    case SYS_OPEN:
      get_args(f, &arg[0], 1);
      validate_str((const void*)arg[0]);
      arg[0] = getpage_ptr((const void *)arg[0]);
      f->eax = syscall_open((const char *)arg[0]);
      break;

    case SYS_FILESIZE:
      get_args(f, &arg[0], 1);
      f->eax = syscall_filesize(arg[0]);
      break;

    case SYS_READ:
      get_args(f, &arg[0], 3);
      validate_buffer((const void*)arg[1], (unsigned)arg[2]);
      arg[1] = getpage_ptr((const void *)arg[1]);
      f->eax = syscall_read(arg[0], (void *) arg[1], (unsigned) arg[2]);
      break;

    case SYS_WRITE:
      get_args(f, &arg[0], 3);
      validate_buffer((const void*)arg[1], (unsigned)arg[2]);
      arg[1] = getpage_ptr((const void *)arg[1]); 
      f->eax = syscall_write(arg[0], (const void *) arg[1], (unsigned) arg[2]);
      break;

    case SYS_SEEK:
      get_args(f, &arg[0], 2);
      syscall_seek(arg[0], (unsigned)arg[1]);
      break;

    case SYS_TELL:
      get_args(f, &arg[0], 1);
      f->eax = syscall_tell(arg[0]);
      break;

    case SYS_CLOSE:
      get_args(f, &arg[0], 1);
      syscall_close(arg[0]);
      break;

    default:
      break;
  }

  printf ("system call!\n");
  thread_exit ();
}

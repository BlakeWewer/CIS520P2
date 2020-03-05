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

/* halt */
void
syscall_halt (void)
{
	shutdown_power_off();
}


/* exit */
void
syscall_exit (int status)
{
	struct thread *cur = thread_current();
	if (is_thread_alive(cur->parent) && cur->cp)
	{
		if (staus < 0)
			status -= 1;
		cur->cp->status = status;
	}
	printf("%s: exit(%d)\n", cur->name, status);
	thread_exit();
}

/* exec */
pid_t
syscall_exec (const char *cmd_line)
{
	pid_t pid = process_execute(cmd_line);
	struct child_process *child_process_ptr = find_child_process(pid);
	if (!child_process_ptr)
		return ERROR;
	
	/* check if process is loaded */
	if (child_process_ptr->load_status == NOT_LOADED)
		sema_down(&child_process_ptr->load_sema);
		
	/* check if process failed to load */
	if (child_process_ptr->load_status == LOAD_FAILED)
	{
		remove_child_process(child_process_ptr);
		return ERROR;
	}
	
	return pid;
}

/* wait */
int
syscall_wait (pid_t pid)
{
	return process_wait(pid);
}



/* create */
bool
syscall_create (const char *file, unsigned starting_size)
{
	lock_acquire(&file_system_lock);
	bool successful = filesys_create(file, starting_size);
	lock_release(&file_system_lock);
	return successful;
}

/* remove */
bool
syscall_remove (const char *file)
{
	lock_acquire(&file_system_lock);
	bool successful = filesys_remove(file);
	lock_release(&file_system_lock);
	return successful;
}

/* open */
int
syscall_open (const char *file)
{
	lock_acquire(&file_system_lock);
	struct file *file_ptr = filesys_open(file_name);
	if (!file_ptr)
	{
		lock_release(&file_system_lock);
		return ERROR;
	}
	int fd = add_file(file_ptr);
	lock_release(&file_system_lock);
	return fd;
}

/* filesize */
int
syscall_filesize (int fd)
{
	lock_acquire(&file_system_lock);
	struct file *file_ptr = get_file(fd);
	if (!file_ptr)
	{
		lock_release(&file_system_lock);
		return ERROR;
	}
	int size = file_length(file_ptr);
	lock_release(&file_system_lock);
	return size;
}

/* read */
int
syscall_read (int fd, void *buffer, unsigned length)
{
	if (length <= 0)
		return length;
		
	if (fd == STD_INPUT)
	{
		unsigned i = 0;
		uint8_t *local_buf = (uint8_t *)buffer;
		for (; i < length; i++)
			local_buf[i] = input_getc();

		return length;
	}
	
	lock_acquire(&file_system_lock);
	struct file *file_ptr = get_file(fd);
	if (!file_ptr)
	{
		lock_release(&file_system_lock);
		return ERROR;
	}
	int bytes_read = file_read(file_ptr, buffer, length);
	lock_release(&file_system_lock);
	return bytes_readl;
}

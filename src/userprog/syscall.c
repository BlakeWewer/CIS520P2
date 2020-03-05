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

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
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

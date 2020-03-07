#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/synch.h"
#include <user/syscall.h>

#define CLOSE_ALL -1
#define ERROR -1

#define NOT_LOADED 0
#define LOAD_SUCCESS 1
#define LOAD_FAIL 2

struct lock sys_file_lock;

void init (void);

void halt (void);

void exit (int status);

pid_t exec (const char *cmd_line);

int wait (pid_t pid);

bool create (const char *file, unsigned initial_state);

bool remove (const char *file);

int open (const char *file_name);

int filesize (int fd);

int read(int fd, void *buffer, unsigned size);

int write (int fd, const void *buffer, unsigned size);

void seek (int fd, unsigned position);

unsigned tell (int fd);

void close (int fd);

#endif /* userprog/syscall.h */

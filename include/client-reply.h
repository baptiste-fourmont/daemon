#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "cassini.h"
#include "common.h"


int list_task_reply(int fd, uint64_t taskid);
int get_times_and_exit_code_reply(int fd, uint64_t taskid);
int remove_task_reply(int fd);
int get_stderr_reply(int fd);
int get_stdout_reply(int fd);
int create_task_reply(int fd, uint64_t taskid);
int terminate_daemon_reply(int fd);
#define SIZE_PATH 1024
#include <assert.h>
#include <unistd.h>

#include "client-request.h"
#include "server-reply.h"
#include "timing-text-io.h"
/*
    Common path
char* pipes_directory = "./pipes";
char* request_pipe = "saturnd-request-pipe";
char* reply_pipe = "saturnd-reply-pipe
*/

char* pipes_directory;
char* request_pipe;
char* reply_pipe;

uint16_t operation;
uint64_t taskid;
uint16_t opcode;

int read_uint8(int fd, void* buf);
int read_uint16(int fd, void* buf);
int read_uint32(int fd, void* buf);
int read_uint64(int fd, void* buf);
int read_int64(int fd, void* buf);

int write_uint16(int fd, void* buf);
int write_uint32(int fd, void* buf);
int write_uint64(int fd, void* buf);
int write_int64(int fd, void* buf);

struct timing read_timing(int fd);
uint64_t read_taskid(int fd);
uint16_t read_opcode(int fd);

void launch_daemon();
int choose_pipes();
int get_the_last_task();
int get_last_task_number();
void initialize_default();
char* read_file(const char* filename);

String create_string(char* str, int len);
pid_t spawnchild(const char* program, char** arg_list, int taskid);
commandLine create_command(int argc, char** argv, int len[argc]);
commandLine read_command(int fd);
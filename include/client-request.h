#include <stdint.h>

#ifndef CLIENT_REQUEST_H
#define CLIENT_REQUEST_H

#define CLIENT_REQUEST_LIST_TASKS 0x4c53              // 'LS'
#define CLIENT_REQUEST_CREATE_TASK 0x4352             // 'CR'
#define CLIENT_REQUEST_REMOVE_TASK 0x524d             // 'RM'
#define CLIENT_REQUEST_GET_TIMES_AND_EXITCODES 0x5458 // 'TX'
#define CLIENT_REQUEST_TERMINATE 0x544d               // 'TM'
#define CLIENT_REQUEST_GET_STDOUT 0x534f              // 'SO'
#define CLIENT_REQUEST_GET_STDERR 0x5345              // 'SE'

#endif // CLIENT_REQUEST_H


int list_task_request(int fd, uint16_t opcode);
int get_times_and_exit_code_request(int fd, uint16_t opcode, uint64_t taskid);
int get_stderr_request(int fd, uint64_t taskid);
int get_stdout_request(int fd, uint64_t taskid);
int remove_task_request(int fd, uint16_t opcode, uint64_t taskid);
int terminate_daemon_request(int fd, uint16_t opcode);
int create_task_request(int fd, char* minutes_str, char* hours_str,
                        char* daysofweek_str, int argc, int optind,
                        char* argv[]);
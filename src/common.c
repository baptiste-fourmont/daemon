#define SIZE_PATH 1024
#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "timing-text-io.h"

int read_uint16(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = read(fd, buf, sizeof(uint16_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

int read_uint32(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = read(fd, buf, sizeof(uint32_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

int read_uint64(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = read(fd, buf, sizeof(uint64_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

int read_int64(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = read(fd, buf, sizeof(int64_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

int read_uint8(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = read(fd, buf, sizeof(uint8_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

/* Same things for the write */

int write_uint16(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = write(fd, buf, sizeof(uint16_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

int write_uint32(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = write(fd, buf, sizeof(uint32_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

int write_uint64(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = write(fd, buf, sizeof(uint64_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

int write_int64(int fd, void* buf) {
    if (buf == NULL) {
        return 1;
    }
    int n = write(fd, buf, sizeof(int64_t));
    if (n < 0) {
        return 1;
    }
    return 0;
}

/* Option -p */
int choose_pipes() {
    if (pipes_directory == NULL) {
        char* username = getenv("USER");
        if (!username) {
            free(username);
            return -1;
        }
        pipes_directory = malloc(SIZE_PATH);
        if (pipes_directory == NULL) {
            free(pipes_directory);
        }
        sprintf(pipes_directory, "/tmp/%s", username);
    } else {
        request_pipe = malloc(SIZE_PATH);
        if (request_pipe == NULL) {
            free(request_pipe);
        }
        sprintf(request_pipe, "%s/saturnd-request-pipe", pipes_directory);
        reply_pipe = malloc(SIZE_PATH);
        if (reply_pipe == NULL) {
            free(reply_pipe);
        }
        sprintf(reply_pipe, "%s/saturnd-reply-pipe", pipes_directory);
    }
    return 0;
}

void initialize_default() {
    char* username = getenv("USER");

    if (pipes_directory == NULL) {
        if (!username) {
            free(username);
        }
        pipes_directory = malloc(SIZE_PATH);
        if (pipes_directory == NULL) {
            free(pipes_directory);
        }
    }

    sprintf(pipes_directory, "/tmp/%s", username);

    request_pipe = malloc(SIZE_PATH);
    if (request_pipe == NULL) {
        free(request_pipe);
    }
    sprintf(request_pipe, "%s/saturnd-request-pipe", pipes_directory);
    reply_pipe = malloc(SIZE_PATH);
    if (reply_pipe == NULL) {
        free(reply_pipe);
    }
    sprintf(reply_pipe, "%s/saturnd-reply-pipe", pipes_directory);
}

uint16_t read_opcode(int fd) {
    uint16_t opcode;
    if (read_uint16(fd, &opcode) == 1) {
        exit(EXIT_FAILURE);
    }
    return opcode;
}

uint64_t read_taskid(int fd) {
    uint64_t taskid;
    read_int64(fd, &taskid);
    return taskid;
}

struct timing read_timing(int fd) {
    struct timing time;

    assert(read(fd, &time, sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t)) != -1);

    time.minutes = be64toh(time.minutes);
    time.hours = be32toh(time.hours);

    return time;
}

pid_t spawnchild(const char* program, char** arg_list, int taskid) {
    pid_t child_pid;
    char path[256];
    sprintf(path, "/tmp/%s/tasks/task-%d", getenv("USER"), taskid);
    
    child_pid = fork();
    if (child_pid != 0) {
        int status;
        wait(&status);
        char path_exit[1024];
        strcpy(path_exit, path);
        strcat(path_exit, "/exitcode");
        int exit_code = open(path_exit, O_RDWR | O_CREAT);
        // ecriture du exit code du processus fils dans exitcode
        int a = WEXITSTATUS(status);
        write(exit_code, &a, sizeof(int));

        close(exit_code);
        return child_pid;
    } else{

        char path_stdo[256];
        strcpy(path_stdo, path);
        strcat(path_stdo, "/stdout");

        int stdo = open(path_stdo, O_RDWR | O_CREAT | O_TRUNC);
        if (stdo < 0) {
            perror("FAILURE open stdout");
            exit(EXIT_FAILURE);
        }

        if (dup2(stdo, 1) == -1) {
            perror("Errror duping stdout to file");
            exit(EXIT_FAILURE);
        }

        char path_stderr[256];
        strcpy(path_stderr, path);
        strcat(path_stderr, "/stderr");

        int stderr_ = open(path_stderr, O_RDWR | O_CREAT | O_TRUNC);
        if (dup2(stderr_, 2) == -1) {
            perror("Errror duping stdout to file");
            exit(EXIT_FAILURE);
        }

        close(stdo);
        close(stderr_);

        execvp(program, arg_list);
        perror("execvp");
        exit(EXIT_FAILURE);
    }
}

commandLine create_command(int argc, char** argv, int len[argc]) {
    commandLine res;
    res.argc = be32toh(argc);
    String* str = malloc(sizeof(String) * argc);
    assert(str != NULL);
    for (int i = 0; i < argc; i++) {
        str[i] = create_string(argv[i], len[i]);
        // printf("arg[%d] = '%s' (%d)\n", i, argv[i], len[i]);
    }
    res.argv = str;
    return res;
}


commandLine create_commande(int argc, char** argv, int len[argc]) {
    commandLine res;
    res.argc = be32toh(argc);
    String* str = malloc(sizeof(String) * argc);
    assert(str != NULL);
    for (int i = 0; i < argc; i++) {
        str[i] = create_string(argv[i], len[i]);
        // printf("arg[%d] = '%s' (%d)\n", i, argv[i], len[i]);
    }
    res.argv = str;
    return res;
}

commandLine read_command(int fd) {
    commandLine cmd;
    uint32_t size;
    if (read_uint32(fd, &size) == 1) {
        perror("Read length command");
        exit(EXIT_FAILURE);
    }

    size = be32toh(size);
    char** res = malloc(sizeof(char*) * size);
    assert(res != NULL);
    int len[size];
    for (int i = 0; i < size; i++) {
        uint32_t length;
        if (read(fd, &length, sizeof(uint32_t)) == -1) {
            free(res);
            exit(EXIT_FAILURE);
        }
        length = be32toh(length);
        res[i] = malloc(length + 1);
        int b_len = read(fd, res[i], length);
        len[i] = b_len;
        // printf("arg[%d] = '%s' (%d)\n", i, res[i], len[i]);
        if (b_len == -1) {
            free(res);
            exit(EXIT_FAILURE);
        }
    }
    if (size <= 0 || res[0] == NULL) {
        perror("ERROR: Doesn't respect the create command");
        exit(EXIT_FAILURE);
    }
    close(fd);
    cmd = create_command(size, res, len);
    return cmd;
}

char* read_file(const char* filename) {
    ssize_t size;
    int fd, len;
    struct stat st;
    char* buf;

    stat(filename, &st);
    len = st.st_size;

    buf = (char*)malloc(len + 1);
    assert(buf != NULL);

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        free(buf);
        exit(EXIT_FAILURE);
    }

    size = read(fd, buf, len);
    if (size > 0) {
        buf[len] = '\0';
        close(fd);
        return buf;
    }

    close(fd);
    return NULL;
}

String create_string(char* str, int len) {
    char* str_arg = malloc(len);
    assert(str_arg);
    strncpy(str_arg, str, len);
    String res;
    uint32_t size = len;
    res.L = size;
    // printf("arg is : '%s' instead of '%s'\n", str_arg, str);
    res.Chaine = str_arg;
    return res;
}
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <syslog.h>
#include <stdio.h>
#include <time.h>

#include "client-reply.h"
#include "common.h"

#define PATH_LEN 256
#define SECONDES 10

/* Chaque tâche est donnée par le nom d'un programme, plus des arguments. Il
 * faut donc juste appeler ce programme avec fork + une variante d'exec (par
 * exemple execvp)
 *
 * On doit créer un self pipe (pour le plus dure sinon faire comme dans le main)
 */

/**
 * @brief Dans un handler, on peut écrire lire écrire des variables globales de
 * type volatie sig_atomic_t
 *
 */

/**
 * @brief Architecture du daemon
 *
 * /tmp/<username>/saturnd/ -> pipes / -> request
 *                                     -> reply
 *                          -> taches / -> un doissier par tâche
 *                                      -> id / -> la commannde
 *                                              -> quand
 *                                              -> id
 * @brief Comment utiliser saturnd
 * ./cassini -p "./run/pipes" -l
 * cd <Dossier du projet>
 * hexdump -C ./run/pipes/saturnd-request-pipe
 */

/**
 * @brief Lanch task the daemon
 *  Fork + kill le processus parent
 */
uint64_t lastk_task;

void launch_daemon() {
    // Double fork
    pid_t pid = fork();
    assert(pid != -1);

    if (pid != 0) {
        exit(EXIT_SUCCESS);
    }

    // Close task
    pid = fork();
    assert(pid != -1);
    if (pid != 0) {
        exit(EXIT_SUCCESS);
    }
}

void start_daemon() {
    pid_t pid;
    pid_t sid;

    // Fork off the parent process
    pid = fork();

    if (pid < 0) {
        perror("ERROR FORK");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // Child process become session leader
    sid = setsid();
    if (sid < 0) {
        perror("ERROR: Creation de session");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
}

int rmtree(char* path) {
    assert(path);
    size_t path_len;
    char* full_path;
    DIR* dir;
    struct stat stat_path, stat_entry;
    struct dirent* entry;

    // stat for the path
    stat(path, &stat_path);

    // if path does not exists or is not dir - exit with status -1
    if (S_ISDIR(stat_path.st_mode) == 0) {
        fprintf(stderr, "%s: %s\n", "Is not directory", path);
        return -1;
    }

    // if not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL) {
        fprintf(stderr, "%s: %s\n", "Can`t open directory", path);
        return -1;
    }

    // the length of the path
    path_len = strlen(path);

    // iteration through entries in the directory
    while ((entry = readdir(dir)) != NULL) {
        // skip entries "." and ".."
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        // determinate a full path of an entry
        full_path = calloc(path_len + strlen(entry->d_name) + 1, sizeof(char));
        strcpy(full_path, path);
        strcat(full_path, "/");
        strcat(full_path, entry->d_name);

        // stat for the entry
        stat(full_path, &stat_entry);

        // recursively remove a nested directory
        if (S_ISDIR(stat_entry.st_mode) != 0) {
            rmtree(full_path);
            continue;
        }

        // remove a file object
        if (unlink(full_path) == 0)
            printf("Removed a file: %s\n", full_path);
        else
            printf("Can`t remove a file: %s\n", full_path);
        free(full_path);
    }

    // remove the devastated directory and close the object of it
    if (rmdir(path) == 0)
        printf("Removed a directory: %s\n", path);
    else
        printf("Can`t remove a directory: %s\n", path);

    closedir(dir);
    return 0;
}

int directory_found(char* path) {
    assert(path);
    struct stat s;
    if (path == NULL) {
        return -1;
    }
    int err = stat(path, &s);
    if (-1 == err) {
        if (ENOENT == errno) {
            return -1;
        } else {
            perror("stat");
            exit(1);
        }
    } else {
        if (S_ISDIR(s.st_mode)) {
            return 0;
        } else {
            return -1;
        }
    }
}

int pipes_found() {
    char path[PATH_LEN];
    char* username = getenv("USER");
    if (!username) {
        exit(1);
    }
    sprintf(path, "/tmp/%s", username);
    if (directory_found(path) == 0) {
        return 0;
    }
    return -1;
}

/**
 * @brief Create a tube object
 *
 * @return int
 */
void create_tube() {
    initialize_default();
    assert(mkdir(pipes_directory, 0700) != -1);
    assert((mkfifo(request_pipe, 0777)) != -1);
    assert((mkfifo(reply_pipe, 0777)) != -1);
}

void create_task_list() {
    char path[PATH_LEN];
    sprintf(path, "%s/tasks", pipes_directory);
    assert(mkdir(path, 0700) != -1);
}

int delete_task_file(int id) {
    // Check si pipes_directory est null car sinon faut l'asigner à
    // /tmp/username/
    char path[PATH_LEN];
    sprintf(path, "%s%s%d/", pipes_directory, "/tasks/task-", id);

    if (rmtree(path) == -1) {
        return -1;
    }
    return 0;
}

int create_task_file(struct timing* time, commandLine res) {
    char path[PATH_LEN];
    char buffer[13];
    int fd;

    int id = get_last_task_number() + 1;
    // Create task id folder
    sprintf(path, "%s%s%d", pipes_directory, "/tasks/task-", id);
    if (mkdir(path, S_IRWXU) == -1) {
        perror("Create DIR");
        exit(EXIT_FAILURE);
    }

    sprintf(path, "%s%s%d%s", pipes_directory, "/tasks/task-", id, "/id");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("File ID");
        return -1;
    }
    // Format id to decimal.
    int length = sprintf(buffer, "%d", id);
    write(fd, buffer, length);
    close(fd);

    // Now create time file human readable
    sprintf(path, "%s%s%d%s", pipes_directory, "/tasks/task-", id, "/time");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("Can not create file time");
        return -1;
    }
    write(fd, time, sizeof(struct timing));
    close(fd);

    char* time_string = malloc(TIMING_TEXT_MIN_BUFFERSIZE);
    timing_string_from_timing(time_string, time);
    printf("TIMING à l'écriture: %s\n", time_string);
    free(time_string);

    sprintf(path, "%s%s%d%s", pipes_directory, "/tasks/task-", id, "/argc");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("Can not create argc file");
        return -1;
    }
    uint32_t size = be32toh(res.argc);
    write(fd, &size, sizeof(uint32_t));
    close(fd);

    sprintf(path, "%s%s%d%s", pipes_directory, "/tasks/task-", id, "/argv");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("Can not create argument file");
        return -1;
    }

    assert(res.argv);
    for (int i = 0; i < htobe32(res.argc); i++) {
        if (res.argv[i].Chaine != NULL) {
            printf("ARGV[%d]: %s\n", i, res.argv[i].Chaine);
            fflush(stdout);
            dprintf(fd, "%s\n", res.argv[i].Chaine);
        }
    }

    close(fd);
    return 0;
}

int get_last_task_number() {
    int last_file = 0;
    DIR* dirp;
    struct dirent* entry;

    char path[PATH_LEN];
    sprintf(path, "%s/tasks", pipes_directory);

    dirp = opendir(path);
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            char* c = strchr(entry->d_name, '-') + 1;
            int nb = strtol(c, NULL, 10);
            if (nb > last_file) {
                last_file = nb;
            }
        }
    }
    closedir(dirp);
    return last_file;
}

int count_tasks() {
    int file_count = 0;
    DIR* dirp;
    struct dirent* entry;

    char path[PATH_LEN];
    sprintf(path, "%s/tasks", pipes_directory);

    dirp = opendir(path);
    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            // printf("%s\n",entry->d_name);
            file_count++;
        }
    }
    closedir(dirp);
    // printf("NOMBRE TASK:%d\n", file_count);
    return file_count;
}

void choppy(char* s) {
    while (*s && *s != '\n') s++;
    *s = 0;
}

int write_task(int id, int reply) {
    char path[PATH_LEN];
    int fd;
    // Check if task directory exist
    // printf("%d\n",id);
    sprintf(path, "%s%s%d", pipes_directory, "/tasks/task-", id);
    fd = open(path, O_RDONLY | O_DIRECTORY);
    if (fd < 0) {
        perror("Not a Directory");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
    // TASK[0].TASKID <uint64>, TASK[0].TIMING <timing>, TASK[0].COMMANDLINE
    // <commandline>

    // Write taskid
    uint64_t newid = be64toh(id);
    write_uint64(reply, &newid);

    // READ timing
    sprintf(path, "%s%s%d%s", pipes_directory, "/tasks/task-", id, "/time");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("Can not open time file");
        return -1;
    }
    struct timing time;
    read(fd, &time, sizeof(struct timing));
    close(fd);

    write(reply, &time, sizeof(time));

    sprintf(path, "%s%s%d%s", pipes_directory, "/tasks/task-", id, "/argc");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("Can not open argc file");
        return -1;
    }
    uint32_t argc;
    read_uint32(fd, &argc);
    argc = be32toh(argc);
    write_uint32(fd, &argc);
    close(fd);

    // On récupère la commandline
    sprintf(path, "%s%s%d%s", pipes_directory, "/tasks/task-", id, "/argv");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("Can not open arguments file");
        return -1;
    }

    // Substitution en attendant memcpy
    // on supprime \0 du string et \n
    char c;
    int i = 0;
    int j = 0;
    char line[80];
    uint32_t size_len;
    int size;

    while ((read(fd, &c, 1) == 1)) {
        line[i++] = c;
        if (c == '\n') {
            line[i] = 0;
            i = 0;
            j = 0;
            choppy(line);
            size = sizeof(line) / sizeof(line[0]);
            for (j = 0; j < size - 1; j++) {
                if (line[j] != 0)
                    ;
                break;
            }
            size_len = strlen(line);
            printf("%s | %d\n", line, size_len);
            /*
            write(reply, &size_len, sizeof(uint32_t));
            write(reply, line, size_len);
            */
        }
    }
    close(fd);
    return 0;
}

int write_list_task(int fd) {
    int ret;
    char path[PATH_LEN];
    int nbtask = count_tasks();
    nbtask = be32toh(nbtask);
    assert(write_uint32(fd, &nbtask) != 1);

    /**
     * @brief Fixer ici task_id
     *  si on supprime un id là on part du principe que ca va de 1 à nombre de
     * task
     */
    for (int i = 1; i <= htobe32(nbtask); i++) {
        sprintf(path, "%s%s%d", pipes_directory, "/tasks/task-", i);
        ret = open(path, O_RDONLY | O_DIRECTORY);
        if (ret != -1) {
            write_task(i, fd);
        }
        close(ret);
    }

    return 0;
    close(fd);
}

char* get_time(char *task){ 
    char path[PATH_LEN];
    int fd;

    // READ timing
    sprintf(path, "%s%s%s%s", pipes_directory, "/tasks/", task, "/time");
    fd = open(path, O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
    if (fd == -1) {
        perror("Can not open time file");
        //return -1;
    }
    uint64_t minutes;
    uint32_t hours;
    uint8_t daysofweek;

    assert(read_uint64(fd, &minutes) != 1);
    assert(read_uint32(fd, &hours) != 1);
    assert(read_uint8(fd, &daysofweek) != 1);

    struct timing* tme = malloc(sizeof(struct timing));
    if (tme == NULL) {
        free(tme);
        exit(EXIT_FAILURE);
    }
    tme->minutes = minutes;
    tme->hours = hours;
    tme->daysofweek = daysofweek;
    char* time_string = malloc(TIMING_TEXT_MIN_BUFFERSIZE);
    timing_string_from_timing(time_string, tme);
    return time_string;
}


void launch_tasks() {
    DIR* dirp;
    char path[525];
    int fd, sz;
    char* c = (char*)calloc(100, sizeof(char));
    struct dirent* entry;
    char path_task[PATH_LEN];

    sprintf(path_task, "/tmp/%s/tasks", getenv("USER"));
    dirp = opendir(path_task);

    while ((entry = readdir(dirp)) != NULL) {
        if (entry->d_type == DT_DIR && entry->d_name[0] != '.') {
            time_t rtime;
            struct tm * tm_time;
            time (&rtime);
            tm_time = localtime (&rtime);

            char* current_time = malloc(sizeof(char*));
            char* task_time = get_time(entry->d_name);
            sprintf(current_time, "%d %d %d", tm_time->tm_min, tm_time->tm_hour, 0);

            if(strcmp(task_time, current_time) == 0){
                sprintf(path, "%s/%s/%s", path_task, entry->d_name, "argv");
                fd = open(path, O_RDONLY);
                if (fd < 0) {
                    perror("Error : File does not exists");
                    exit(1);
                }
                sz = read(fd, c, 10);
                c[sz] = '\0';

                char** argument_list = malloc(
                    0);
                assert(argument_list);
                char* cut_args;
                int size;
                cut_args = strtok(c, "\n");
                for (size = 0; cut_args != NULL; size++) {
                    char **tmp = realloc(argument_list, sizeof(char*) * (size+8));
                    if(tmp != NULL){
                        argument_list = tmp;
                    }
                    argument_list[size] = cut_args;
                    cut_args = strtok(NULL, "\n");
                }
                char* c = strchr(entry->d_name, '-') + 1;
                int id = strtol(c, NULL, 10);
                spawnchild(argument_list[0], argument_list, id);
            }
        }
    }
}

int main(int argc, char** argv) {
    launch_daemon();
    printf("The daemon is started ... \n");

    if (pipes_found() != 0) {
        create_tube();
        create_task_list();
    } else {
        initialize_default();
    }
    printf("Pipes Directory: %s\n", pipes_directory);
    printf("Request Pipes: %s\n", request_pipe);
    printf("Reply Pipes: %s\n", reply_pipe);

    // Attempted to be a demon

    int ret;
    int fd;
    struct pollfd fds;
    short revents;

    // int wstatus;
    uint64_t taskid;
    uint16_t errcode;
    // pid_t child;
    char path[PATH_LEN];

    /* watch stdout for ability to write (almost always true) */
    while (1) {
        int read_request = open(request_pipe, O_RDONLY, O_NONBLOCK);
        fds.fd = read_request;
        fds.events = POLLIN;
        ret = poll(&fds, 1, SECONDES * 1000);
        revents = fds.revents;
        if (ret == -1) {
            perror("poll");
            exit(1);
        } else if (ret == 0) {
            launch_tasks();
        } else {
            if (revents & (POLLIN)) {
                uint16_t sent;
                uint16_t get_opcode = read_opcode(fds.fd);
                get_opcode = htobe16(get_opcode);
                switch (get_opcode) {
                    case CLIENT_REQUEST_LIST_TASKS:
                        printf("RECEIVED REQUEST: List Task\n");
                        sent = SERVER_REPLY_OK;
                        break;
                    case CLIENT_REQUEST_CREATE_TASK:
                        printf("RECEIVED REQUEST: Create Task\n");
                        uint64_t minutes;
                        uint32_t hours;
                        uint8_t daysofweek;

                        read_uint64(fds.fd, &minutes);
                        read_uint32(fds.fd, &hours);
                        read_uint8(fds.fd, &daysofweek);

                        struct timing* tme = malloc(sizeof(struct timing));
                        if (tme == NULL) {
                            free(tme);
                            exit(EXIT_FAILURE);
                        }
                        tme->minutes = minutes;
                        tme->hours = hours;
                        tme->daysofweek = daysofweek;

                        commandLine res = read_command(fds.fd);
                        printf("Nombre d'arguments de la commande: %d\n",
                               htobe32(res.argc));
                        create_task_file(tme, res);

                        sent = SERVER_REPLY_OK;
                        break;
                    case CLIENT_REQUEST_REMOVE_TASK: {
                        taskid = read_taskid(fds.fd);
                        taskid = be64toh(taskid);
                        printf("RECEIVE REQUEST: Remove Task %ld\n", taskid);
                        if (delete_task_file(taskid) == -1) {
                            sent = SERVER_REPLY_ERROR;
                            errcode = SERVER_REPLY_ERROR_NOT_FOUND;
                        } else {
                            sent = SERVER_REPLY_OK;
                        }
                    } break;
                    case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
                    	//get_times_and_exit_code_request(fds.fd,get_opcode, taskid);
                        if (count_tasks() > 0) {
                            sent = SERVER_REPLY_OK;
                        } else {
                            sent = SERVER_REPLY_ERROR;
                            errcode = SERVER_REPLY_ERROR_NOT_FOUND;
                        }
                        break;
                    case CLIENT_REQUEST_TERMINATE:
                        printf("RECEIVE REQUEST: Terminate Daemon\n");
                        sent = SERVER_REPLY_OK;
                        goto exit;
                        break;
                    case CLIENT_REQUEST_GET_STDOUT:
                        printf("RECEIVE REQUEST: Get Stdout\n");
                        taskid = read_taskid(fds.fd);
                        taskid = be64toh(taskid);
                        sprintf(path, "/tmp/%s/tasks/task-%ld/stdout",
                                getenv("USER"), taskid);
                        fd = open(path, O_RDONLY);
                        if (fd == -1) {
                            perror("TASK does not exist");
                            sent = SERVER_REPLY_ERROR;
                            errcode = SERVER_REPLY_ERROR_NOT_FOUND;
                        } else {
                            sent = SERVER_REPLY_OK;
                        }
                        close(fd);
                        break;
                    case CLIENT_REQUEST_GET_STDERR:
                        printf("RECEIVE REQUEST: Get Stderr\n");
                        taskid = read_taskid(fds.fd);
                        taskid = be64toh(taskid);
                        sprintf(path, "/tmp/%s/tasks/task-%ld/stderr",
                                getenv("USER"), taskid);
                        fd = open(path, O_RDONLY);
                        if (fd == -1) {
                            perror("TASK does not exist");
                            sent = SERVER_REPLY_ERROR;
                            errcode = SERVER_REPLY_ERROR_NOT_FOUND;
                        } else {
                            int check;
                            sprintf(path, "/tmp/%s/tasks/task-%ld/exitcode",
                                    getenv("USER"), taskid);
                            check = open(path, O_RDONLY);
                            if (check == -1) {
                                errcode = SERVER_REPLY_ERROR_NEVER_RUN;
                            } else {
                                sent = SERVER_REPLY_OK;
                            }
                            close(check);
                        }
                        close(fd);
                        break;
                    default:
                        sent = SERVER_REPLY_OK;
                }

                // We sent OK or ERROR
                int reply_write = open(reply_pipe, O_WRONLY);
                if (sent == SERVER_REPLY_OK) {
                    printf("OK\n");
                    sent = be16toh(sent);
                    write_uint16(reply_write, &sent);
                    switch (get_opcode) {
                        case CLIENT_REQUEST_LIST_TASKS:
                            write_list_task(reply_write);
                            break;
                        case CLIENT_REQUEST_CREATE_TASK: {
                            uint64_t newtask;
                            lastk_task = lastk_task + 1;
                            newtask = be64toh(lastk_task);
                            write_uint64(reply_write, &newtask);
                        } break;
                        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
                            break;
                        case CLIENT_REQUEST_GET_STDERR: {
                            char path[256];
                            sprintf(path, "/tmp/%s/tasks/task-%ld/stderr",
                                    getenv("USER"), taskid);
                            char* buf = read_file(path);
                            int len = htobe32(strlen(buf));
                            write(reply_write, &len, sizeof(uint32_t));
                            write(reply_write, buf, len);
                            free(buf);

                        } break;
                        case CLIENT_REQUEST_GET_STDOUT: {
                            char path[256];
                            sprintf(path, "/tmp/%s/tasks/task-%ld/stdout",
                                    getenv("USER"), taskid);
                            char* buf = read_file(path);
                            if (buf == NULL) {
                                free(buf);
                            }
                            uint32_t len = htobe32(strlen(buf));     
                            write(reply_write, &len, sizeof(uint32_t));
                            write(reply_write, buf, len);
                            free(buf);
                        } break;
                    }
                } else {
                    // ERROR suvi du type de l'erreur
                    sent = htobe16(SERVER_REPLY_ERROR);
                    errcode = htobe16(errcode);
                    assert(write(reply_write, &sent, sizeof(sent)) != -1);
                    assert(write(reply_write, &errcode, sizeof(errcode)) != -1);
                }
                close(reply_write);
                if (get_opcode == CLIENT_REQUEST_TERMINATE) {
                    break;
                    goto exit;
                }
            } else if (revents & (POLLHUP)) {
                printf("Cassini finish\n");
                close(fds.fd);
                break;
            } else if (revents & (POLLNVAL)) {
                printf("Invalid polling request.\n");
                break;
            } else if (revents & (POLLERR)) {
                printf("ERROR in poll descriptor\n");
                goto exit;
                break;
            }
        }
    }

exit:
    printf("Daemon is terminated\n");
    exit(EXIT_SUCCESS);
}

/**
 * @brief On ajoute les réponses communes entre cassini et saturnd
 *
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "cassini.h"

int list_task_request(int fd, u_int16_t opcode) {
    uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
    if (fd >= 0) {
        opcode = htobe16(operation);
        void* buffer = malloc(sizeof(uint16_t));
        if (buffer == NULL) {
            return -1;
        }
        *((uint16_t*)buffer) = opcode;
        assert(write(fd, buffer, sizeof(uint16_t)) != -1);
        free(buffer);
        return 0;
    } else {
        return -1;
    }
}
int get_times_and_exit_code_request(int fd, u_int16_t opcode, uint64_t taskid) {
    uint16_t operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
    opcode = htobe16(operation);
    taskid = htobe64(taskid);

    void* buffer;
    buffer = malloc(sizeof(uint16_t) + sizeof(uint64_t));
    if (buffer == NULL) {
        free(buffer);
        return -1;
    }
    *((uint16_t*)buffer) = opcode;
    *((uint64_t*)(buffer + 2)) = taskid;
    assert(write(fd, buffer, sizeof(uint16_t) + sizeof(uint64_t)) != -1);
    free(buffer);
    return 0;
}

int get_stderr_request(int fd, uint64_t taskid) {
    uint16_t opcode;
    uint16_t operation = CLIENT_REQUEST_GET_STDERR;
    opcode = htobe16(operation);
    taskid = htobe64(taskid);

    void* buffer = malloc(sizeof(uint16_t) + sizeof(uint64_t));
    if (buffer == NULL) {
        free(buffer);
        return -1;
    }
    *((uint16_t*)buffer) = opcode;
    *((uint64_t*)(buffer + sizeof(uint16_t))) = taskid;
    assert(write(fd, buffer, sizeof(uint16_t) + sizeof(uint64_t)) != -1);
    free(buffer);
    return 0;
}

int get_stdout_request(int fd, uint64_t taskid) {
    uint16_t opcode;
    uint16_t operation = CLIENT_REQUEST_GET_STDOUT;
    opcode = htobe16(operation);
    taskid = htobe64(taskid);

    void* buffer = malloc(sizeof(uint16_t) + sizeof(uint64_t));
    if (buffer == NULL) {
        free(buffer);
        return -1;
    }
    *((uint16_t*)buffer) = opcode;
    *((uint64_t*)(buffer + 2)) = taskid;
    assert(write(fd, buffer, sizeof(uint16_t) + sizeof(uint64_t)) != -1);
    free(buffer);
    return 0;
}

int remove_task_request(int fd, u_int16_t opcode, uint64_t taskid) {
    uint16_t operation = CLIENT_REQUEST_REMOVE_TASK;
    opcode = htobe16(operation);
    taskid = htobe64(taskid);

    void* buffer = malloc(sizeof(uint16_t) + sizeof(uint64_t));
    if (buffer == NULL) {
        free(buffer);
        return -1;
    }
    *((uint16_t*)buffer) = opcode;
    *((uint64_t*)(buffer + 2)) = taskid;
    assert(write(fd, buffer, sizeof(uint16_t) + sizeof(uint64_t)) != -1);
    free(buffer);
    return 0;
}

/* Option terminate daemon */
int terminate_daemon_request(int fd, u_int16_t opcode) {
    if (fd >= 0) {
        opcode = htobe16(opcode);
        void* buffer = malloc(sizeof(uint16_t));
        if (buffer == NULL) {
            free(buffer);
            return 1;
        }
        *((uint16_t*)buffer) = opcode;
        assert(write(fd, buffer, 2) != -1);
        free(buffer);
    }
    close(fd);
    return 0;
}

int create_task_request(int fd, char* minutes_str,
                        char* hours_str, char* daysofweek_str, int argc, int  optind,
                        char* argv[]) {
    uint16_t operation = CLIENT_REQUEST_CREATE_TASK;
    uint16_t opcode = htobe16(operation);
    // On créer une structure timing pour y stocker le temps
    struct timing* t = malloc(sizeof(struct timing));

    assert(write(fd, &opcode, sizeof(uint16_t)) != -1);
    // On assigne le temps dans la structure
    timing_from_strings(t, minutes_str, hours_str, daysofweek_str);
    // On envoie le timing dans la requête
    assert(write(fd, t,
                 sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t)) != -1);

    // on va maintenant récupèrer les arguments restants
    int restant = htobe32(argc - optind);
    assert(write(fd, &restant, sizeof(uint32_t)) != -1);

    // On parcourt les arguments restant
    // On veut écrire le commandline (ARGC<uint32> argv[0] à
    // argv[argc-1])
    for (; optind < argc; optind++) {
        int len = htobe32(strlen(argv[optind]));
        assert(write(fd, &len, sizeof(uint32_t)) != -1);
        // On récupère les arguments de la commande
        // test-1 par exemple dans le test 2
        assert(write(fd, argv[optind], strlen(argv[optind])) != -1);
    }
    free(t);
    return 0;
}
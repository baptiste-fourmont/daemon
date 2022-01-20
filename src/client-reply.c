#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "cassini.h"

int list_task_reply(int fd, uint64_t taskid) {
    uint16_t resp;
    uint32_t nbtasks;

    read_uint16(fd, &resp);
    read_uint32(fd, &nbtasks);

    int count = be32toh(nbtasks);

    while (count > 0) {
        struct timing* strtime = malloc(sizeof(struct timing));
        char* time = malloc(TIMING_TEXT_MIN_BUFFERSIZE);
        if (time == NULL) {
            free(time);
            return -1;
        }

        read_uint64(fd, &taskid);
        int n = read(fd, strtime,
                     sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint8_t));
        if (n < 0) {
            return -1;
        }

        timing_string_from_timing(time, strtime);
        taskid = be64toh(taskid);
        printf("%ld: ", taskid);
        printf("%s ", time);
        free(time);
        /**
         * @brief Affichage actuel
         * tâche: minute hours daysofweek
         * Par exemple:
         * 0: * * *
         * Et maintenant on va récupérer la suite de commande
         * On récupère la commandline noté cmdline
         *
         * commandline:
         * ARGC <uint32>, ARGV[0] <string>, ..., ARGV[ARGC-1]
         * <string>
         */
        uint32_t cmdline;
        read_uint32(fd, &cmdline);
        int taille = be32toh(cmdline);
        uint32_t tmp;

        for (int i = 0; i < taille; i++) {
            // On récupère la taille de argv[i]
            read_uint32(fd, &tmp);
            char* str = malloc(sizeof(char));
            if (str == NULL) {
                free(str);
                return -1;
            } else {
                tmp = be32toh(tmp);
                assert(read(fd, str, tmp) != -1);
                // On affiche l'argv[i]
                printf("%s ", str);
                free(str);
            }
        }
        printf("\n");  // On passe à la prochaine tâche
        count--;
    }
    return 0;
}

int get_times_and_exit_code_reply(int fd, uint64_t taskid) {
    uint16_t rep;
    read_uint16(fd, &rep);

    // on vérifie si rep est un ok
    if (rep == be16toh(SERVER_REPLY_OK)) {
        // On récupère le nombre de runs
        uint32_t nbruns;
        read_uint32(fd, &nbruns);
        int compteur = be32toh(nbruns);

        while (compteur > 0) {
            int64_t time;
            uint16_t exitcode;

            read_int64(fd, &time);
            read_uint16(fd, &exitcode);

            // On convertir le temps récupèrer en time
            time = be64toh(time);
            time_t rawtime = (time_t)time;
            // Retourne le temps converti dans la bonne zone
            struct tm* ptm = localtime(&rawtime);
            if (ptm == NULL) {
                return -1;
            }
            /* ptm->tm_year commence à 1900 et month
             * commence à 0 */
            printf("%02d-%02d-%02d %02d:%02d:%02d %d\n", ptm->tm_year + 1900,
                   ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min,
                   ptm->tm_sec, be16toh(exitcode));
            compteur--;
            // On ne doit pas free(ptm) parce que les pointeurs
            // retournées par localtime sont des poiunteurs statique
            // dans la mémoire
        }
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

int remove_task_reply(int fd) {
    uint16_t rep;
    if(read_uint16(fd, &rep) == -1){
        return -1;
    }
    rep = be16toh(rep);
    if (rep == SERVER_REPLY_ERROR) {
        printf("%u", rep);
    }
    return 0;
}

int get_stderr_reply(int fd) {
    uint16_t ok;
    uint32_t output;

    read_uint16(fd, &ok);
    read_uint32(fd, &output);

    output = be32toh(output);
    char* str = malloc(output);
    if (str == NULL) {
        free(str);
        return -1;
    }
    int n = read(fd, str, output);
    if (n < 0) {
        return -1;
    }
    // On affiche le output sous forme de string
    printf("%s", str);
    free(str);
    return 0;
}

int get_stdout_reply(int fd) {
    uint16_t reponse;
    read_uint16(fd, &reponse);
    // On vérifie si la réponse attendu est bien un ok
    if (reponse == be16toh(SERVER_REPLY_OK)) {
        uint32_t taille;
        read_uint32(fd, &taille);
        // On récupère la réponse et on la renvoie
        // sous forme de string

        taille = be32toh(taille);
        char* str = malloc(taille);
        if (str == NULL) {
            return -1;
        }
        int n = read(fd, str, taille);
        if (n < 0) {
            return -1;
        }
        printf("%s", str);
        free(str);
        // On renvoie succès pour dire qu'on a pas d'erreur
        return EXIT_SUCCESS;
    }
    return 1;
}

int create_task_reply(int fd, uint64_t taskid) {
    if (fd >= 0) {
        uint16_t rep;
        // read(fd, &rep, sizeof(uint16_t));
        read_uint16(fd, &rep);
        rep = be16toh(rep);
        read_uint64(fd, &taskid);
        taskid = be64toh(taskid);
        printf("%lu", taskid);
        return 0;
    } else {
        return -1;
    }
}

int terminate_daemon_reply(int fd) {
    if (fd >= 0) {
        uint16_t ok;
        if (read_uint16(fd, &ok) > 0) {
            return 1;
        }
    }
    close(fd);
    return 0;
}
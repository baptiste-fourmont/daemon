#include "cassini.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#include "common.h"

const char usage_info[] =
    "\
   usage: cassini [OPTIONS] -l -> list all tasks\n\
      or: cassini [OPTIONS]    -> same\n\
      or: cassini [OPTIONS] -q -> terminate the daemon\n\
      or: cassini [OPTIONS] -c [-m MINUTES] [-H HOURS] [-d DAYSOFWEEK] COMMAND_NAME [ARG_1] ... [ARG_N]\n\
          -> add a new task and print its TASKID\n\
             format & semantics of the \"timing\" fields defined here:\n\
             https://pubs.opengroup.org/onlinepubs/9699919799/utilities/crontab.html\n\
             default value for each field is \"*\"\n\
      or: cassini [OPTIONS] -r TASKID -> remove a task\n\
      or: cassini [OPTIONS] -x TASKID -> get info (time + exit code) on all the past runs of a task\n\
      or: cassini [OPTIONS] -o TASKID -> get the standard output of the last run of a task\n\
      or: cassini [OPTIONS] -e TASKID -> get the standard error\n\
      or: cassini -h -> display this message\n\
\n\
   options:\n\
     -p PIPES_DIR -> look for the pipes in PIPES_DIR (default: /tmp/<USERNAME>/saturnd/pipes)\n\
";

/**
 * @brief Main function of cassini
 *
 * @param argc
 * @param argv
 * @return int
 *
 * uint8_t	1 byte unsigned integer
 * uint16_t	2 byte unsigned integer
 * uint32_t	4 byte unsigned integer
 * uint64_t	8 byte unsigned integer
 */

/* We don't want to check all read
   Maybe we can catch all
*/

int main(int argc, char* argv[]) {
    errno = 0;
    int fd;
    char* minutes_str = "*";
    char* hours_str = "*";
    char* daysofweek_str = "*";

    int opt;

    uint16_t operation = CLIENT_REQUEST_LIST_TASKS;
    uint64_t taskid;
    uint16_t opcode;

    char* strtoull_endp;
    while ((opt = getopt(argc, argv, "hlcqm:H:d:p:r:x:o:e:")) != -1) {
        switch (opt) {
            case 'm':
                minutes_str = optarg;
                break;
            case 'H':
                hours_str = optarg;
                break;
            case 'd':
                daysofweek_str = optarg;
                break;
            case 'p':
                pipes_directory = strdup(optarg);
                choose_pipes();
                break;
            case 'l':
                operation = CLIENT_REQUEST_LIST_TASKS;
                break;
            case 'c':
                operation = CLIENT_REQUEST_CREATE_TASK;
                break;
            case 'q':
                operation = CLIENT_REQUEST_TERMINATE;
                break;
            case 'r':
                operation = CLIENT_REQUEST_REMOVE_TASK;
                taskid = strtoull(optarg, &strtoull_endp, 10);
                if (strtoull_endp == optarg || strtoull_endp[0] != '\0') {
                    goto error;
                }
                break;
            case 'x':
                operation = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
                taskid = strtoull(optarg, &strtoull_endp, 10);
                if (strtoull_endp == optarg || strtoull_endp[0] != '\0') {
                    goto error;
                }
                break;
            case 'o':
                operation = CLIENT_REQUEST_GET_STDOUT;
                taskid = strtoull(optarg, &strtoull_endp, 10);
                if (strtoull_endp == optarg || strtoull_endp[0] != '\0') {
                    goto error;
                }
                break;
            case 'e':
                operation = CLIENT_REQUEST_GET_STDERR;
                taskid = strtoull(optarg, &strtoull_endp, 10);
                if (strtoull_endp == optarg || strtoull_endp[0] != '\0') {
                    goto error;
                }
                break;
            case 'h':
                printf("%s", usage_info);
                return 0;
            case '?':
                fprintf(stderr, "%s", usage_info);
                goto error;
        }
    }

    // --------
    // | TODO |
    // --------

    /** REQUEST **/
    if (pipes_directory == NULL) {
        initialize_default();
    }
    fd = open(request_pipe, O_WRONLY);
    switch (operation) {
        case CLIENT_REQUEST_CREATE_TASK: {
            /**
         * @brief Cr??er une t??che
         * REQUETE: OPCODE='CR' <uint16>, TIMING <timing>, COMMANDLINE
         <commandline>
         * TIMING: MINUTES <uint64>, HOURS <uint32>, DAYSOFWEEK <uint8>
         * COMMANDLINE: ARGC <uint32>, ARGV[0] <string>, ..., ARGV[ARGC-1]
         <string>
         */

            // On v??rfie si il n'y a pas d'erreur
            // On ??crit le OPCODE converti avec htobe pour qu'il soit
            // lisible pour le d??mon
            if (create_task_request(fd, minutes_str, hours_str, daysofweek_str,
                                    argc, optind, argv) == -1) {
                goto error;
            }

        } break;
        case CLIENT_REQUEST_LIST_TASKS:
            /**
             * @brief Lister les t??ches
             * Requ??teLIST
             * OPCODE='LS' <uint16>
             *
             * On va d'abord lire le pipe de requ??te ( et v??rifier si on
             * peut l'ouvrir afin d'??crire la requ??te ) Puis on va ??crire
             * l'opcode sur le pipe de requ??te
             */
            opcode = CLIENT_REQUEST_LIST_TASKS;
            if (list_task_request(fd, opcode) == -1) {
                goto error;
            }

            break;
        case CLIENT_REQUEST_TERMINATE:
            /**
             * @brief Terminer le d??mon
             * Requ??te TERMINATE
             * OPCODE='TM' <uint16>
             * On ouvre le tube de requ??te en ??criture seule
             * Puis on y ??crit l'opcode
             */
            opcode = CLIENT_REQUEST_TERMINATE;
            if (terminate_daemon_request(fd, opcode) == -1) {
                goto error;
            } else {
                return EXIT_SUCCESS;
            }
            break;
        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES:
            /**
             * @brief Lister l'heure d'ex??cution et la valeur de retour
             * de toutes les ex??cutions pr??c??dentes de la t??che
             * REQUETE: OPCODE='TX' <uint16>, TASKID <uint64>
             * On ouvre le tube de requ??te en ??criture seule
             * Puis on y ??crit l'opcode puis la tache
             */
            opcode = CLIENT_REQUEST_GET_TIMES_AND_EXITCODES;
            if (get_times_and_exit_code_request(fd, opcode, taskid) == -1) {
                goto error;
            }
            break;
        case CLIENT_REQUEST_REMOVE_TASK:
            /**
             * @brief Supprimer une t??che
             * REQUETE: OPCODE='RM' <uint16>, TASKID <uint64>
             * On ouvre le tube de requ??te en ??criture seule
             * Puis on y ??crit l'opcode puis la tache
             */
            opcode = CLIENT_REQUEST_REMOVE_TASK;
            if (remove_task_request(fd, opcode, taskid) == -1) {
                goto error;
            }
            break;
        case CLIENT_REQUEST_GET_STDERR:
            /**
             * @brief STDERR
             * Affiche la sortie erreur standard de la derni??re ex??cution
             * de la t??che
             * REQUETE: OPCODE='SE' <uint16>, TASKID <uint64> On
             * lit le tube de requ??te en ??criture seule on ??crit l'op??ration
             * converti avec htobe ainsi que le taskid aussi converti
             */
            if (get_stderr_request(fd, taskid) == -1) {
                goto error;
            }
            break;
        case CLIENT_REQUEST_GET_STDOUT:
            /**
             * @brief
             * Afficher la sortie standard de la derni??re ex??cution de la
             * t??che
             * REQUETE: OPCODE='SO' <uint16>, TASKID <uint64>
             */
            if (get_stdout_request(fd, taskid) == -1) {
                goto error;
            }
            break;
        default:
            break;
    }
    close(fd);

    fd = open(reply_pipe, O_RDONLY);
    switch (operation) {
        case CLIENT_REQUEST_CREATE_TASK:

            /**
             * @brief REPONSE
             * REPTYPE='OK' <uint16>, TASKID <uint64>
             * On ouvre le pipe de reply en lecture seulement
             * On lit le OK puis on lit l'identifiant de la t??che
             * Et on l'affiche
             */
            if (create_task_reply(fd, taskid) == -1) {
                goto error;
            }
            break;
        case CLIENT_REQUEST_LIST_TASKS: {
            /**
             * R??ponse LIST
             *
             * REPTYPE='OK' <uint16>, NBTASKS=N <uint32>,
             * TASK[0].TASKID <uint64>, TASK[0].TIMING <timing>,
             * TASK[0].COMMANDLINE <commandline>,
             *
             * On ouvre le tube de r??ponse pour y lire l?? r??ponse du d??mon
             * On va d'abord lire le OK <uint16> puis lire le NBTASK
             *
             * Puis pour chaque t??che allant de 0 ?? n-1
             * - On va d'abord lire le num??ro de l?? tache not?? taskid
             * - On va initialiser une structure timing not?? strtime
             * et ?? l'aide timing_string on va assign?? les minutes, heures,
             * jours de la semaine
             * - Et en suite on va r??cup??rer la commandline et la renvoyer
             */

            if (list_task_reply(fd, taskid) == -1) {
                goto error;
            }
        } break;
        case CLIENT_REQUEST_TERMINATE:
            /**
             * R??ponse TERMINATE
             * REPTYPE='OK' <uint16>
             * On lit la r??ponse mais on ne l'affiche pas voir r??ponse 16
             */
            if (terminate_daemon_reply(fd) > 0) {
                goto error;
            }
            break;
        case CLIENT_REQUEST_GET_TIMES_AND_EXITCODES: {
            /**
             * REPONSE de la requ??te:
             * OK: REPTYPE='OK' <uint16>, NBRUNS=N <uint32>
             *     RUN[0].TIME <int64>, RUN[0].EXITCODE <uint16>
             * ERROR: REPTYPE='ER' <uint16>, ERRCODE <uint16>
             *
             * On ouvre le tube de r??ponse en lecture seule
             * on y lit la r??ponse not?? rep
             * Si la r??ponse est ok
             *  On lit le ok puis on r??cup??re le nbruns
             *  apr??s pour chaque tache allant de 0 ?? n-1
             *    On r??cup??re le temp et le code de exit
             * Si la r??ponse est error on goto error afin de
             * terminer sur un exit(1) voir test 14
             */

            if (get_times_and_exit_code_reply(fd, taskid) == -1) {
                goto error;
            } else {
                if (get_times_and_exit_code_reply(fd, taskid) == 1) {
                    goto exit;
                }
            }
        } break;
        case CLIENT_REQUEST_REMOVE_TASK: {
            /**
             * REPONSE de la requ??te:
             * OK: REPTYPE='OK' <uint16>
             * ERROR: REPTYPE='ER' <uint16>, ERRCODE <uint16>
             *
             * On ouvre le tube de r??ponse en lecture seule
             * on y lit la r??ponse not?? rep
             * Si la r??ponse est ok on lit ok
             * Si la r??ponse est error on affiche l'erreur
             */

            if (remove_task_reply(fd) == -1) {
                goto error;
            }
        }

        break;
        case CLIENT_REQUEST_GET_STDERR: {
            /**
             * @brief R??ponse
             */

            // On r??cup??re le Ok
            if (get_stderr_reply(fd) == -1) {
                goto error;
            }
        } break;
        case CLIENT_REQUEST_GET_STDOUT: {
            /**
             * @brief R??ponse de STDOUT
             * REPONSE: OK Ou ERROR
             */
            if (get_stdout_reply(fd) == -1) {
                goto error;
            } else {
                if (get_stdout_reply(fd) == 1) {
                    goto exit;
                }
            }
        } break;
        default:
            break;
    }

    return EXIT_SUCCESS;
exit:
    return EXIT_FAILURE;

error:
    if (errno != 0) perror("main");
    free(pipes_directory);
    if (request_pipe != NULL) {
        free(request_pipe);
    }
    if (reply_pipe != NULL) {
        free(reply_pipe);
    }
    pipes_directory = NULL;
    request_pipe = NULL;
    reply_pipe = NULL;
    return EXIT_FAILURE;
}

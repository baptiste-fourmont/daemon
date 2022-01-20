#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>

struct timing {
  uint64_t minutes;
  uint32_t hours;
  uint8_t daysofweek;
};


struct String {
    uint32_t L;
    char *Chaine;
};
typedef struct String String;

struct commandLine {
    uint32_t argc;
    String *argv;
};
typedef struct commandLine commandLine;


#endif // TIMING_H

#ifndef TIMING_TEXT_IO_H
#define TIMING_TEXT_IO_H

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "timing.h"

#define TIMING_TEXT_MIN_BUFFERSIZE 1024


// Main functions :

int timing_from_strings(struct timing * dest, char * minutes_str, char * hours_str, char * daysofweek_str);
/* Writes the result in *dest. In case of success, returns 0. In case of failure, returns -1. */

int timing_string_from_timing(char * dest, const struct timing * timing);
/* Writes a text representation of timing in the buffer pointed to by dest, and adds a trailing
   '\0'. The buffer must be able to hold at least TIMING_TEXT_MIN_BUFFERSIZE characters. Returns the
   number of characters written, *excluding* the trailing '\0'. */



// Helper functions

int timing_field_from_string(uint64_t * dest, const char * string, unsigned int min, unsigned int max);
int timing_range_from_string(uint64_t * dest, const char * string, unsigned int min, unsigned int max);
int timing_uint_from_string(unsigned long int * dest, const char * string);

int timing_string_from_field(char * dest, unsigned int min, unsigned int max, uint64_t field);
int timing_string_from_range(char * dest, unsigned int start, unsigned int stop);


#endif // TIMING_TEXT_IO_H

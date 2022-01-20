#include "timing-text-io.h"

/* Writes the result in *dest. In case of success, returns 0. In case of failure, returns -1. */
int timing_from_strings(struct timing* dest, char* minutes_str, char* hours_str,
                        char* daysofweek_str) {
    uint64_t field;

    // Minutes
    if (timing_field_from_string(&field, minutes_str, 0, 59) <= 0) return -1;
    dest->minutes = htobe64(field);

    // Hours
    if (timing_field_from_string(&field, hours_str, 0, 23) <= 0) return -1;
    dest->hours = htobe32((uint32_t)(field));

    // Days of the week
    if (timing_field_from_string(&field, daysofweek_str, 0, 6) <= 0) return -1;
    dest->daysofweek = (uint8_t)field;

    return 0;
}

int timing_field_from_string(uint64_t* dest, const char* string,
                             unsigned int min, unsigned int max) {
    uint64_t result = 0;
    unsigned int pos = 0;

    if (string[pos] == 0) {
        return 0;

    } else if (string[pos] == '*') {
        for (unsigned int i = min; i <= max; i++) {
            result <<= 1;
            result |= 1;
        }
        *dest = result;
        return 1;

    } else {
        unsigned int range_from_string_result;
        range_from_string_result =
            timing_range_from_string(&result, string + pos, min, max);
        if (range_from_string_result <= 0) return 0;
        pos += range_from_string_result;

        while (string[pos] == ',') {
            pos++;
            range_from_string_result =
                timing_range_from_string(&result, string + pos, min, max);
            if (range_from_string_result <= 0) return 0;
            pos += range_from_string_result;
        }
    }

    *dest = result;
    return pos;
}

/* Adds range to *dest */
int timing_range_from_string(uint64_t* dest, const char* string,
                             unsigned int min, unsigned int max) {
    unsigned long int start;
    unsigned long int end;
    unsigned int pos = 0;
    if (!(min <= max && max <= min + 63)) return 0;

    int uint_from_string_result;
    uint_from_string_result = timing_uint_from_string(&start, string + pos);
    if (uint_from_string_result <= 0) return 0;
    pos += uint_from_string_result;

    if (string[pos] == '-') {
        pos++;
        uint_from_string_result = timing_uint_from_string(&end, string + pos);
        if (uint_from_string_result <= 0) return 0;
        pos += uint_from_string_result;
    } else {
        end = start;
    }

    if (start < min || end < start || max < end) return 0;

    uint64_t mask = 1LL << (start - min);
    for (unsigned long int i = start; i <= end; i++) {
        *dest |= mask;
        mask <<= 1;
    }

    return pos;
}

int timing_uint_from_string(unsigned long int* dest, const char* string) {
    char* endp;
    if (!isdigit(string[0])) return 0;
    *dest = strtoul(string, &endp, 10);
    return endp - string;
}

/* Writes a text representation of timing in the buffer pointed to by dest, and
   adds a trailing
   '\0'. The buffer must be able to hold at least TIMING_TEXT_MIN_BUFFERSIZE
   characters. Returns the number of characters written, *excluding* the
   trailing '\0'. */
int timing_string_from_timing(char* dest, const struct timing* timing) {
    unsigned int pos = 0;

    // Minutes
    pos +=
        timing_string_from_field(dest + pos, 0, 59, htobe64(timing->minutes));

    dest[pos] = ' ';
    pos++;

    // Hours
    pos += timing_string_from_field(dest + pos, 0, 23, htobe32(timing->hours));

    dest[pos] = ' ';
    pos++;

    // Days of the week
    pos += timing_string_from_field(dest + pos, 0, 6, timing->daysofweek);

    return pos;
}

int timing_string_from_field(char* dest, unsigned int min, unsigned int max,
                             uint64_t field) {
    if (!(min <= max && max <= min + 63)) return 0;

    unsigned int pos = 0;

    int range_active = 0;
    unsigned int range_start;
    unsigned int range_stop;
    uint64_t mask = 1LL;

    for (unsigned int i = min; i <= max + 1; i++) {
        if (!range_active) {
            if ((mask & field) != 0) {
                range_active = 1;
                range_start = i;
            }
        } else {
            if (i == max + 1 || (mask & field) == 0) {
                range_stop = i - 1;

                if (pos > 0) {
                    dest[pos] = ',';
                    pos++;
                }

                if (range_start == min && range_stop == max) {
                    dest[pos] = '*';
                    pos++;
                    dest[pos] = '\0';
                } else {
                    pos += timing_string_from_range(dest + pos, range_start,
                                                    range_stop);
                }

                range_active = 0;
            }
        }
        mask <<= 1;
    }

    return pos;
}

int timing_string_from_range(char* dest, unsigned int start,
                             unsigned int stop) {
    int sprintf_result;
    if (stop == start)
        sprintf_result = sprintf(dest, "%u", start);
    else
        sprintf_result = sprintf(dest, "%u-%u", start, stop);
    return sprintf_result;
}
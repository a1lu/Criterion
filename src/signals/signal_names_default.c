#include "signal_names.h"
#include <stdio.h>

#define BUF_SIZE    10

char const *get_signal_name(int signal)
{
    static char str[BUF_SIZE];
    switch (signal) {
        default:
            snprintf(str, BUF_SIZE, "%d", signal);
            return str;
    }
}

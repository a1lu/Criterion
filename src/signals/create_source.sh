#!/bin/sh

rm signal_names.c

cat << EOF > signal_names.c
#include "signals/signal_names.h"
#include <stdio.h>

#define BUF_SIZE 10
char const * get_signal_name(int signal){
    static char str[BUF_SIZE];
    switch(signal){
EOF

for i in `kill -l`
do
  echo -e "\tcase `kill -l $i`: return \"SIG_$i\";" >> signal_names.c
done

cat << EOF >> signal_names.c

        default:
            snprintf(str, BUF_SIZE, "%d", signal);
            return str;
    }
}
EOF

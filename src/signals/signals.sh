file=/usr/include/bits/signum.h
#file2=signal.h
if [ $# -lt 1 ]
then
    echo "Not enough arguments. Exit!"
    exit
fi

output=`cat $1 | grep -oE \
    "\bSIG[[:alnum:]]+[[:space:]]+([0-9]+|SIG[[:alnum:]]+|\(.*\(\)\))"`


python - << EOF > signal_names.h
print("""
char const * get_signal_name(const int signal);
""")
EOF

#python - << EOF > $2/signal_names.c
python - << EOF > signal_names.c
text="""${output}"""
#print(text)

print("""
#include <signal.h>
#include "signal_names.h"
#define str(x) #x

char const * get_signal_name(const int signal){""")
signals = []
for i, line in enumerate(text.split("\n")):
    list = line.split()
    sig = list[0]
    num = " ".join(list[1:])
    signals += [(sig,num)]
    if i == 0:
        print("    if (signal == "+sig+") return str("+sig+");")
    else:
        print("    else if (signal == "+sig+") return str("+sig+");")

print("    else return \"\";\n}")
EOF

#include "glob_match.h"
#include "stdio.h"

int main(int argc, char *argv[])
{
    int m=0;
    /*printf("%s\n",glob_match(argv[1],argv[2], &m)?"true":"false" );*/
    printf("%s\n",match(argv[1],argv[2])?"true":"false" );

}

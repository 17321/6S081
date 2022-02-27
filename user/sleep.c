#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if(argc!=2){
        printf("format should be sleep %%d");
        // return 1;
        exit(1);
    }
    sleep(atoi(argv[1]));
    // return 0;
    exit(0);
}
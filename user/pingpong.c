#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
    if(argc > 1){
        printf("usage:pingpang");
        exit(1);
    }
    char buf[5];
    int p[2];
    pipe(p);
    
    if(fork()==0){
      read(p[0],buf,5);
      printf("%d: received ping\n",getpid());
      exit(0);
    }else{
      write(p[1],"ping",5);
    }

    wait((int *)0);
    if(fork()==0){
      write(p[1],"ping",5);
      exit(0);
    }else{
      read(p[0],buf,5);
      printf("%d: received pong\n",getpid());
    }

    exit(0);
}

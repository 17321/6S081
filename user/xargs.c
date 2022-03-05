#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAX_LEN 512
// if(fork1() == 0){
//     close(1);            //***********
//     dup(p[1]);
//     close(p[0]);
//     close(p[1]);
//     runcmd(pcmd->left);  //管道左边关闭标准输出
// }
// if(fork1() == 0){
//     close(0);            //***********
//     dup(p[0]);
//     close(p[0]);
//     close(p[1]);
//     runcmd(pcmd->right); //管道右边关闭标准输入
// }

int main(int argc, char *argv[])
{
    if(argc > MAXARG){
        fprintf(2, "xargs: too many args\n");
        exit(1);
    }
    
    char buf[MAX_LEN];
    char *x_argv[MAXARG];
    int index=0;
    int n;          //read_size
    for(int i=1;i<argc;i++){
        x_argv[i-1]=argv[i];
    }

    //右管道fd=0为左管道输出

    while (1){
        index=0;
        while ((n=read(0,&buf[index],sizeof(char)))!=0){
            if(buf[index]=='\n'){
                buf[index]=0;
                break;
            }
            index++;
        }

        if(n==0){
            break;
        }
             
        x_argv[argc-1]=buf;

        if(fork()==0){
            exec(argv[1],x_argv);
        }

        wait(0);
    }

    exit(0);
}

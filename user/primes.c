#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
void prime(int *lp){
    //lp[0]在关闭lp[1]前会一直阻塞
    close(lp[1]);

    int p[2];
    pipe(p);
    
    int first;
    //只有主进程写入p[1],子进程读取lp[0](主进程中的p[0])后才能递归创建子进程
    if(read(lp[0],&first,sizeof(int))>0){
        printf("prime %d\n",first);
        if(fork()==0){
            prime(p);
        }
        close(p[0]);

        //主进程从lp[0]写入p[1]
        int n=0;
        //关闭两个写端，read才会停止等待
        while (read(lp[0],&n,sizeof(int))>0){
            if(n%first!=0){
                write(p[1],&n,sizeof(int));
            }
        }
        close(lp[0]);
        close(p[1]);

        //****等待后面的递归子进程全部完成后返回exit(0),
        wait(0);
        // 否则,若父进程先exit(0)，子进程被init收养，相当于primes命令结束后,继续运行子进程直至所有子进程完成(问题在于主进程结束时间不可预测)
        // 可能出现如下结果
        // $ primes
        // prime 2
        // prime 3
        // prime 5$
        //  prime 7
        // prime 11
        // prime 13
        // prime 17
        // prime 19
        // prime 23
        // prime 29
        // prime 31
        exit(0);
    }else{
        close(lp[0]);
        close(p[1]);
        exit(0);
    }
}


int main(int argc, char *argv[])
{
    int p[2];
    pipe(p);

    for(int i=2;i<=35;++i){
        write(p[1],&i,sizeof(int));
    }
    if(fork()==0){
        prime(p);
    }

    close(p[0]);
    close(p[1]);

    wait(0);
    exit(0);
}

#include<stdio.h>

int main(){
    int x[4][3]={{1,2,3},{4,5,6},{7,8,9},{10,11,12}};
    int (*p)[3]=x;
    printf("%u, %u, %u\n",(x+3),*(x+3),*(x+2)+3);
    printf("%u, %u, %u\n",(p+3),*(p+3),*(p+2)+3);
    int a[4]={19,17,14,18};
    int (*r)[4]=&a;
    printf("a[0]:%d\ta[1]:%d\ta[2]:%d\ta[3]:%d\n",*a,a[1],*(a+2),a[3]);
    int *q[3];
    q[0]=*(x+2)+1;
    q[1]=x[1];
    q[2]=x[0]+6;
    printf("*q[0]:%d\t*q[1]:%d\t*q[2]:%d\n",*q[0],*q[1],*q[2]); 
    return 0;
}

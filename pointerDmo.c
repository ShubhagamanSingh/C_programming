#include<stdio.h>
int main(){
    int a=10;
    int *p,**q,***r;
    p=&a;
    q=&p;
    r=&q;
    printf("Value of a: %u,\taddress of a: %u\n",a,&a);
    printf("Value of p: %u,\taddress of p: %u\n",p,&p);
    printf("Value of q: %u,\taddress of q: %u\n",q,&q);
    printf("Value of r: %u,\taddress of r: %u\n",r,&r);
    printf("Value of *p: %u\n",*p);
    printf("Value of *q: %u\n",*q);
    printf("Value of **q: %u\n",**q);
    printf("Value of *r: %u\n",*r);
    printf("Value of **r: %u\n",**r);
    printf("Value of ***r: %u\n",***r);
    return 0;
}
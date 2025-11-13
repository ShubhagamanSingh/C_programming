#include<stdio.h>
#include<string.h>

int main(){
    int a[10]={1,2,3,4,5,6,7,8,9,10};
    int* p,i=0; 
    char *c;
    long int *l;
    p=a;
    c=(char *)a;
    l=(long int *)a;
    printf("p=%u\t(p+1)=%u\tdif=%d\n",p,(p+1),(int)(p+1)-(int)p);
    printf("c=%u\t(c+1)=%u\tdif=%d\n",c,(c+1),(int)(c+1)-(int)c);
    printf("l=%u\t(l+1)=%u\tdif=%d\n",l,(l+1),(int)(l+1)-(int)l);
    for(i=0;i<10;i++){
        printf("i = %d\ta = %d\n",i,*(a+i));
        printf("*p=%d\t*(p+i)=%d\t",*p,*(p+i));
        printf("*c=%d\t*(c+i)=%d\t",*c,*(c+i));
        printf("*l=%d\t*(l+i)=%d\t",*l,*(l+i));
        printf("\n");
    }
    return 0;
}
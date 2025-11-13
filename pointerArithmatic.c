#include<stdio.h>
#include<string.h>

int main(){
    int a[10]={1,2,3,4,5};
    int* p; 
    char *c;
    long int *l;
    printf("p=%u\t(p+1)=%u\tdif=%d\n",p,(p+1),(int)(p+1)-(int)p);
    printf("c=%u\t(c+1)=%u\tdif=%d\n",c,(c+1),(int)(c+1)-(int)c);
    printf("l=%u\t(l+1)=%u\tdif=%d\n",l,(l+1),(int)(l+1)-(int)l);
    return 0;
}
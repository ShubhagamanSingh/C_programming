#include<stdio.h>
#define print(x) printf("%d",x)
int x;
void q(int z){
    z+=x;
    print(z);
}
void p(int *y){
    int x=*y+2;
    q(x);
    *y=x-1;
    print(x);
}
int main(){
    x=5;
    p(&x);
    print(x);
    return 0;
}
#include<stdio.h>

int main(){
    char c[10]="Hello";
    printf("%s\n",c);
    printf("%s\n",&c[0]);
    printf("%s\n",&0[c]);
    return 0;
}
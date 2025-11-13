#include<stdio.h>
void swap(int *a, int *b){
    int *t;
    *t=*a;
    *a=*b;
    *b=*t;
}
int main(){
    int x=10,y=20;
    printf("Before swap, Value of x: %d,\tValue of y: %d\n",x,y);
    swap(&x,&y);
    printf("After swap, Value of x: %d,\tValue of y: %d\n",x,y);
    return 0;
}
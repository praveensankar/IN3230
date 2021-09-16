#include <stdio.h>
#include <string.h>
struct student{
    char name[50];
    int age;
};
int main()
{
    int a=5;
    int arr[5]={1,2,3,4,5};
    int *p = &a;
    int *q = arr;
    struct  student s1;
    strcpy(s1.name, "praveen");
    s1.age=10;

    printf(" address of a : %d",&a);
    printf(" \n address of p : %d \t value in p : %d",&p,p);
    printf("\n value pointed by p : %d \n",*p);
    printf("\n arr[0] : %d \t arr[1] : %d \n",*q, *(q+1));
    printf("\n student name : %s \t age : %d", s1.name, s1.age);
    
    
    struct student *s2 = &s1;
    s2->age = 25;
    printf("\n student name : %s \t age : %d", s2->name, s1.age);

  struct student *s3 = (struct  student *) malloc(sizeof(struct student));
    s3->age = 30;
    strcpy(s3->name, "alex");
    printf("\n student name : %s \t age : %d", s3->name, s3->age);
  
    return;
}
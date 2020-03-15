#include <stdio.h>
#include <stdlib.h>
#include "uint128.h"

void fib_ctz()
{
    struct uint128 t1,t2,t3;
    t1.upper = 0;
    t1.lower = 1;
    t2.upper = 0;
    t2.lower = 0;
    
    for(int i=2;i<=180;i++)
    {
        add128(&t3,t1,t2);
	t2 = t1;
	t1 = t3;
        printf("%d : %llx%llx\n",i , t3.upper, t3.lower);
    }
}

int main()
{
    fib_ctz();
}

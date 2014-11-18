#include <stdio.h>
#include "threadsalive.h"

void thread1(void *arg)
{
    int *i = (int *)arg;
    *i += 7;
    fprintf(stderr, "begin t1: %d\n", *i);
    ta_yield();
    *i += 7;
    fprintf(stderr, "end t1: %d\n", *i);
}

void thread2(void *arg)
{
    int *i = (int *)arg;
    *i -= 7;
    fprintf(stderr, "begin t2: %d\n", *i);
    ta_yield();
    *i -= 7;
    fprintf(stderr, "end t2: %d\n", *i);
}

int main(int argc, char **argv)
{
    printf("Tester for stage 1.  Uses all four stage 1 library functions.\n");

    ta_libinit();
    printf("finished libinit \n");
    int i = 0;
    for (i = 0; i < 2; i++) {
        ta_create(thread1, (void *)&i);
	printf("created thread 1 \n");
        ta_create(thread2, (void *)&i);
	printf("created thread 2 \n");
    }

    int rv = ta_waitall();
    if (rv) {
        fprintf(stderr, "%d threads were not ready to run when ta_waitall() was called\n", -rv);
    }
    return 0;
}

#include "locks.h"

int var = 0;
static struct rw_lock_t lock = RW_LOCK_INIT;

void* thread1_ex(void* data)
{
    while(1)
    {
        r_lock(&lock);
	printf("Var: %d\n", var);
        if(var > 1024)
	{
		r_unlock(&lock);		
        	break;
	}	
        r_unlock(&lock);
    }

    return 0;
}

void* thread2_ex(void* data)
{
    while(1)
    {
        w_lock(&lock);
        if(++var > 1024)
	{
		w_unlock(&lock);		
        	break;
	}	
	//printf("Var: %d\n", var);
        w_unlock(&lock);
    }

    return 0;
}

void* thread4_ex(void* data)
{
    while(1)
    {
        w_lock(&lock);
        if(++var > 1024)
	{
		w_unlock(&lock);		
        	break;
	}	
	//printf("Var: %d\n", var);
        w_unlock(&lock);
    }

    return 0;
}

void* thread3_ex(void* data)
{
    while(1)
    {
        r_lock(&lock);
	printf("Var: %d\n", var);
        if(var > 1024)
	{
		r_unlock(&lock);		
        	break;
	}
        r_unlock(&lock);
    }

    return 0;
}

int main(int argc, char** argv)
{
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;
    pthread_t thread4;

    pthread_create(&thread1, NULL, thread1_ex, NULL);
    pthread_create(&thread2, NULL, thread2_ex, NULL);
    pthread_create(&thread3, NULL, thread3_ex, NULL);
    pthread_create(&thread4, NULL, thread4_ex, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_join(thread4, NULL);
}

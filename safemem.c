#include <stdio.h>
#include <pthread.h>
#define LOCKSTEPTHREAD_IMPL
#include "lockstepthread.h"

//You would presumably bury this mutex in a library and declare it extern in your header.
//This wouldn't restrict you to a single file project.
pthread_mutex_t myMutex;
#define SAFEPTR_RESOURCE_LOCK() {pthread_mutex_lock(&myMutex);}
#define SAFEPTR_RESOURCE_UNLOCK() {pthread_mutex_unlock(&myMutex);}


#define C_SAFEMEM_IMPL
#define C_SAFEMEM_DEBUG
#include "safemem.h"

void tfunc(){
	safepointer ptrs[100];
	//ptrs[42] = safepointer_malloc(1000, 1); //My special buffer.
	ptrs[42] = SAFEPTR_MALLOC(int,1000,1);
	for(size_t i = 0;i<10000;i++){
		if(i%100 != 42) ptrs[i%100] = SAFEPTR_MALLOC(int,30,1);
		if(i%100 == 0) puts("Another hundred iterations!\n");
		if(i%100 != 42) 
			if(safepointer_deref(ptrs[i%100]) == NULL)
			{	
				printf("We... didn't actually get any memory? iteration %zu\n",i);
				exit(1);
			}
		safepointer sp = ptrs[i%100];
		for(int i = 0; i < 35; i++)
		{	
			
			SAFEPTR_RESOURCE_LOCK()
				
			int x = 0;
			//Since garbage collection never occurs during threads and only during the between-threads area,
			//We dont have to guard access to malloc'd memory.
			//SAFEPTR_GUARD_IF(int, sp, i)
			//	p[i] = 47;
			SAFEPTR_TRY_WRITE(int, sp, i, 47);
			SAFEPTR_TRY_GET(x, int, sp, i);
			printf("\nValue is %d", x);
			SAFEPTR_RESOURCE_UNLOCK()
		}
		if(i%100 != 42)	safepointer_free(ptrs[i%100]); //Guarantee that we occasionally leak something.
		if(i%100 == 42)	safepointer_keepalive(ptrs[i%100],200);
	}
}


int main(){
	safepointer ptrs[100];
	myMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	lsthread t1, t2, t3, t4, t5, t6;

	init_lsthread(&t1);
	init_lsthread(&t2);
	init_lsthread(&t3);
	init_lsthread(&t4);
	init_lsthread(&t5);
	init_lsthread(&t6);

	t1.execute = tfunc;
	t2.execute = tfunc;
	t3.execute = tfunc;
	t4.execute = tfunc;
	t5.execute = tfunc;
	t6.execute = tfunc;
	
	start_lsthread(&t1);
	start_lsthread(&t2);
	start_lsthread(&t3);
	start_lsthread(&t4);
	start_lsthread(&t5);
	start_lsthread(&t6);
	for(size_t i = 0;i<10;i++){
		lock(&t1);
		lock(&t2);
		lock(&t3);
		lock(&t4);
		lock(&t5);
		lock(&t6);
		if(i%100 != 42) ptrs[i%100] = SAFEPTR_MALLOC(int,3000,1);
		if(i%100 == 0) puts("Another hundred iterations!\n");
		if(safepointer_deref(ptrs[i%100]) == NULL){	printf("(main) We... didn't actually get any memory? iteration %zu\n",i);
		exit(1);
		}
		safepointer_collect_garbage();
		if(i%100 != 42)	safepointer_free(ptrs[i%100]); //Guarantee that we occasionally leak something.
		if(i%100 == 42)	safepointer_keepalive(ptrs[i%100],200);
		step(&t5);
		step(&t4);
		step(&t6);
		step(&t3);
		step(&t2);
		step(&t1);
	}
	kill_lsthread(&t1);
	kill_lsthread(&t2);
	kill_lsthread(&t3);
	kill_lsthread(&t6);
	kill_lsthread(&t4);
	kill_lsthread(&t5);


	destroy_lsthread(&t1);
	destroy_lsthread(&t2);
	destroy_lsthread(&t3);
	destroy_lsthread(&t6);
	destroy_lsthread(&t4);
	destroy_lsthread(&t5);
	safepointer_collect_all();
	return 0;
}

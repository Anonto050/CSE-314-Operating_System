#include<stdio.h>
#include<unistd.h> // for sleep
#include<pthread.h>
#include<stdlib.h>


void * threadFunc1(void * arg)
{
	int i;
	for(i=1;i<=5;i++)
	{
		printf("%s\n",(char*)arg);
		sleep(1);
	}

	return NULL;
}

void * threadFunc2(void * arg)
{
	int i;
	for(i=1;i<=5;i++)
	{
		printf("%s\n",(char*)arg);
		sleep(1);
	}
	return NULL;
}





int main(void)
{	
	pthread_t thread1;
	pthread_t thread2;
	
	const char * message1 = "i am thread 1";
	const char * message2 = "i am thread 2";	
	
	pthread_create(&thread1,NULL,threadFunc1,(void*)message1 );
	pthread_create(&thread2,NULL,threadFunc2,(void*)message2 );

	while(1);
	return 0;
}

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include "mutexutil.h"
#include "semutil.h"
#include "condutil.h"
#include "threadutil.h"
#include "commutil.h"



static CTMutex *gpMutex = NULL;
static CTSemaphore *gpSem = NULL;
static int giSize = 0;
static CTCond	*gpCond = NULL;

DWORD thread1(void *arg)
{
	CTMutexLock(gpMutex);
	printf("in thread1\n");
	CTMutexUnlock(gpMutex);
	return 0;
}

DWORD thread2(void *arg)
{
	CTSleep(2);
	CTMutexLock(gpMutex);
	printf("in thread2\n");
	CTMutexUnlock(gpMutex);

	return 0;
}

int TestMutex()
{
	CTThreadHandle hThread1, hThread2;
	gpMutex = CTCreateMutex();
	if (gpMutex == NULL) {
		fprintf(stderr, "Create mutex failed\n");
		return -1;
	}

	CTCreateThread(&hThread1, thread1, NULL);
	CTCreateThread(&hThread2, thread2, NULL);

	CTSleep(INT_MAX);

	CTDestroyMutex(gpMutex);
	CTCloseThreadHandle(hThread1);
	CTCloseThreadHandle(hThread2);

	return 0;
}


// the following three functions are used to test Semaphore functions
DWORD SemProducer(void *arg)
{
	int item_count = 0;

	while (item_count < 100)
	{
		if (item_count % 2)
			CTSleep(2000);
		else
			CTSleep(1000);
		CTSemPost(gpSem);
		item_count++;
		printf("produce one more item, item_count: %d\n", item_count);
	}

	return 0;
}

DWORD SemConsumer(void *arg)
{
	int loop_count = 0;

	while (1)
	{
		CTSemWait(gpSem);
		printf("consume one item, loop_count:\t%d\n", ++loop_count);
	}

	return 0;
}

int TestSemaphore()
{
	CTThreadHandle hProducer, hConsumer;
	gpSem = CTCreateSemaphore(0);
	if (gpSem == NULL) {
		fprintf(stderr, "Create Semaphore failed\n");
		return -1;
	}

	CTCreateThread(&hProducer, SemProducer, NULL);
	CTCreateThread(&hConsumer, SemConsumer, NULL);

	CTWaitThread(hProducer);
	CTWaitThread(hConsumer);
	CTCloseThreadHandle(hProducer);
	CTCloseThreadHandle(hConsumer);

	CTDestroySemaphore(gpSem);
	

	return 0;
}


// the following three functions are used to test Semaphore functions
DWORD CondProducer(void *arg)
{
	int loop_count = 0;
	while (1) {
		CTMutexLock(gpMutex);
		while (giSize > 10) {
			printf("before CondWait\n");
			CTCondWait(gpCond, gpMutex);
			printf("after CondWait, loop_count:%d\n", ++loop_count);
		}

		giSize++;
		printf("giSize:%d\n", ++giSize);
		CTMutexUnlock(gpMutex);
	}

	return 0;
}

DWORD CondConsumer(void *arg)
{
	while (1) {
		CTSleep(1000);
		CTMutexLock(gpMutex);
		giSize--;
		printf("Consumer giSize:%d\n", giSize);
		CTCondSignal(gpCond);
		CTMutexUnlock(gpMutex);
	}
	

	
	return 0;
}

int TestCond()
{
	CTThreadHandle hProducer, hConsumer;
	gpCond = CTCreateCond();
	if (gpCond == NULL) {
		fprintf(stderr, "Create Condition failed\n");
		return -1;
	}

	gpMutex = CTCreateMutex();
	if (gpMutex == NULL) {
		fprintf(stderr, "Create mutex failed\n");
		return -1;
	}

	CTCreateThread(&hProducer, CondProducer, NULL);
	CTCreateThread(&hConsumer, CondConsumer, NULL);

	CTWaitThread(hProducer);
	CTWaitThread(hConsumer);
	CTCloseThreadHandle(hProducer);
	CTCloseThreadHandle(hConsumer);

	CTDestroyCond(gpCond);
	CTDestroyMutex(gpMutex);

	return 0;
}


int main()
{
	// TestMutex();
	// TestSemaphore();
	TestCond();
	return 0;
}
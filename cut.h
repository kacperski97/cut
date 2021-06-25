#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#define BUFF_SIZE 1024
#define SMALL_BUFF_SIZE 64
#define CPU_FIELDS 9
#define NUMBER_OF_FUNCTIONS 5

//Use when compiling with gcc or clang (generate warnings)

extern void *readData();
extern void *analyzeData();
extern void *printData();
extern void *watchdog();
extern void *logData();

//Use when compiling with clang without warnings, doesn't compile with gcc
/*
extern void *readData(void*);
extern void *analyzeData(void*);
extern void *printData(void*);
extern void *watchdog(void*);
extern void *logData(void*);
*/
extern void signalHandler(int);
extern void addElementToList(char *message);
extern char *removeElementFromList(void);
extern _Bool isEmptyList(void);

struct listElement{
	char* message;
	struct listElement *next;
};

extern _Bool activityFlags[NUMBER_OF_FUNCTIONS];
extern _Bool terminationFlag;
extern struct listElement *head;

extern pthread_mutex_t lockRawData;
extern pthread_mutex_t lockAnalyzedData;
extern pthread_mutex_t lockLog;

extern int coresAmount;

extern char **rawData;
extern unsigned int **analyzedData;

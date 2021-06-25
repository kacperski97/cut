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

struct listElement{
	char* message;
	struct listElement *next;
};

void *readData(void*);
void *analyzeData(void*);
void *printData(void*);
void *watchdog(void*);
void *logData(void*);

void signalHandler(int);
void addElementToList(char *message);
char *removeElementFromList(void);
_Bool isEmptyList(void);

_Bool activityFlags[NUMBER_OF_FUNCTIONS];
_Bool terminationFlag;
struct listElement *head = NULL;

pthread_mutex_t lockRawData = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockAnalyzedData = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockLog = PTHREAD_MUTEX_INITIALIZER;

int coresAmount = 0;

char **rawData;
unsigned int **analyzedData;

int main(){
	int i;
	FILE *f;
	char buff[BUFF_SIZE];

	head = NULL;
	
	for(i = 0; i < NUMBER_OF_FUNCTIONS; i++) {
		activityFlags[i] = 0;
	}

	f = popen("cat /proc/stat | grep \'cpu\'", "r");
	if (f == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}

	while (fgets(buff, sizeof(buff), f) != NULL) {
		coresAmount += 1;
	}
	
	rawData = malloc((unsigned long)coresAmount * sizeof(char*));
	analyzedData = malloc((unsigned long)coresAmount * sizeof(int*));
	
	for(i = 0; i < coresAmount; i++) {
		rawData[i] = malloc(SMALL_BUFF_SIZE * sizeof(char));
		analyzedData[i] = malloc((CPU_FIELDS - 1) * sizeof(int));
	}

	printf("%d, %s\n", coresAmount, "cat /proc/stat | grep \'cpu\'");
 	pclose(f);
	
    signal(SIGTERM, *signalHandler);
	signal(SIGINT, *signalHandler);

    pthread_t r, a, p, w, l;

    pthread_create(&r, NULL, &readData, NULL);
    pthread_create(&a, NULL, &analyzeData, NULL);
    pthread_create(&p, NULL, &printData, NULL);
	pthread_create(&w, NULL, &watchdog, NULL);
	pthread_create(&l, NULL, &logData, NULL);
    pthread_join(r, NULL);
    pthread_join(a, NULL);
    pthread_join(p, NULL);
	pthread_join(w, NULL);
	pthread_join(l, NULL);

	for(i = 0; i < coresAmount; i++) {
		free(rawData[i]);
		free(analyzedData[i]);
	}
	free(rawData);
	free(analyzedData);

	struct listElement *previous;

	while(head != NULL){
		previous = head;
		head = head->next;
		free(previous);		
	}

	pthread_mutex_destroy(&lockRawData);
	pthread_mutex_destroy(&lockAnalyzedData);
	pthread_mutex_destroy(&lockLog);


    printf("Program terminated\n");
    exit(0);
}

void *readData(){
	int i;	
	FILE *f;
	
    for(;;){
		if(terminationFlag){
			pthread_exit(NULL);
		}

		activityFlags[0] = 1;
		f = popen("cat /proc/stat | grep \'cpu\'", "r");
		if (f == NULL) {
			printf("Failed to run command\n" );
			exit(1);
		}
		
		pthread_mutex_lock(&lockRawData);
		
		for(i = 0; i < coresAmount; i++) {
			free(rawData[i]);
			rawData[i] = malloc(SMALL_BUFF_SIZE * sizeof(char));
			fgets(rawData[i], SMALL_BUFF_SIZE * sizeof(char), f);
		}
		
		pthread_mutex_unlock(&lockRawData);
		
		pclose(f);

		pthread_mutex_lock(&lockLog);

		addElementToList("Reader: loaded data from /proc/stat\n");

		pthread_mutex_unlock(&lockLog);

		sleep(1);
    }
}

void *analyzeData(){
	int i, j;
	unsigned int oldData[coresAmount][CPU_FIELDS - 1];
	
	for(i = 0; i < coresAmount; i++){
		for(j = 0; j < CPU_FIELDS - 1; j++){
			oldData[i][j] = 0;
		}
	}
	
    for(;;){
		if(terminationFlag){
			pthread_exit(NULL);
		}

		activityFlags[1] = 1;
		pthread_mutex_lock(&lockRawData);
		pthread_mutex_lock(&lockAnalyzedData);
		
		for(i = 0; i < coresAmount; i++){
			char delimiter[] = " ";
			strtok(rawData[i], delimiter);
			for(j = 0; j < CPU_FIELDS - 1; j++){
				char *c = strtok(NULL, " ");
				unsigned int value = 0;
				
				if(c != NULL){
					value = (unsigned int) atoi(c);
				}
				
				analyzedData[i][j] = value - oldData[i][j];
				oldData[i][j] = value;
			}
		}
		
		pthread_mutex_unlock(&lockAnalyzedData);
		pthread_mutex_unlock(&lockRawData);

		pthread_mutex_lock(&lockLog);

		addElementToList("Analyzer: analized raw data\n");

		pthread_mutex_unlock(&lockLog);
		
		sleep(1);
    }
}

void *printData(){
	int i;
	unsigned int nonIdled, idled, totald;
	
	sleep(2);

    for(;;){
		if(terminationFlag){
			pthread_exit(NULL);
		}

		activityFlags[2] = 1;
		//system("clear");
		pthread_mutex_lock(&lockAnalyzedData);
		nonIdled = analyzedData[0][0] + analyzedData[0][1] + analyzedData[0][2] + analyzedData[0][5] + analyzedData[0][6] + analyzedData[0][7];
		idled = analyzedData[0][3] + analyzedData[0][4];
		totald = nonIdled + idled;
			
		if(totald != 0){
			printf("cpu: %lf %%\n", (((double)(totald - idled) / totald)) * 100.0);
		}else{
				printf("A");
		}
		for(i = 1; i < coresAmount; i++){
			nonIdled = analyzedData[i][0] + analyzedData[i][1] + analyzedData[i][2] + analyzedData[i][5] + analyzedData[i][6] + analyzedData[i][7];
			idled = analyzedData[i][3] + analyzedData[i][4];
			totald = nonIdled + idled;
			
			if(totald != 0){
				printf("cpu%d: %lf %%\n", i, (((double)(totald - idled) / totald)) * 100.0);
			}else{
				printf("A");
			}
		}
		
		printf("\n");
		
		pthread_mutex_unlock(&lockAnalyzedData);

		pthread_mutex_lock(&lockLog);

		addElementToList("Printer: printed analyzed data\n");

		pthread_mutex_unlock(&lockLog);

        sleep(1);
    }
}

void *watchdog(){
	int i;

	sleep(4);

	for(;;){
		if(terminationFlag){
			pthread_exit(NULL);
		}

		for(i = 0; i < NUMBER_OF_FUNCTIONS - 1; i++) {
			if(!activityFlags[i]){
				pthread_mutex_lock(&lockLog);

				addElementToList("Watchdog: Watchdog timeout occured\n");

				pthread_mutex_unlock(&lockLog);
				signalHandler(1);
				pthread_exit(NULL);
			}	

			activityFlags[i] = 0;
		}

		pthread_mutex_lock(&lockLog);

		addElementToList("Watchdog: no timeout occured\n");

		pthread_mutex_unlock(&lockLog);		

		sleep(2);
	}
}

void *logData(){
	int i = 0;
	FILE *f = fopen("log.txt", "w");
	char buff[SMALL_BUFF_SIZE];

	for(;;){
		activityFlags[3] = 1;
		if(terminationFlag){
			while(!isEmptyList()){
				sprintf (buff, "%d: ", i);
				fputs(buff, f);
				fputs(removeElementFromList(), f);
				i += 1;	
			}

			sprintf (buff, "%d: Program terminated.\n", i);
			fputs(buff, f);
	
			fclose(f);
			pthread_exit(NULL);
		}

		if(!isEmptyList()){
			pthread_mutex_lock(&lockLog);
			
			sprintf (buff, "%d: ", i);
			fputs(buff, f);
			fputs(removeElementFromList(), f);
			i += 1;
			
			pthread_mutex_unlock(&lockLog);
		}

		sleep(1);
	}
}

void signalHandler(int a) {
	a = 1;
	terminationFlag = 1;
}

void addElementToList(char *message){
	if(head == NULL){
		struct listElement *temp = malloc(sizeof(struct listElement));
		temp->message = message;
		temp->next = NULL;
		head = temp;
	}else{
		struct listElement *temp = head;

		while(temp->next != NULL){
			temp = temp->next;
		}

		struct listElement *elementToAdd = malloc(sizeof(struct listElement));
		elementToAdd->message = message;
		elementToAdd->next = NULL;
		temp->next = elementToAdd;
	}
}

char *removeElementFromList(){
	char *c = head->message;

	struct listElement *newHead = head->next;
	free(head);
	head = newHead;
	
	return c;
}

_Bool isEmptyList(){
	if(head == NULL){
		return 1;
	}

	return 0;
}


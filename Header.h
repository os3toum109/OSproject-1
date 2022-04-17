#include<stdio.h>
#define printable(a) (((a) >= 32 && (a) <= 126) || (a) == '\t' || (a) == '\n')
#define MAX_PROCESSES 100
#define MAX_BURSTS 1000
#define MAX_TOKEN_LENGTH 30
#define MAX_LINE_LENGTH (1<<16)
#define COMMENT_CHAR '#'
#define COMMENT_LINE -1
#define NUMBER_OF_PROCESSORS 4

typedef struct burst burst;
typedef struct process process;
typedef struct process_node process_node;
typedef struct process_queue process_queue;

struct burst
{
    int length;
    int step;
};

struct process
{
    int pid;
    int arrivalTime;
    int startTime;
    int endTime;
    int waitingTime;
    int currentBurst;
    int numberOfBursts;
    burst bursts[MAX_BURSTS];
    int priority;
    int quantumRemaining;
    int currentQueue;
};

struct process_node
{
   process *data;
    process_node *next;
};

struct process_queue
{
    int size;
    process_node *front;
    process_node *back;
};

void error(char *);
void error_malformed_input_line(char *);
void error_many_bursts(int);

process_node *createProcessNode(process *);
void initializeProcessQueue(process_queue *);
void enqueueProcess(process_queue *, process *);
void dequeueProcess(process_queue *);
void enqueueProcessToFront(process_queue* q, process* p);

char *readLine(FILE *);
char *readLineHelper(char *, int, FILE *);
int readInt(char **);
int empty(char *);
int readProcess(process *, FILE *);
int compareByArrival(const void *, const void *);

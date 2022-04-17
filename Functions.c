#include "sch-helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

int currentPid = 0;
int currentArrivalTime = 0;

void error(char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(-1);
}
void error_malformed_input_line(char *line)
{
    int i;
    for (i=0;i<strlen(line);i++)
	{
        if (!printable(line[i]))
		{
            line = "[Error]:<Cannot display line due to unprintable characters>\n";
            break;
        }
    }
    fprintf(stderr, "[Error]: malformed input line:\n%s\n",line);
    exit(-1);
}
void error_many_bursts(int pid)
{
    fprintf(stderr, "[Error]: too many bursts provided for process with id %d.\n",pid);
    exit(-1);
}

process_node *createProcessNode(process *pr)
{
    process_node *node = (process_node*) malloc(sizeof(process_node));
    if (node == NULL) error("out of memory");
    node->data = pr;
    node->next = NULL;
    return node;
}
void initializeProcessQueue(process_queue *qu)
{
    qu->front = qu->back = NULL;
    qu->size = 0;
}
void enqueueProcess(process_queue *qu, process *pr)
{
    process_node *node = createProcessNode(pr);

    if (qu->front == NULL)
    {
        assert(qu->back == NULL);
        qu->front = qu->back = node;
    }
    else
    {
        assert(qu->back != NULL);
        qu->back->next = node;
        qu->back = node;
    }
    qu->size++;
}
void enqueueProcessToFront(process_queue* qu, process* pr)
{
    process_node* node = createProcessNode(pr);

    if (qu->front == NULL)
	{
        assert(qu->back == NULL);
        qu->front = qu->back = node;
    }
    else
	{
        assert(qu->back != NULL);
        node->next = qu->front;
        qu->front = node;

    }
    qu->size++;
}
void dequeueProcess(process_queue *qu)
{
    process_node *deleted = qu->front;

    assert(qu->size > 0);
    if (qu->size == 1)
	{
        qu->front = NULL;
        qu->back = NULL;
    } else
	{
        assert(qu->front->next != NULL);
        qu->front = qu->front->next;
    }
    free(deleted);
    qu->size--;
}

char *readLine(FILE * Ptr)
{
    char *prefix = (char*) calloc(1, 1);
    if (prefix == NULL) error("out of memory");
    return readLineHelper(prefix, 16, Ptr);
}
char *readLineHelper(char *prefix, int n,FILE * ptr)
{
    int prefixlen = strlen(prefix);
    char *result = (char*) calloc(n, 1);
    if (result == NULL) error("out of memory");
    
    assert(prefixlen < n);
    memcpy(result, prefix, prefixlen);
    
    if (fgets(result+prefixlen, n-prefixlen, ptr) == NULL) return NULL;
    if (strchr(result, '\n') == NULL) return readLineHelper(result, 2*n, ptr);
    
    free(prefix);
    return result;
}
int readInt(char **buf)
{
    int result = 0;
    
    while (isspace(**buf)) (*buf)++;
    
    if (**buf == '\0') return -1;
    
    while (**buf && !isspace(**buf))
	{
        int new_result = 0;
        if (**buf < '0' || **buf > '9') return -1;
        
        new_result = result * 10 + (**buf - '0');
        
        if (new_result < result) return -1;
        result = new_result;
        (*buf)++;
    }
    return result;
}
int empty(char *s)
{
    while (isspace(*s)) s++;
    return (*s == '\0');
}
int readProcess(process *dest, FILE * PTR)
{
    int i;
    int priority = -1;
    int arrivalTime = 0;
    int numberOfBurts = 0;
    int firstBurst = 0;
    int cpuBurstLength = 0;
    int ioBurstLength = 0;
    
    char *line = readLine(PTR);
    char *ptr = line;
    
    if (line == NULL)
	{
		return 0;
	}
    if (empty(line) || line[0] == COMMENT_CHAR)
	{
		return COMMENT_LINE;
	}
    
	if (ptr[0] == 'p')
	{
        ptr = ptr + 5; 
        if ((priority = readInt(&ptr)) < 0) error_malformed_input_line(line);
        if ((numberOfBurts = readInt(&ptr)) < 0) error_malformed_input_line(line);
        if ((firstBurst = readInt(&ptr)) < 0) error_malformed_input_line(line);
        dest->pid = currentPid;
        dest->arrivalTime = currentArrivalTime;
        dest->bursts[0].length = firstBurst;
        dest->bursts[0].step = 0;
        dest->numberOfBursts = 1;
        currentPid++;
        while (!empty(ptr))
		{

            if ((ioBurstLength = readInt(&ptr)) == -1)
			{
                error_malformed_input_line(line);
            }
            dest->bursts[dest->numberOfBursts].step = 0;
            dest->bursts[dest->numberOfBursts].length = ioBurstLength;
            dest->numberOfBursts++;

            if ((cpuBurstLength = readInt(&ptr)) == -1)
			{
                error_malformed_input_line(line);
            }
            
            if (dest->numberOfBursts == MAX_BURSTS)
			{
                error_many_bursts(dest->pid);
            }
            dest->bursts[dest->numberOfBursts].step = 0;
            dest->bursts[dest->numberOfBursts].length = cpuBurstLength;
            dest->numberOfBursts++;
        }
    }
    else if (ptr[0] == 'i')
	{
        ptr = ptr + 5;
        int idleTime = 0;
        if ((idleTime = readInt(&ptr)) == -1)
		{
            error_malformed_input_line(line);
        }
        currentArrivalTime += idleTime;
        return 2;
    }
    else if (ptr[0] == 'D')
	{
        return 0;
    }
    free(line);
    return 1;
}
int compareByArrival(const void *aa, const void *bb)
{
    process *a = (process*) aa;
    process *b = (process*) bb;
    if (a->arrivalTime < b->arrivalTime) return -1;
    if (a->arrivalTime > b->arrivalTime) return 1;
    return 0;
}

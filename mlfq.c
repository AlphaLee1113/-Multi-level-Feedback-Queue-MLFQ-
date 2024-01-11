/*
    COMP3511 Fall 2022 
    PA2: Simplified Multi-Level Feedback Queue (MLFQ)

    Your name: Lee Wai Kiu
    Your ITSC email: wkleeak@connect.ust.hk 

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks. 

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Define MAX_NUM_PROCESS
// For simplicity, assume that we have at most 10 processes

#define MAX_NUM_PROCESS 10
#define MAX_QUEUE_SIZE 10
#define MAX_PROCESS_NAME 5
#define MAX_GANTT_CHART 300

// Keywords (to be used when parsing the input)
#define KEYWORD_TQ0 "tq0"
#define KEYWORD_TQ1 "tq1"
#define KEYWORD_PROCESS_TABLE_SIZE "process_table_size"
#define KEYWORD_PROCESS_TABLE "process_table"

// Assume that we only need to support 2 types of space characters: 
// " " (space), "\t" (tab)
#define SPACE_CHARS " \t"

// Process data structure
// Helper functions:
//  process_init: initialize a process entry
//  process_table_print: Display the process table
struct Process {
    char name[MAX_PROCESS_NAME];
    int arrival_time ;
    int burst_time;
    int remain_time; // remain_time is needed in the intermediate steps of MLFQ 
};
void process_init(struct Process* p, char name[MAX_PROCESS_NAME], int arrival_time, int burst_time) {
    strcpy(p->name, name);
    p->arrival_time = arrival_time;
    p->burst_time = burst_time;
    p->remain_time = 0;
}
void process_table_print(struct Process* p, int size) {
    int i;
    printf("Process\tArrival\tBurst\n");
    for (i=0; i<size; i++) {
        printf("%s\t%d\t%d\n", p[i].name, p[i].arrival_time, p[i].burst_time);
    }
}

// A simple integer queue implementation using a fixed-size array
// Helper functions:
//   queue_init: initialize the queue
//   queue_is_empty: return true if the queue is empty, otherwise false
//   queue_is_full: return true if the queue is full, otherwise false
//   queue_peek: return the current front element of the queue
//   queue_enqueue: insert one item at the end of the queue
//   queue_dequeue: remove one item from the beginning of the queue
//   queue_print: display the queue content, it is useful for debugging
struct Queue {
    int values[MAX_QUEUE_SIZE];
    int front, rear, count;
};
void queue_init(struct Queue* q) {
    q->count = 0;
    q->front = 0;
    q->rear = -1; //the back
}
int queue_is_empty(struct Queue* q) {
    return q->count == 0;
}
int queue_is_full(struct Queue* q) {
    return q->count == MAX_QUEUE_SIZE;
}

int queue_peek(struct Queue* q) {
    return q->values[q->front];
}
void queue_enqueue(struct Queue* q, int new_value) {
    if (!queue_is_full(q)) {                    // Assume array is A[0..9] MAX_QUEUE_SIZE=10
        if ( q->rear == MAX_QUEUE_SIZE -1)  // q->rear=9
            q->rear = -1;
        q->values[++q->rear] = new_value; // then add value at A[0]
        q->count++;                     // now q->rear = 
    }
}
void queue_dequeue(struct Queue* q) {
    q->front++;                           // q->front = 9
    if (q->front == MAX_QUEUE_SIZE)     /// q->front =10 now
        q->front = 0;               /// then set q->front =0
    q->count--;
}
void queue_print(struct Queue* q) {
    int c = q->count;
    printf("size = %d\n", c);
    int cur = q->front;
    printf("values = ");
    while ( c > 0 ) {
        if ( cur == MAX_QUEUE_SIZE )
            cur = 0;
        printf("%d ", q->values[cur]);
        cur++;
        c--;
    }
    printf("\n");
}

// A simple GanttChart structure
// Helper functions:
//   gantt_chart_update: append one item to the end of the chart (or update the last item if the new item is the same as the last item)
//   gantt_chart_print: display the current chart
struct GanttChartItem {
    char name[MAX_PROCESS_NAME];
    int duration;
};
void gantt_chart_update(struct GanttChartItem chart[MAX_GANTT_CHART], int* n, char name[MAX_PROCESS_NAME], int duration) {
    int i;
    i = *n;
    // The new item is the same as the last item  //compare if they have the same name
    if ( i > 0 && strcmp(chart[i-1].name, name) == 0) 
    {
        chart[i-1].duration += duration; // update duration
    } 
    else
    {
        strcpy(chart[i].name, name);
        chart[i].duration = duration;
        *n = i+1;
    }
}
void gantt_chart_print(struct GanttChartItem chart[MAX_GANTT_CHART], int n) {
    int t = 0;
    int i = 0;
    printf("Gantt Chart = ");
    printf("%d ", t);
    for (i=0; i<n; i++) {
        t = t + chart[i].duration;     
        printf("%s %d ", chart[i].name, t);
    }
    printf("\n");
}

// Global variables
int tq0 = 0, tq1 = 0;
int process_table_size = 0;
struct Process process_table[MAX_NUM_PROCESS];

// Helper function: Check whether the line is a blank line (for input parsing)
int is_blank(char *line) {
    char *ch = line;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) )
            return 0;
        ch++;
    }
    return 1;
}
// Helper function: Check whether the input line should be skipped
int is_skip(char *line) {
    if ( is_blank(line) )
        return 1;
    char *ch = line ;
    while ( *ch != '\0' ) {
        if ( !isspace(*ch) && *ch == '#')
            return 1;
        ch++;
    }
    return 0;
}
// Helper: parse_tokens function
void parse_tokens(char **argv, char *line, int *numTokens, char *delimiter) {
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}

// Helper: parse the input file
void parse_input() {
    FILE *fp = stdin;
    char *line = NULL;
    ssize_t nread;
    size_t len = 0;

    char *two_tokens[2]; // buffer for 2 tokens
    char *three_tokens[3]; // buffer for 3 tokens
    int numTokens = 0, n=0, i=0;
    char equal_plus_spaces_delimiters[5] = "";

    char process_name[MAX_PROCESS_NAME];
    int process_arrival_time = 0;
    int process_burst_time = 0;

    strcpy(equal_plus_spaces_delimiters, "=");
    strcat(equal_plus_spaces_delimiters,SPACE_CHARS);

    while ( (nread = getline(&line, &len, fp)) != -1 ) {
        if ( is_skip(line) == 0)  {
            line = strtok(line,"\n");

            if (strstr(line, KEYWORD_TQ0)) {
                // parse tq0
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &tq0);
                }
            } 
            else if (strstr(line, KEYWORD_TQ1)) {
                // parse tq0
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &tq1);
                }
            }
            else if (strstr(line, KEYWORD_PROCESS_TABLE_SIZE)) {
                // parse process_table_size
                parse_tokens(two_tokens, line, &numTokens, equal_plus_spaces_delimiters);
                if (numTokens == 2) {
                    sscanf(two_tokens[1], "%d", &process_table_size);
                }
            } 
            else if (strstr(line, KEYWORD_PROCESS_TABLE)) {

                // parse process_table
                for (i=0; i<process_table_size; i++) {

                    getline(&line, &len, fp);
                    line = strtok(line,"\n");  

                    sscanf(line, "%s %d %d", process_name, &process_arrival_time, &process_burst_time);
                    process_init(&process_table[i], process_name, process_arrival_time, process_burst_time);

                }
            }

        }
        
    }
}
// Helper: Display the parsed values
void print_parsed_values() {
    printf("%s = %d\n", KEYWORD_TQ0, tq0);
    printf("%s = %d\n", KEYWORD_TQ1, tq1);
    printf("%s = \n", KEYWORD_PROCESS_TABLE);
    process_table_print(process_table, process_table_size);
}

// Global variables
// int tq0 = 0, tq1 = 0;
// int process_table_size = 0;
// struct Process process_table[MAX_NUM_PROCESS];

    // char name[MAX_PROCESS_NAME];
    // int arrival_time ;
    // int burst_time;
    // int remain_time;


// TODO: Implementation of MLFQ algorithm
void mlfq() {

    // Initialize the gantt chart
    struct GanttChartItem chart[MAX_GANTT_CHART];
    int sz_chart = 0;

    // TODO: implement your MLFQ algorithm here

    struct Queue q1; //for tq0 which is first quantum=2
    struct Queue q2; // for tq1 which is first quantum=4
    struct Queue q3;

    queue_init(&q1);
    queue_init(&q2);
    queue_init(&q3);

    //calculate the total burst time first
    int total_burst_time = 0;
    for(int i=0; i<process_table_size; i++){
        total_burst_time+=process_table[i].burst_time;
    }
    // printf("the total_burst_time = %d\n", total_burst_time);

    int q1_remaining_time=tq0;
    int q2_remaining_time=tq1;
    int counting_time=0;
    int count_adding_for_q1=0; //prevent the same process enter q1 again
    int count_adding_for_q2=0;
    int current_process_index=0;
    int current_minus_how_many=0;
    char current_process_name[MAX_PROCESS_NAME];

    // printf("check arrival time = %d\n", process_table[0].arrival_time);
    while(counting_time<total_burst_time){
    // while(counting_time<7){ //7 is ok
        //dynamically add the process to q1 if the process arrival time is larger than the counting time
        for(int i=count_adding_for_q1; i<process_table_size; i++){
            if(process_table[i].arrival_time<=counting_time){ // use <= because maybe already soneone is executing in q1
                queue_enqueue(&q1,process_table[i].burst_time);
                // printf("check arrival time = %d , countinf time =  %d\n", process_table[0].arrival_time, counting_time);
                break; //because only want one process get in q1, not the whole list(imagine t=0)
            }
        }
        //if q1 is not empty
        if(queue_is_empty(&q1)==0){ 
            //first check which process is this
            for(int i=count_adding_for_q1; i<process_table_size; i++){
                //arrival time is same/larger than the current time // it burst time match the queue_peek
                //printf("queue_peek(&q1) = %d\n",queue_peek(&q1));
                if(process_table[i].arrival_time<=counting_time && process_table[i].burst_time==queue_peek(&q1)){
                    current_process_index=i;
                    strcpy(current_process_name, process_table[i].name);
                    count_adding_for_q1+=1;
                    break; //only find the first process 
                }
            }
            // printf("now processing = %s\n", current_process_name);
            //check if the burst time of the first item is larger equal than the quantum of queue 1
            //if yes then
            // printf("queue_peek = %d and q1_remaining_time = %d\n",queue_peek(&q1), q1_remaining_time);
            if(queue_peek(&q1)>q1_remaining_time){
                //addd counting time by the quantum
                counting_time+=tq0;
                // printf("now counting time = %d\n",counting_time);
                gantt_chart_update(chart, &sz_chart, current_process_name,tq0); //because the function will add tq0 to the previous one
                // reduce the burst time 
                process_table[current_process_index].burst_time-=tq0;
                //move to queue 2
                queue_enqueue(&q2,process_table[current_process_index].burst_time);
                //Remove the item from q1
                queue_dequeue(&q1);
                // printf("now counting time = %d\n",counting_time);
                continue;
            }
            //check if the burst time is smaller/equal than the quantum of queue 1
            // then we can directly add the process to the gantt chart
            else if(queue_peek(&q1)<=q1_remaining_time){
                //addd counting time by the queue_peek first
                counting_time+=queue_peek(&q1);
                //because we already know the queue_peek is smaller than quantum ,so just use queue_peek in gantt chart
                gantt_chart_update(chart, &sz_chart, current_process_name, queue_peek(&q1)); //because the function will add tq0 to the previous one
                // set burst time to 0
                process_table[current_process_index].burst_time=0;
                //Remove the item from q1
                queue_dequeue(&q1);
                // printf("now counting time = %d\n",counting_time);
                continue;
            }
        }
        // meaning that queue 1 is already empty so we look at queue 2
        else if((queue_is_empty(&q1)!=0) && (queue_is_empty(&q2)==0)){ 
            //first check which process is this
            for(int i=count_adding_for_q2; i<process_table_size; i++){
                //arrival time is same/larger than the current time // it burst time match the queue_peek
                //checking if the process had entered q1 before && find the process which have the same remainf burst time
                if(process_table[i].arrival_time<=counting_time && (current_minus_how_many+process_table[i].burst_time==queue_peek(&q2))&&(process_table[i].remain_time>=0)){
                    // printf("queue_peek(&q2) = %d\n",queue_peek(&q2));
                    // printf("process_table[i].burst_time = %d and current_minus_how_many = %d\n",process_table[i].burst_time,current_minus_how_many);
                    if(process_table[i].remain_time==0){
                        process_table[i].remain_time=q2_remaining_time; //set remain_time as q2 quantum
                    }
                    current_process_index=i;
                    strcpy(current_process_name, process_table[i].name);
                    break; //only find the first process 
                }
            }
            // printf("now processing = %s\n", current_process_name);
            // printf("queue_peek = %d and process_table[current_process_index].remain_time = %d\n",queue_peek(&q2), process_table[current_process_index].remain_time);

            //add 1 first because may have other new process come in
            counting_time+=1;
            process_table[current_process_index].remain_time-=1;
            current_minus_how_many+=1;
            // reduce the burst time 
            process_table[current_process_index].burst_time-=1;
            gantt_chart_update(chart, &sz_chart, current_process_name,1); //because the function will add tq0 to the previous one
            
            if(process_table[current_process_index].remain_time==0 || process_table[current_process_index].burst_time==0){
                process_table[current_process_index].remain_time=-1; //distinguish that this process can no longer stay in q2
                current_minus_how_many=0;
                if(current_process_index==0){
                    count_adding_for_q2=1;
                }
                else{
                    count_adding_for_q2=current_process_index;
                }
                
                if(process_table[current_process_index].burst_time>0){
                    queue_dequeue(&q2);
                    queue_enqueue(&q3,process_table[current_process_index].burst_time);
                }
                else if(process_table[current_process_index].burst_time==0){
                    queue_dequeue(&q2);
                }
            }
            // printf("now counting time = %d\n",counting_time);
            continue;

        }
        //both q1 and q2 is empty
        else if((queue_is_empty(&q1)!=0) && (queue_is_empty(&q2)!=0)){
           for(int i=0; i<process_table_size; i++){
                //arrival time is same/larger than the current time // it burst time match the queue_peek
                // printf("queue_peek(&q3) = %d\n",queue_peek(&q3));
                if(process_table[i].arrival_time<=counting_time && process_table[i].burst_time==queue_peek(&q3)){
                    current_process_index=i;
                    strcpy(current_process_name, process_table[i].name);
                    count_adding_for_q1+=1;
                    break; //only find the first process 
                }
            }
            // printf("now processing = %s\n", current_process_name);
            //check if the burst time of the first item is larger equal than the quantum of queue 1
            //if yes then
            // printf("queue_peek = %d and q1_remaining_time = %d\n",queue_peek(&q3), q1_remaining_time);

            //addd counting time by the queue_peek first
            counting_time+=queue_peek(&q3);
            //because we already know the queue_peek is smaller than quantum ,so just use queue_peek in gantt chart
            gantt_chart_update(chart, &sz_chart, current_process_name, queue_peek(&q3)); //because the function will add tq0 to the previous one
            // set burst time to 0
            process_table[current_process_index].burst_time=0;
            //Remove the item from q1
            queue_dequeue(&q3);
            // printf("now counting time = %d\n",counting_time);
            continue;
        }
    }

    // Check if q1 is empty, if yes then start executing q2

    // printf("check q1 is empty");

    // gantt_chart_update(struct GanttChartItem chart[MAX_GANTT_CHART], int* n, char name[MAX_PROCESS_NAME], int duration)

    // At the end, uncomment this line to display the final Gantt chart
    gantt_chart_print(chart, sz_chart);
}


int main() {
    parse_input();
    print_parsed_values();
    mlfq();
    return 0;
}
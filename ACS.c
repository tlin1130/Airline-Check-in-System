/*
    University of Victoria
    CSC 360
    Assignment 2 
    Han-wei Lin	
    V00803987

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <limits.h>

// structure definitions 
typedef struct customer{
    int id;
    int classType;
    int arrivalTime;
    int serviceTime;
    int time_enter_q;
    int time_leave_q;
} customer; 

struct Queue{ 
    int front; 
    int rear;
    int size;
    unsigned capacity; 
    customer* array; 
}; 

// Constants
#define MAX_CUSTOMERS 256

/*
    Global variables
*/
 
int totalCustomer;
pthread_mutex_t mutex_c; //guards clerks[]
pthread_mutex_t mutex_q[2]; //guards queues
struct timeval start;
void* runner(void* param);
int clerks[4] = {0,0,0,0};
struct Queue* queue[2]; //queue[0] is for business queue; queue[1] is for economy queue
int availableClerks = 4;
pthread_mutex_t mutex_acl; //acl stands for availability of clerk
pthread_cond_t convar_acl;
pthread_mutex_t mutex_t[2]; //guards wait_time[]
int wait_time[2] = {0,0}; //wait_time[0] for business class; wait_time[1] for 
pthread_mutex_t mutex_num[2]; //guards num[]
int num[2] = {0,0}; //num[0] for # of customer in business class; num[1] for # of customer in economy class

/*
    Helper function
*/

//return 1 if data is negative; 0 if postive  
int isNegative(int data){
    if (data < 0){
        return 1;
    } else {
        return 0;
    }
}

//replace any ':' with ',' in s 
void replaceColon(char s[]){
	int i = 0;
	while (s[i] != '\0'){
		if (s[i] == ':'){
			s[i] = ',';
		}
		i++;
	}
}

//parse the customer data
void parseCustomerData(char fileContents[totalCustomer][32], customer customers[totalCustomer]){
    int i;
    for (i = 0; i < totalCustomer; i++){
        replaceColon(fileContents[i]);
        int j = 0;
        int customerArray[4];
        char* token = strtok(fileContents[i], ",");
        while (token != NULL){
	    customerArray[j] = atoi(token);
            //check if illegal values appear in input file 
            if (isNegative(customerArray[j]) == 1){
                printf("Error: Illegal value(s) found in input file\n");
                exit(-1);
            }
	    token = strtok(NULL, ",");
	    j++;
        }

        customer c = {
            customerArray[0], //cid
            customerArray[1], //class type
            customerArray[2], //arrival time
            customerArray[3], //service time
            0,
            0,
        }; 
        customers[i] = c;
    }
}

/*
    Queue functions
*/

// creates a queue data structure and returns a ptr to it 
struct Queue* createQueue(unsigned capacity){ 
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue)); 
    queue->capacity = capacity; 
    queue->front = 0;
    queue->rear = capacity - 1;
    queue->size = 0;
    queue->array = (customer*) malloc(queue->capacity * sizeof(customer)); 
    return queue; 
}
 
// if queue is full, returns 1; otherwise 0
int isFull(struct Queue* queue){
    return (queue->size == queue->capacity);
} 

// if queue is empty, returns 1; otherwise 0
int isEmpty(struct Queue* queue){
    return (queue->size == 0); 
}

// adds a customer into queue
// explicitly check if queue is full before using enqueue()
void enqueue(struct Queue* queue, customer* c){ 
    if (isFull(queue)){
        return;
    } 
    queue->rear = (queue->rear + 1) % queue->capacity; 
    queue->array[queue->rear] = *c; 
    queue->size = queue->size + 1;
} 

// removes and returns a customer from queue
// explicitly check if queue is empty before using dequeue()
customer dequeue(struct Queue* queue){ 
    if (isEmpty(queue)){ 
        return;
    } 
    customer c = queue->array[queue->front]; 
    queue->front = (queue->front + 1) % queue->capacity; 
    queue->size = queue->size - 1; 
    return c; 
}

//returns the size of queue
int sizeOfQueue(struct Queue* queue){
    return queue->size;
}

/*
    The runner function run by each thread 
*/

void* runner(void* param){

    customer* c = (customer*)param;
    int classType = c->classType;
    struct timeval currentTime;
    int t;

    //wait for arrival 
    int arrivalTime = c->arrivalTime;
    usleep(arrivalTime * 100000);
    printf("A customer arrives: customer id: %d\n", c->id);
    
    // update num[]
    pthread_mutex_lock(&mutex_num[classType]);
    num[classType] += 1;
    pthread_mutex_unlock(&mutex_num[classType]);
    
    //enter queue
    pthread_mutex_lock(&mutex_q[classType]);
    gettimeofday(&currentTime, NULL);
    t = currentTime.tv_sec - start.tv_sec;
    c->time_enter_q = t;
    enqueue(queue[classType],c);
    int size = sizeOfQueue(queue[classType]);
    printf("A customer enters queue: queue id: %d and the length of queue is %d\n", classType, size);
    pthread_mutex_unlock(&mutex_q[classType]);
    
    //request clerk to process customer 
    pthread_mutex_lock(&mutex_acl);
    while(availableClerks == 0){
        pthread_cond_wait(&convar_acl, &mutex_acl);
    }
    availableClerks = availableClerks - 1;
    pthread_mutex_unlock(&mutex_acl);
    
    //get an available clerk
    int clerkID;
    int getClerkID = 0; // once clerk id is got, this var becomes 1
    int index = 0;
    pthread_mutex_lock(&mutex_c);
    while (getClerkID == 0){
        if (clerks[index] == 0){
            clerkID = index;
            clerks[index] = 1;
            getClerkID = 1;         
        } else {
            index = index + 1;
        }    
    }
    pthread_mutex_unlock(&mutex_c);

    // get a customer from queue0; if queue0 empty, get a customer from queue1
    pthread_mutex_lock(&mutex_q[0]);
    pthread_mutex_lock(&mutex_q[1]);
    customer cc;
    if (!isEmpty(queue[0])){
        cc = dequeue(queue[0]);
    } else {
        cc = dequeue(queue[1]);
    }
    pthread_mutex_unlock(&mutex_q[0]);
    pthread_mutex_unlock(&mutex_q[1]);
    
    // process customer cc
    int cid = cc.id;
    int serviceTime = cc.serviceTime;
    gettimeofday(&currentTime, NULL);
    t = currentTime.tv_sec - start.tv_sec;
    cc.time_leave_q = t;
    printf("A clerk starts serving a customer: start time: %d, the customer id: %d, the clerk id: %d\n", t, cid, clerkID);
    usleep(serviceTime * 100000);
    pthread_mutex_lock(&mutex_c);
    clerks[clerkID] = 0;
    pthread_mutex_unlock(&mutex_c);
    gettimeofday(&currentTime, NULL);
    t = currentTime.tv_sec - start.tv_sec;
    printf("A clerk finishes serving a customer: end time: %d, the customer id: %d, the clerk id: %d\n", t, cid, clerkID);

    // update availableClerks and broadcast
    pthread_mutex_lock(&mutex_acl);
    availableClerks = availableClerks + 1;
    pthread_cond_broadcast(&convar_acl);
    pthread_mutex_unlock(&mutex_acl);

    // upadte wait_time[]
    pthread_mutex_lock(&mutex_t[cc.classType]);
    t = (cc.time_leave_q - cc.time_enter_q);
    wait_time[cc.classType] += t;
    pthread_mutex_unlock(&mutex_t[cc.classType]);
    pthread_exit(0);
}

//main function
int main(int argc, char* argv[]){

    if (argc == 1){
        printf("Error: Input file not provided\n");
        return -1;
    } else if (argc > 2){
        printf("Error: Too many input arguments\n");
        return -1;
    } 
   
    /* read input data file */
    FILE* ptr_file;
    ptr_file = fopen(argv[1],"r");
    char buff[8];
    int i;

    // check if file is opened sucessfully and if file is empty
    if (!ptr_file){
        printf("Error: Failed to read %s\n", argv[1]);
        return -1;
    } else if (feof(ptr_file) != 0){
        printf("Error: %s is empty\n", argv[1]);
        return -1;
    }

    // get the first line which stores number of total customers
    fgets(buff, 8, ptr_file);
    totalCustomer = atoi(buff);
    //printf("Total customers: %d\n",totalCustomer); //test
    if (isNegative(totalCustomer) == 1){
        printf("Error: Illegal values in %s\n", argv[1]);
        return -1;
    }

    //store customer data into fileContents
    char fileContents[totalCustomer][32];      
    for (i = 0; i < totalCustomer; i++){
        fgets(fileContents[i], 32, ptr_file);
    }
    fclose(ptr_file);
    /* done reading input data file */

    //testing code  
    /*for (i = 0; i < totalCustomer; i++){
        printf("%s", fileContents[i]);
    }*/

    customer customers[totalCustomer];
    parseCustomerData(fileContents, customers);  
    
    // testing code
    //printf("Flow passed!\n");
    /*for (i = 0; i < totalCustomer; i++){
        customer c = customers[i];
        printf("%d %d %d %d\n", c.id, c.classType, c.arrivalTime, c.serviceTime);   
    }*/
    
    //initialize mutex for queue0 & queue1
    for (i = 0; i < 2; i++){
        if (pthread_mutex_init(&mutex_q[i], NULL) != 0) {
            printf("Error: Failed to initialize mutex\n");
            exit(-1);
        }
    }
    //initialize mutex for mutex_t[]
    for (i = 0; i < 2; i++){
        if (pthread_mutex_init(&mutex_t[i], NULL) != 0) {
            printf("Error: Failed to initialize mutex\n");
            exit(-1);
        }  
    }
    //initialize mutex for mutex_num[]
    for (i = 0; i < 2; i++){
        if (pthread_mutex_init(&mutex_num[i], NULL) != 0) {
            printf("Error: Failed to initialize mutex\n");
            exit(-1);
        }  
    }
    //initialize mutex and cond for acl 
    if (pthread_mutex_init(&mutex_acl, NULL) != 0) {
        printf("Error: Failed to initialize mutex\n");
        exit(-1);
    }  
    if (pthread_cond_init(&convar_acl, NULL) != 0) {
        printf("Error: Failed to initialize conditional variable\n");
        exit(-1);
    }    
    //initialize mutex for clerks array
    if (pthread_mutex_init(&mutex_c, NULL) != 0) {
        printf("Error: Failed to initialize mutex\n");
        exit(-1);
    }  
    
    //initialize pthread_attr_t attr and pthread_t customerThreads[]
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        printf("Error: Failed to initialize attr\n");
        exit(-1);
    }
    
    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
        printf("Error: Failed to set detachstate\n");
        exit(-1);
    }
    
    //configuration before child threads creation
    pthread_t customerThreads[totalCustomer];
    gettimeofday(&start, NULL);
    queue[0] = createQueue(512);
    queue[1] = createQueue(512);
    
    //create a thread for each customer
    for (i = 0; i < totalCustomer; i++) {
        if (pthread_create(&customerThreads[i], &attr, runner, (void*)&customers[i]) != 0){
            printf("Error: Failed to create pthread.\n");
            exit(-1);
        }
    }

    //wait for all threads to terminate
    for (i = 0; i < totalCustomer; i++) {
        if (pthread_join(customerThreads[i], NULL) != 0) {
            printf("Error: failed to join pthread.\n");
            exit(-1);
        }
    }
    
    //print final output statistics 
    printf("Total customers served: %d\n",totalCustomer);
    printf("Average waiting time summary:\n");
    int time = wait_time[0] + wait_time[1];
    int avg = time / totalCustomer;
    printf("The average waiting time for all cutomers in the system: %d seconds\n", avg);
    avg = wait_time[0] / num[0];
    printf("The average waiting time for business-class cutomers in the system: %d seconds\n", avg);
    avg = wait_time[1] / num[1];
    printf("The average waiting time for economy-class cutomers in the system: %d seconds\n", avg);

    //destroy mutex and conditional variable
    if (pthread_attr_destroy(&attr) != 0) {
        printf("Error: Failed to destroy attr\n");
        exit(-1);
    }
    for (i = 0; i < 2; i++) {
        if (pthread_mutex_destroy(&mutex_q[i]) != 0) {
            printf("Error: Failed to destroy mutex\n");
            exit(-1);
        }
    }
    for (i = 0; i < 2; i++) {
        if (pthread_mutex_destroy(&mutex_t[i]) != 0) {
            printf("Error: Failed to destroy mutex\n");
            exit(-1);
        }
    }
    for (i = 0; i < 2; i++) {
        if (pthread_mutex_destroy(&mutex_num[i]) != 0) {
            printf("Error: Failed to destroy mutex\n");
            exit(-1);
        }
    }
    if (pthread_mutex_destroy(&mutex_acl) != 0) {
        printf("Error: Failed to destroy mutex\n");
        exit(-1);
    }
    if (pthread_cond_destroy(&convar_acl) != 0) {
        printf("Error: Failed to destroy convar\n");
        exit(-1);
    }
    if (pthread_mutex_destroy(&mutex_c) != 0) {
        printf("Error: Failed to destroy mutex\n");
        exit(-1);
    }

    pthread_exit(NULL);
    return 0;

}

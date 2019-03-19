/*
    Muhammet Tayyip Muslu
    2015510101
    via Ubuntu LTS 18.04 (tried debian 8.0.0 jessie)
    Operating Systems Assignment III - Parallel File Copy (Async I/O)
*/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <aio.h> // parallel file r/w
#include <errno.h>
#include <pthread.h> // for multi-threading

#define FILE_SIZE 50
#define MAIN_FILE "source.txt"
#define COPY_FILE "destination.txt"
#define MAX_THREAD 10

static int* thread_start; // ofset start
static int* thread_stop;  // ofset stop
static const char ucase[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

void* process_arguments(int argc, char *args[]);
void* split_file_size();
void* create_main_file();
void* create_join_threads();
void* child();

int thread_num = -1; // number of threads initial -1
char source_file_path[255];
char destination_file_path[255];

int main(int argc, char *args[]){
    // main process
    process_arguments(argc,args);
    if(thread_num < 1) return 1;
    thread_start = malloc( thread_num * sizeof(int)); thread_stop = malloc( thread_num * sizeof(int)); // determine ofset array size
    split_file_size(); // determine ofsets
    create_main_file();// write main file randomly
    create_join_threads();// read main file, than write to new file via multi-threading
    return 0;
}
void* process_arguments(int argc, char *args[]){
    int i;
    if(argc == 4){
        if(strcmp(args[1], "-") == 0){ // sourcefile
            strcpy(source_file_path, MAIN_FILE);
        }
        else{
            strcpy(source_file_path, args[1]);
            strcat(source_file_path, "/");
            strcat(source_file_path, MAIN_FILE);
        }
        if(strcmp(args[2], "-") == 0){ // sourcefile
            strcpy(destination_file_path, COPY_FILE);
        }
        else{
            strcpy(destination_file_path, args[2]);
            strcat(destination_file_path, "/");
            strcat(destination_file_path, COPY_FILE);
        }
        thread_num = atoi(args[3]);
        if(thread_num > MAX_THREAD){ 
            printf("\n\nMaximum thread number is exceeded, num must not be greater then 10 !\n\n");
            thread_num = -1;
        }
        else printf("\n--------Entered Arguments--------\n\nSource file path => %s\nDestination file path => %s\nThread num => %d\n", source_file_path, destination_file_path, thread_num);
    }
    else{
        // not enougf arguments
        printf("\n\nMissing arguments, please enter arguments like (./executablefile sourcefile destinationfile threadnums\n\n");
    }
}
// determine threads ofset
void* split_file_size(){
    printf("\nTotal FILE SIZE ==> %d bytes ( 0 - %d )\n", FILE_SIZE, FILE_SIZE-1);
    int size_mean = FILE_SIZE / thread_num;
    int i, start = 0;
    for(i = 0; i < thread_num - 1; i++){
        thread_start[i] = start;
        thread_stop[i] = start + size_mean - 1;
        start += size_mean;
    }
    thread_start[thread_num-1] = start;
    thread_stop[thread_num-1] = FILE_SIZE-1;
    printf("\n--------Threads Business Sharing--------\n\n");
    for(i = 0; i < thread_num; i++){
        printf("Process %d start %d stop %d\n", (i+1), thread_start[i], thread_stop[i]);
    }
}
// create a main file randomly
void* create_main_file(){
    int fd,err,ret;
    char data[FILE_SIZE];
    struct aiocb aio_w;
    int i;
    for(i = 0; i < FILE_SIZE; i++) data[i] = ucase[i%26];
    
    fd = open (source_file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        perror("open");
    }
    memset(&aio_w, 0, sizeof(aio_w));
    aio_w.aio_fildes = fd;
    aio_w.aio_buf = data;
    aio_w.aio_nbytes = sizeof(data);
    if (aio_write(&aio_w) == -1) {
        printf(" Error at aio_write(): %s\n", strerror(errno));
        close(fd);
        exit(2);
    }
    while (aio_error(&aio_w) == EINPROGRESS) { }
    err = aio_error(&aio_w);
    ret = aio_return(&aio_w);

    if (err != 0) {
        printf ("Error at aio_error() : %s\n", strerror (err));
        close (fd);
        exit(2);
    }

    if (ret != sizeof(data)) {
        printf("Error at aio_return()\n");
        close(fd);
        exit(2);
    }

    close(fd);
    printf ("\n\nSource file is created randomly\n\n");
}
// create, and join threads
void* create_join_threads(){
    // create child processes
    pthread_t childs[thread_num]; int ID[thread_num];
    int i;
    for (i = 0; i < thread_num; i++) ID[i] = i;

    for(i = 0; i < thread_num; i++) pthread_create(&childs[i], NULL, child, (void *)&ID[i]);
    for(i = 0; i < thread_num; i++) pthread_join(childs[i], NULL);
}
// threads function
void* child(void *p){
    // read data from main_file in ofset between thread_start and thread_stop
    int id = *(int *)p;

    int fd = 0,err,ret;
    struct aiocb aio_r,aio_w;
    int buf_size = thread_stop[id]-thread_start[id]+1; // buffer size per thread
    char data[buf_size]; // buf size 
    // read from source file
    fd = open(source_file_path, O_RDONLY);
    if (fd == -1) {
        perror("open");
    }

    memset(&aio_r, 0, sizeof(aio_r));
    memset(&data, 0, sizeof(data));
    
    aio_r.aio_fildes = fd;
    aio_r.aio_buf = data;
    aio_r.aio_offset = thread_start[id];
    aio_r.aio_nbytes = sizeof(data);
    aio_r.aio_lio_opcode = LIO_READ; // aio_lio_opcode: The operation to be performed; enum { LIO_READ, LIO_WRITE, LIO_NOP };

    if (aio_read(&aio_r) == -1) {
        printf("Error at aio_read(): %s\n",
        strerror(errno));
        exit(2);
    }
    while (aio_error(&aio_r) == EINPROGRESS) { }
    err = aio_error(&aio_r);
    ret = aio_return(&aio_r);

    if (err != 0) {
        printf ("Error at aio_error() : %s\n", strerror (err));
        close (fd);
        exit(2);
    }

    if (ret != buf_size) {
        printf("Error at aio_return()\n");
        close(fd);
        exit(2);
    }
    
    close(fd);
    printf("Thread[%d] read from %d - %d\n", id, thread_start[id],thread_stop[id]);
    fprintf(stdout, "%s\n", data);

    // write to destination file
    fd = open (destination_file_path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        perror("open");
    }

    memset(&aio_w, 0, sizeof(aio_w));
    aio_w.aio_fildes = fd;
    aio_w.aio_buf = data;
    aio_w.aio_offset = thread_start[id]; // starting write byte
    aio_w.aio_nbytes = sizeof(data); // write until ofset + byte 

    if (aio_write(&aio_w) == -1) {
        printf("Error at aio_write(): %s\n", strerror(errno));
        close(fd);
        exit(2);
    }
    while (aio_error(&aio_w) == EINPROGRESS) { }
    err = aio_error(&aio_w);
    //ret = aio_return(&aio_w);

    if (err != 0) {
        printf ("Error at aio_error() : %s\n", strerror (err));
        close (fd);
        exit(2);
    }
    
    if (ret != sizeof(data)) {
        printf("Error at aio_return()\n");
        close(fd);
        exit(2);
    }
    
    close(fd);
    printf("Thread[%d] write %d - %d to new file\n", id, thread_start[id],thread_stop[id]);
    fprintf(stdout, "%s\n", data);
}
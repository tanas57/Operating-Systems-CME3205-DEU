#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h> 

#define TAXI 10 // # of taxi
#define STUDENT 100 // # of students
#define SEAT 4 // # of seats

#define TAXI_MOVING_TIME 15
#define STUDENT_WAIT_TIME 5 
#define GENERAL_WAIT_TIME 2

void* student(void *number);
void* taxi(void *number);
int   taxi_id();
int rnd(int param); // create random number

sem_t taxi_stop;  // taksi duragı
sem_t seats[TAXI];// her taksi için koltuklar
sem_t moved[TAXI];// taksinin hareket edip etmediğini kontrol etme
sem_t taxi_mutex; // taksi işlemlerinin sıralı olması için
sem_t std_mutex;  // öğrencilerin taksi işlemlerini beklemesi için

int movingCount = 0, inCar = -1, car = -1; // # of carried students

int main(){

    int i = 0;
    pthread_t students[STUDENT];    int StudentID[STUDENT]; // students threads
    pthread_t drivers[TAXI];        int TaxiID[TAXI];       // Taxi drivers threads

    for (i = 0; i < STUDENT; i++) StudentID[i] = i;
    for (i = 0; i < TAXI; i++) TaxiID[i] = i;

    // definition of semaphores
    sem_init(&taxi_stop, 0, TAXI);
    sem_init(&taxi_mutex, 0, 1);
    sem_init(&std_mutex, 0, 1);
    for(i=0;i<TAXI;i++) sem_init(&seats[i], 0, SEAT); // seat for each taxi
    for(i=0;i<TAXI;i++) sem_init(&moved[i], 0, 1); // seat for each taxi

    for(i=0;i<TAXI;i++) pthread_create(&drivers[i], NULL, taxi, (void *)&TaxiID[i]);

    for(i=0;i<STUDENT;i++) pthread_create(&students[i], NULL, student, (void *)&StudentID[i]);

    for(i=0;i<STUDENT;i++) pthread_join(students[i], NULL);
 
    for(i=0;i<TAXI;i++) pthread_join(drivers[i], NULL);


    printf("\n\n%d students moved through %d taxis.\n", STUDENT, TAXI);

    return 0;
}

void* taxi(void *number){
    
    int i = 0, value;
    while(1){

        sem_post(&std_mutex); // öğrenci threadinin taxiyi beklemesi, hemen peş peşe çalışmasını engellemeye çalıştım.
        
        sleep(GENERAL_WAIT_TIME);

        if(movingCount == STUDENT) break; // finish the taxi-thread if there is no student}
        sem_wait(&taxi_mutex); // taksiler print ederken yazılar karışıyordu fix

        for(i = 0; i < TAXI; i++){ // print taxi and seats
            sem_getvalue(&moved[i], &value);
            if(value == 1){ // Durakta yok ise yazdırmıyoruz
                sem_getvalue(&seats[i], &value);
                printf("T%d[%d] ", (i+1),(value%SEAT));
            }
        }
        printf("\n");
        if(movingCount>0) printf("\n\nToplam taşınan öğrenci : %d\n\n", movingCount);
        int id = *(int *)number;

        sem_getvalue(&seats[id], &value);
        if(value == 4) printf("Taxi[%d] is sleeping\n", (id+1));
        else if(value == 3) printf("Taxi[%d] : The last three students, let's get up!\n", (id+1));
        else if(value == 2) printf("Taxi[%d] : The last two students, let's get up!\n", (id+1));
        else if(value == 1) printf("Taxi[%d] : The last one students, let's get up!\n", (id+1));
        else if(value == 0) {

            int val = rnd(TAXI_MOVING_TIME);
            printf("\nTaxi[%d] was moved, and sleep(%d)\n\n", (id+1), val);
            movingCount += SEAT;
            sem_wait(&moved[id]); // taxi moved
           
            for(i = 0; i < SEAT; i++){
                sem_post(&seats[id]); // reset seats after taxi moved
            }
            
            sem_post(&taxi_mutex);
            sleep(val); // taxi moving time
            sem_post(&moved[id]); // taxi is coming back
            
        }

        sem_post(&taxi_mutex);
    }
}
void* student(void *number)
{
    sem_wait(&std_mutex);
    sleep(rnd(STUDENT_WAIT_TIME)); // waiting time
    int id = *(int *)number, i = 0;
    // student was comes in taxi stop
    sem_wait(&taxi_stop);
    printf("Student[%d] in taxi-stop\n", id);
    // select a taxi if there is
    // random taxi in taxi-stop
    int tID = -1; // taxi id
    tID = taxi_id();
    // wait taxi's seat
    sem_wait(&seats[tID]);
    printf("Student[%d] ==> Taxi[%d]\n", id, (tID+1));
    sem_post(&taxi_stop);
    // free taxi-stop
}

int taxi_id(){

    /*
    int i = 0, value, selected_taxi = -1, selected_taxiSeats = -1;
    for(i = 0; i < TAXI; i++){
        sem_getvalue(&moved[i], &value);
        if(value == 1){ // taxi durakta
            sem_getvalue(&seats[i], &value); // taksinin boş koltuk sayısı
            if((value % SEAT) > selected_taxiSeats){
                selected_taxi = i; selected_taxiSeats = value;
            }
        }
    }
    printf("deneme select %d", selected_taxi);
    if(selected_taxi == -1) taxi_id();
    return selected_taxi;
    */

    
    inCar++;
    if(inCar % SEAT == 0) car++;
    
    return car % TAXI;
}

int rnd(int param){
    srand(time(0));
    return (rand() % TAXI_MOVING_TIME);
}
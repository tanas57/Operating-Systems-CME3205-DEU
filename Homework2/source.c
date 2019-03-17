/*
	Operating Systems - Homework2 # Multi-Threaded Web Server
	Muhammet Tayyip Muslu
	2015510101
	via Ubuntu LTS 18.04
*/
/*
	The server only handles html and jpeg files
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define MYPORT 5757
#define MAX_THREADS 10
#define PATH "/home/muslu/Masaüstü/opsis/hw2/ht_docs" // do not put "/" at the end
#define BUF_SIZE 2048 // 2mib

sem_t responder;  // respond max_threads at same time with semafor

// for thread arguments
struct arg_struct {
    void* socket; // socket
    char file_name[100]; // filename
};
// html file threads
void* HtmlThread(void *arguments){
	sem_wait(&responder);
	
	FILE *html_file;
	char *send_buf;
	struct arg_struct *args = (struct arg_struct *)arguments;
	
	struct tm *ptr;
	time_t local;
	char times[30];

	char *file_data; 
	char send_buf_temp[BUF_SIZE];
	int file_size;

	int *connfd_thread = (int *)args->socket;
	static char buffer[BUF_SIZE + 1];
	char file[300];
	strcpy(file, PATH);
	strcat(file,"/");
	strcat(file, args->file_name);

	html_file = fopen(file, "r");				//open the html file
	if (html_file == NULL) {						//check to make sure fopen worked(if there was a file) if there was no file, return 404 message
		sprintf(buffer, "HTTP/1.0 404 Not Found\r\n\r\n");
        	send(*connfd_thread, buffer, strlen(buffer), 0);
	}
	else{
		time(&local);								//Get the current time and date
		ptr = localtime(&local);
		strftime(times, 30, "%a, %d %b %Y %X %Z", ptr);
		// read html file
		fseek(html_file, 0, SEEK_END);				//seek to the end of the tfile
		file_size = ftell(html_file);				//get the byte offset of the pointer(the size of the file)
		fseek(html_file, 0, SEEK_SET);				//seek back to the begining of the file
		file_data = (char *)malloc(file_size + 1);		//allocate memmory for the file data
		fread(file_data, 1, (file_size), html_file);	//read the file data into a string
		file_data[file_size] = '\0';

		sprintf(send_buf_temp, "HTTP/1.0 200 OK\nDate: %s\nContent Length: %d\nConnection: close\nContent-Type: text/html\n\n%s\n\n", times, file_size, file_data); //format and create string for output to client
		int a = strlen(send_buf_temp);		//get the length of the string we just created
		send_buf = (char *)malloc(a);		//allocate space for the reply
		strcpy(send_buf, send_buf_temp);	//copy to the correctly sized string
		send(*connfd_thread, send_buf, a, 0);	//send the info to the client
		free(file_data);
	}
	close(*connfd_thread);												//close the current connection
	fflush(stdout);														//make sure all output is printed
	
	sem_post(&responder);
	pthread_exit(NULL);
}
// jpeg file threads
void* JpegThread(void *arguments){
	// wait join semaphore
	sem_wait(&responder);
	
	FILE *img_file;
	char *send_buf;
	struct arg_struct *args = (struct arg_struct *)arguments;
	
	struct tm *ptr;
	time_t local;
	char times[30];

    int     j, buflen, len;
    long    idx, nbytes;
    char   *fstr;
    static char buffer[BUF_SIZE + 1];

	int *connfd_thread = (int *)args->socket;

	char file[300];
	strcpy(file, PATH);
	strcat(file,"/");
	strcat(file, args->file_name);

	img_file = fopen(file, "r");		//open the JPEG file
	
	if (img_file == NULL) {				//check to make sure fopen worked(if there was a file) if there was no file, return 404 message
		sprintf(buffer, "HTTP/1.0 404 Not Found\r\n\r\n");
        	send(*connfd_thread, buffer, strlen(buffer), 0);
	}
	else{
		sprintf(buffer, "HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\r\n");
		send(*connfd_thread, buffer, strlen(buffer), 0);

		while ((nbytes = fread(buffer, 1, BUF_SIZE, img_file)) > 0) send(*connfd_thread, buffer, nbytes, 0);
	}
	close(*connfd_thread);	//close the current connection
	fflush(stdout);			//make sure all output is printed
	
	sem_post(&responder);
	pthread_exit(NULL);
}
void connectionParser(void* p){
	char recv_buf[BUF_SIZE]; char file_name[100];
	int temp; int recv_len = 0;
	char* errormsg;
	int *new_sock = (int *)p; //type cast the argument

	temp = (recv (*new_sock, recv_buf, sizeof(recv_buf), 0));
	//check for error when receiving
	if (temp < 0) perror("Receive Error");

	temp = temp/sizeof(char);
	recv_buf[temp-2] = '\0'; // finish char array
	
	if ((strncmp(recv_buf, "GET ", 4)) != 0) {	//check to make sure that a GET request was made
		strcpy(errormsg, "Invalid Command Entered\nPlease Use The Format: GET <file_name>\n");
		send(*new_sock, errormsg, 69, 0);
		close(*new_sock);
	}
	else {
		recv_len = strlen(recv_buf); // request length
		int i;
		int j = 0;  //remove the forward slash if neccissary 

		for (i=4; i<recv_len ;i++, j++) {
			if ( (recv_buf[i] == '\0') || (recv_buf[i] == '\n') || (recv_buf[i] == ' ') ){ //If the end of the file path is reached, break.
				break;
			}
			else if ( recv_buf[i] == '/') { //do nothing except increment i to jump the forward slash
				--j;
			}
			else {
				file_name[j] = recv_buf[i];	//copy the file name character by character
			}
		}	
		file_name[j] = '\0'; //add a null terminator to the end of the file name string
	}

	int *connfd_thread = (int *)p;

	char * send_buf;
	struct tm *ptr;
	time_t local;
	char times[30];

	time(&local);								//Get the current time and date
	ptr = localtime(&local);
	strftime(times, 30, "%a, %d %b %Y %X %Z", ptr);

	pthread_t newThread;

	if (strstr(file_name, ".html") != NULL) {
		printf("Accept");
		struct arg_struct args;
		args.socket = p;
		strcpy(args.file_name, file_name);
		
		pthread_create(&newThread, NULL, HtmlThread, (void *)&args);		//create a thread and receive data
		pthread_join(newThread, NULL);

	}
	else if(strstr(file_name, ".jpeg") != NULL) {
		printf("Accept");
		struct arg_struct args;
		args.socket = p;
		strcpy(args.file_name, file_name);

		pthread_create(&newThread, NULL, JpegThread, (void *)&args);		//create a thread and receive data
		pthread_join(newThread, NULL);
	}
	else if(strstr(file_name, ".") == NULL && strlen(file_name) < 1){
		// go index page
		printf("Accept");
		struct arg_struct args;
		args.socket = p;
		strcpy(args.file_name, "index.html");

		pthread_create(&newThread, NULL, HtmlThread, (void *)&args);		//create a thread and receive data
		pthread_join(newThread, NULL);
	}
	else{
		printf(ANSI_COLOR_RED	"NOT Accept");
		static char buffer[BUF_SIZE + 1];
		sprintf(buffer, "HTTP/1.0 400 Bad Request\r\n\r\n");
    		send(*connfd_thread, buffer, strlen(buffer), 0);
		close(*connfd_thread);
	}
	printf(" [%s] requested at %s\n"	ANSI_COLOR_RESET, file_name, times);
}

int main() {
	int socket_desc; int test; int id; 
	struct sockaddr_in addr;
	
	if( (socket_desc = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		puts("Could not create socket");
		return 1;
	}

	memset(&addr.sin_zero, 0, sizeof(addr.sin_zero));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(MYPORT);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	if( bind(socket_desc, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		puts("Binding failed");
        return 1;
	}
	
	// semafore for respond 10 thread at same time
	sem_init(&responder, 0, MAX_THREADS);

	printf("\n------------------------SERVER LOGS------------------------\n");
	int acceptID; int value;
	while(1){		//loop to count the number of connections/threads
		if( listen(socket_desc, 3) != 0) {
			puts("Listen error");
			return 1;;
		}
		acceptID = accept(socket_desc, NULL, NULL);
		if (acceptID < 0) {
			perror("Accept Error");
			return (errno);
		}
		
		sem_getvalue(&responder, &value);
		if(value > 0){
			// determine whether is socked asked 
			connectionParser(&acceptID); 
		}
		else {
			puts("Server busy\n");
			static char buffer[BUF_SIZE + 1];
			int *connfd_thread = (int *)&acceptID;
			sprintf(buffer, "HTTP/1.0 406 Not Acceptable\r\n\r\n");
			send(*connfd_thread, buffer, strlen(buffer), 0);
			close(*connfd_thread);
		}
	}
	close(&socket_desc);	//close the socket
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define QUIT_STRING "exit"
#define SERVER_ADDR "140.159.153.159"
#define SERVER_PORT 502
#define DATA_LENGTH 256

/*linked list object */
typedef struct s_word_object word_object;
struct s_word_object{
	char *word;
	word_object *next;
};

/*list_head: Shared between two threads, must be accessed with list_lock*/
static word_object *list_head;
static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t list_data_ready = PTHREAD_COND_INITIALIZER;
static pthread_cond_t list_data_flush = PTHREAD_COND_INITIALIZER;


/* add object to list*/
static void add_to_list(char *word) {
	word_object *last_object, *tmp_object;

	char *tmp_string=strdup(word);
	tmp_object=malloc(sizeof(word_object));
	
	pthread_mutex_lock(&list_lock);/* lock*/

	if(list_head==NULL){
		last_object=tmp_object;    //allocate enough memory
		list_head = last_object;
		pthread_mutex_unlock(&list_lock);
	} else{	
		last_object= list_head;
		while(last_object->next){
					last_object=last_object->next;
		}

		last_object->next=tmp_object;
		last_object=last_object->next;


	}
	last_object ->word=tmp_string;
	last_object ->next=NULL;

	pthread_mutex_unlock(&list_lock);
	pthread_cond_signal(&list_data_ready);

}


 
static word_object *list_get_first(void){

	word_object *first_object;

	first_object=list_head;
	list_head=list_head->next;

	return first_object;

}

void *print_func(void *arg){
	word_object *current_object;
	fprintf(stderr, "Print thread starting\n");
	while(1){
	pthread_mutex_lock(&list_lock);
	
		while(list_head==NULL){
		pthread_cond_wait(&list_data_ready,&list_lock);
		}

		current_object=list_get_first();

		pthread_mutex_unlock(&list_lock);

		printf("Print Thread: %s\n",current_object ->word);
		free(current_object->word);
		free(current_object);

		pthread_cond_signal(&list_data_flush);
	}

	return arg;
}

static void list_flush(void){

pthread_mutex_lock(&list_lock);

	while(list_head != NULL){
	pthread_cond_signal(&list_data_ready);
	pthread_cond_wait(&list_data_flush, &list_lock);

	}
	pthread_mutex_unlock(&list_lock);

}

static void start_server(void){
	int socket_fd;
	struct sockaddr_in server_address;
	struct sockaddr_in client_address;
	socklen_t client_address_len;
	int want_quit=0;
	fd_set read_fds;
	int bytes;
	char data[DATA_LENGTH];
	pthread_t print_thread;

	fprintf(stderr,"starting server\n");
	
	pthread_create(&print_thread,NULL,print_func,NULL);

	socket_fd=socket(AF_INET,SOCK_DGRAM,0);

	memset(&server_address,0,sizeof(server_address));
	server_address.sin_family=AF_INET;
	server_address.sin_port=htons(SERVER_PORT);
	server_address.sin_addr.s_addr=INADDR_ANY;
	
	if(bind(socket_fd,(struct sockaddr *)&server_address,sizeof(server_address))<0){
		fprintf(stderr,"Bind Failed\n");
		exit(1);
	}

	FD_ZERO(&read_fds);
	FD_SET(socket_fd, &read_fds);
	while(!want_quit){
		/*wait until data has arrived*/
		if(select(socket_fd+1, &read_fds,NULL,NULL,NULL)<0){
			fprintf(stderr,"Slect failed\n");
			exit(1);

		}

		if(!FD_ISSET(socket_fd, &read_fds))continue;
		/*read input Data */
		bytes=recvfrom(socket_fd, data, sizeof(data),0,
				(struct sockaddr *)&client_address,&client_address_len);

		if(bytes<0){
			fprintf(stderr, "RECVFROM FAILED\n");
			exit(1);
		}
		/*Process Data*/
		if(!strcmp(data,QUIT_STRING))want_quit=1;
		else add_to_list(data);

	}
	list_flush();
}

static void start_client(count){
	int sock_fd;
	struct sockaddr_in addr;
	char input_word[DATA_LENGTH];

	fprintf(stderr,"Accepting %i input strings\n",count);

	if((sock_fd=socket(AF_INET,SOCK_DGRAM,0))<0){
		fprintf(stderr, "socket failed\n");
		exit(1);
	}

	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(SERVER_PORT);
	addr.sin_addr.s_addr=inet_addr(SERVER_ADDR);

if(connect(sock_fd, (struct sockaddr *)&addr, sizeof(addr))){
		fprintf(stderr,"connect Failed\n");
		exit(1);
	}
	
	while(scanf("%256s",input_word)!=EOF){
		if(send(sock_fd,input_word,strlen(input_word)+1,0)<0){
			fprintf(stderr,"Send Failed\n");
			exit(1);
		}
		if(!--count)break;
	}

}
int main(int argc, char **argv){
	int c;
	int option_index;
	int count=-1;
	int server=0;


	static struct option long_options[]={
	{"count", required_argument, 0, 'c'},
	{"server", no_argument,      0,  's' },
	{0,	0,		     0,  0}
	};

	while(1) {
		c= getopt_long(argc, argv, "c:", long_options, &option_index);
		if(c==-1){
		break;
		}
		switch(c) {
		  		case 'c':
				count=atoi(optarg);							                               	      break;
				case 's':
					server=1;
					break;
		}
		
	}


	if(server)start_server();
	else start_client(count);
	// start new thread for prinitng
	/*pthread_create(&print_thread,NULL, print_func,NULL);

	fprintf(stderr, "Accepting %i input strings\n", count);

	while(scanf("%256s", input_word) !=EOF) {
		add_to_list(input_word);
		if(!--count) break;

	}

	list_flush();*/
	
	return 0;

}

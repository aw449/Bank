#include "server.h"


account_t* p;
int shmid;
sem_t actionCycleSemaphore;
//static pthread_attr_t	user_attr;
static pthread_attr_t	kernel_attr;
sem_t *reado;
sem_t *writeo;
sem_t *welcome;


void organized_cleaning(int signale){
	shmctl(shmid, IPC_RMID, NULL);
	/*Note: shmid is a global var. Further, sigint is blocked while shared memory is being created, 
	so shmid is guaranteed to be a pointer to a valid block of shared memory*/
	sem_destroy(reado);
	sem_destroy(writeo);
	sem_destroy(welcome);
	raise(signale);
}

void ChildSigHandler(int signale){
	pid_t pid;
	int status;
	while( (pid = waitpid(-1,&status,WNOHANG)) == -1){
		sleep(1);
	}
	printf("Child process killed; PID: %d\n", (int)pid );
}
void print_handler(int signale, siginfo_t *ignore, void *ignore2){
	if ( signale == SIGALRM )
	{
		sem_post( &actionCycleSemaphore );
	}
}

void periodic_printing(){
	int i;
	//Print account info every 20 seconds. Raise a signal in Server main function.
	for(i = 0;i < 20; i++){
		printf("Account name: %s \n", p[i].name);
		printf("Balance: %d \n", p[i].balance);
		if(p[i].session){
			printf("In session: Yes");
		}
		else{
			printf("In session: No");
		}
	}
}
void *
periodic_action_cycle_thread( void * ignore )
{
	struct sigaction	action;
	struct itimerval	interval;


	pthread_detach( pthread_self() );			// Don't wait for me, Argentina ...
	action.sa_flags = SA_SIGINFO | SA_RESTART;
	action.sa_sigaction = print_handler;
	sigemptyset( &action.sa_mask );
	sigaction( SIGALRM, &action, 0 );			// invoke periodic_action_handler() when timer expires
	interval.it_interval.tv_sec = 20;
	interval.it_interval.tv_usec = 0;
	interval.it_value.tv_sec = 20;
	interval.it_value.tv_usec = 0;
	setitimer( ITIMER_REAL, &interval, 0 );			// every 20 seconds
	for ( ;; )
	{
		sem_wait( &actionCycleSemaphore );		// Block until posted

		sem_wait(reado);
		sem_wait(welcome);
		readers++;
		if(readers == 1){
			sem_wait(writeo);
		}
		sem_post(welcome);
		sem_post(reado);
		
		periodic_printing();
		
		sem_wait(welcome);
		readers--;
		if(readers == 0){
			sem_post(writeo);
		}
		sem_post(welcome);
		sched_yield();					// necessary?
	}
	return 0;//We'll never get here... haha
}


int socks(const char* port){
	int sd;
	int n;
	int sporkd;
	int pid;
	struct sockaddr_in addr;
	struct addrinfo	addrinfo;
	struct addrinfo *result;
	socklen_t addrlen;
	struct sockaddr_storage them;
	char message[256];
	int on = 1;

	struct sigaction action;

	action.sa_handler = ChildSigHandler;
	action.sa_flags = 0;
	sigemptyset (&action.sa_mask);
	sigaddset(&action.sa_mask,SIGCHLD);

	addrinfo.ai_flags = AI_PASSIVE;		// for bind()
	addrinfo.ai_family = AF_INET;		// IPv4 only
	addrinfo.ai_socktype = SOCK_STREAM;	// Want TCP/IP
	addrinfo.ai_protocol = 0;		// Any protocol
	addrinfo.ai_addrlen = 0;
	addrinfo.ai_addr = NULL;
	addrinfo.ai_canonname = NULL;
	addrinfo.ai_next = NULL;
	if ( getaddrinfo( 0, port, &addrinfo, &result ) != 0 ){
		fprintf( stderr, "\x1b[1;31mgetaddrinfo( %s ) failed errno is %s.  File %s line %d.\x1b[0m\n", CLIENT_PORT, strerror( errno ), __FILE__, __LINE__ );
		return -1;
	}
	else if ( errno = 0, (sd = socket( result->ai_family, result->ai_socktype, result->ai_protocol )) == -1 ){
				printf("socket failed");
				return -1;
	}

	else if (setsockopt( sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) ) == -1 )
	{
		printf( "setsockopt() failed");
		return 0;
	}
	else if (errno = 0, bind( sd, result->ai_addr, result->ai_addrlen ) == -1)
	{
		printf( "bind() failed in  line  errno \n" );
		close( sd );
		return 0;
	}
	else if ( listen( sd, 100 ) == -1 )
	{
		printf("listen failed");
		close( sd );
		return 0;
	}

	else
	{
		printf("%s\n","Server running...waiting for connections.");


		if(sigaction(SIGCHLD,&action,NULL)<0){
			perror("Sigaction failed");
			return 1;
		}
		while((sporkd = accept(sd,(struct sockaddr *)&them, &addrlen)) != -1){
			addrlen = sizeof(struct sockaddr_storage);


			pid = fork();
			if(pid < 0)
			{
				perror("Forking error.");
				close(sporkd); // Make sure this is ok
			}
			else if(pid == 0)//Is Child process
			{
				close(sd);

				//client_session(sporkd);
				exit(0);//This'll send a sigchld signal.
			}
			else//Is parent process
			{
				printf("Created child process %d\n", pid);
				close(sporkd);
			}
		}
	}
	return 0;

}

void sharingcaring(){
	/* Shared Memory Section*/

	int shmid;
	int i;
	account_t* temp = p;
	key_t key;

	int size;
	size = 20 * sizeof(account_t); // 20 accounts
	if(errno = 0, (key = ftok("testplan.txt",42)) == -1){
		printf("ftok failed; errno :  %s\n", strerror( errno ));
		exit( 1 );
	}
	else if(errno = 0, (shmid = shmget(key,size,0666 | IPC_CREAT | IPC_EXCL)) == -1){
		printf("shmget failed; errno :  %s\n", strerror( errno ));
		exit( 1 );
	}
	else if(errno = 0, (p = (account_t*) shmat(shmid,NULL,0)) == (void*) -1) {
		//No. p is an array of pointers to account_t's.
		//We need its size to be right. And we cast it as an account_t*,
		//so p[0] is the first account_t, p[1] is the second account_t, etc.
		printf( "shmat() failed; errno :  %s\n", strerror( errno ) );
		exit( 1 );
	}
	
	for(i = 0; i< 20 ; i++){
		if ((p = init()) != NULL){
			p++;
		}
		else
		{
			printf("Error Initializing shared memory Line: %d.",__LINE__);
		}
	}
	p = temp;

	//shared mem sucess.  Begin Server/Client Comunnications.
}

int main(){

	int sd;
	char *func = "server main";
	pthread_t		tid;
	struct sigaction memclean;
	if((reado = sem_open("reado",O_CREAT,0640,20)) == SEM_FAILED){
			printf("Read semaphore init fail.");
			exit(1);
		}
		else if((writeo = sem_open("writeo",O_CREAT,0640,1)) == SEM_FAILED){
			printf("Write semaphore init fail.");
			exit(1);
		}
		else if((welcome = sem_open("welcome",O_CREAT,0640,1)) == SEM_FAILED){
			printf("Welcome semaphore init fail.");
			exit(1);
		}
	readers = 0;
	//Semaphores at ready. No one reading. 
	memclean.sa_handler = organized_cleaning;
	sigemptyset (&memclean.sa_mask);
	sigaddset (&memclean.sa_mask, SIGINT);
	memclean.sa_flags = 0;
	sigaction(SIGINT, &memclean, NULL);
	sigprocmask(SIG_BLOCK, &memclean.sa_mask, NULL);
	//Note, there are forks in the server, but no threads... The forked processes are 2threaded. 
	
	//Shared Memory Setup
	sharingcaring();
	//Shared Memory Init

	
	//account data[20] = p;//Not sure if this is ok?
	
	

	sigprocmask(SIG_UNBLOCK, &memclean.sa_mask, NULL);


	if ( pthread_attr_init( &kernel_attr ) != 0 )
	{
		printf( "pthread_attr_init() failed in %s()\n", func );
		return 0;
	}
	else if ( pthread_attr_setscope( &kernel_attr, PTHREAD_SCOPE_SYSTEM ) != 0 )
	{
		printf( "pthread_attr_setscope() failed in %s() line %d\n", func, __LINE__ );
		return 0;
	}
	else if ( pthread_create( &tid, &kernel_attr, periodic_action_cycle_thread, 0 ) != 0 )
	{
		printf( "pthread_create() failed in %s()\n", func );
		return 0;
	}

	//Server-Client Service
	socks("54261");


	return 0;
}

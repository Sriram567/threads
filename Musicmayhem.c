#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define COLOR_RED     "\x1b[1;31m"
#define COLOR_GREEN   "\x1b[1;32m"
#define COLOR_YELLOW  "\x1b[1;33m"
#define COLOR_BLUE    "\x1b[1;34m"
#define COLOR_MAGENTA "\x1b[1;35m"
#define COLOR_CYAN    "\x1b[1;36m"
#define COLOR_RESET   "\x1b[0m"

int numacoustic;
int numelectric;
int numperformers;
int numtshirt;
int t1,t2;
int tseconds;
int numstages;

struct stage
{
	int stageid; // To access the stage 

	char personname[100]; // To know the name of musician performing on the stage

	int personid; // Id of person performing on the stage

	pthread_mutex_t stagelock;

	int status; // To know how many people are performing on the stage . Can be 0 , 1 ,2 (musician and a singer).

	int type; // Acoustic or electric 

	int stagetime; // Time of performance of player on that stage
};
// Performer stuct 
struct performer
{
	// Musician and Singer both are performers
	char name[100]; // Name of the performer

	pthread_t pid;

	pthread_mutex_t performerlock;

	int performerid; // Id to access

	char singername[100]; // Name of the singer, if a singer joined the performer

	int stage; // Stage number on which the performer is performing

	int tshirtstatus; // Collected or waiting

	int instrument; // Instrument that the performer plays

	int win; // For race condition . (Who got the semaphore first will win the race).

	int impatienttime; // t given

	int performancetime; // Time for which peformer performs

	int waittime; // Arrival time 
};

pthread_mutex_t tshirtlock[1000];
int tshirtarr[1000];
struct performer * performerarr[1000]; // To store the performer details so that others can access or update

struct stage * stagearr[1000]; // To store the stage details so that others can acess or update


sem_t tshirt; // semaphore for allowing only less than a particular number of students from taking tshirts simultaneously

sem_t acoustic; // Semaphore for acoustic stages. 

sem_t electric; // Semaphore for electric stages

sem_t duo; // Semaphore for dual performances


// Co-ordinator semaphore regulates that only some particular number of students can be collecting the tshirts
void * coordinator(void *args)
{
	struct performer * per= (struct performer * )args;
	sem_wait(&tshirt);
	if(strlen(per->name) != 0 && strcmp(per->name," ")!= 0)
	printf(COLOR_YELLOW "%s collecting tshirt\n"COLOR_RESET, per->name );
	sem_post(&tshirt);
	return NULL;

}
// maps the instrument to int values . since they are easy to handle
int assigner(char c)
{
	if(c == 'p')
	{
		return 1;
	}
	else if(c == 'g')
	{
		return 2;

	}
	else if( c == 'v')
	{
		return 3;
	}
	else if(c == 'b')
	{
		return 4;
	}
	else if(c == 's')
	{
		return 5;
	}
	
}
// To get back the instrument names from integers.
char reassign(int c)
{
	if(c == 1)
	{
		return 'p';
	}
	else if(c == 2)
	{
		return 'g';

	}
	else if( c == 3)
	{
		return 'v';
	}
	else if(c == 4)
	{
		return 'b';
	}
	else if(c == 5)
	{
		return 's';
	}

}
// performers searching for acoustic stage enters into this function
void * acousticfun(void * args)
{
	int yes = 0;
	struct performer * per = (struct performer *)args;

	struct timespec * tm = (struct timespec *)malloc(sizeof(struct timespec)); // to let them wait only for a certain since they are so impatient
	clock_gettime(CLOCK_REALTIME , tm);
	tm->tv_sec +=tseconds;
	//printf(" %s WAITING FOR %d secondds\n",per->name,tseconds);


	int rt = sem_timedwait(&acoustic, tm); // To ensure only a particular number of people are allowed at a instant of time

	pthread_mutex_lock(&per->performerlock); // Since all the instance threads of a performer may be try to change this simulatenously
	// To avoid that we employ a lock.
	if(per->win == 1 )
	{

		pthread_mutex_unlock(&per->performerlock);    // Decrease the semaphore value only if enters before max waiting time.
		if(rt!=-1)
		{
			sem_post(&acoustic);
		}
		return NULL;
	}
	else
	{
		per->win = 1;  // Change the value of win so that only one instance of performer can execute
	}
	pthread_mutex_unlock(&per->performerlock);

	if(rt == -1) // Sem_timedwait return -1 when the maxwaittime exceeds
	{
		printf("%s left beccause of impatience\n", per->name);
		return NULL;
	}
	else
	{

		for(int i=0;i<numstages;i++)
		{
			pthread_mutex_lock(&(stagearr[i]->stagelock)); // lock the stage since there is possibilty that two musicians take up same stage
			if(stagearr[i]->type == 0) // Check whether it is the required stage or not
			{
				if(stagearr[i]->status == 0)
				{
					strcpy(stagearr[i]->personname ,per->name);  // fill in attributes. TO let the stage know which performer is performing
					stagearr[i]->personid = per->performerid;  // Id of the performer performing
					stagearr[i]->status = 1;  // Set the status to one since only one is performing for now
					if(per->instrument!=5)    // Increase the available slots for a singer to join . don't increment if singer gives solo performance
					{
						sem_post(&duo);
					}
					per->stage = i;
					pthread_mutex_unlock(&(stagearr[i]->stagelock));
					break;
				}
			}
			pthread_mutex_unlock(&(stagearr[i]->stagelock));
		}

		int time =  t1 + rand()%(t2-t1+1); // calcuate the time of performance
		printf(COLOR_MAGENTA"%s is performing %c at the acoustic stage (id %d)for %d seconds\n"COLOR_RESET,per->name,((char)reassign(per->instrument)), per->stage ,time);

		sleep(time); // To simulate the time of performance.
		for(int kk = 0;kk<1;kk++)
		{
			pthread_mutex_lock(&((stagearr[per->stage])->stagelock));
			if((stagearr[per->stage])->status == 2) // check if any singer has joined him or not
			{

				yes = 155;
				stagearr[per->stage]->status = 0;
				pthread_mutex_unlock(&((stagearr[per->stage])->stagelock));
				sleep(2);
				break;
			}
			pthread_mutex_unlock(&((stagearr[per->stage])->stagelock));

			if((stagearr[per->stage])->status != 2)
			{

				int val = sem_trywait(&duo);
				if(val==-1) // Check if any singer has pastthrough the semaphore and waiting for the slot
				{
					sleep(1);
					yes = 155;
					sleep(2); // if yes decrement the value;
					pthread_mutex_lock(&((stagearr[per->stage])->stagelock));
					stagearr[per->stage]->status = 0;
					pthread_mutex_unlock(&((stagearr[per->stage])->stagelock));

				}
				else
				{		// reset the values and exit
					pthread_mutex_lock(&((stagearr[per->stage])->stagelock));
					stagearr[per->stage]->status = 0;
					pthread_mutex_unlock(&((stagearr[per->stage])->stagelock));
				}
			}		
		}

	}
	printf(COLOR_GREEN"%s performance at acoustic stage(id %d) finished\n"COLOR_RESET,per->name,per->stage);

	sem_post(&acoustic);
	if(yes == 155) // if a singer has joined musician make a thread for the singer to let him collect the t-shirt.
	{
		yes = 0;
		pthread_t singer;
		struct performer * ss = (struct performer *)(malloc(sizeof(struct performer)));
		strcpy(ss->name , per->singername);
		pthread_create(&singer, NULL , coordinator, (void *)ss);
	}
	coordinator((void *)per);
	return NULL;

} 

void * electricfun(void * args)
{
	int yes = 0;
	struct performer * per = (struct performer *)args;

	struct timespec * tm2 = (struct timespec *)malloc(sizeof(struct timespec));// to let them wait only for a certain time

	clock_gettime(CLOCK_REALTIME , tm2);
	tm2->tv_sec +=tseconds;
	//printf(" %s WAITING FOR %d secondds in electric\n",per->name,tseconds);



	int rt = sem_timedwait(&electric, tm2);// To ensure only a particular number of people are allowed at a instant of time


	pthread_mutex_lock(&per->performerlock); // Since all the instance threads of a performer may be try to change this simulatenously
	// To avoid that we employ a lock.

	if(per->win == 1)
	{

		pthread_mutex_unlock(&per->performerlock);// Increase the semaphore value only if enters before max waiting time.

		if(rt!=-1)
		{
			sem_post(&electric);
		}
		return NULL;
	}
	else
	{
		per->win = 1;		// Change the value of win so that only one instance of performer can execute

	}
	pthread_mutex_unlock(&per->performerlock);

	if(rt == -1) // Sem_timedwait return -1 when the maxwaittime exceeds
	{
		printf("%s left beccause of impatience\n", per->name);
		return NULL;
	}
	else
	{

		for(int i=0;i<numstages;i++)
		{
			pthread_mutex_lock(&(stagearr[i]->stagelock));// lock the stage since there is possibilty that two musicians take up same stage

			if(stagearr[i]->type == 1)
			{
				if(stagearr[i]->status == 0) // Check whether it is the required stage or not
				{
					strcpy(stagearr[i]->personname ,per->name);// fill in attributes. TO let the stage know which performer is performing

					stagearr[i]->personid = per->performerid;// fill in attributes. TO let the stage know which performer is performing

					stagearr[i]->status = 1;// Set the status to one since only one is performing for now

					//printf("%s got a stage of id %d\n", per->name , i);
					if(per->instrument != 5)// Increase the available slots for a singer to join . don't increment if singer gives solo performance
					{
						sem_post(&duo);
					}

					per->stage = i;
					pthread_mutex_unlock(&(stagearr[i]->stagelock));
					break;
				}
			}
			pthread_mutex_unlock(&(stagearr[i]->stagelock));
		}

		int time =  t1 + rand()%(t2-t1+1);// calcuate the time of performance

		printf(COLOR_BLUE"%s is performing %c at the electic stage (id %d) for %d seconds\n"COLOR_BLUE,per->name ,((char)(reassign(per->instrument))),per->stage,time);
		sleep(time);// To simulate the time of performance.
		for(int kk = 0;kk<1;kk++)
		{
			//printf("ENTERED\n");

			pthread_mutex_lock(&(stagearr[per->stage]->stagelock));
			if((stagearr[per->stage])->status == 2)// check if any singer has joined him or not
			{   	sleep(1);

				yes = 155;

				stagearr[per->stage]->status = 0;
				pthread_mutex_unlock(&(stagearr[per->stage]->stagelock));
				sleep(2);
				break;
			}
			pthread_mutex_unlock(&(stagearr[per->stage]->stagelock));
			//printf("EXITED once\n");
			if(stagearr[per->stage]->status != 2)
			{
				int val = sem_trywait(&duo);
				if(val==-1) // Check if any singer has pastthrough the semaphore and waiting for the slot
				{
					sleep(1);
					yes =  155;			//	printf("enterd wrongfastly\n");
					sleep(2);// if yes decrement the value;
					pthread_mutex_lock(&((stagearr[per->stage])->stagelock));
					stagearr[per->stage]->status = 0;
					pthread_mutex_unlock(&((stagearr[per->stage])->stagelock));
				}
				else
				{

					// reset the values and exit
					//("enterd corectly\n");
					pthread_mutex_lock(&((stagearr[per->stage])->stagelock));
					stagearr[per->stage]->status = 0;
					pthread_mutex_unlock(&((stagearr[per->stage])->stagelock));
					//		printf("exitedd corectly\n");

				}
			}		
		}

	}
	printf(COLOR_GREEN"%s performance at electric stage(id %d) finished\n"COLOR_RESET,per->name,per->stage);

	sem_post(&electric);
	if(yes==155)// if a singer has joined musician make a thread for the singer to let him collect the t-shirt.
	{
		yes = 0;
		pthread_t singer;
		struct performer * ss = (struct performer *)(malloc(sizeof(struct performer)));
		strcpy(ss->name , per->singername);
		pthread_create(&singer, NULL , coordinator, (void *)ss);
	}
	coordinator((void *)per);
	//printf("done\n");
	return NULL;
} 
// Function where singer search for a musician to join
void * duofun(void * args)
{
	int yes = 0;
	struct performer * per = (struct performer *)args; 

	struct timespec * tm3 = (struct timespec *)malloc(sizeof(struct timespec)); // to let the singer wait until the maximum waiting time;
	clock_gettime(CLOCK_REALTIME , tm3);
	tm3->tv_sec +=tseconds;


	int rt = sem_timedwait(&duo, tm3);// To ensure only a particular number of people are allowed at a instant of time

	pthread_mutex_lock(&per->performerlock); // Since all the instance threads of a performer may be try to change this simulatenously
	// To avoid that we employ a lock.
	if(per->win == 1)
	{
		pthread_mutex_unlock(&per->performerlock);
		if(rt!=-1)	// Increase the semaphore value only if enters before max waiting time.
		{
			sem_post(&duo);
		}
		return NULL;
	}
	else
	{
		per->win = 1;		// Change the value of win so that only one instance of performer can execute
	}
	pthread_mutex_unlock(&per->performerlock);

	if(rt == -1)// Sem_timedwait return -1 when the maxwaittime exceeds
	{
		printf("%s left beccause of impatience\n", per->name);
		return NULL;
	}
	else
	{

		for(int i=0;i<numstages;i++)
		{
			pthread_mutex_lock(&(stagearr[i]->stagelock)); // Since the stage array is accessed by many performer threads to avoid perfection issues , employ a lock


			if(stagearr[i]->status == 1) // A singer can enter if and only if there is a musician performing.
			{
				stagearr[i]->status = 2; // Change the status of the stage to represent that the singer has joined the stage.
				printf(COLOR_CYAN" Singer %s joined %s performance ont the stage %d, performance extended by 2 seconds\n"COLOR_RESET, per->name , stagearr[i]->personname, i);
				//sem_post(&duo);

				strcpy(performerarr[stagearr[i]->personid]->singername,per->name ); // To the performer know the name of the singer who joined
				// The perfomer use the name when creating the thread for singer to collect t-shirt
				per->stage =i;  // To record on which does the performance is going on
				pthread_mutex_unlock(&(stagearr[i]->stagelock));
				break;
			}

			pthread_mutex_unlock(&(stagearr[i]->stagelock));
		}


	}
	return NULL;
} 
// Function where the distrubutions take place
void * performerfun(void * args)
{

	struct performer * per = (struct performer *)args;
	//	printf("HELLO WORLD %d \n",per->instrument);
	sleep(per->waittime);  // To simulate the arrival time
	printf(COLOR_RED"%s %c arrived\n"COLOR_RESET,per->name,((char)reassign(per->instrument)));

	// DIVIDE THEM ACCORDING TO THEIR REQUIREMENTS
	// for those perform only on one stage, there is no need to have extra threads
	//For musicians we must have one instance waiting on the acoustic semaphore and another on the electric semaphore
	// For singers in additional we need to wait on the duo semaphore , since the singer can join solo performances.
	if(per->instrument != 5) 
	{
		if(per->instrument == 3) // Only on acoustic stage
		{
			acousticfun((void *)per);
		}
		else if(per->instrument == 4) // Only on electric stage
		{
			electricfun((void *)per);
		}
		else // On bothe stages
		{
			// Make two instances (simply two threads) and the one which past their respective semaphore will survive.
			pthread_t aa,bb,cc;
			pthread_create(&(aa), NULL, acousticfun, (void *) (per));
			pthread_create(&(bb), NULL, electricfun, (void *) (per));
			pthread_join(aa, NULL);
			pthread_join(bb, NULL);
		}

	}
	else if(per->instrument == 5) // For singer 
	{
		pthread_t aa, bb, cc ;
		pthread_create(&(aa), NULL, acousticfun, (void *) (per));
		pthread_create(&(bb), NULL, electricfun, (void *) (per));
		pthread_create(&(cc), NULL, duofun, (void *) (per));
		pthread_join(aa, NULL);
		pthread_join(bb, NULL);
		pthread_join(cc, NULL);
	}
	return NULL;

}
int main()
{
	char instr;
	//      char name[1000];
	printf("Please enter the value of ");
	printf("Number of performers: ");

	scanf("%d",&numperformers  );
	printf("Number of acoustic stages: ");

	scanf("%d", &numacoustic  );
	printf("Number of electric stages: ");
	scanf("%d", &numelectric  );
	printf("NUmber of co-ordinators");
	scanf("%d", &numtshirt );

printf("t1 : ");

	scanf("%d" , &t1);
printf("t2 : ");
	
scanf("%d" ,&t2);
printf("t : ");

	scanf("%d" , &tseconds);
printf("Enter info about the performers: ");

	//      printf("tseconds is t1 = %d, t2 = %d , t = %d\n",t1,t2,tseconds);

	sem_init(&acoustic, 0,numacoustic ); // Initialize acoustic semaphore to number of acoustic stages

	sem_init(&electric, 0 ,numelectric); // Initialize the electric semaphore to number of electric stages

	sem_init(&duo, 0, 0);  // Initialize the number of stages available for a singer to join to 0
	// Since initially there wouldn't be any stages with a musician so that singer can join

	sem_init(&tshirt , 0 , numtshirt); // Initialize the Tshirt co-ordinators semaphore to number of co-ordinators present.

	numstages = numacoustic + numelectric;

	int j = 0;

	/* 0 represent acoustic stage
	 * 1 represent electric stage
	 */
	// Status attribute of a stage the number of performers on the stage
	for( j=0;j<numacoustic;j++)  // Initialize the stages
	{
		stagearr[j] = (struct stage *)(malloc(sizeof(struct stage)));
		stagearr[j]->type = 0;
		stagearr[j]->status = 0;
	}

	for( j=j+0;j<numacoustic+numelectric;j++)
	{
		stagearr[j] = (struct stage *)(malloc(sizeof(struct stage)));
		stagearr[j]->type = 1;
		stagearr[j]->status = 0;
	}
	for(int jj =0;jj<numtshirt;jj++)
	{
		pthread_mutex_init(&tshirtlock[jj], NULL);
	}

	for(int i=0;i<numperformers;i++)
	{
		fflush(stdin);
		performerarr[i] = (struct performer *)(malloc(sizeof(struct performer)));

		scanf("%s ",performerarr[i]->name);
		//printf("name os %s\n", performerarr[i]->name);
		fflush(stdin);
		scanf("%c", &instr);
		//printf("char is %c\n",instr);
		performerarr[i]->performerid= i;
		performerarr[i]->win = 0;
		performerarr[i]->instrument = assigner(instr);
		//printf("char is %d\n", performerarr[i]->instrument);


		scanf("%d", &(performerarr[i]->waittime));

	}
	for(int i=0;i<numperformers;i++)
	{
		pthread_create(&(performerarr[i]->pid), NULL, performerfun, (void *) (performerarr[i]));

	}
	for(int i=0;i<numperformers;i++)
	{
		pthread_join((performerarr[i]->pid), NULL);
	}
	printf(COLOR_RED"FINISHED\n"COLOR_RESET);
} 


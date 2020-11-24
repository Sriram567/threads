#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

int numpharma;

int numzones;

int numstd;

int waitingstd = 0;

pthread_mutex_t waitstd;

#define COLOR_RED     "\x1b[1;31m"
#define COLOR_GREEN   "\x1b[1;32m"
#define COLOR_YELLOW  "\x1b[1;33m"
#define COLOR_BLUE    "\x1b[1;34m"
#define COLOR_MAGENTA "\x1b[1;35m"
#define COLOR_CYAN    "\x1b[1;36m"
#define COLOR_RESET   "\x1b[0m"

struct pharma
{
	int pharmaid;// Id to access 

	int numbatch; // Number of batches prepared 

	int numcompbatch; // Number of batches used up by the zones
	
	int numvaccines; // Nunber of vaccines in each batch

	int waittime; // Time of manufacture
	
	pthread_t pid; 
	
	pthread_mutex_t pharmalock;	
};

struct zone
{
	int zoneid; // Id to access

	int heldvaccines; // Number of vaccines in the zone
	
	int slots; // NUmber of slots declared
	
	int frompharma; // The pharmacy company id from which the vaccines are taken
	
	int invac; // whether in vaccination phase or not
	
	int compstd;
	
	pthread_t zid;
	
	pthread_mutex_t zonelock;
};
int cnttt = 0;
struct student
{
	int stid; // Id to access
	int status; // To know how many times tested negative for antibody test
	pthread_t sid;
	pthread_mutex_t studentlock;

};

struct pharma * pharmaarr[1000]; // To access student thread variables from other threads
struct student * studentarr[1000]; //  To access student thread variables from other threads
struct zone * zonearr[1000]; // To access zone thread variables from other threads

int status[1000];
float probarr[1000]; // To store the probabilities of vaccines success
// Function to let the company thread wait until all the batches are consumed
void pharmawait(struct pharma * reqcompany, int batches)
{

	while(1)
	{
		pthread_mutex_lock(&(reqcompany->pharmalock)); // Zone thread will be modifying the value of " num batches " 
		//so to synchronize the process lock is set up

		if(reqcompany->numcompbatch <= 0) // check if there are any batches left
		{
			printf(COLOR_RED"All the vaccines prepared by Pharmaceutical Company %d are emptied. Resuming production now.\n"COLOR_RESET,reqcompany->pharmaid);
			break;
		}
		pthread_mutex_unlock(&(reqcompany->pharmalock));
	}
	pthread_mutex_unlock(&(reqcompany->pharmalock));

}
// Pharmacy thread enters here
void * pharmafun(void * args)
{
	struct pharma * company = (struct pharma *)args;

	while(1)
	{
		int w  =  2+ rand()%4; //preparation time

		int batches =  1 + rand()%5; //Get the number of batches to prepare
 
		int vaccines = 10 + rand()%11; // Number of vaccines to prepare

		sleep(w);  // To simulate the manufacture time

		pthread_mutex_lock(&(company->pharmalock)); // lock the company since no zone should
							    // access before complete filling 		
		company->numbatch = batches;

		company->numcompbatch = batches;

		company->numvaccines = vaccines;
//		printf("pharacompany %d prepared %d batches each of %d vaccines\n",company->pharmaid, batches , vaccines);
		printf(COLOR_RED"Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %0.3f . \n Waiting for all the vaccines to be used to resume production\n"COLOR_RESET,company->pharmaid, vaccines, probarr[company->pharmaid]);
		pthread_mutex_unlock(&(company->pharmalock)); // Unlock it.

		pharmawait(company,batches); // Wait until all the batches are consumed
	}

	return NULL;
}

// Zone thread busy waits here

void * vaccinationzone(struct zone * vacinzone)
{ int has = 1;

	if(vacinzone->slots<=0)
	{
//		while(1)
//			printf("yes\n\n\n");
		return NULL;
	}

	while(1)
	{
		pthread_mutex_lock(&waitstd);
                int ws = waitingstd;
                pthread_mutex_unlock(&(waitstd));
		
                pthread_mutex_lock(&(vacinzone->zonelock));	// The invac are accessed by the students students should not access interleaving an update
                if( ws <= 0 && (vacinzone->compstd > 0) ) // Enters into vaccination zone since there are no students waiting
                {

			printf(COLOR_CYAN"vaccination zone %d entering vaccination phase\n"COLOR_RESET, vacinzone->zoneid);
			vacinzone->invac = 1;
                        vacinzone->heldvaccines -= vacinzone->compstd;
			// decrease the held vaccines 
			//by the number of students taken up the slots in that zone
                        pthread_mutex_unlock(&(vacinzone->zonelock));
                        break;
                }
                pthread_mutex_unlock(&(vacinzone->zonelock));


		pthread_mutex_lock(&(vacinzone->zonelock));
		if(vacinzone->compstd >= vacinzone->slots)
		{
			
                        printf(COLOR_CYAN"vaccination zone %d entering vaccination phase\n"COLOR_RESET, vacinzone->zoneid);
			vacinzone->invac = 1;

//			printf("vaccination zome of zone id %d aagamaha\n", vacinzone->zoneid);
			vacinzone->heldvaccines -= vacinzone->compstd; 	// decrease the held vaccines 
									//by the number of students taken up the slots in that zone
										
			//		printf("Slots completed\n");
			pthread_mutex_unlock(&(vacinzone->zonelock));
			break;
		}
		pthread_mutex_unlock(&(vacinzone->zonelock));


	}
}
// Thread representing the zones enters into this function
void * zonefun(void * args)
{
	struct zone * vaczone = (struct zone *)args;
        vaczone->invac = 0; //  invac = 0 says the zone is not in vaccination phase

	struct pharma * pcompany; // to iterate through the pharma companies
	while(1)
	{
		while(1)
		{
			int find = 0;
			for(int i=0;i<numpharma;i++)
			{

				pcompany = pharmaarr[i];
				pthread_mutex_lock(&(pcompany->pharmalock)); // The zone shouldn't take batches until every aspect is updated and
															// No two zones should access the same company simultaneously

				if(pcompany->numbatch>0)
				{
					printf(COLOR_GREEN"Pharmaceutical Company %d is delivering a vaccine batch to Vaccination Zone %d which has success probability %0.3f \n"COLOR_RESET, pcompany->pharmaid,  vaczone->zoneid, probarr[pcompany->pharmaid]);
					find = 1;
					pcompany->numbatch--;  // decreasing the number of batches 
					vaczone->heldvaccines = pcompany->numvaccines; // Number of vaccines taken
					vaczone->frompharma = pcompany->pharmaid; // To know from which pharmacy the vaccines are taken
//					printf("zone %d took %d vaccines from company %d and remainaing batches are %d\n", vaczone->zoneid, pcompany->numvaccines, pcompany->pharmaid,pcompany->numbatch);
					sleep(1);
					printf(COLOR_GREEN"Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n"COLOR_RESET, pcompany->pharmaid,  vaczone->zoneid);
					pthread_mutex_unlock(&(pcompany->pharmalock));
					break;
				}
				else
				{
					pthread_mutex_unlock(&(pcompany->pharmalock));
				}

			}
			if(find == 1)  // if the zone finds a batch of vaccines
 			{
				break;
			}
		}

		while(1)
		{
			if(vaczone->invac==0)
			{
				pthread_mutex_lock(&waitstd); // waiting students are updated student thread
											  // we shouldnot access a thread when we another thread is changing it
				int ws = waitingstd ;     
                                pthread_mutex_unlock(&waitstd);
				if(ws>0) // allocate the slots if and only if when there are waiting students
				{
				pthread_mutex_lock(&(vaczone->zonelock)); 
				 
				// Number of slots available are decreased student and student should take slots until evrything 
				// regarding the slots get updated.

				pthread_mutex_lock(&waitstd);
                                
				vaczone->slots = waitingstd;
                                
                                pthread_mutex_unlock(&waitstd);

				int numslots = 1 + rand()%8; // Generating random number of slots

				if(vaczone->slots > numslots)
                                {
                                        vaczone->slots =  numslots;
                                }

				// Number of slots should not be greater than the number of vaccines held.
				if(vaczone->slots > vaczone->heldvaccines)
				{
					vaczone->slots =  vaczone->heldvaccines;
				}

				if(vaczone->slots > 0)
				{
					printf(COLOR_MAGENTA"Vaccination zone %d is ready to vaccinate with  %d slots \n"COLOR_RESET, vaczone->zoneid, vaczone->slots);
				}

				pthread_mutex_unlock(&(vaczone->zonelock));

				//Enter into vaccination zone only if the slots declared are non - zero
				if(vaczone->slots>0)
				{
					vaccinationzone(vaczone);
				}
				/*
				   if(vaczone->heldvaccines <= 0)
				   {
				   pthread_mutex_lock(&(pcompany->pharmalock));	
				   pcompany->numcompbatch--;
				   pthread_mutex_unlock(&(pcompany->pharmalock));

				   printf("",vaczone->zoneid);

				   break;
				   }*/
				}
			}
			else
			{
                                pthread_mutex_lock(&(vaczone->zonelock));

				if(vaczone->compstd <= 0)
				{
					vaczone->invac = 0;
					vaczone->slots = 0;
//					printf("zone  %d  completed\n",vaczone->zoneid);
				}
                                pthread_mutex_unlock(&(vaczone->zonelock));

				if(vaczone->heldvaccines <= 0)
				{
					pthread_mutex_lock(&(pcompany->pharmalock));
					pcompany->numcompbatch--;
					pthread_mutex_unlock(&(pcompany->pharmalock));
					
                                        printf(COLOR_RED"Vaccination zone %d has run out of vaccines\n"COLOR_RESET,vaczone->zoneid);

					break;
				}

			}

		}
	}
	return NULL;
}

// Every student thread  enters here 
void * studentfun(void * args)
{
	int enter = 0;
	struct student * shishya = (struct student * )args; 
	status[shishya->stid] = 0; // He wasn't tested yet
	int waitsfor = 1 + rand()%5; //To simulate the situation that students enter in random times
	sleep(waitsfor);
	pthread_mutex_lock(&waitstd);
	waitingstd++;
	pthread_mutex_unlock(&waitstd);


	int i = 0;
	int got = 0; // variable to know whether he has found a slot or not.
	int have  = -3;
	while( status[shishya->stid] > -3 && status[shishya->stid] <= 0 )
	{
		struct zone * zoneofvac;
		int ctt = 0;
		while(got == 0)
		{

			if(ctt == 0)
			{
			if(status[shishya->stid]==0)
			{
				printf(COLOR_BLUE"Student %d has arrived for his 1st round of vaccination\n "COLOR_RESET,shishya->stid);
			}
			if(status[shishya->stid]==-1)
                        {
                                printf(COLOR_BLUE"Student %d has arrived for his 2nd round of vaccination\n "COLOR_RESET,shishya->stid);
                        }
			if(status[shishya->stid]==-2)
                        {
                                printf(COLOR_BLUE"Student %d has arrived for his 3rd round of vaccination\n "COLOR_RESET,shishya->stid);
                        }
			}

			got =0;
			i  = (i+1)%numzones; // to iterate through zones.
			if(ctt == 0) 
			printf(COLOR_BLUE"Student %d is waiting to be allocated a slot on a Vaccination Zone\n"COLOR_RESET, shishya->stid);
			zoneofvac = zonearr[i];
			pthread_mutex_lock(&(zoneofvac->zonelock)); // since a zone should not be accessed by two student threads simultaneously

			if( (zoneofvac->compstd < zoneofvac->slots) && (zoneofvac->invac == 0) ) // For the student to register the vaccination zone should be not in vaccination phase	and should have atleast one slot left
			{
			
				got = 1; // To break from the loop
				have =  zoneofvac->slots;
				zoneofvac->compstd++; // Increase the number of registered students of s zone
				pthread_mutex_lock(&waitstd); // Global lock for waiting students
                                waitingstd--;
                                pthread_mutex_unlock(&waitstd);
				printf(COLOR_BLUE"Student %d assigned a slot on the Vaccination Zone  %d and waiting to be vaccinated\n "COLOR_RESET,shishya->stid , zoneofvac->zoneid);
			}

			pthread_mutex_unlock(&(zoneofvac->zonelock));
		/*	if(got)
			{

				pthread_mutex_lock(&waitstd);
				waitingstd--;
				pthread_mutex_unlock(&waitstd);
			}*/
			ctt++;
		}
		ctt = 0;
		got = 0;

		while(1)
		{
			pthread_mutex_lock(&(zoneofvac->zonelock));
			int zoc = zoneofvac->invac;
			pthread_mutex_unlock(&(zoneofvac->zonelock));
			//After the student picks up a slot he goes and busy waits until the zone (from where he picked up the slot ) enters into vaccination phase.
			if(zoc==1) 
			{

				pthread_mutex_lock(&(zoneofvac->zonelock)); // To signal the zone that student got vaccinated.
				zoneofvac->compstd--;
				pthread_mutex_unlock(&(zoneofvac->zonelock));
				printf(COLOR_YELLOW"Student %d vaccination Zone %d is getting vaccinated\n"COLOR_RESET,shishya->stid, zoneofvac->frompharma);
				sleep(2);
				printf(COLOR_YELLOW"Student  %d on Vaccination Zone %d has been vaccinated which has success probability %0.3f \n"COLOR_RESET, shishya->stid , zoneofvac->zoneid , probarr[zoneofvac->frompharma]);

				int a = (int)(probarr[zoneofvac->frompharma]*1000); // To simulate randomness and probability
				if((rand()%1000) > a ) // tests Negative 
				{

					                                        printf("Student %d has tested negative for antibodies.\n",shishya->stid);

					status[shishya->stid]--; 
					pthread_mutex_lock(&waitstd);
					waitingstd++;			// He again has to wait for another slot.
					pthread_mutex_unlock(&waitstd);


				}
				else // tests positve
				{
					printf(COLOR_YELLOW"Student %d has tested positive for antibodies.\n"COLOR_RESET,shishya->stid);
					status[shishya->stid] = 1;    // He tests positive and can happily enter the college.
				}
				if(status[shishya->stid] == -3) // if a student test negative for 3 times. 
				{

					pthread_mutex_lock(&waitstd);
					waitingstd--;
					pthread_mutex_unlock(&waitstd);
					printf(COLOR_RED"Student %d is sent home . He is sad :(\n"COLOR_RESET, shishya->stid);
					cnttt++;
				}

				break;
			}
		}
	}

	return NULL;
}

int main()
{
	printf("Enter number of pharmacetucals :");

	scanf("%d",&numpharma);

	printf("Enter number of vaccination zones :");

	scanf("%d",&numzones);

	printf("Enter number of std :");

	scanf("%d",&numstd);

	printf("Enter the probability array :");

	for(int i=0;i<numpharma;i++)
	{
		scanf("%f", &probarr[i]);
	//	probarr[i]=0.5;
	}

	//I am creating a thread for every pharmacy
	
	// I am initialising the number of batch to 0
	
	// Here the "pharmafun" function allocates random number of batches for each production of company
	

	for(int i=0;i<numpharma;i++)
	{
		pharmaarr[i] = (struct pharma * ) malloc(sizeof(struct pharma));

		pharmaarr[i]->pharmaid = i;

		pharmaarr[i]->numbatch = 0;

		pthread_create(&(pharmaarr[i]->pid) ,NULL, pharmafun , (void *)pharmaarr[i] );
	}
	

	/*
		Seperate thread for each Vaccinezone
	*/

	for(int i=0;i<numzones;i++)
	{
		zonearr[i] = (struct zone * ) malloc(sizeof(struct zone));

		zonearr[i]->zoneid = i;

		zonearr[i]->compstd = 0;

		pthread_create(&(zonearr[i]->zid) ,NULL, zonefun , (void *)zonearr[i] );
	}
	sleep(1);

	//A seperate thread for each Student
	
	for(int i=0;i<numstd;i++)
	{
		studentarr[i] = (struct student * ) malloc(sizeof(struct student));

		studentarr[i]->stid = i;

		pthread_create(&(studentarr[i]->sid) ,NULL, studentfun , (void *)studentarr[i] );
	}

// A student exits if he tests positive for antibodies test or if he tests negative three times


	for(int i=0;i<numstd;i++)
	{
		pthread_join(studentarr[i]->sid, NULL);
	}
	printf(COLOR_RED"\n----------------SIMULATION OVER-------------------\n"COLOR_RED);
	return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>


#define FILENAME "random_numbers"


struct prime{
	int data;
	struct prime *next;
};

sem_t s;

struct prime *head=NULL;


FILE *fp;
int size,nlines,ncores,minPrime=-1,maxPrime=-1;

char *file_cont;
char **numbers;

void *find_prime(void *arg);

int
main(int argc,char **argv)
{

	char n;
	
	//used for open()
	int 	fd1,fd2,fd3;
	//check return values from system calls
	int	ret;
	//file info
	struct stat status;
	
	//semaphore
	sem_init(&s,0,1);

	//open file with numbers
	fp=fopen(FILENAME,"r");
	if(!fp){
		fprintf(stderr,"---Failed to open file!\n\n");
		exit(1);
	}
	ret=stat(FILENAME,&status);
	if( ret < 0 ){
		fprintf(stderr,"---Failed to get file size!\n\n");
		fclose(fp);
		exit(1);
	}
	size=status.st_size;

	printf("\n[+++] File size is : %d bytes\n",size);


	/*
	 * Get number of cores of PC 
	 * Bash script will calculate that for us and 
	 * Store that number  in file called
	 * num_of_cores
	 * open it and get number of cores of machine
	 * on the end delete file num_of_cores-no longer needed
	 */	
	system("./get_num_of_cores.sh");

	fd2=open("num_of_cores",O_RDONLY);
	read(fd2, &n,1);
	//convert char to int
	ncores= n -'0';
	printf("[+++] Number of cores that you have: %d\n",ncores);

	//get rid of file
	close(fd2);
	system("rm num_of_cores");


	//get line numbers
	char command[64];
	char get_lines[128];
	sprintf(command,"sed -n '$=' %s > num_of_lines",FILENAME);
	system(command);
	fd2=open("num_of_lines",O_RDONLY);
	if(fd2 < 0 ){
		fprintf(stderr,"---Failed to get number of lines in random numbers file!\n\n");
		close(fd2);
		exit(1);
	}
	read(fd2, get_lines, sizeof(get_lines));
	nlines=atoi(get_lines);
	printf("[+++] Number of lines in file: %d\n",nlines);
	close(fd2);

	system("rm num_of_lines");


	//read whole file
	file_cont=malloc(size);
	fread(file_cont,size,1,fp);
	//all read ???
	if(strlen(file_cont)!=size){
		fprintf(stderr,"---Reading file failed!\n\n");
		perror(NULL);
		fclose(fp);
		free(file_cont);
		exit(1);
	}

	/*
	 * 1-Remove '\n' with '\0' 	
	 * 2-Store beginning of each number in numbers
	*/	
	numbers=malloc(nlines*sizeof(char*));
	int k=1;
	numbers[0]=&file_cont[0];
	for(int i=1;i<size;i++){
		if( file_cont[i]=='\n'){
			file_cont[i]='\0';
			numbers[k]=&file_cont[i+1];
			k++;
		}
	}
	//everything is ready
	pthread_t 	t[ncores];
	//one element-one argument for one thread
	int		argpos[ncores];
	for(int i=0;i<ncores;i++)
		argpos[i]=i;
	for(int i=0; i<ncores;i++)
		pthread_create(&t[i],NULL,find_prime,(void*)&argpos[i]);
	for(int i=0;i<ncores;i++)
		pthread_join(t[i],NULL);


	struct prime *temp;
	int count=0;
	if(head!=NULL){
		printf("\n\n\nPrime numbres are: \n");
		temp=head;
		while(temp!=NULL){
			printf("%d\n",temp->data);
			count++;
			temp=temp->next;
		}
	}else
		printf("We dont have prime numbers in file!\n");

	printf("\n\nStats: \n\n");

	printf("[+++] Number of prime numbers in file:\t\t[%d]\n",count);
	printf("[+++] Biggest prime number:\t\t\t[%d]\n",maxPrime);
	printf("[+++] Smallest prime number:\t\t\t[%d]",minPrime);

	free(numbers);
	free(file_cont);
	
	return 0;
}
void *find_prime(void *arg)
{
	struct prime *new,*ptr;
	int pos=*((int *)arg);
	int num,j,test;
	for(pos;pos<nlines;pos=pos+ncores){	

		num=atoi(numbers[pos]);
		if( (num & 1 ) && ((num%5)!=0)){
			//seek further
			test=1;
			for(j=3;j*j<=num;j+=2){
				if( (num%j) == 0){
					test=0;
					break;
				}
			}	
			if(test){
				

				/*
				 * Prime number is found.
				 * This is critical section for all our threads.
				 * We should prevent other threads from this part of code
				 * While we are here.	
				*/
				sem_wait(&s);

				new=(struct prime *)malloc(sizeof(struct prime));
				new->data=num;
				new->next=NULL;

				//maybe this num is new min or  max
				if(maxPrime < num)
					maxPrime=num;
				if(minPrime > num || minPrime==-1)
					minPrime=num;
				
				//add new number in linked list
				if(head==NULL)
					head=new;
				else{
					ptr=head;
					while(ptr->next!=NULL)
						ptr=ptr->next;
					ptr->next=new;
				}
				//Done with critical section
				sem_post(&s);

			}
		}else
			continue;

	}
	pthread_exit(NULL);
}
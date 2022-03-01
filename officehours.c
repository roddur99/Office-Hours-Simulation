#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1
#define CLASSC 2
#define CLASSD 3
#define CLASSE 4

/*synchronization variables here */

sem_t semA; //semaphore for student A thread
sem_t semB; //semaphore for student B thread

pthread_mutex_t profLock; //mutex to guard critical regions involving students entering and leaving the office


/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;

static int WaitingA; //students from class A waiting to get in the office
static int WaitingB; //students from class B waiting to get in the office

static int consec_A; //consecutive students from class A answered by prof
static int consec_B; //consecutive students from class B answered by prof

static int inviteA; //flag that turns 1 when student from class A receives permission to get in
static int inviteB; //flag that turns 1 when student from class B receives permission to get in

typedef struct 
{
	int arrival_time;  // time between the arrival of this student and the previous student
	int question_time; // time the student needs to spend with the professor
	int student_id;
	int class;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
	students_in_office = 0;
	classa_inoffice = 0;
	classb_inoffice = 0;
	students_since_break = 0;
	
	consec_A = 0;
	consec_B = 0;
  
	inviteA = 0;
	inviteB = 0;

	/* Initialize your synchronization variables (and 
	* other variables you might use) here
	*/
	sem_init(&semA, 0, 0);
	sem_init(&semB, 0, 0);
	
	pthread_mutex_init(&profLock, NULL);

	/* Read in the data file and initialize the student array */
	FILE *fp;

	if((fp=fopen(filename, "r")) == NULL) 
	{
		printf("Cannot open input file %s for reading.\n", filename);
		exit(1);
	}

	int i = 0;
	while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && 
			i < MAX_STUDENTS ) 
	{
		i++;
	}

	fclose(fp);
	return i;
}

/* Code executed by professor to simulate taking a break 
 * You do not need to add anything here.  
 */
static void take_break() 
{
	printf("The professor is taking a break now.\n");
	sleep(5);
	assert( students_in_office == 0 );
	students_since_break = 0;
}

/* Code for the professor thread. This is fully implemented except for synchronization
 * with the students.  See the comments within the function for details.
 */
void *professorthread(void *junk) 
{
	printf("The professor arrived and is starting his office hours\n");

	/* Loop while waiting for students to arrive. */
	while (1) 
	{  
		pthread_mutex_lock(&profLock); //lock mutex to prevent race conditions
	
		//checking if professor needs to take a break
		if(students_since_break == professor_LIMIT && students_in_office == 0)
		{	
			take_break();
		}
	
		//checking whether students from class A are waiting to be entered and whether they are compatible to enter
		if(students_in_office < MAX_SEATS && classb_inoffice == 0 && (consec_A < 5 || WaitingB == 0)
				&& WaitingA > 0 && students_since_break != professor_LIMIT && inviteB == 0 && inviteA == 0)
		{
			WaitingA--; //reduce waiting line for students from class A
			inviteA = 1; //flag that a student from class A has been granted permission to enter
			sem_post(&semA); //student from class A is released, they are free to leave
			consec_B = 0; //class B's consecutive streak is cut
			consec_A++; // prof has seen a student from class A, so a streak is initiated
		}
	
		//checking whether students from class B are waiting to be entered and whether they are compatible to enter
		if(students_in_office < MAX_SEATS && classa_inoffice == 0 && (consec_B < 5 || WaitingA == 0)  
				&& WaitingB > 0 && students_since_break != professor_LIMIT && inviteA == 0 && inviteB == 0)
		{
			WaitingB--; // reduce waiting line for students from class B
			inviteB = 1; //flag that a student from B has been granted permission to enter
			sem_post(&semB); //student from class B is released, they are free to leave
			consec_A = 0; //class A's consecutive streak is cut
			consec_B++; // prof has seen a student from class B, so a streak is initiated
		}
	
		pthread_mutex_unlock(&profLock);
	}
	pthread_exit(NULL);
}


/* Code executed by a class A student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classa_enter() 
{
	pthread_mutex_lock(&profLock); //same mutex used because professor thread shares some variables
	
	WaitingA++; //student added to waiting line
	
	pthread_mutex_unlock(&profLock);
	
	sem_wait(&semA); //student is waited until prof thread grants permission to enter
	
	pthread_mutex_lock(&profLock);
	
	students_since_break += 1;
	students_in_office += 1;
	classa_inoffice += 1;
	inviteA = 0;
	
	pthread_mutex_unlock(&profLock);
}

/* Code executed by a class B student to enter the office.
 * You have to implement this.  Do not delete the assert() statements,
 * but feel free to add your own.
 */
void classb_enter() 
{
	pthread_mutex_lock(&profLock); //same mutex used because professor thread shares some variables
	
	WaitingB++; //student added to waiting line
	
	pthread_mutex_unlock(&profLock);
	
	sem_wait(&semB); //student is waited until prof thread grants permission to enter
	
	pthread_mutex_lock(&profLock);

	students_since_break += 1;
	students_in_office += 1;
	classb_inoffice += 1;
	inviteB = 0;
	
	pthread_mutex_unlock(&profLock);
}

/* Code executed by a student to simulate the time he spends in the office asking questions
 * You do not need to add anything here.  
 */
static void ask_questions(int t) 
{
	sleep(t);
}


/* Code executed by a class A student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classa_leave() 
{
	pthread_mutex_lock(&profLock); //same mutex used because professor thread shares some variables
  
	students_in_office -= 1;
	classa_inoffice -= 1;
	
	pthread_mutex_unlock(&profLock);

}

/* Code executed by a class B student when leaving the office.
 * You need to implement this.  Do not delete the assert() statements,
 * but feel free to add as many of your own as you like.
 */
static void classb_leave() 
{
	pthread_mutex_lock(&profLock); //same mutex used because professor thread shares some variables
	
	students_in_office -= 1;
	classb_inoffice -= 1;
	
	pthread_mutex_unlock(&profLock);
}

/* Main code for class A student threads.  
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classa_student(void *si) 
{
	student_info *s_info = (student_info*)si;
	sleep(s_info->arrival_time); //sleep thread till arrival

	/* enter office */
	classa_enter();

	printf("Student %d from class A enters the office\n", s_info->student_id);
	
	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classb_inoffice == 0 );
  
	/* ask questions  --- do not make changes to the 3 lines below*/
	printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
	ask_questions(s_info->question_time);
	printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);

	/* leave office */
	classa_leave();  

	printf("Student %d from class A leaves the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

	pthread_exit(NULL);
}

/* Main code for class B student threads.
 * You do not need to change anything here, but you can add
 * debug statements to help you during development/debugging.
 */
void* classb_student(void *si) 
{
	student_info *s_info = (student_info*)si;
	sleep(s_info->arrival_time); //sleep thread till arrival

	/* enter office */
	classb_enter();

	printf("Student %d from class B enters the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
	assert(classa_inoffice == 0 );

	printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
	ask_questions(s_info->question_time);
	printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);

	/* leave office */
	classb_leave();        

	printf("Student %d from class B leaves the office\n", s_info->student_id);

	assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
	assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
	assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

	pthread_exit(NULL);
}

/* Main function sets up simulation and prints report
 * at the end.
 * GUID: 355F4066-DA3E-4F74-9656-EF8097FBC985
 */
int main(int nargs, char **args) 
{
	int i;
	int result;
	int student_type;
	int num_students;
	void *status;
	pthread_t professor_tid;
	pthread_t student_tid[MAX_STUDENTS];
	student_info s_info[MAX_STUDENTS];

	if (nargs != 2) 
	{
		printf("Usage: officehour <name of inputfile>\n");
		return EINVAL;
	}

	num_students = initialize(s_info, args[1]);
	if (num_students > MAX_STUDENTS || num_students <= 0) 
	{
		printf("Error:  Bad number of student threads. "
			   "Maybe there was a problem with your input file?\n");
		return 1;
	}

	printf("Starting officehour simulation with %d students ...\n",
			num_students);

	result = pthread_create(&professor_tid, NULL, professorthread, NULL);

	if (result) 
	{
		printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
		exit(1);
	}

	for (i=0; i < num_students; i++) 
	{

		s_info[i].student_id = i;
		//sleep(s_info[i].arrival_time); -- commented this out because I chose to make the threads sleep till arrival time after they are created
                
		student_type = random() % 2;

		if (s_info[i].class == CLASSA)
		{
			result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
		}
		else // student_type == CLASSB
		{
			result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
		}

		if (result) 
		{
			printf("officehour: thread_fork failed for student %d: %s\n", 
					i, strerror(result));
			exit(1);
		}
	}

	/* wait for all student threads to finish */
	for (i = 0; i < num_students; i++) 
	{
		pthread_join(student_tid[i], &status);
	}

	/* tell the professor to finish. */
	pthread_cancel(professor_tid);

	printf("Office hour simulation done.\n");

	return 0;
}




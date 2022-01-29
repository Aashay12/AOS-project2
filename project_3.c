#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "utility.h"

#define seller_h 1
#define seller_m 3
#define seller_l 6
#define total_seller (seller_h + seller_m + seller_l)
#define row 10
#define columns 10
#define total_run_time 60

// Seller Structure
typedef struct s_struct
{
	char s_number;
	char s_type;
	queue *s_queue;

} seller;

// Customer Structure
typedef struct c_struct
{
	char c_number;
	int arrival_time;
	int response_time;
	int turnaround_time;

} customer;

int present_time;
int N = 0;
char available_seats_matrix[row][columns][5];
//Response times for H,L,M respectively
int rt_h;
int rt_l;
int rt_m;
//Turnaround times for H,L,M respectively
int tat_h;
int tat_l;
int tat_m;

//Thread variables
pthread_t seller_t[total_seller];
pthread_mutex_t thCount_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_waiting_for_clock_tick_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reservation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_completion_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_cond = PTHREAD_COND_INITIALIZER;

//Function declarations
void queueDisplay(queue *q);
void initialize_sThreads(pthread_t *thread, char s_type, int no_of_sellers);
void wait_for_thread_to_serve_present_time();
void revive_sThreads();
void *sell(void *);
queue *generate_custQueue(int);
int comapre_arrivalTime(void *value1, void *value2);
int findAvailableSeat(char s_type);

int thCount = 0;
int clockWaitThread = 0;
int thPresent = 0;
int verbose = 0;
int main(int argc, char **argv)
{

	if (argc == 2)
	{
		N = atoi(argv[1]);
	}

	//Initaizlize the theater layout with all seats empty
	for (int i = 0; i < row; i++)
	{
		for (int j = 0; j < columns; j++)
		{
			strncpy(available_seats_matrix[i][j], "-", 1);
		}
	}

	//thread creation
	//Set parameters including the Number, type and generate the queue.
	initialize_sThreads(seller_t, 'H', seller_h);
	initialize_sThreads(seller_t + seller_h, 'M', seller_m);
	initialize_sThreads(seller_t + seller_h + seller_m, 'L', seller_l);

	//Wait for threads and synchronized clock tick.
	while (1)
	{
		pthread_mutex_lock(&thCount_mutex);
		if (thCount == 0)
		{
			pthread_mutex_unlock(&thCount_mutex);
			break;
		}
		pthread_mutex_unlock(&thCount_mutex);
	}

	//Simulate for each minute
	printf("\nSimulation:-");
	printf("\n----------------------------------------------------------------------------------------------------------\n");
	printf("Time 	Seller      	Activity				Response Time 		Turnaround Time");
	printf("\n----------------------------------------------------------------------------------------------------------\n");
	clockWaitThread = 0;
	//For first tick
	revive_sThreads();

	do
	{
		//Wake up all thread
		wait_for_thread_to_serve_present_time();
		present_time = present_time + 1;
		revive_sThreads();
		//Wait for thread completion
	} while (present_time < total_run_time);

	//Wakeup all thread so that no more thread keep waiting for clock Tick in limbo
	revive_sThreads();

	while (thPresent)
		;

	printf("----------------------------------------------------------------------------------------------------------\n");
	//Display concert chart
	printf("\n\n");
	printf("----------------------------------------------------------------------------------------------------------\n");
	printf("Tickets after sale period ended");
	printf("\n----------------------------------------------------------------------------------------------------------\n\n");

	//Count the number of customers - High, Medium and Low respectively.
	int h_cust = 0, m_cust = 0, l_cust = 0;
	for (int r = 0; r < row; r++)
	{
		for (int c = 0; c < columns; c++)
		{
			if (c != 0)
				printf("\t");
			printf("%5s", available_seats_matrix[r][c]);
			if (available_seats_matrix[r][c][0] == 'H')
				h_cust++;
			if (available_seats_matrix[r][c][0] == 'M')
				m_cust++;
			if (available_seats_matrix[r][c][0] == 'L')
				l_cust++;
		}
		printf("\n");
	}

	printf("\n----------------------------------------------------------------------------------------------------------\n\n");

	printf("\n----------------------------------------------------------------------------------------------------------\n");
	printf("Multi-threaded Ticket Sellers");
	printf("\nThe number of customers entered are: %02d\n", N);
	printf("----------------------------------------------------------------------------------------------------------\n\n");

	//Print the number of customers in each category
	printf(" ------------------------------------------------------------------\n");
	printf("||%3c || No of Customers || BookedSeat  || Returned || Throughput||\n", ' ');
	printf(" ------------------------------------------------------------------\n");
	printf("||%3c || %15d || %8d || %8d || %.2f         ||\n", 'H', seller_h * N, h_cust, (seller_h * N) - h_cust, (h_cust / 60.0));
	printf("||%3c || %15d || %8d || %8d || %.2f         ||\n", 'M', seller_m * N, m_cust, (seller_m * N) - m_cust, (m_cust / 60.0));
	printf("||%3c || %15d || %8d || %8d || %.2f         ||\n", 'L', seller_l * N, l_cust, (seller_l * N) - l_cust, (l_cust / 60.0));
	printf(" -----------------------------------------------------------------\n");
	printf("\n");

	//Print the metrics for each category
	printf(" ----------------------------------------------------\n");
	printf("||%3c   || Avg response Time || Avg turnaround time||\n", ' ');
	printf(" ----------------------------------------------------\n");
	printf("|| %3c  || %3f          || %.2f 		   ||\n", 'H', rt_h / (N * 1.0), tat_h / (N * 1.0));
	printf("|| %3c  || %3f          || %.2f 		   ||\n", 'L', rt_l / (6.0 * N), tat_l / (6.0 * N));
	printf("|| %3c  || %3f          || %.2f 		   ||\n", 'M', rt_m / (3.0 * N), tat_m / (3.0 * N));
	printf(" ----------------------------------------------------\n");
	return 0;
}

//Initialize all threads Params - (seller number, type and seller queue)
void initialize_sThreads(pthread_t *thread, char s_type, int no_of_sellers)
{
	//Create all threads
	for (int t_no = 0; t_no < no_of_sellers; t_no++)
	{
		seller *s_Param = (seller *)malloc(sizeof(seller));
		s_Param->s_number = t_no;
		s_Param->s_type = s_type;
		s_Param->s_queue = generate_custQueue(N);

		pthread_mutex_lock(&thCount_mutex);
		thCount++;
		pthread_mutex_unlock(&thCount_mutex);
		if (verbose)
			printf("Creating thread %c%02d\n", s_type, t_no);
		pthread_create(thread + t_no, NULL, &sell, s_Param);
	}
}

void queueDisplay(queue *q)
{
	for (node *ptr = q->head; ptr != NULL; ptr = ptr->next)
	{
		customer *cust = (customer *)ptr->value;
		printf("[%d,%d]", cust->c_number, cust->arrival_time);
	}
}

void wait_for_thread_to_serve_present_time()
{
	//Check if all threads has finished their jobs for this time slice
	while (1)
	{
		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		if (clockWaitThread == thPresent)
		{
			clockWaitThread = 0;
			pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
			break;
		}
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
	}
}

//For fisrt time clock revive the threads
void revive_sThreads()
{
	pthread_mutex_lock(&condition_mutex);
	if (verbose)
		printf("00:%02d Main Thread Broadcasting Clock Tick\n", present_time);
	pthread_cond_broadcast(&condition_cond);
	pthread_mutex_unlock(&condition_mutex);
}

//Function to sell ticket to a customer and check for seat avaialbility or timestamp
void *sell(void *t_args)
{
	//Initializing thread
	seller *args = (seller *)t_args;
	queue *customer_queue = args->s_queue;
	queue *s_queue = create_queue();
	char s_type = args->s_type;
	int s_number = args->s_number + 1;

	pthread_mutex_lock(&thCount_mutex);
	thCount--;
	thPresent++;
	pthread_mutex_unlock(&thCount_mutex);

	customer *cust = NULL;
	int random_wait_time = 0;

	while (present_time < total_run_time)
	{
		//Waiting for clock tick
		pthread_mutex_lock(&condition_mutex);
		if (verbose)
			printf("%02d  ||  %c%02d || Waiting for next clock tick\n", present_time, s_type, s_number);

		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		clockWaitThread++;
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);

		pthread_cond_wait(&condition_cond, &condition_mutex);
		if (verbose)
			printf("%02d  ||  %c%02d || Received Clock Tick\n", present_time, s_type, s_number);
		pthread_mutex_unlock(&condition_mutex);

		// Sell
		if (present_time == total_run_time)
			break;
		//All New Customer Came
		while (customer_queue->size > 0 && ((customer *)customer_queue->head->value)->arrival_time <= present_time)
		{
			customer *temp = (customer *)dequeue(customer_queue);
			enqueue(s_queue, temp);
			printf("%02d  ||  %c%d 	||	%c%d%02d arrived			   ||			||			 ||\n", present_time, s_type, s_number, s_type, s_number, temp->c_number);
		}
		//Serve next customer
		if (cust == NULL && s_queue->size > 0)
		{
			cust = (customer *)dequeue(s_queue);
			cust->response_time = present_time - cust->arrival_time;

			printf("%02d  ||  %c%d 	||	Serving %c%d%02d	          	   ||%8d		||			 ||\n", present_time, s_type, s_number, s_type, s_number, cust->c_number, cust->response_time);
			switch (s_type)
			{
			case 'H':
				random_wait_time = (rand() % 2) + 1;
				rt_h = rt_h + cust->response_time;
				break;
			case 'M':
				random_wait_time = (rand() % 3) + 2;
				rt_m = rt_m + cust->response_time;
				break;
			case 'L':
				random_wait_time = (rand() % 4) + 4;
				rt_l = rt_l + cust->response_time;
			}
		}
		if (cust != NULL)
		{
			if (random_wait_time == 0)
			{
				// Function to sell Seats
				pthread_mutex_lock(&reservation_mutex);

				// Function to Find Number of Seats
				int seatIndex = findAvailableSeat(s_type);
				if (seatIndex == -1)
				{
					printf("%02d  ||  %c%d 	||		%c%d%02d informed that tickets are Sold Out.\n", present_time, s_type, s_number, s_type, s_number, cust->c_number);
				}
				else
				{
					int row_no = seatIndex / columns;
					int col_no = seatIndex % columns;
					cust->turnaround_time = cust->turnaround_time + present_time;
					sprintf(available_seats_matrix[row_no][col_no], "%c%d%02d", s_type, s_number, cust->c_number);
					printf("%02d  ||  %c%d 	||	%c%d%02d assigned seat %d,%d    	   ||   		||%15d  	 ||\n", present_time, s_type, s_number, s_type, s_number, cust->c_number, row_no, col_no, cust->turnaround_time);

					switch (s_type)
					{
					case 'H':

						tat_h = tat_h + cust->turnaround_time;
						break;
					case 'M':
						tat_m = tat_m + cust->turnaround_time;
						break;
					case 'L':
						tat_l = tat_l + cust->turnaround_time;
					}
				}
				pthread_mutex_unlock(&reservation_mutex);
				cust = NULL;
			}
			else
			{
				random_wait_time--;
			}
		}
	}

	while (cust != NULL || s_queue->size > 0)
	{
		if (cust == NULL)
			cust = (customer *)dequeue(s_queue);
		printf("%02d  ||  %c%d 	||	Ticket Sale Closed.		   ||			||			 ||\n", present_time, s_type, s_number, s_type, s_number, cust->c_number);
		cust = NULL;
	}
	pthread_mutex_lock(&thCount_mutex);
	thPresent--;
	pthread_mutex_unlock(&thCount_mutex);
}

//Fucntion to check if a seat is empty. If it is then return the index for the same.
int findAvailableSeat(char s_type)
{
	int seatIndex = -1;
	if (s_type == 'H')
	{
		for (int row_no = 0; row_no < row; row_no++)
		{
			for (int col_no = 0; col_no < columns; col_no++)
			{
				if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
				{
					seatIndex = row_no * columns + col_no;
					return seatIndex;
				}
			}
		}
	}
	else if (s_type == 'M')
	{
		int mid = row / 2;
		int row_jump = 0;
		int next_row_no = mid;
		for (row_jump = 0;; row_jump++)
		{
			int row_no = mid + row_jump;
			if (mid + row_jump < row)
			{
				for (int col_no = 0; col_no < columns; col_no++)
				{
					if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
					{
						seatIndex = row_no * columns + col_no;
						return seatIndex;
					}
				}
			}
			row_no = mid - row_jump;
			if (mid - row_jump >= 0)
			{
				for (int col_no = 0; col_no < columns; col_no++)
				{
					if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
					{
						seatIndex = row_no * columns + col_no;
						return seatIndex;
					}
				}
			}
			if (mid + row_jump >= row && mid - row_jump < 0)
			{
				break;
			}
		}
	}
	else if (s_type == 'L')
	{
		for (int row_no = row - 1; row_no >= 0; row_no--)
		{
			for (int col_no = columns - 1; col_no >= 0; col_no--)
			{
				if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
				{
					seatIndex = row_no * columns + col_no;
					return seatIndex;
				}
			}
		}
	}

	return -1;
}

//Function to generate customer queue.
queue *generate_custQueue(int N)
{
	queue *customer_queue = create_queue();
	char c_number = 0;
	while (N--)
	{
		customer *cust = (customer *)malloc(sizeof(customer));
		cust->c_number = c_number;
		cust->arrival_time = rand() % total_run_time;
		enqueue(customer_queue, cust);
		c_number++;
	}
	sort(customer_queue, comapre_arrivalTime);
	node *ptr = customer_queue->head;
	c_number = 0;
	while (ptr != NULL)
	{
		c_number++;
		customer *cust = (customer *)ptr->value;
		cust->c_number = c_number;
		ptr = ptr->next;
	}
	return customer_queue;
}

int comapre_arrivalTime(void *value1, void *value2)
{
	customer *c1 = (customer *)value1;
	customer *c2 = (customer *)value2;
	if (c1->arrival_time < c2->arrival_time)
	{
		return -1;
	}
	else if (c1->arrival_time == c2->arrival_time)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

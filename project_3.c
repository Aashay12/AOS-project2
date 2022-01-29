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
#define concert_row 10
#define concert_col 10
#define total_simulation_duration 60

//Seller Structure
typedef struct seller_struct
{
	char seller_no;
	char seller_type;
	queue *seller_queue;

} seller;

//Customer Structure
typedef struct customer_struct
{
	char customer_no;
	int arrival_time;
	int response_time;
	int turnaround_time;

} customer;

int current_time_slice;
int N = 0;
char available_seats_matrix[concert_row][concert_col][5];
int response_time_for_H;
int response_time_for_L;
int response_time_for_M;

int turnaround_time_for_H;
int turnaround_time_for_L;
int turnaround_time_for_M;

//Thread variables
pthread_t seller_t[total_seller];
pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_waiting_for_clock_tick_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t reservation_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thread_completion_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_cond = PTHREAD_COND_INITIALIZER;

//Function declarations
void print_queue(queue *q);
void create_seller_threads(pthread_t *thread, char seller_type, int no_of_sellers);
void wait_for_thread_to_serve_current_time_slice();
void wakeup_all_seller_threads();
void *sell(void *);
queue *generate_customer_queue(int);
int compare_by_arrival_time(void *value1, void *value2);
int findAvailableSeat(char seller_type);

int thread_count = 0;
int threads_waiting_for_clock_tick = 0;
int active_thread = 0;
int verbose = 0;
int main(int argc, char **argv)
{

	if (argc == 2)
	{
		N = atoi(argv[1]);
	}

	//Initialize Global Variables
	//Mark "-" for all locations of available_seats_matrix[row][col] Jyoti
	for (int r = 0; r < concert_row; r++)
	{
		for (int c = 0; c < concert_col; c++)
		{
			strncpy(available_seats_matrix[r][c], "-", 1);
		}
	}

	//Create all threads
	create_seller_threads(seller_t, 'H', seller_h);
	create_seller_threads(seller_t + seller_h, 'M', seller_m);
	create_seller_threads(seller_t + seller_h + seller_m, 'L', seller_l);

	//Wait for threads to finish initialization and wait for synchronized clock tick
	while (1)
	{
		pthread_mutex_lock(&thread_count_mutex);
		if (thread_count == 0)
		{
			pthread_mutex_unlock(&thread_count_mutex);
			break;
		}
		pthread_mutex_unlock(&thread_count_mutex);
	}

	//Simulate each time quanta/slice as one iteration
	printf("\nThread simulation starts here:");
	printf("\n---------------------------------------------------------------------------------------\n");
	printf("Time 	SellerName	Activity			Response Time 	Turnaround Time");
	printf("\n---------------------------------------------------------------------------------------\n");
	threads_waiting_for_clock_tick = 0;
	wakeup_all_seller_threads(); //For first tick

	do
	{

		//Wake up all thread
		wait_for_thread_to_serve_current_time_slice();
		current_time_slice = current_time_slice + 1;
		wakeup_all_seller_threads();
		//Wait for thread completion
	} while (current_time_slice < total_simulation_duration);

	//Wakeup all thread so that no more thread keep waiting for clock Tick in limbo
	wakeup_all_seller_threads();

	while (active_thread)
		;

	printf("\nThread simulation Ended\n\n");
	//Display concert chart
	printf("\n\n");
	printf("Final Concert Seat Chart");
	printf("\n****************************\n\n");

	int h_customers = 0, m_customers = 0, l_customers = 0;
	for (int r = 0; r < concert_row; r++)
	{
		for (int c = 0; c < concert_col; c++)
		{
			if (c != 0)
				printf("\t");
			printf("%5s", available_seats_matrix[r][c]);
			if (available_seats_matrix[r][c][0] == 'H')
				h_customers++;
			if (available_seats_matrix[r][c][0] == 'M')
				m_customers++;
			if (available_seats_matrix[r][c][0] == 'L')
				l_customers++;
		}
		printf("\n");
	}

	printf("\n\n*****************************************");
	printf("\nMulti-threaded Ticket Sellers");
	printf("\nInput N = %02d\n", N);
	printf("*****************************************\n\n");

	printf(" ---------------------------------------------------------\n");
	printf("|%3c | No of Customers | GotSeat  | Returned | Throughput|\n", ' ');
	printf(" ---------------------------------------------------------\n");
	printf("|%3c | %15d | %8d | %8d | %.2f      |\n", 'H', seller_h * N, h_customers, (seller_h * N) - h_customers, (h_customers / 60.0));
	printf("|%3c | %15d | %8d | %8d | %.2f      |\n", 'M', seller_m * N, m_customers, (seller_m * N) - m_customers, (m_customers / 60.0));
	printf("|%3c | %15d | %8d | %8d | %.2f      |\n", 'L', seller_l * N, l_customers, (seller_l * N) - l_customers, (l_customers / 60.0));
	printf(" --------------------------------------------------------\n");
	printf("\n");

	printf(" -----------------------------------------------\n");
	printf("|%3c   | Avg response Time | Avg turnaround time|\n", ' ');
	printf(" ------------------------------------------------\n");
	printf("| %3c  | %3f          | %.2f 		|\n", 'H', response_time_for_H / (N * 1.0), turnaround_time_for_H / (N * 1.0));
	printf("| %3c  | %3f          | %.2f 		|\n", 'L', response_time_for_L / (6.0 * N), turnaround_time_for_L / (6.0 * N));
	printf("| %3c  | %3f          | %.2f 		|\n", 'M', response_time_for_M / (3.0 * N), turnaround_time_for_M / (3.0 * N));
	printf(" ------------------------------------------------\n");
	return 0;
}

void create_seller_threads(pthread_t *thread, char seller_type, int no_of_sellers)
{
	//Create all threads
	for (int t_no = 0; t_no < no_of_sellers; t_no++)
	{
		seller *seller_param = (seller *)malloc(sizeof(seller));
		seller_param->seller_no = t_no;
		seller_param->seller_type = seller_type;
		seller_param->seller_queue = generate_customer_queue(N);

		pthread_mutex_lock(&thread_count_mutex);
		thread_count++;
		pthread_mutex_unlock(&thread_count_mutex);
		if (verbose)
			printf("Creating thread %c%02d\n", seller_type, t_no);
		pthread_create(thread + t_no, NULL, &sell, seller_param);
	}
}

void print_queue(queue *q)
{
	for (node *ptr = q->head; ptr != NULL; ptr = ptr->next)
	{
		customer *cust = (customer *)ptr->value;
		printf("[%d,%d]", cust->customer_no, cust->arrival_time);
	}
}

void wait_for_thread_to_serve_current_time_slice()
{
	//Check if all threads has finished their jobs for this time slice
	while (1)
	{
		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		if (threads_waiting_for_clock_tick == active_thread)
		{
			threads_waiting_for_clock_tick = 0;
			pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
			break;
		}
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);
	}
}
void wakeup_all_seller_threads()
{

	pthread_mutex_lock(&condition_mutex);
	if (verbose)
		printf("00:%02d Main Thread Broadcasting Clock Tick\n", current_time_slice);
	pthread_cond_broadcast(&condition_cond);
	pthread_mutex_unlock(&condition_mutex);
}

void *sell(void *t_args)
{
	//Initializing thread
	seller *args = (seller *)t_args;
	queue *customer_queue = args->seller_queue;
	queue *seller_queue = create_queue();
	char seller_type = args->seller_type;
	int seller_no = args->seller_no + 1;

	pthread_mutex_lock(&thread_count_mutex);
	thread_count--;
	active_thread++;
	pthread_mutex_unlock(&thread_count_mutex);

	customer *cust = NULL;
	int random_wait_time = 0;

	while (current_time_slice < total_simulation_duration)
	{
		//Waiting for clock tick
		pthread_mutex_lock(&condition_mutex);
		if (verbose)
			printf("00:%02d %c%02d Waiting for next clock tick\n", current_time_slice, seller_type, seller_no);

		pthread_mutex_lock(&thread_waiting_for_clock_tick_mutex);
		threads_waiting_for_clock_tick++;
		pthread_mutex_unlock(&thread_waiting_for_clock_tick_mutex);

		pthread_cond_wait(&condition_cond, &condition_mutex);
		if (verbose)
			printf("00:%02d %c%02d Received Clock Tick\n", current_time_slice, seller_type, seller_no);
		pthread_mutex_unlock(&condition_mutex);

		// Sell
		if (current_time_slice == total_simulation_duration)
			break;
		//All New Customer Came
		while (customer_queue->size > 0 && ((customer *)customer_queue->head->value)->arrival_time <= current_time_slice)
		{
			customer *temp = (customer *)dequeue(customer_queue);
			enqueue(seller_queue, temp);
			printf("00:%02d 	%c%d 		Customer No %c%d%02d arrived\n", current_time_slice, seller_type, seller_no, seller_type, seller_no, temp->customer_no);
		}
		//Serve next customer
		if (cust == NULL && seller_queue->size > 0)
		{
			cust = (customer *)dequeue(seller_queue);
			cust->response_time = current_time_slice - cust->arrival_time;

			printf("00:%02d 	%c%d 		Serving Customer No %c%d%02d	%8d\n", current_time_slice, seller_type, seller_no, seller_type, seller_no, cust->customer_no, cust->response_time);
			switch (seller_type)
			{
			case 'H':
				random_wait_time = (rand() % 2) + 1;
				response_time_for_H = response_time_for_H + cust->response_time;
				break;
			case 'M':
				random_wait_time = (rand() % 3) + 2;
				response_time_for_M = response_time_for_M + cust->response_time;
				break;
			case 'L':
				random_wait_time = (rand() % 4) + 4;
				response_time_for_L = response_time_for_L + cust->response_time;
			}
		}
		if (cust != NULL)
		{
			//printf("Wait time %d\n",random_wait_time);
			if (random_wait_time == 0)
			{
				//Selling Seat
				pthread_mutex_lock(&reservation_mutex);

				// Find seat
				int seatIndex = findAvailableSeat(seller_type);
				if (seatIndex == -1)
				{
					printf("00:%02d 	%c%d 			Customer No %c%d%02d has been told Concert Sold Out.\n", current_time_slice, seller_type, seller_no, seller_type, seller_no, cust->customer_no);
				}
				else
				{
					int row_no = seatIndex / concert_col;
					int col_no = seatIndex % concert_col;
					cust->turnaround_time = cust->turnaround_time + current_time_slice;
					sprintf(available_seats_matrix[row_no][col_no], "%c%d%02d", seller_type, seller_no, cust->customer_no);
					printf("00:%02d 	%c%d 		Customer No %c%d%02d assigned seat %d,%d %15d\n", current_time_slice, seller_type, seller_no, seller_type, seller_no, cust->customer_no, row_no, col_no, cust->turnaround_time);

					switch (seller_type)
					{
					case 'H':

						turnaround_time_for_H = turnaround_time_for_H + cust->turnaround_time;
						break;
					case 'M':
						turnaround_time_for_M = turnaround_time_for_M + cust->turnaround_time;
						break;
					case 'L':
						turnaround_time_for_L = turnaround_time_for_L + cust->turnaround_time;
					}
				}
				pthread_mutex_unlock(&reservation_mutex);
				cust = NULL;
			}
			else
			{
				random_wait_time--;
			}
		} // else {
		  // }
	}

	while (cust != NULL || seller_queue->size > 0)
	{
		if (cust == NULL)
			cust = (customer *)dequeue(seller_queue);
		printf("00:%02d 	%c%d 		Ticket Sale Closed. Customer No %c%d%02d Leaves\n", current_time_slice, seller_type, seller_no, seller_type, seller_no, cust->customer_no);
		cust = NULL;
	}
	pthread_mutex_lock(&thread_count_mutex);
	active_thread--;
	pthread_mutex_unlock(&thread_count_mutex);
}

int findAvailableSeat(char seller_type)
{
	int seatIndex = -1;

	if (seller_type == 'H')
	{
		for (int row_no = 0; row_no < concert_row; row_no++)
		{
			for (int col_no = 0; col_no < concert_col; col_no++)
			{
				if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
				{
					seatIndex = row_no * concert_col + col_no;
					return seatIndex;
				}
			}
		}
	}
	else if (seller_type == 'M')
	{
		int mid = concert_row / 2;
		int row_jump = 0;
		int next_row_no = mid;
		for (row_jump = 0;; row_jump++)
		{
			int row_no = mid + row_jump;
			if (mid + row_jump < concert_row)
			{
				for (int col_no = 0; col_no < concert_col; col_no++)
				{
					if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
					{
						seatIndex = row_no * concert_col + col_no;
						return seatIndex;
					}
				}
			}
			row_no = mid - row_jump;
			if (mid - row_jump >= 0)
			{
				for (int col_no = 0; col_no < concert_col; col_no++)
				{
					if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
					{
						seatIndex = row_no * concert_col + col_no;
						return seatIndex;
					}
				}
			}
			if (mid + row_jump >= concert_row && mid - row_jump < 0)
			{
				break;
			}
		}
	}
	else if (seller_type == 'L')
	{
		for (int row_no = concert_row - 1; row_no >= 0; row_no--)
		{
			for (int col_no = concert_col - 1; col_no >= 0; col_no--)
			{
				if (strcmp(available_seats_matrix[row_no][col_no], "-") == 0)
				{
					seatIndex = row_no * concert_col + col_no;
					return seatIndex;
				}
			}
		}
	}

	return -1;
}

queue *generate_customer_queue(int N)
{
	queue *customer_queue = create_queue();
	char customer_no = 0;
	while (N--)
	{
		customer *cust = (customer *)malloc(sizeof(customer));
		cust->customer_no = customer_no;
		cust->arrival_time = rand() % total_simulation_duration;
		enqueue(customer_queue, cust);
		customer_no++;
	}
	sort(customer_queue, compare_by_arrival_time);
	node *ptr = customer_queue->head;
	customer_no = 0;
	while (ptr != NULL)
	{
		customer_no++;
		customer *cust = (customer *)ptr->value;
		cust->customer_no = customer_no;
		ptr = ptr->next;
	}
	return customer_queue;
}

int compare_by_arrival_time(void *value1, void *value2)
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

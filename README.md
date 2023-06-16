# Ticketing System - Advanced OS-project2

The provided code is a simulation of a ticket selling system with multiple ticket sellers (threads) selling tickets to customers (also represented as threads). The code uses pthreads library for multi-threading.

## Code Functionality:

- The main function initializes the theater layout, creates seller threads with their respective queues, and starts the simulation.

- The code includes necessary header files and defines constants and structures for ticket sellers and customers.

- Global variables and thread-related variables are declared, including mutexes, condition variable, and pthreads.

- The code defines functions for displaying the customer queue, initializing seller threads, waiting for threads to serve the present time, 
  and selling tickets.

- The simulation runs for a specified total run time (60 minutes) in one-minute increments.

- Within each time increment, the code wakes up the seller threads and waits for them to complete their tasks.

- After the simulation ends, the code displays the ticket sales statistics, including the number of customers entered, booked seats, returned tickets, and throughput for each seller type.

- The code also calculates and displays the average response time and average turnaround time for each seller type.

- The program ends and returns 0.

If you have specific questions or need assistance with a particular part of the code, please let me know and I'll be happy to help.

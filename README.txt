ACS.c is a task scheduler for airline check-in simulations and runs in a Linux environment. 

Instructions for compiling and executing
1. To compile the source code, simply execute 'make' from the terminal.
2. To run the executable, execute './ACS <input text file>' from the terminal. Ex: ./ACS customer.txt

The input text file must follow the format specified below:
The first line contains the number of customers that will be simulated.
After that each line contains the info about a single customer, such that:
1. The first character specifies the unique ID of the customer.
2. A colon immediately follows the unique ID of the customer.
3. Immediately following is an integer equal to either 0 (business class) or 1 (economy class) that indicates the class type of the customer.
4. A comma immediately follows the previous number.
5. Immediately following is an integer that indicates the arrival time of the customer.
6. A comma immediately follows the previous number.
7. Immediately following is an integer that indicates the service time of the customer. 
8. A newline ends a line.

There are three sample input text files provided:
customers.txt
- This input file correctly follows the specified format.
customers2.txt
- This input file includes illegal values intentionally to demonstrate that the code can handle illegal cases
customers3.txt
- This input file matches the example in the assignment spec




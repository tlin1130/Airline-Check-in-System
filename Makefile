all: ACS

ACS: ACS.c
	gcc ACS.c -o ACS -pthread

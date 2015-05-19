OBJS = healthcenterserver doctor patient1 patient2
CC = gcc

all: healthcenterserver doctor patient1 patient2

healthcenterserver:

	$(CC) healthcenterserver.c -o healthcenterserver 

doctor:

	$(CC) doctor.c -o doctor 

patient1:

	$(CC) patient1.c -o patient1 

patient2:

	$(CC) patient2.c -o patient2 

clean:

	rm -rf $(OBJS)





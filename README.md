# Medical-Appointment-System
a. (1) Achieved the TCP communication between health center server and patients.
   (2) Achieved the UDP communication between doctor and patients.
   (3) Read information from txt files and output message to command line screen.
   (4) Achieved many functions like number typed check, mark appointment reserved and others required in project descriptions.
   (5) Use fork() to achieve multiprocess implementation.
   (6) Write Makefile to compile files automatically.

b. healthcenterserver.c
   It establish TCP connections with many patients and receives the requests from those patients. This program handles with patients’s authentication and appointment booking.

   docotr.c
   The single file has two doctor using different port number. It establish UDP connections with patients and deal with appointment confirmation for patients.

   patient1.c
   It establish TCP connection with health center server to finish phase 1 authentication and phase 2 appointment booking. And then establish UDP connection with doctor to finish phase 3 appointment confirmation.

   patient2.c
   It establish TCP connection with health center server to finish phase 1 authentication and phase 2 appointment booking. And then establish UDP connection with doctor to finish phase 3 appointment confirmation.

c. (1) Use cd command to enter the directory that has files.

   (2) Type “make” in the command line to compile files.

   (3) Type “./healthcenterserver” to execute health center server program.

   (4) Open another command line and type “./doctor” to execute doctor program.

   (5) Open another command line and type “./patient1” to execute patient1 program.

   (6) When the program is executed, you will be asked to enter a number to choose the available time. You can choose 7 or 80 or anything else to check. And you will see “Your choice does not match with the time indices displayed. Please re-enter a correct time index:”, then enter a valid choice.

   (7) You can use the same command line or open another command line and type “./patient2” to execute patient2 program.

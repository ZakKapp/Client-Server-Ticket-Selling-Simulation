# Client-Server-Ticket-Selling-Simulation
A project using a client and server program to simulate the selling of airline tickets where a client can connect to the server and ask to purchase available tickets.
The goal of this project is to practice Server-Client interactions using TCP/IP protocol in order to simulate a client purchasing airline tickets from a server.
The instructions for the project are as follows:

1) Both the server and the clients program should be written in C/C++ under Linux/Unix.

2) The client program(s) should connect using socket to the running server program.

3) If the server is not available (not running, connection problems, etc.) the client should
timeout (try several connection attempts in some given interval (see Timeout) in the ini
file (see more details below) and exit.

4) The connection IP address of the server, the port number and the timeout should be read
from a regular text file at each client [re]start (see details below).

5) For each client the server should listen in a separate thread (Not mandatory but highly
recommended!) The software will be tested at least with two clients connected
simultaneously to the server application.

6) Initially, when the server program starts all the tickets are available for sale. The tickets
should be allocated from a rectangular map, where each pair of (column, row) represents
one particular seat. The size of the map (# of rows, # of columns) should be provided as
command line arguments when the server is launched. If there are no such parameters
given, some default values (ex. 10x10) should be considered. Ex.: ./server 10 15 means
that the server program will distribute tickets from 10 rows, each row containing 15 seats.

7) The client software can act:
  a. Each time when a new purchase is to happen we have to introduce from
the terminal the row and column for the seat we want to purchase. If the
seat is available and valid (see row and column), the server will allocate it
and will notify the client or otherwise send the corresponding [error]
message. A second purchase cannot be initiated from the same client until
the first one is not closed successfully or unsuccessfully. This process can
go on without disconnecting the client until all the tickets are sold. In case
of asking for an erroneous seat, the client should be notified.
  b. The client can ask for random seats, generating the row and column pairs
randomly. This process is repeated –with some delay random delay of
3/5/7 seconds) until all the seats are allocated.
  c. The operation a) and b) should be launched from command line when the
client software is started. If in the command line there is “manual” the
manual way of purchasing is activated, if “automatic” is invoked the client
should purchase tickets automatically. Ex. ./client manual means that the
client will try to connect using the default IP and Port number and the
client will purchase the tickets by introducing at the standard input each
time the row and column for the ticket to be purchased. If we call the client
like this: ./client automatic myinifile.txt in that case the client software
will connect to the server application using the information coming from
the myinifile.txt and the generation of the rows and columns for the
tickets will happen fully automatically. In both cases the simulation runs or
can run until all the tickets are sold. If that is the case the clients are
disconnected from the server.

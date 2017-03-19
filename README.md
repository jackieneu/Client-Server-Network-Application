# Client-Server-Network-Application

Final school project for Intro to Computer Networks.

Created client and server programs to transfer a text file over a socket. The server program was written in C++ and the client was written in Python. The client program could request the server to send a list of the current directory or could request the server to send a text file from the current directory.

To run the code, first start the server using the command given below. Then start the client and provide one of the optional command line inputs. The client can request a list of the current directory from the server with -l <port_num>, the server will then send only the text files in the current directory to the client. This program does not send all the files in the current directory since the server can only reliably send text files. The client can also request a text file to be sent using the -g <file_name> <port_num> command. The server will verify the file exists and send it to the client on the specified data port. 


To Run and Compile Server Code:

```
   g++ -g -Wall ftserver.cpp -o ftserver
   ./ftserver <port_num>
```

   Example:
```
   ./ftserver 30051
```

To Run and Compile Client Code:

```
   python ftclient.py <host_name> <port_num> -g <file_name> <data_port> -l <data_port>
```

   Examples:
```
   python ftclient.py host -l 30051

   python ftclient.py host -g test.txt 30051

   python ftclient.py host 30052
```

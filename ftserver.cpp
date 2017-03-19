/* 
CS 372 Project 2
Server Code
Jackie Lamb
*/

#include <vector>
#include <string>
#include <iostream>  
#include <dirent.h>
#include <sys/types.h> /*basic system data types */
#include <sys/socket.h> /* basic socket definitions */
#include <sys/time.h> /* timeval struct for select */
#include <netinet/in.h> /* sockaddr in and other internet defs */
#include <arpa/inet.h> /* inet(3) functions */
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h> /*for bzero */
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/select.h>
using namespace std;

#define LISTENQ 1  //BACKLOG FOR LISTEN

#define MAXLINE 4001
#define MAXSOCKADDR 128
#define BUFFSIZE 8192

static volatile int run = 1;

void sigIntHandler(int sig){
  signal(sig, SIG_IGN);
  printf("\nProgram terminating due to SIGINT\n");
  exit(0);
}

void receiveMessage(char* recvline, int sockfd){
  bzero(recvline, MAXLINE);
  if(read(sockfd, recvline, MAXLINE) == 0){
    perror("client terminated prematurely");
    close(sockfd);
  }
  printf("%s\n", recvline);
}

void sendMessage(char* sendline, int sockfd){
  if((write(sockfd, sendline, strlen(sendline))) == -1){
    fprintf(stderr, "Failure sending message\n");
    close(sockfd);
    exit(1);
  }
}

int startSock(int portNum){
  int listenfd;
  struct sockaddr_in servaddr;

  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(portNum);

  bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

  listen(listenfd, LISTENQ);
  printf("Server open on %d\n",portNum);

  return listenfd;
}

int startDataSock(int portNum, char* hostname){
  int sockfd;
  struct sockaddr_in servaddr;
  struct hostent *he;
  struct in_addr **addr_list;
  char ip[100];
  int i;

  he = gethostbyname(hostname);
  addr_list = (struct in_addr **) he->h_addr_list;
  for(i=0; addr_list[i] != NULL; i++){
    strcpy(ip, inet_ntoa(*addr_list[i]));
  }    
 
  sockfd = socket(AF_INET, SOCK_STREAM, 0);

  //server address
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(portNum);
  inet_pton(AF_INET, ip, &servaddr.sin_addr);

  //Setup connection to the address
  connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)); //connnects socket

  return sockfd;
}

const char *get_filename_ext(const char *filename) {
  const char *dot = strrchr(filename, '.');
  if(!dot || dot == filename) return "";
  return dot;
}

int getDir(vector<string> &files){
  DIR *dir;
  struct dirent *ent;
  int i = 0;
  files.clear();

  if ((dir = opendir ("./")) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      if(strcmp(get_filename_ext(ent->d_name), ".txt")==0){
	files.push_back(string(ent->d_name));
	i++;
      }
    }  
    closedir (dir);
  } else {
    perror ("Could not open directory.");
    exit(1);
  }
  return i;
}

void getList(char* recvline, int sockfd, char* host){
  char dataPort[1024];
  int dataPortNum;
  int dataSockfd;
  char buffer[1024];
  vector<string> files = vector<string>();
  string buffer2;

  //Get Data port
  strcpy(dataPort, &recvline[33]);
  dataPortNum = atoi(dataPort);
  
  printf("Sending directory contents to %s:%d\n", host,dataPortNum);

  //Get Data Socket
  dataSockfd = startDataSock(dataPortNum, host);

  //Send message about sending directory
  snprintf(buffer, sizeof(buffer), "Receiving directory structure from ");
  sendMessage(buffer, sockfd);

  //Send Directory
  getDir(files);
  bzero(buffer, 1024);
  buffer2.erase(buffer2.begin(), buffer2.end());
  for(unsigned int i = 0; i<files.size(); i++){
    buffer2 += " ";
    buffer2 += files[i];
  }
  sprintf(buffer, "%s", buffer2.c_str());
  sendMessage(buffer, dataSockfd);

  close(sockfd);
  close(dataSockfd);
}

void getFile(char* hostname, char * recvline, int sockfd, char* host ){
  char *endString;
  char* pch;
  char dataPort[1024];
  int dataPortNum;
  int dataSockfd;
  char buffer[1024];
  char fileName[1024];
  int quoteStart = 0;
  int quoteEnd = 0;
  char fileBuff[1024];
  FILE *f;

  //Get Port Number
  endString = strrchr(recvline, 't');
  strcpy(dataPort, &endString[2]);
  dataPortNum = atoi(dataPort);

  //Get filename
  pch = strchr(recvline,'"');
  quoteStart = pch - recvline;
  bzero(buffer, 1024);
  strcpy(buffer, &recvline[quoteStart+1]);
  pch = strchr(buffer,'"');
  quoteEnd = pch - buffer;
  bzero(fileName,1024);
  strncpy(fileName, buffer, quoteEnd);

  //Create data socket connection
  dataSockfd = startDataSock(dataPortNum, host);

  //Check that file is valid
  f = fopen(fileName,"r");
  if(f == NULL){
    printf("File not found. Sending error message to %s:%d\n",host, dataPortNum);

    bzero(buffer, 1024);
    sprintf(buffer, "%s:%d says FILE NOT FOUND", hostname, dataPortNum);
    sendMessage(buffer, sockfd);
  }else{
    printf("Sending '%s' to %s:%d\n",fileName,host, dataPortNum);

    bzero(buffer, 1024);
    sprintf(buffer, "Receiving '%s' from %s:%d", fileName, hostname, dataPortNum);
    sendMessage(buffer, sockfd);

    //Transfer file
    int file_size;
    while((file_size = fread(fileBuff, sizeof(char), 1024, f)) > 0){
      if(send(dataSockfd, fileBuff, file_size, 0) < 0){
	printf("Error sending file\n");
	break;
      }
      //printf("%s\n",fileBuff);
      bzero(fileBuff, 1024);
    }

    bzero(buffer, 1024);
    sprintf(buffer, "File transfer complete.");
    sendMessage(buffer, sockfd);
  }
  close(sockfd);
  close(dataSockfd);

}

int main(int argc, char **argv){
  int listenfd;
  int sockfd;
  int portNum;
  char recvline[MAXLINE];
  socklen_t clilen;
  struct sockaddr_in cliaddr;
  char host[1024];
  char service[20];
  char hostname[1024];

  //Get this connection's hostname
  hostname[1023] = '\0';
  gethostname(hostname, 1023);

  //Handle SIGINT
  signal(SIGINT, sigIntHandler);

  //Check for port input
  if(argc < 2){
    fprintf(stderr, "usage: ./chatserve [port number]\n");
    return -1;
  }
  portNum = atoi(argv[1]);

  //Create Socket Listener
  listenfd = startSock(portNum);

  //Keep Server Running until SIGINT
  while(1){
  clilen = sizeof(cliaddr);
  sockfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
  getnameinfo((struct sockaddr *)&cliaddr, clilen, host, sizeof host, service, sizeof service, 0);
  printf("Connection from %s.\n", host);

  //Messaging between client and server
  bzero(recvline, MAXLINE);
  
  //receive message and check for input type
  receiveMessage(recvline, sockfd);

      //-l
      if((strstr(recvline,"List directory requested on port")) != NULL){
	getList(recvline, sockfd, host);
      }

      //-get
      if((strstr(recvline,"File"))!= NULL){
	getFile(hostname, recvline, sockfd, host);
      }
  }
  return 0;
}

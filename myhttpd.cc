
const char * usage =
"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <string>
int QueueLength = 5;
pthread_mutex_t mutex;
// Processes time request
void processHTTPRequest( int socket );


void processRequestThread(int socket){
  processHTTPRequest(socket);
  close(socket); 
}
void poolSlave(int socket){
  while(1){
  
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    pthread_mutex_lock(&mutex);
    int slaveSocket = accept( socket,(struct sockaddr *)&clientIPAddress,(socklen_t*)&alen);
  
    pthread_mutex_unlock(&mutex);
  processHTTPRequest(slaveSocket); close(slaveSocket);

  } 

}

int
main( int argc, char ** argv )
{
  // Print usage if not enough arguments
  if ( argc < 2 ) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }
  
  // Get the port from the arguments
  int port;
  int flag;
  if (argc ==2) {
    flag = 2;
    port = atoi( argv[1] );
  } else if (argc ==3) {
    flag = 3;
    port = atoi( argv[2]);
    std::string flag = argv[1];
    

  }
  
  // Set the IP address and port for this server
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);
  
  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    close(masterSocket);
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
		       (char *) &optval, sizeof( int ) );
   
  // Bind the socket to the IP address and port
  int error = bind( masterSocket,
		    (struct sockaddr *)&serverIPAddress,
		    sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    close(masterSocket);
    exit( -1 );
  }
  
  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    close(masterSocket);
    exit( -1 );
  }

if (flag == 3 && strcmp(argv[1], "-f") == 0) {
  while(1){
  
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    int slaveSocket = accept( masterSocket,(struct sockaddr *)&clientIPAddress,(socklen_t*)&alen);

    

    pid_t slave = fork();
   
    if(slaveSocket == -1 && errno == EINTR){ continue;}
   if(slave == 0){
      processHTTPRequest(slaveSocket); 
      close(slaveSocket); 
      exit(EXIT_SUCCESS);
    }
    close(slaveSocket);
  }
  } else if (flag == 3 && strcmp(argv[1], "-t") == 0) {
      while(1){
        struct sockaddr_in clientIPAddress;
        int alen = sizeof( clientIPAddress );
        int slaveSocket = accept( masterSocket,(struct sockaddr *)&clientIPAddress,(socklen_t*)&alen);
        
        
        pthread_t tid;

        pthread_create(&tid, NULL, (void * (*)(void *))processRequestThread, (void *)slaveSocket);
      
      }




  } else if (flag ==3 && strcmp(argv[1], "-p") == 0) {
    pthread_t tid[5];
    for(int i=0; i < 5;i++){
      pthread_create(&tid[i], NULL,(void *(*)(void *))poolSlave,
(void *)masterSocket);
    }
      pthread_join(tid[0], NULL);
   


  } else if (flag == 2) {
  while ( 1 ) {

    // Accept incoming connections
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    int slaveSocket = accept( masterSocket,
			      (struct sockaddr *)&clientIPAddress,
			      (socklen_t*)&alen);

    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }
    
    // Process request.
    if (slaveSocket > 0) {
      

      processHTTPRequest( slaveSocket );
      
    }
    // Close socket
    close( slaveSocket );
  }
  }
  
}

void
processHTTPRequest( int fd )
{
  
  // Buffer used to store the name received from the client
  const int MaxRequest =4*1024;
  char request[ MaxRequest + 1 ];
  int reqLength = 0;
  int n;


  // Currently character read
  unsigned char newChar;
  // The client should send <name><cr><lf>
  // Read the name of the client character by character until a
  // <CR><LF> is found.
  //
    
  while ( reqLength < MaxRequest &&
	  ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

    if ( reqLength > 4 
    && request[reqLength - 3] == '\r'
    && request[reqLength - 2] == '\n'
    && request[reqLength - 1] == '\r'
    && newChar == '\n') {
      // Discard previous <CR> from name
      reqLength--;
      break;
    }

    request[ reqLength ] = newChar;
    reqLength++;
    
  }

  // Add null character at the end of the string
  request[ reqLength ] = 0;
//  off_t tempLength = fileSize(fd);
//  int Size = (int)tempLength;
  
//get the name of the http request
  if (strstr(request, "Authorization: Basic dXNlcjpwYXNzd29yZA") == NULL ) {
    const char * var = "HTTP/1.1 401 Unauthorized\r\n"
    "WWW-Authenticate: Basic realm=\"myhttpd-cs252\"";
    write(fd, var, strlen(var));
    printf("%s\n", request);
    close(fd);
    return;
  }
  
  
  char*requestName;
  requestName = strtok(request, " ");
  requestName = strtok(NULL, " ");

  std::string result = requestName;
  if (result == "/" || result == "favicon.ico") {
    result = "/index.html";
  }
  
  result = "http-root-dir/htdocs" + result;
  printf("%s\n", result.c_str());






  int docfd = open(result.c_str(), O_RDONLY);
  std::string error;
  if (docfd < 0) {
    error = 
    "HTTP/1.1 404 File Not Found\r\n"
    "Server: CS252 HTTP Server\r\n"
    "Content-type: text/plain\r\n" 
    "\r\n";
    write(fd, error.c_str(), error.length());
    
    
  } else {
//if statements based on what type of request it is
  std::string HTTPHeader;
  if (strstr(result.c_str(), "html") != NULL) {
    HTTPHeader =
      "HTTP/1.1 200 Document follows\r\n"
      "Server: CS252 HTTP Server\r\n"
      "Content-type: text/html\r\n"
      "\r\n";
    
    
     
  } else if (strstr(result.c_str(), "gif") != NULL) {
    HTTPHeader = 
      "HTTP/1.1 200 Document follows\r\n"
      "Server: CS252 HTTP Server\r\n"
      "Content-type: image/gif\r\n"
      "\r\n";
    


  } else if (strstr(result.c_str(), "txt") != NULL) {
    HTTPHeader = 
      "HTTP/1.1 200 Document follows\r\n"
      "Server: CS252 HTTP Server\r\n"
      "Content-type: text/plain\r\n"
      "\r\n";
    
  }

  else if (strstr(result.c_str(), "png") != NULL) {
    HTTPHeader =
      "HTTP/1.1 200 Document follows\r\n"
      "Server: CS252 HTTP Server\r\n"
      "Content-type: image/png\r\n"
      "\r\n";

  }

  else if (strstr(result.c_str(), "svg") != NULL) {
    HTTPHeader =
      "HTTP/1.1 200 Document follows\r\n"
      "Server: CS252 HTTP Server\r\n"
      "Content-type: image/svg+xml\r\n"
      "\r\n";

  }
  
  write(fd, HTTPHeader.c_str(), strlen(HTTPHeader.c_str()));
  char buf[690000];
  int num;
  while (num = read(docfd, buf, 690000)) {
    if (write(fd, buf, num) != num) {
      perror("write");
      bzero(buf, 690000);
      break;
    }
  }
  
  close(docfd);
  }
  close(fd);
}

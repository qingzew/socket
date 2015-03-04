#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <signal.h>
#include <fstream>
#include <ctime>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "restart.h"

#define MAXLINE 4096

//void handler(int sig) {
//    std::cout<<"hello"<<std::endl;
//}

void Child(int sockFd) {
    char    buff[4096];
    int     n;

    n = recv(sockFd, buff, MAXLINE, 0);
    buff[n] = '\0';

    std::cout<<buff<<std::endl;

    std::time_t result = std::time(0);

    std::ofstream out("/tmp/output", std::ios::app);
    out<<boost::this_thread::get_id()<<" "<<buff<<" "<<std::asctime(std::localtime(&result));
    out.close();

}

int main(int argc, char const* argv[])
{

    pid_t pid, sid;

    //fork the parent process
    pid = fork();

    if (pid < 0) {
        exit(EXIT_FAILURE);
    }

    //we got a good pid, close the parent process
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    //change file mask
    umask(0);

    //create a new signature id for our child
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    //change directory
    //if we can not find the directory we exit with failure
    if ((chdir("/")) < 0) {
        exit(EXIT_FAILURE);
    }

    //close standard file description
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //main process
    int    listenfd, connfd;
    struct sockaddr_in     servaddr;

    if( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 ){
        printf("create socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

//    timeval timeout = {1, 0};
//    if (setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeval)) < 0)
//    {
//        exit(1);
//    }
//
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6666);

    if( bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1){
        printf("bind socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

    if( listen(listenfd, 10) == -1){
        printf("listen socket error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }

//    struct sigaction action;
//    action.sa_handler = handler;
//    sigemptyset(&action.sa_mask);
//    action.sa_flags = 0;
//    action.sa_flags |= SA_RESTART;
//
//    sigaction(SIGALRM, &action, NULL);
//
//    alarm(3);

    printf("======waiting for client's request======\n");
    while(1){
        if((connfd = accept(listenfd, (struct sockaddr*)NULL, NULL)) == -1){
            if (errno == EINTR) {
                std::cout<<strerror(errno)<<" : "<<errno;
                break;
            }
        }

        boost::thread thrd(boost::bind(&Child, connfd));
        thrd.join();
        close(connfd);
    }

    close(listenfd);

	//close the log
	closelog();

	return 0;
}
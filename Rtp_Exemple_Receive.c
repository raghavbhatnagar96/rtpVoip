/*******
 * 
 * FILE INFO:
 * project: udpserv
 * file:    udpserv.c
 * started on:  05/01/03 15:14:26
 * started by:  Julien Dupasquier <jdupasquier@wanadoo.fr>
 * 
 * 
 * TODO:
 * 
 * BUGS:
 * 
 * UPDATE INFO:
 * updated on:  05/15/03 16:18:24
 * updated by:  Julien Dupasquier <jdupasquier@wanadoo.fr>
 * 
 *******/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>
#include <math.h>
#include <errno.h>
#include "Config.h"
#include "RTP.h"
#include "Macros.h"
#include "Proto.h"
#include "Rtp_Exemple_Receive.h"
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <time.h>
#include <stdint.h> 
#include "Config.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#define BUFSIZE 1024
#define MAXDATASIZE 1024 // max number of bytes we can get at once 


// FONCTIONS Local@udpserv.c

static void us_serve(struct us *,int ac,char **av);
static void us_conf(struct us *);
static int us_start(struct us *);
static int us_setup(struct us *us);
static struct us *us_init(int, char **);
static void us_event(struct us *us, int cid, int *len,int ac,char **av);


int us_start(struct us *us)
{
    t_listener *srv;
    int len;
    srv = us->listeners;
    if ((srv->fd = socket(srv->type, srv->family, 0)) == -1)
    {
    perror("socket");
    exit(EXIT_FAILURE);
    }
    #ifdef HAVE_INET6
    if (srv->family == AF_INET6)
    {
        struct sockaddr_in6 *sin;
        MEM_ALLOC(sin);
        sin->sin6_len = sizeof (*sin);
        sin->sin6_addr = IN6_ADDR_ANY;
        sin->sin6_port = htons(srv->port);
        len = sin->sin6_len;
        srv->add = (struct sockaddr *) sin;
    }
    else
    #endif
    {
    struct sockaddr_in *sin;
    MEM_ALLOC(sin);
    //sin->sin_len = sizeof (*sin);
    sin->sin_addr.s_addr = INADDR_ANY;
    sin->sin_port = htons(srv->port);
    srv->add = (struct sockaddr *) sin;
    srv->len = sizeof(*sin); 
    }
    if ((bind(srv->fd, srv->add, srv->len)) == -1)
    {
    perror("bind");
    exit(EXIT_FAILURE);   
    }
    }


void us_conf(struct us *us)
{
    t_listener      *srv;
    t_client        *client;


    /* Listen Config */
    MEM_ALLOC(srv);
    srv->type = SOCK_DGRAM;
    srv->family = AF_INET;
    srv->port = UDP_PORT;
    us->listeners =  srv;

    #ifdef HAVE_INET6
    MEM_ALLOC(srv);
    srv->type = SOCK_DGRAM;
    srv->family = AF_INET6;
    srv->port = UDP_PORT + 2;
    us->listeners = srv;
    #endif

    /* Client Config */
    MEM_ALLOC(client);
    srv->clients = client;
    #ifdef HAVE_INET6
    if (srv->type == AF_INET6)
    client->len = sizeof(struct sockaddr_in6);
    #endif
    if (srv->type == AF_INET)
    client->len = sizeof(struct sockaddr_in);
    MEM_SALLOC(client->add, client->len);
}

struct us *us_init(int ac, char **av)
{
    struct us *us;
    MEM_ALLOC(us);
    /*
    * Eventually it would be necessary to make a list of listeners from
    * Of a conf file
    */
    us_conf(us); /* XXX no use */
    (void)us_start(us);
    return (us);
}


void us_event(struct us *us, int cid, int *len,int ac, char **av)
{
    char msg[MAX_PACKET_LEN];
    unsigned char buffer[BUFSIZE];
    t_listener *srv;
    t_client *client;
    int error;
    int fd;
    /*For PulseAudio*/
    static const pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = 8000,
    .channels = 2
    };
    pa_simple *playStream = NULL;
    if (!(playStream = pa_simple_new(NULL, av, PA_STREAM_PLAYBACK, NULL, "playback", &ss, NULL, NULL, &error))) {
    fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    goto finish;
    }
    srv = us->listeners;
    client = srv->clients;
    if (FD_ISSET(srv->fd, &(us->fdset)))
    {
        /* A new message arrives */
        RTP_Receive(cid, srv->fd, msg, len, client->add);
        //Print_context(msg, *len, cid);
        int i;
        for(i=0;i<BUFSIZE;i++)//as msg not taken as input into pa simple write
        {
            buffer[i]=msg[i];    
        }
        if (pa_simple_write(playStream, buffer, sizeof(buffer), &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_write() failed: %s\n", pa_strerror(error));
        goto finish;
        }
    }

    finish:
        if (pa_simple_drain(playStream, &error) < 0) {
        fprintf(stderr, __FILE__": pa_simple_drain() failed: %s\n", pa_strerror(error));
        goto finish;
        }
    if (playStream)
        pa_simple_free(playStream);
}


int us_setup(struct us *us)
{
    int max = 0;
    t_listener *srv;
    FD_ZERO(&(us->fdset));
    srv = us->listeners;
    FD_SET(srv->fd, &(us->fdset));
    if (srv->fd > max)
    max = srv->fd;
    return ++max;
}

void us_serve(struct us *us,int ac, char **av) //Create cid here
{
    int max;
    int cid;
    int len;
    int endofExp=100000000;
    int timeDescriptor;
    struct timespec currentTime;
    struct itimerspec newTime;
    unsigned long long currentExp, sumExp;
    ssize_t readTime;
    /*Setting up timer*/
    /*Made changes as the recieved packets were not the full data*/
    clock_gettime(CLOCK_REALTIME, &currentTime);
    newTime.it_value.tv_sec = currentTime.tv_sec ;
    newTime.it_value.tv_nsec = currentTime.tv_nsec;
    newTime.it_interval.tv_sec=0;
    newTime.it_interval.tv_nsec = 1000;
    /*These system calls create and operate on a timer that delivers timer
       expiration notifications via a file descriptor.*/
    timeDescriptor = timerfd_create(CLOCK_REALTIME, 0);
    /*Create CID*/
    RTP_Create(&cid);
    while (1)
    {
        max = us_setup(us);
        switch (select(max, &us->fdset, NULL, NULL, NULL))
        {
            case -1:
            if (errno != EINTR) /* The interrupt is not serious */
            {
            perror("select");
            exit (EXIT_FAILURE);
            }
            break;
            case 0:
            perror("I understand nothing");
            /* If there is a timeout it is possible, if not */
            exit (EXIT_FAILURE);
            default:
            {
                /* timerfd_settime() arms (starts) or disarms (stops) the timer referred
       to by the file descriptor fd.*/
                /*The timer will expire when the value of the
              timer's clock reaches the value specified in
              newTime.it_value.*/
                timerfd_settime(timeDescriptor, TFD_TIMER_ABSTIME, &newTime, NULL);
                //us_event(us, cid, &len,ac,av);
                sumExp = 0;
                while(sumExp < endofExp)
                {
                    printf("a");
                    readTime = read(timeDescriptor, &currentExp, sizeof(uint64_t));
                    //printf("%d", readTime);
                    readTime!= sizeof(uint64_t);
                    sumExp = sumExp + currentExp;
                    us_event(us, cid, &len,ac,av);
                }
            }
        }
    RTP_Destroy(cid);
    }
}
int main(int ac, char **av)
{
    struct us *server;
    server = us_init(ac, av);
    us_serve(server,ac,av);
    return 0;
}

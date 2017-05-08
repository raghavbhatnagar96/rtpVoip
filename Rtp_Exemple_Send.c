/*******
 *
 * FILE INFO:
 * project: RTP_lib
 * file:    Rtp_Exemple_Send.c
 * started on:  03/26/03
 * started by:  Cedric Lacroix <lacroix_cedric@yahoo.com>
 *
 *
 * TODO:
 *
 * BUGS:
 *
 * UPDATE INFO:
 * updated on:  05/14/03 17:11:47
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
#include "Config.h"
#include "RTP.h"
#include "Types.h"
#include "Proto.h"
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include "Config.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/timerfd.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h> 
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#define BUFSIZE 1024
int check = 1;

/*
Signal handler for the client.
*/
void my_handler_for_sigint(int signumber)//handler to handle SIGINT ctrl+C
{
    char ans[2];
    if (signumber == SIGINT)
    {
        printf("received SIGINT\n");
        printf("Program received a CTRL-C\n");
        printf("Terminate Y/N : "); 
        scanf("%s", ans);
        if ((strcmp(ans,"Y") == 0)||(strcmp(ans,"y") == 0))
        {
            check = 0;
            exit(0); 
        }
        else
        {
        printf("Continung ..\n");
        }
    }
}


/*Main Function */
int main(int argc, char *argv[])
{
    /*Definitions for rtp */
    char buffer[MAX_PAYLOAD_LEN];
    char buf[BUFSIZE];
    context cid;
    u_int32     period;
    u_int32     t_inc;
    u_int16     size_read;
    u_int16     last_size_read;
    period = Get_Period_us(PAYLOAD_TYPE); 
    Init_Socket();
    RTP_Create(&cid);
    RTP_Add_Send_Addr(cid, "127.0.0.1", UDP_PORT, 6);
    Set_Extension_Profile(cid, 27);
    Add_Extension(cid, 123456);
    Add_Extension(cid, 654321);
    Add_CRSC(cid, 12569);
    conx_context_t   *coucou = NULL;
    /*end*/
    
    /*For PulseAudio*/
    static const pa_sample_spec ss = {
    .format = PA_SAMPLE_S16LE,
    .rate = 8000,
    .channels = 2
    };
    pa_simple *recordStream = NULL;
    int error;
    if (!(recordStream = pa_simple_new(NULL, argv[0], PA_STREAM_RECORD, NULL, "record", &ss, NULL, NULL, &error))) {
    fprintf(stderr, __FILE__": pa_simple_new() failed: %s\n", pa_strerror(error));
    goto finish;
    }
    /*end*/
    
    //Signal handler for sigint
    if (signal(SIGINT, my_handler_for_sigint) == SIG_ERR)//register signal handler
        printf("\ncan't catch SIGINT\n");

    // if (argc != 3) {
    //     fprintf(stderr,"usage: ./client <hostname> <portnumber>\n");
    //     exit(1);
    // }
    while(1)
    {
        if (pa_simple_read(recordStream, buf, sizeof(buf), &error) < 0) 
        {
            fprintf(stderr, __FILE__": pa_simple_read() failed: %s\n", pa_strerror(error));
            goto finish;
        }
        size_read = BUFSIZE;
        RTP_Send(cid, t_inc, 0, PAYLOAD_TYPE, buf, size_read);
        
    }

    RTP_Destroy(cid);
    Close_Socket();

    finish:
        if (recordStream)
        pa_simple_free(recordStream);

    return 0;
}


/************************************************************************
*   Program Name: SNTPv4 Server											*
*   Description:  The server starts by binding a listener socket. After	*
*	receiving a packet, the server checks the date and time	that it was	*
*	received. It then constructs an SNTP packet to send back to	the		*
*	client. Dirctly before sending, it checks the latest date and time,	*
*	updates the packet, then sends it. The server closes the socket and *
*	repeats from the beginning.											*
*																		*
*   Created By:     Michael Bradford: 14025807, Mitch Ford: 14021226	*
*   																	*
*   Date Created:   24/11/2015											*
*   Date Modified:  02/12/2015											*
*																		*
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MYPORT 1123        /* the port client connects to */
#define MAXBUFLEN 100

void convertTime (int *ntpsec, int *ntpusec, struct timeval time);

struct packet 
{
	uint8_t livnm;	/*leap indicator, version number and mode*/
	uint8_t stratum;
	uint8_t poll;
	int8_t precision;
	int32_t root_delay;
	uint32_t root_dispersion;
	uint32_t resource_identifier;
	uint32_t reference_timestamp_1;	/*timestamps are split into seconds and microseconds*/
    uint32_t reference_timestamp_2;
	uint32_t originate_timestamp_1;
	uint32_t originate_timestamp_2;
	uint32_t recieve_timestamp_1;
    uint32_t recieve_timestamp_2;
	uint32_t transmit_timestamp_1;
    uint32_t transmit_timestamp_2;
};

int main( void) 
 {
    int sockfd, ntpsec, ntpusec;
    struct sockaddr_in my_addr;    /* info for my addr i.e. server */
    struct sockaddr_in their_addr; /* client's address info */
    struct packet received;	/*packet received from client*/
    struct packet replied;	/*packet sent to client*/
    struct timeval time;
    int addr_len, numbytes;
    
    while (1)
    {
       if( (sockfd = socket( AF_INET, SOCK_DGRAM, 0)) == -1)
       {
		 perror( "Listener socket");
		 exit( 1);
       }

       memset( &my_addr, 0, sizeof( my_addr));    /* zero struct */
       my_addr.sin_family = AF_INET;              /* host byte order ... */
       my_addr.sin_port = htons( MYPORT); /* ... short, network byte order */
       my_addr.sin_addr.s_addr = INADDR_ANY;   /* any of server IP addrs */

       if( bind( sockfd, (struct sockaddr *)&my_addr, sizeof( struct sockaddr)) == -1)	/*bind socket*/
	   {
	     perror( "Listener bind");
	     exit( 1);
	   }

       addr_len = sizeof( struct sockaddr);

       memset( &received, 0, sizeof(received));	/*clear the recieve packet*/

       if( (numbytes = recvfrom( sockfd, (void *)&received, sizeof(received), 0, (struct sockaddr *)&their_addr, (socklen_t *)&addr_len)) == -1)
	   {	/*wait for a packet from a client*/
	     perror( "Listener recvfrom");
	     exit( 1);
	   }

       memset( &replied, 0, sizeof(replied));	/*clear the reply packet*/

       gettimeofday(&time, NULL);	/*get current date and time*/
       convertTime(&ntpsec, &ntpusec, time);	/*convert date and time to NTP timestamp format*/
       
       replied.recieve_timestamp_1 = htonl(ntpsec);	/*set receive timestamp to current*/
       replied.recieve_timestamp_2 = htonl(ntpusec);	/*date and time in network bite order*/

       replied.livnm = 0x24;	/*set LI to 0, VN to 4 and Mode to 4*/
       replied.stratum = 1;
       replied.poll = received.poll;	/*copy the poll from request to reply*/

       replied.reference_timestamp_1 = htonl(ntpsec);	/*set reference timestamp to current*/
       replied.reference_timestamp_2 = htonl(ntpusec);	/*date and time in network bite order*/
       
       replied.originate_timestamp_1 = received.transmit_timestamp_1;	/*copy transmit timestamp from request*/
       replied.originate_timestamp_2 = received.transmit_timestamp_2;	/*to originate timestamp in reply*/

       gettimeofday(&time, NULL);	/*get current date and time*/

       convertTime(&ntpsec, &ntpusec, time);	/*convert date and time to NTP timestamp format*/
       
       replied.transmit_timestamp_1 = htonl(ntpsec);	/*set transmit timestamp to current*/
       replied.transmit_timestamp_2 = htonl(ntpusec);	/*date and time in network bite order*/

       if( (numbytes = sendto( sockfd, (void *)&replied, sizeof(replied), 0, (struct sockaddr *)&their_addr,sizeof(struct sockaddr))) == -1)
       {	/*send the packet to the client*/
	     perror( "Talker sendto");
	     exit( 1);
       }

       memset( &their_addr,0, sizeof(their_addr));  /* zero struct */
       their_addr.sin_family = AF_INET;     /* host byte order .. */

       close( sockfd);	/*close the socket*/
    }
    return 0;
  }

/********************************************************************
*   Function Name: Convert Time										*
*   Description:  Reads unix time and converts it to ntp time for	*
*	both seconds and microseconds.									*
*																	*
*   Inputs: time - A structure containing the date and time in 		*
*	seconds and microseconds										*
*   Outputs: ntpsec - The NTP time in seconds.						*
	     ntpusec - The NTP time in microseconds.					*
*																	*
*   Created By:	waitingkuo.blogspot.co.uk/2012/06/conversion-		*
*	between-ntp-time-and-unix.html									*
*   																*
*   Date Created:   24/11/2015										*
*   Date Modified:  01/12/2015										*
*																	*
********************************************************************/

void convertTime (int *ntpsec, int *ntpusec, struct timeval time)
{
  *ntpsec = time.tv_sec + 0x83AA7E80;
  *ntpusec = (uint32_t)((double)(time.tv_usec+1) * (double)(1LL<<32) * 1.0e-6 );
}
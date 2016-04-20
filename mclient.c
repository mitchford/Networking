/********************************************************************************
*   Program Name: SNTPv4 Manycast Client										*
*   Description:  The client is run on IP 224.0.0.1.  It creates an SNTP client	*
*	request packet which is sent to the all servers on the network. The			*
*	client waits until a reply is received, and checks if the reply is			*
*	valid. If it is not valid then the client resends the packet, if it is		*
*	valid then it gets the current date and time and uses it along with the		*
*	received packet to calculate the system clock offset. Finally, the			*
*	client closes the socket.													*
*																				*
*   Created By:     Michael Bradford: 14025807, Mitch Ford: 14021226			*
*   																			*
*   Date Created:   02/12/2015													*
*   Date Modified:  02/12/2015													*
*																				*
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>         /* for gethostbyname() */
#define PORT 1123     /* server port the client connects to */

void convertTime (unsigned int *ntpsec, unsigned int *ntpusec, struct timeval time);

struct packet 
{
	uint8_t livnm;	/*leap indicator, version number and mode*/
	uint8_t stratum;
	uint8_t poll;
	int8_t precision;
	int32_t root_delay;
	uint32_t root_dispersion;
	uint32_t resource_identifier;
	uint32_t reference_timestamp_1;	/*timestamps split into seconds and microseconds*/
    uint32_t reference_timestamp_2;
	uint32_t originate_timestamp_1;
	uint32_t originate_timestamp_2;
	uint32_t recieve_timestamp_1;
    uint32_t recieve_timestamp_2;
	uint32_t transmit_timestamp_1;
    uint32_t transmit_timestamp_2;
};

int main( int argc, char * argv[]) 
{
    int sockfd, numbytes;
    unsigned int ntpsec, ntpusec, sec, usec, dsec, dusec;
    struct hostent *he;
    struct sockaddr_in their_addr;    /* server address info */   
    struct packet request;	/*packet sent to server*/
    struct packet reply;	/*packet received from server*/
    struct timeval time;
	
    if( argc != 1)	/*ensure no argument has been entered for hostname or IP address*/
      {	
        fprintf( stderr, "usage: talker hostname\n");
        exit(1);
      }
	  
    argv[1] = "224.0.0.1";	/*set the destination IP*/
	
    if( (he = gethostbyname(argv[1])) == NULL)	/*resolve server host name or IP address*/
      {
        perror( "Talker gethostbyname");
        exit(1);
      }   
    if( (sockfd = socket( AF_INET, SOCK_DGRAM, 0)) == -1) 
       {
         perror( "Talker socket");
         exit(1);
       }
	   
    gettimeofday(&time, NULL);	/*get current date and time*/
    convertTime(&ntpsec, &ntpusec, time);	/*convert date and time to NTP timestamp format*/
    
    memset( &request, 0, sizeof(request));	/*set the request packet to all 0s*/
    request.livnm = 0x23;	/*set LI to 0, VN to 4 and Mode to 3*/
    request.transmit_timestamp_1 = htonl(ntpsec);	/*set transmit timestamp to current*/
    request.transmit_timestamp_2 = htonl(ntpusec);	/*date and time in network bite order*/

    memset( &their_addr,0, sizeof(their_addr));  /* zero struct */
    their_addr.sin_family = AF_INET;     /* host byte order .. */
    their_addr.sin_port = htons( PORT);  /* .. short, netwk byte order */
    their_addr.sin_addr = *((struct in_addr *)he -> h_addr);
    
    if( (numbytes = sendto( sockfd, (void *)&request, sizeof(request), 0, (struct sockaddr *)&their_addr,sizeof(struct sockaddr))) == -1)	/*send request packet to the IP address*/
      {
	    perror( "Talker sendto");
	    exit( 1);
      }

    socklen_t len = sizeof( struct sockaddr);
    
	if( (numbytes = recvfrom( sockfd, (void *)&reply, sizeof(reply), 0, (struct sockaddr *)&their_addr, &len)) == -1)	/*receive a packet from the server*/
      {
	    perror( "Talker recv");
	    exit( 1);
      }

    gettimeofday(&time, NULL);	/*get current date and time*/
    convertTime(&ntpsec, &ntpusec, time);	/*convert date and time to NTP timestamp format*/
	
    reply.recieve_timestamp_1 = ntohl(reply.recieve_timestamp_1);	/*conver timestamps to host byte*/
    reply.originate_timestamp_1 = ntohl(reply.originate_timestamp_1);	/*order for calculations*/
    reply.transmit_timestamp_1 = ntohl(reply.transmit_timestamp_1);
    reply.recieve_timestamp_2 = ntohl(reply.recieve_timestamp_2);
    reply.originate_timestamp_2 = ntohl(reply.originate_timestamp_2);
    reply.transmit_timestamp_2 = ntohl(reply.transmit_timestamp_2);

    sec = ((reply.recieve_timestamp_1 - reply.originate_timestamp_1) + (reply.transmit_timestamp_1 - ntpsec)) / 2;
    usec = ((reply.recieve_timestamp_2 - reply.originate_timestamp_2) + (reply.transmit_timestamp_2 - ntpusec)) / 2;
	
    dsec = (ntpsec - reply.originate_timestamp_1) - (reply.transmit_timestamp_1 - reply.recieve_timestamp_1);
    dusec = (ntpusec - reply.originate_timestamp_2) - (reply.transmit_timestamp_2 - reply.recieve_timestamp_2);

    sec = abs(sec - dsec);	/*calculate the clock offset, accounting for roundtrip dleay*/
    usec = abs(usec - dusec);
    
    printf ("Clock is off by %d.%d seconds \n", sec, usec);	/*print clock offset*/
	
    close( sockfd);	/*close the socket*/
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
void convertTime (unsigned int *ntpsec, unsigned int *ntpusec, struct timeval time)
{
  *ntpsec = time.tv_sec + 0x83AA7E80;
  *ntpusec = (uint32_t)((double)(time.tv_usec+1) * (double)(1LL<<32) * 1.0e-6 );
}
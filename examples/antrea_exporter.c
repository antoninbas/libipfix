/*
**     exporter.c - example exporter
**
**     Copyright Fraunhofer FOKUS
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <ipfix.h>
#include <ipfix_def_antrea.h>
#include <ipfix_fields_antrea.h>
#include "mlog.h"

int main ( int argc, char **argv )
{
    char      *optstr="hc:p:vstu";
    int       opt;
    char      chost[256];
    int       protocol = IPFIX_PROTO_TCP;
    int       j;

    char      srcIP[4]  = {1, 2, 3, 4};
    char      dstIP[4]  = {5, 6, 7, 8};
    uint16_t  srcPort  = 20000;
    uint16_t  dstPort  = 30000;
    uint8_t   proto    = 6;
    uint32_t  packets  = 100;
    uint32_t  bytes    = 12340;
    char     sourcePodName[32] = "nsA/podA";
    char     destPodName[32] = "nsB/podB";


    ipfix_t           *ipfixh  = NULL;
    ipfix_template_t  *ipfixt  = NULL;
    int               sourceid = 12345;
    int               port     = IPFIX_PORTNO;
    int               verbose_level = 0;

    /* set default host */
    strcpy(chost, "localhost");

    /** process command line args
     */
    while( ( opt = getopt( argc, argv, optstr ) ) != EOF )
    {
	switch( opt )
	{
	  case 'p':
	    if ((port=atoi(optarg)) <0) {
		fprintf( stderr, "Invalid -p argument!\n" );
		exit(1);
	    }
            break;

	  case 'c':
            strcpy(chost, optarg);
	    break;

          case 's':
              protocol = IPFIX_PROTO_SCTP;
              break;

          case 't':
              protocol = IPFIX_PROTO_TCP;
              break;

          case 'u':
              protocol = IPFIX_PROTO_UDP;
              break;

          case 'v':
              verbose_level ++;
              break;

	  case 'h':
	  default:
              fprintf( stderr, "usage: %s [-hstuv] [-c collector] [-p portno]\n" 
                       "  -h               this help\n"
                       "  -c <collector>   collector address\n"
                       "  -p <portno>      collector port number (default=%d)\n"
                       "  -s               send data via SCTP\n"
                       "  -t               send data via TCP (default)\n"
                       "  -u               send data via UDP\n"
                       "  -v               increase verbose level\n\n",
                       argv[0], IPFIX_PORTNO  );
              exit(1);
	}
    }

    /** init loggin
     */
    mlog_set_vlevel( verbose_level );

    /** init lib 
     */
    if ( ipfix_init() <0) {
        fprintf( stderr, "cannot init ipfix module: %s\n", strerror(errno) );
        exit(1);
    }

    /** open ipfix exporter
     */
    if ( ipfix_open( &ipfixh, sourceid, IPFIX_VERSION ) <0 ) {
        fprintf( stderr, "ipfix_open() failed: %s\n", strerror(errno) );
        exit(1);
    }

    /** set collector to use
     */
    if ( ipfix_add_collector( ipfixh, chost, port, protocol ) <0 ) {
        fprintf( stderr, "ipfix_add_collector(%s,%d) failed: %s\n", 
                 chost, port, strerror(errno));
        exit(1);
    }

    /** get template
     */
    if ( ipfix_new_data_template( ipfixh, &ipfixt, 9) <0 ) {
        fprintf( stderr, "ipfix_new_template() failed: %s\n", 
                 strerror(errno) );
        exit(1);
    }
    if ( (ipfix_add_field( ipfixh, ipfixt, 
                           0, IPFIX_FT_SOURCEIPV4ADDRESS, 4 ) <0 )
         || (ipfix_add_field( ipfixh, ipfixt,
                               0, IPFIX_FT_DESTINATIONIPV4ADDRESS, 4 ) <0 )
         || (ipfix_add_field( ipfixh, ipfixt, 0, IPFIX_FT_SOURCETRANSPORTPORT, 2 ) <0)
         || (ipfix_add_field( ipfixh, ipfixt, 0, IPFIX_FT_DESTINATIONTRANSPORTPORT, 2 ) <0)
         || (ipfix_add_field( ipfixh, ipfixt,
                                         0, IPFIX_FT_PROTOCOLIDENTIFIER, 1 ) <0)
         || (ipfix_add_field( ipfixh, ipfixt,
                                       0, IPFIX_FT_PACKETDELTACOUNT, 4 ) <0 )
         || (ipfix_add_field( ipfixh, ipfixt, 0, IPFIX_FT_OCTETDELTACOUNT, 4 ) <0 )) {
        fprintf( stderr, "ipfix_new_template() failed: %s\n",
                 strerror(errno) );
        exit(1);
    }
    if ( (ipfix_add_field( ipfixh, ipfixt, IPFIX_ENO_ANTREA, IPFIX_FT_SOURCEPODNAME, 65535 ) <0 )
         || (ipfix_add_field( ipfixh, ipfixt, IPFIX_ENO_ANTREA, IPFIX_FT_DESTINATIONPODNAME, 65535 ) <0 )) {
         fprintf( stderr, "ipfix_new_template() new fields failed: %s\n",
                          strerror(errno) );
         exit(1);
     }

    /** export some data
     */
    for( j=0; j<10; j++ ) {

        printf( "[%d] export some data ... ", j );
        fflush( stdout) ;

        if ( ipfix_export( ipfixh, ipfixt, srcIP, dstIP, &srcPort, &dstPort, &proto, &packets, &bytes, sourcePodName, destPodName ) <0 ) {
            fprintf( stderr, "ipfix_export() failed: %s\n", 
                     strerror(errno) );
            exit(1);
        }

        if ( ipfix_export_flush( ipfixh ) <0 ) {
            fprintf( stderr, "ipfix_export_flush() failed: %s\n", 
                     strerror(errno) );
            exit(1);
        }

        printf( "done.\n" );
        bytes++;
        sleep(1);
    }

    printf( "data exported.\n" );

    /** clean up
     */
    ipfix_delete_template( ipfixh, ipfixt );
    ipfix_close( ipfixh );
    ipfix_cleanup();
    exit(0);
}

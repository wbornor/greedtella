/************************************************************************
 * This file is part of Greedtella.
 *
 * Greedtella is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Greedtella is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Greedtella; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Copyright 2003 William Bornor
 *
 * $Id: greedtella.h,v 1.1.1.1 2005/01/31 21:50:19 wbornor Exp $
 ************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#define DFLT_PORT_IN 6346 //8436 
#define CONNECT_MSG "GNUTELLA CONNECT/0.6\r\nUser-Agent: Greedtella/1.0\r\nX-Ultrapeer: False\r\nBye-Packet: 0.1\r\n\r\n"
#define CONNECT_MSG_SIZE (sizeof(CONNECT_MSG) - 1)
#define CONNECT_OK_MSG "GNUTELLA/0.6 200 OK\r\n\r\n"
#define CONNECT_OK_MSG_SIZE (sizeof(CONNECT_OK_MSG) - 1)
#define CONNECT_MSG_RESPONSE_SIZE 4*1024 //4 kb 
#define CONNECT_STATUS_RESPONSE_SIZE 16 

#define GOODBYE_MSG "Shutting Down\r\n\r\n\0"
#define GOODBYE_MSG_SIZE (sizeof(GOODBYE_MSG) - 1)
#define MAX_GOODBYE_MSG_SIZE 512


#define MAX_QUERY 200 //maximum num of characters in a query including null terminator
#define QUERY_MIN_SPEED 0 //in kb/s
#define GUID_SIZE 16
#define MAX_FILE_NAME_SIZE 30
#define MAX_PAYLOAD_SIZE 4*1024 //4kb 

#define PING 0x00
#define PONG 0x01
#define BYE 0x02
#define PUSH 0x40
#define QUERY 0x80
#define QUERYHIT 0x81

#define TTL 5

#define HTTP_HEADER_SIZE 1024 //1kb

typedef struct _msgId
{
	char guid[GUID_SIZE] __attribute__((__packed__));
} __attribute__((__packed__)) msgId_t;

typedef struct _descHdr
{
	msgId_t msgId __attribute__((__packed__));
	uint8_t payloadDesc __attribute__ ((packed));
	uint8_t ttl  __attribute__ ((packed));
	uint8_t hops  __attribute__ ((packed));
	uint32_t payloadLen  __attribute__ ((packed));

} __attribute__((__packed__)) descHdr_t;

typedef struct _pingPkt
{
	descHdr_t header __attribute__((__packed__));
}  __attribute__((__packed__)) pingPkt_t;

typedef struct _pongPkt
{
	descHdr_t header __attribute__((__packed__));
	uint16_t port __attribute__ ((packed));
	uint32_t ipAddr __attribute__ ((packed));
	uint32_t numFilesShared __attribute__ ((packed));
	uint32_t numKBShared __attribute__ ((packed));
}  __attribute__((__packed__)) pongPkt_t;

typedef struct _queryPkt
{
	descHdr_t header __attribute__((__packed__));
	uint16_t minSpeed __attribute__((__packed__)); //in kb/s
	char query[MAX_QUERY] __attribute__((__packed__)); 

} __attribute__((__packed__)) queryPkt_t;

typedef struct _byePkt
{
	descHdr_t header __attribute__((__packed__));
	uint16_t returnCode __attribute__((__packed__)); 
	char msg[MAX_GOODBYE_MSG_SIZE] __attribute__((__packed__));

} __attribute__((__packed__)) byePkt_t;

typedef struct _queryResultSet
{
	uint32_t fIndex __attribute__((__packed__));
	uint32_t fSize __attribute__((__packed__)); //in bytes
	char *fName __attribute__((__packed__));

}  __attribute__((__packed__)) queryResultSet_t;

typedef struct _queryHitPkt
{
	descHdr_t header __attribute__((__packed__));
	uint8_t numHits __attribute__((__packed__));
	uint16_t port __attribute__((__packed__));
	uint32_t ipaddress __attribute__((__packed__));
	uint32_t speed __attribute__((__packed__)); //in kb/s
	queryResultSet_t *results __attribute__((__packed__));
	char serventID[GUID_SIZE] __attribute__((__packed__));

} __attribute__((__packed__)) queryHitPkt_t;


extern int gConnect(struct sockaddr_in *servaddr);
extern int gClose(const int sd);
extern int getGUID(msgId_t *msgId);
extern int gBuildPktHdrGeneric(descHdr_t *pktHdr, const msgId_t *msgId, const uint8_t payloadDesc, const uint32_t payloadLen);
extern int gBuildPktHdr(descHdr_t *pktHdr, const uint8_t payloadDesc, const uint32_t payloadLen);
extern int gSend(const int sd, void *buf, const int bufLen);
extern int gSendPing(const int sd);
extern int gSendPong(const int sd, const msgId_t *msgId, const char *ipAddr, const uint32_t numFilesShared, const uint32_t numKBShared);
extern int gSendQuery(const int sd, const char *query);
extern int gRecv(const int sd, void *buf, const int bufLen);
extern int gRecvPktHdr(const int sd, descHdr_t *pktHeader);
extern int gRecvPktBody(const int sd, void *buf, const int bufLen);
extern int gParseQueryResultSet(char *payload, queryResultSet_t *qrs, uint32_t *resultSetSize);
extern int gParseQueryHit(char *payload, queryHitPkt_t *hit);
extern int gRecvQueryHit(const int sd, queryHitPkt_t *hit);
extern void dump_hex(FILE *out, const char *title, const char * buf, int bufLen); 
int httpGet(const int sd, const uint32_t fIndex, const uint32_t fSize, const char *fName, char *fBuf);
int httpConnect(const struct sockaddr_in *servaddr);

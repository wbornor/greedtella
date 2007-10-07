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
 * $Id: greedtellalib.c,v 1.1.1.1 2005/01/31 21:50:19 wbornor Exp $
 ************************************************************************/

#include "greedtella.h"


int gConnect(struct sockaddr_in *servaddr)
{
	int sd;
	char buf[CONNECT_MSG_RESPONSE_SIZE];

	memset(buf, '\0', CONNECT_MSG_RESPONSE_SIZE);
	
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0)
		return -1;

	if(connect(sd, (struct sockaddr*) servaddr, sizeof(struct sockaddr)) < 0)
		return -1;

	if(send(sd, CONNECT_MSG, CONNECT_MSG_SIZE, 0) < 0)
		return -1;

	gRecv(sd, &buf, CONNECT_MSG_RESPONSE_SIZE);  
	
	printf("gConnect(): recvd from servent: %s\n",buf);

	if(strncmp(buf, "GNUTELLA/0.6 200", CONNECT_STATUS_RESPONSE_SIZE) == 0)
	{
		if(send(sd, CONNECT_OK_MSG, CONNECT_OK_MSG_SIZE, 0) < 0)
			return -1;

		//connection accepted	
		return(sd);
	}
	else
	{
		//connection failed
		return -1;
	}
}

int gClose(const int sd)
{
	byePkt_t byePkt;
	uint32_t payloadSize;

	if(sd < 0)
		return -1;
	//Should make a more robust bye function.
	//One that handles different error codes and messages

	byePkt.returnCode = htons(200);
	memset(byePkt.msg, '\0', MAX_GOODBYE_MSG_SIZE);
	memcpy(byePkt.msg, GOODBYE_MSG, GOODBYE_MSG_SIZE);

	payloadSize = GOODBYE_MSG_SIZE + sizeof(byePkt.returnCode);

	if(gBuildPktHdr(&byePkt.header, BYE, payloadSize) < 0)
		return -1;

	printf("gClose(): sending BYE packet\n");

	gSend(sd, &byePkt, sizeof(byePkt.header)+payloadSize);

	sleep(5);
	printf("gClose(): closing socket\n");

	return(close(sd));
}

int getGUID(msgId_t *msgId)
{
	int i; 
	struct timeval tv;

	if(!msgId)
		return(-1);

	if(gettimeofday(&tv, 0) < 0)
		return(-1);

	srandom((unsigned int)tv.tv_usec);

	for(i = 0; i < GUID_SIZE; i++)
	{
		if(i == 8)
			msgId->guid[i] = (char)0xff;
		else if(i == 15)
			msgId->guid[i] = (char)0x00;
		else
			msgId->guid[i] = (char)random();
	}

	return(0);
}
		
int gBuildPktHdrGeneric(descHdr_t *pktHdr, const msgId_t *msgId, const uint8_t payloadDesc, const uint32_t payloadLen)
{
	if(pktHdr < 0 || payloadLen < 0)
		return -1;

	//check for valid descriptor
	switch(payloadDesc)
	{
		case PING: break;
		case PONG: break;
		case BYE: break;
		case PUSH: break;
		case QUERY: break;
		case QUERYHIT: break;
		default: return(-1);
	}
	
	//build packet
	memcpy((void*)pktHdr->msgId.guid, (void*)msgId->guid, GUID_SIZE);
	pktHdr->ttl = (payloadDesc == BYE || payloadDesc == PING ? (uint8_t)1 : (uint8_t)TTL);
	pktHdr->payloadDesc = payloadDesc;
	pktHdr->hops = (uint32_t)0;
	pktHdr->payloadLen = payloadLen; 

	return 0;
}

int gBuildPktHdr(descHdr_t *pktHdr, const uint8_t payloadDesc, const uint32_t payloadLen)
{

	msgId_t msgId;

	if(getGUID(&msgId) < 0)
		return -1;

	return(gBuildPktHdrGeneric(pktHdr, &msgId, payloadDesc, payloadLen));

}

int gSend(const int sd, void *buf, const int bufLen)
{
	int err;
	int numBytesSent = 0;

	if(buf <= 0  || bufLen < 0 || sd < 0)
		return -1;

	if(bufLen == 0)
		return 0;
	

	while(numBytesSent < bufLen)
	{
		err = send(sd, buf+numBytesSent, bufLen-numBytesSent, 0);
		if(err < 0)
			return -1;
		numBytesSent += err;
	}

	dump_hex(stdout, "Send", (char*)buf, numBytesSent);

	return(0);
}

int gSendPing(const int sd)
{
	pingPkt_t pingPkt;
	uint32_t payloadSize = 0;

	if(sd < 0)
		return -1;

	if(gBuildPktHdr(&pingPkt.header, PING, payloadSize) < 0)
		return -1;

	printf("gSendPing(): sending PING packet (%d bytes)\n", sizeof(pingPkt));

	return(gSend(sd, &pingPkt, sizeof(pingPkt)));

}

int gSendPong(const int sd, const msgId_t *msgId, const char *ipAddr, const uint32_t numFilesShared, const uint32_t numKBShared)
{
	pongPkt_t pongPkt;
	uint32_t payloadSize;

	if(sd < 0)
		return -1;


	pongPkt.port = DFLT_PORT_IN;
	if(inet_pton(AF_INET, ipAddr, &pongPkt.ipAddr) <= 0)
		return -1;
	pongPkt.numFilesShared = numFilesShared;
	pongPkt.numKBShared = numKBShared;

	payloadSize = sizeof(pongPkt.port) + sizeof(pongPkt.ipAddr)
		+ sizeof(pongPkt.numFilesShared) + sizeof(pongPkt.numKBShared);

	if(gBuildPktHdrGeneric(&pongPkt.header, msgId, PONG, payloadSize) < 0)
		return -1;

	printf("gSendPong(): sending pong (%d bytes)\n", sizeof(pongPkt));

	return(gSend(sd, &pongPkt, sizeof(pongPkt)));

}

int gSendQuery(const int sd, const char *query)
{
	uint32_t queryLen;
	queryPkt_t qPkt;
	uint32_t payloadSize, pktSize;

	if(query <= 0 || sd < 0)
		return(-1);
	
	queryLen = strlen(query) + 1;
	if(queryLen > MAX_QUERY - 1) //MAX_QUERY - 1 because string needs to be null terminated
	{
		//query is too big
		return(-1);
	}

	//build QUERY packet
	qPkt.minSpeed = QUERY_MIN_SPEED;
	memset(qPkt.query, 0, MAX_QUERY);
	memcpy(qPkt.query, query, queryLen);

	payloadSize = queryLen + sizeof(qPkt.minSpeed);

	//build packet header
	if(gBuildPktHdr(&qPkt.header, QUERY, payloadSize) < 0)
		return -1;
	
	pktSize = sizeof(qPkt.header) + payloadSize;

	return (gSend(sd, (void*)&qPkt, pktSize));
}

int gRecv(const int sd, void *buf, const int bufLen)
{
	int totalBytesRcvd = 0;
	int bytesRcvd = 0;

	if(buf <= 0 || bufLen < 0 || sd < 0)
		return -1;

	if(bufLen == 0)
		return 0;

	//while(totalBytesRcvd < bufLen)
	//{
		bytesRcvd = recv(sd, buf + totalBytesRcvd, bufLen - totalBytesRcvd, 0);  
		//printf("gRecv(): bytesRcvd: %d\n", bytesRcvd);

		if(bytesRcvd == 0)
			return(0); 
		if(bytesRcvd < 0)
			return -1;
		totalBytesRcvd += bytesRcvd;
		
		//printf("gRecv(): totalBytesRcvd: %d\n", totalBytesRcvd);
		//return(-1);
	//}

	dump_hex(stdout, "Recieve", (char*)buf, totalBytesRcvd);

	return(0);
}

int gRecvPktHdr(const int sd, descHdr_t *pktHeader)
{

	if(pktHeader <= 0 || sd < 0)
		return -1;

	if(gRecv(sd, pktHeader, sizeof(descHdr_t)) < 0)
		return -1;

	//check for valid descriptor
	switch(pktHeader->payloadDesc)
	{
		case PING: printf("gRecvPktHdr(): received PING header\n"); break;
		case PONG: printf("gRecvPktHdr(): received PONG header\n"); break;
		case BYE: printf("gRecvPktHdr(): received BYE header\n"); break;
		case PUSH: printf("gRecvPktHdr(): received PUSH header\n"); break;
		case QUERY: printf("gRecvPktHdr(): received QUERY header\n"); break;
		case QUERYHIT: printf("gRecvPktHdr(): received QUERYHIT header\n"); break;
		default: return(-1);
	}
			
	//pktHeader->payloadLen = ntohl(pktHeader->payloadLen);
	
	return(0);
}

int gRecvPktBody(const int sd, void *buf, const int bufLen)
{
	if(sd < 0 || buf <= 0 || bufLen < 0)
		return -1;

	if(bufLen == 0)
		return 0;

	return(gRecv(sd, buf, bufLen));
}

int gParseQueryResultSet(char *payload, queryResultSet_t *qrs, uint32_t *resultSetSize)
{
	uint32_t filenameSize, metadataSize, extensionsSize;

	if(payload < 0 || qrs <= 0)
		return -1;
	
	metadataSize = sizeof(qrs->fIndex) + sizeof(qrs->fSize);
	filenameSize = strlen(payload+metadataSize) + 1;
	extensionsSize = strlen(payload+metadataSize+filenameSize) + 1;

	qrs->fName = (char*)malloc(filenameSize);
	
	memcpy((void*)qrs, (void*)payload, metadataSize);
	memcpy((void*)qrs->fName, (void*)payload+metadataSize, filenameSize);

	*resultSetSize = metadataSize + filenameSize + extensionsSize;
	
	return 0;
}

int gParseQueryHit(char *payload, queryHitPkt_t *hit)
{
	int headerSize, metaDataSize, i, parseMarker,serventIDMarker,  resultSetSize;

	if(payload <= 0 || hit <= 0)
		return -1;

	headerSize = sizeof(hit->header); 
	metaDataSize = sizeof(hit->numHits) + sizeof(hit->port)
		+ sizeof(hit->ipaddress) + sizeof(hit->speed);

	memcpy((void*)hit+headerSize, (void*)payload, metaDataSize);
	parseMarker = metaDataSize;

	hit->results = (queryResultSet_t*)malloc(hit->numHits*sizeof(queryResultSet_t));

	for(i=0; i<hit->numHits; i++)
	{
		if(gParseQueryResultSet(payload+parseMarker, &hit->results[i], &resultSetSize) < 0)
			return -1;

		parseMarker += resultSetSize;
	}

	serventIDMarker = hit->header.payloadLen - sizeof(hit->serventID);
	memcpy(hit->serventID, payload+serventIDMarker, sizeof(hit->serventID));

	return 0;
}

int gRecvQueryHit(const int sd, queryHitPkt_t *hit)
{
	char payload[MAX_PAYLOAD_SIZE];

	if(gRecv(sd, (void*)payload, hit->header.payloadLen) < 0)
		return -1;

	if(gParseQueryHit(payload, hit) < 0)
		return -1;

	return 0;
	
}

int httpGet(const int sd, const uint32_t fIndex, const uint32_t fSize, const char *fName, char *fBuf)
{
	char getMsg[HTTP_HEADER_SIZE];
	char replyMsg[HTTP_HEADER_SIZE];
	uint32_t contentLength, httpCode;

	if(sd < 0 || fIndex < 0 || fSize < 0 || fName <= 0 || fBuf <= 0)
		return -1;

	memset(getMsg, '\0', HTTP_HEADER_SIZE);

	sprintf(getMsg, "GET /get/%d/%s HTTP/1.1\r\n"
			"UserAgent: Greedtella/1.0\r\n"
			"Host: 127.0.0.1\r\n"
			"Connection: Keep-Alive\r\n"
			"Range: bytes=0-\r\n"
			"\r\n", fIndex, fName); 
	
	if(gSend(sd, (void*)getMsg, strlen(getMsg)) < 0)
		return -1;

	if(gRecv(sd, (void*)replyMsg, HTTP_HEADER_SIZE) < 0)  
		return -1;

	sscanf(replyMsg, "HTTP/1.1 %d ", &httpCode);
	sscanf(replyMsg, "%*[a-zA-Z0-9!@#$%^&*()_+-=:;,./<>? ]Content-Length: %d\r\n", &contentLength);

	printf("httpCode: %d\n", httpCode);
	printf("contentLength: %d\n", contentLength);

	return 0;
}

int httpConnect(const struct sockaddr_in *servaddr)
{
	int sd;
	
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0)
		return -1;

	if(connect(sd, (struct sockaddr*) servaddr, sizeof(struct sockaddr)) < 0)
		return -1;
	else
		return sd;
}


void dump_hex(FILE *out, const char *title, const char *buf, int bufLen)
{
	int i, x, y, z, end;
	char temp[18];
	const char hex_alphabet_lower[] = "0123456789abcdef";

	fprintf(out, "dum_hex(): start dump of %s:\n", title);

	i = x = end = 0;
	for (;;) {
		if ((x & 0xff) == 0) {					/* i%256 == 0 */
			if (x > 0)
				fputc('\n', out);				/* break after 256 byte chunk */
			fprintf(out, "%s%s\n",
				"Offset  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f  ",
				hex_alphabet_lower);
		}
		if (i == 0)
			fprintf(out, "%5d  ", x & 0xffff);	/* offset, lowest 16 bits */
		if (end) {
			fputs("   ", out);
			temp[i] = ' ';
		} else {
			z = buf[x] & 0xff;
			fprintf(out, "%.2X ", z);
			if (!(isalnum(z) || ispunct(z)))
				z = '.';		/* no non printables */
			temp[i] = z;		/* save it for later ASCII print */
		}
		if (++i >= 16) {
			fputc(' ', out);
			for (y = 0; y < 16; y++) {	/* do 16 bytes ASCII */
				fputc(temp[y], out);
			}
			fputc('\n', out);
			if (end || ((x + 1) >= bufLen))
				break;
			i = 0;
		}
		if (++x >= bufLen)
			end = 1;
	}

	fprintf(out, "dump_hex(): end dump of %s (%d bytes).\n", title, bufLen);
	fflush(out);
}


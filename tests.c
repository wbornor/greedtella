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
 * $Id: tests.c,v 1.1.1.1 2005/01/31 21:50:19 wbornor Exp $
 ************************************************************************/

#include "greedtella.h"
#include "errno.h"
#include "regex.h"

#define IP_ADDR_GOOD "192.168.0.101"
#define IP_ADDR_BAD "0.0.0.5"
#define NUM_FILES_SHARED 1
#define NUM_KB_SHARED 5000


int testConnectClose()
{
	struct sockaddr_in server;
	int sd;

	memset(&server, 0,  sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = htons(DFLT_PORT_IN);
	if(inet_pton(AF_INET, IP_ADDR_GOOD, &server.sin_addr) <= 0)
	{
		printf("tests.c: inet_pton() failed\n");
		return(0);
	}

	//try to connect to a actual servent
	//should work
	sd = gConnect(&server);
	if(sd < 0)
	{
		printf("tests.c: gConnect() to real servent failed\n");
		close(sd);
		return(0);
	}
	

	//connect twice to a bonified servent. should work
	/*
	sd = gConnect(&server);
	if(sd < 0)
	{
		printf("tests.c: gConnect() to real servent twice failed\n");
		close(sd);
		return(0);
	}

	*/

	gClose(sd);

	
	if(inet_pton(AF_INET, IP_ADDR_BAD, &server.sin_addr) <= 0)
	{
		printf("testconnect.c: inet_pton() failed\n");
		close(sd);
		return(0);
	}
	
	//try to connect to a bogus servent
	//should fail
	sd = gConnect(&server);
	if(sd != -1)
	{
		close(sd);
		printf("testconnect.c: gConnect() to bogus servent worked, test failed\n");
		return(0);
	}


	return(1);

}

int setupSocket(struct sockaddr_in *server)
{
	memset(server, 0,  sizeof(server));
	server->sin_family = AF_INET;
	server->sin_port = htons(DFLT_PORT_IN);
	if(inet_pton(AF_INET, IP_ADDR_GOOD, &(server->sin_addr)) <= 0)
	{
		printf("tests.c: inet_pton() failed\n");
		return(0);
	}

	return(1);
}

int testSetupSocket()
{
	struct sockaddr_in server;

	if(!setupSocket(&server))
	{
		printf("tests.c: testsetupSocket() failed\n");
		return(0);
	}
	return(1);
}

int generalPingPongTest()
{
	struct sockaddr_in server;
	descHdr_t inHdr;
	int sd;
	char buf[1024];

	//connect
	setupSocket(&server);
	sd = gConnect(&server);

	//sleep(5);

	//wait for an incoming PING
	if(gRecvPktHdr(sd, &inHdr) < 0)
	{
		
		close(sd);
		printf("tests.c: generalPingPongTest() failed\n");
		return(0);
	}

	//reply with a PONG
	if(inHdr.payloadDesc == PING)
	{
		if(gSendPong(sd, &inHdr.msgId, IP_ADDR_GOOD, NUM_FILES_SHARED, NUM_KB_SHARED) < 0)
		{
			close(sd);
			printf("tests.c generalPingPongTest() gSendPong() failed\n");
			return(0);
		}
	}


	//send a PING
	if(gSendPing(sd) < 0)
	{
		close(sd);
		printf("tests.c generalPingPongTest() gSendPing() failed\n");
		return(0);
	}
		
	//receive the resulting PONG header
	if(gRecvPktHdr(sd, &inHdr) < 0)
	{
		
		close(sd);
		printf("tests.c: generatTest() failed\n");
		return(0);
	}

	//receive the resulting PONG body
	if(inHdr.payloadDesc == PONG)
	{
		if(gRecvPktBody(sd, buf, inHdr.payloadLen) < 0)
		{
			close(sd);
			printf("tests.c generalPingPongTest() gSendPong() failed\n");
			return(0);
		}
	}

	//sleep(5);
	gClose(sd);

	return(1);

}

int testQuerySend()
{
	struct sockaddr_in server;
	int sd;
	descHdr_t inHdr;
	
	setupSocket(&server);
	sd = gConnect(&server);

	//take care of the incoming PING
	gRecvPktHdr(sd, &inHdr);
	if(inHdr.payloadDesc == PING)
		gSendPong(sd, &inHdr.msgId, IP_ADDR_GOOD, NUM_FILES_SHARED, NUM_KB_SHARED);


	//good query, should work
	if(gSendQuery(sd, "animalhouse") < 0)
	{
		gClose(sd);
		printf("tests.c: gSendQuery() failed\n");
		return(0);
	}

	//bad socket descriptor, should fail
	if(gSendQuery(-1, "animal house") != -1)
	{
		gClose(sd);
		printf("tests.c: gSendQuery() should have failed due to invalid params\n");
		return(0);
	}

	//bad query criteria, should fail
	if(gSendQuery(sd, 0) != -1)
	{
		gClose(sd);
		printf("tests.c: gSendQuery() should have failed due to invalid params\n");
		return(0);
	}

	gClose(sd);

	return(1);
}

int testQueryHit()
{
	struct sockaddr_in server;
	int sd, i;
	descHdr_t inHdr;
	queryHitPkt_t queryHitPkt;
	
	setupSocket(&server);
	sd = gConnect(&server);

	//take care of the incoming PING
	gRecvPktHdr(sd, &inHdr);
	if(inHdr.payloadDesc == PING)
		gSendPong(sd, &inHdr.msgId, IP_ADDR_GOOD, NUM_FILES_SHARED, NUM_KB_SHARED);


	gSendQuery(sd, "animalhouse");
	
	gRecvPktHdr(sd, &queryHitPkt.header);
	if(queryHitPkt.header.payloadDesc == QUERYHIT)
	{
		if(gRecvQueryHit(sd, &queryHitPkt) < 0)
		{
			gClose(sd);
			printf("tests.c: gRecvQueryHit() failed\n");
			return(0);
		}

		printf("testQueryHit() Hit Dump (numHits: %d)\n", queryHitPkt.numHits);

		for(i=0; i<queryHitPkt.numHits; i++)
		{
			printf("hitNum: %d\n", i);
			printf("fIndex: %d\nfSize: %d\nfName: %s\n", 
					queryHitPkt.results[i].fIndex,
					queryHitPkt.results[i].fSize,
					queryHitPkt.results[i].fName);

		}
	}

	gClose(sd);

	return(1);
}

int testHttpGet()
{
	struct sockaddr_in server;
	int sd;
	char buf[1024];

	memset((void*)buf, '\0', 1024);
	
	setupSocket(&server);
	sd = httpConnect(&server);

	if(sd < 0)
	{
		printf("tests.c: testHttpGet(): httpConnect() failed\n");
		return(0);
	}

	if(httpGet(sd, 2, 2, "animalhouse.txt", buf) < 0)
	{
			close(sd);
			printf("tests.c: testHttpGet(): httpGet() failed\n");
			return(0);
	}

	close(sd);

	return(1);
}
int main()
{	
	/*
	if(!testConnectClose()) return(1);
	if(!testSetupSocket()) return(1); 
	if(!generalPingPongTest()) return(1);
	if(!testQuerySend()) return(1);
	if(!testQueryHit()) return(1);
	*/

	if(!testHttpGet()) return(1);

	return(0);
}

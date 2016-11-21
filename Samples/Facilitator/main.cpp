#include "RakPeerInterface.h"
#include "RakSleep.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <string.h>
#include "Kbhit.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "RakSleep.h"
#include "UDPProxyServer.h"
#include "UDPProxyCoordinator.h"
#include "NatPunchthroughServer.h"
#include "NatTypeDetectionServer.h"
#include "SocketLayer.h"
#include "Getche.h"
#include "Gets.h"
#include "RakNetStatistics.h"
#include "RelayPlugin.h"

//#define VERBOSE_LOGGING

using namespace RakNet;

static int DEFAULT_RAKPEER_PORT=61111;

char* getCmdOption(char ** begin, int count, const std::string & option)
{
	char ** end = begin + count;
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool cmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

int main(int argc, char **argv)
{
	RakNet::RakPeerInterface *rakPeer=RakNet::RakPeerInterface::GetInstance();
	
	const char* ip1 = NULL;
	const char* ip2 = NULL;

	char* ipArg = getCmdOption(argv, argc, "-i");
    if (ipArg != NULL)
	{
		// Ip was passed in
		ip1 = strtok(ipArg, ",");
		ip2 = strtok(NULL, ",");
	}
	else
	{
		printf("IPs:\n");
	  
		SystemAddress ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ];
		unsigned int i;
		for (i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
		{
			const char* localIP = rakPeer->GetLocalIP(i);
			ipList[i]=localIP;
			ipList[i] = rakPeer->GetLocalIP(i);
			if (strcmp(localIP, UNASSIGNED_SYSTEM_ADDRESS.ToString(false)) != 0)
				printf("%i. %s\n", i+1, localIP);
			else
				break;
		}

		ip1 = ipList[0].ToString(false);
		if (i>=2)
		{
			ip2 = ipList[1].ToString(false);
		}
	}

	char* portArg = getCmdOption(argv, argc, "-p");
    
  // If RakPeer is started on 2 IP addresses, NATPunchthroughServer supports port stride detection, improving success rate
  int sdLen=1;
  RakNet::SocketDescriptor sd[2];
  if (portArg != NULL)
  {
    DEFAULT_RAKPEER_PORT = atoi(portArg);
  }
  
  sd[0].port=DEFAULT_RAKPEER_PORT;
  strcpy(sd[0].hostAddress, ip1);
  printf("Using port %i\n", sd[0].port);
  printf("Using ip %s\n", sd[0].hostAddress);
  if (ip2 != NULL)
  {
    sd[1].port=DEFAULT_RAKPEER_PORT+1;
    strcpy(sd[1].hostAddress, ip2);
	printf("Using port %i\n", sd[1].port);
	printf("Using ip %s\n", sd[1].hostAddress);
    sdLen=2;
  }

  if (rakPeer->Startup(8096,sd,sdLen)!=RakNet::RAKNET_STARTED)
  {
    printf("Failed to start rakPeer! Quitting\n");
    RakNet::RakPeerInterface::DestroyInstance(rakPeer);
    return 1;
  }
  rakPeer->SetTimeoutTime(5000, UNASSIGNED_SYSTEM_ADDRESS);
  printf("Started on %s\n", rakPeer->GetMyBoundAddress().ToString(true));
  printf("\n");

  rakPeer->SetMaximumIncomingConnections(8096);

  NatPunchthroughServer *nps = new NatPunchthroughServer;
  rakPeer->AttachPlugin(nps);

  printf("\nEntering update loop. Press 'q' to quit.\n");

  RakNet::Packet *packet;
  bool quit=false;
  while (!quit)
  {
    for (packet=rakPeer->Receive(); packet; rakPeer->DeallocatePacket(packet), packet=rakPeer->Receive())
    {
		switch(packet->data[0])
		{
			case ID_NEW_INCOMING_CONNECTION:
				printf("Got a connection from %s with guid: %s\n", packet->systemAddress.ToString(), packet->guid.ToString());
				break;
			case ID_CONNECTION_LOST:
				printf(" Lost connection with %s with guid: %s\n", packet->systemAddress.ToString(), packet->guid.ToString());
				break;
      case ID_DISCONNECTION_NOTIFICATION:
				printf("Got a disconnect from %s with guid: %s\n", packet->systemAddress.ToString(), packet->guid.ToString());
				break;
			default:
				int msgType = (int)packet->data[0];
				printf("Received code %i from %s with guid: %s\n", msgType, packet->systemAddress.ToString(), packet->guid.ToString());
		}
    }

    if (kbhit())
    {
      char ch = getch();
      if (ch=='q')
      {
        quit=true;
      }
    }
    RakSleep(30);
  }

  printf("Quitting.\n");
  if (nps)
  {
    rakPeer->DetachPlugin(nps);
    delete nps;
  }
  nps=0;
  rakPeer->Shutdown(100);
  RakNet::RakPeerInterface::DestroyInstance(rakPeer);

  return 0;
}



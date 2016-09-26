#include "RakPeerInterface.h"
#include "RakSleep.h"
#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char **argv)
{
  RakNet::RakPeerInterface *rakPeer=RakNet::RakPeerInterface::GetInstance();
  
  SystemAddress ipList[ MAXIMUM_NUMBER_OF_INTERNAL_IDS ];
  printf("IPs:\n");
  unsigned int i;
  for (i=0; i < MAXIMUM_NUMBER_OF_INTERNAL_IDS; i++)
  {
	  printf ("UIP@: %s\n", rakPeer->GetLocalIP(i));
    ipList[i]=rakPeer->GetLocalIP(i);
	printf ("hkhjks: %s\n", ipList[i].ToString());
    if (ipList[i]!=UNASSIGNED_SYSTEM_ADDRESS)
      printf("%i. %s\n", i+1, ipList[i].ToString(false));
    else
      break;
  }
    
  // If RakPeer is started on 2 IP addresses, NATPunchthroughServer supports port stride detection, improving success rate
  int sdLen=1;
  RakNet::SocketDescriptor sd[2];
  if (argc>1)
  {
    DEFAULT_RAKPEER_PORT = atoi(argv[1]);
  }
  
  sd[0].port=DEFAULT_RAKPEER_PORT;
  strcpy(sd[0].hostAddress, ipList[0].ToString(false));
  printf("Using port %i\n", sd[0].port);
  if (i>=2)
  {
    sd[1].port=DEFAULT_RAKPEER_PORT+1;
    strcpy(sd[1].hostAddress, ipList[1].ToString(false));
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
			default:
				int msgType = (int)packet->data[0];
				printf("Received message ID: %i from %s with guid %s", msgType, packet->systemAddress.ToString(), packet->guid.ToString());
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



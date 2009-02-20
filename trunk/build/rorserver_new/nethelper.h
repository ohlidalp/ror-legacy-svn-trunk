#ifndef HELPER_H__
#define HELPER_H__

#include "RakNetworkFactory.h"
#include "RakPeerInterface.h"
#include "MessageIdentifiers.h"
#include "BitStream.h"
#include "rornetv2.h"

int createBitStream(RakPeerInterface *peer, RakNet::BitStream &stream, unsigned char type, unsigned char sender, unsigned long size);
int sendmessage(RakPeerInterface *peer, SystemAddress address, int type, unsigned int size, char *buffer);
int receivemessage(RakPeerInterface *peer, unsigned char *contentType, unsigned char *source, unsigned long *contentSize, char *buffer, unsigned int bufferSize);
unsigned char getPacketIdentifier(Packet *p);

#endif

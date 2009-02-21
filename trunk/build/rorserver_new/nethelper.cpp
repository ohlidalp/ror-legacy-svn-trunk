#include "nethelper.h"
#include "GetTime.h" //raknet::gettime

using namespace RakNet;

int createBitStream(RakPeerInterface *peer, RakNet::BitStream &stream, unsigned char type, unsigned char sender, unsigned long size)
{
	// write basics into it
	// fist that its a timestamped packet
	stream.Write((unsigned char) ID_TIMESTAMP);
	// second the timestamp itself
	stream.Write((unsigned long) RakNet::GetTime());
	// third our message ID, can conflict with the predefined raknet types, so we use one for all RoR data!
	stream.Write((unsigned char) ROR_DATA_MSG);
	// third our very own message ID
	stream.Write((unsigned char) type);
	// who is the sender?
	stream.Write((unsigned char) sender);
	// third our message length
	stream.Write((unsigned long) size);
	return 0;
}

int sendmessage(RakPeerInterface *peer, SystemAddress address, int type, char source, unsigned int size, char *buffer)
{
	BitStream stream;
	createBitStream(peer, stream, type, source, size);
	
	stream.SerializeBits(true, (unsigned char*)buffer, size*8);

	peer->Send(&stream, MEDIUM_PRIORITY, RELIABLE_SEQUENCED, 0, address, false);
	return 0;
}

int receivemessage(RakPeerInterface *peer, unsigned char *contentType, unsigned char *source, unsigned long *contentSize, char *buffer, unsigned int bufferSize)
{
	int result = 1;
	bool run = true;
	int retries = 10000;
	while(run && retries > 0)
	{
		Packet *packet = peer->Receive();
		if (packet)
		{
			int id = getPacketIdentifier(packet);
			printf("got packed %d\n", id);
			if(id == ROR_DATA_MSG)
			{
				BitStream stream(packet->data, packet->length, false);
				
				// read basic stuff
				unsigned char useTimeStamp=0, typeId=0;
				unsigned long timeStamp=0, size=0;
				int mySize=0;
				stream.Read(useTimeStamp);
				stream.Read(timeStamp);
				stream.Read(typeId); // should be ROR_DATA_MSG
				stream.Read(*contentType);
				stream.Read(*contentSize);
				// TODO: add warning!
				if(*contentSize < bufferSize)
				{
					stream.SerializeBits(false, (unsigned char*)buffer, *contentSize*8);
					result = 0;
				} else 
					result = 2;
				run=false;
			}

			retries--;
			peer->DeallocatePacket(packet);
		}
	}
	if(retries <=0 )
		result = 3;
	return result;
}

// get correct packet type, even with timestamp
unsigned char getPacketIdentifier(Packet *p)
{
    if (p==0)
        return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP)
        return (unsigned char) p->data[sizeof(unsigned char) + sizeof(unsigned long)];
    else
        return (unsigned char) p->data[0];
}
/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef NETWORK_H__
#define NETWORK_H__

#define MAX_CLIENTS 10
#define SERVER_PORT 31117

#include <iostream>
#include <ctime>
#include <cstdlib>


#define RANDOM_DATA_MSG ID_USER_PACKET_ENUM
using namespace std;


// fill a buffer with random data
void random_numbers(unsigned int size, unsigned char *ptr)
{
    srand((unsigned)time(0));
    for(int index=0; index<size; index++, ptr++)
        (*ptr) = int(255.0f*rand()/(RAND_MAX + 1.0f));
}

// hex 'editor like' display to check the content of a buffer
void display_buffer(unsigned int size, unsigned char *buf)
{
    unsigned int c1=1;
    printf("%06x ", 0);
    for(unsigned int c=0;c<size;c++, c1++)
    {
        printf("%02x ", *(buf+c));
        if(c1>20)
        {
            printf("\n%06x ", c);
            c1=0;
        }
    }
    printf("\n");
}


// get correct packet type, even with timestamp
unsigned char GetPacketIdentifier(Packet *p)
{
    if (p==0)
        return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP)
        return (unsigned char) p->data[sizeof(unsigned char) + sizeof(unsigned long)];
    else
        return (unsigned char) p->data[0];
}
#endif

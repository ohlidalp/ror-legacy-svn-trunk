/*
This source file is part of Rigs of Rods
Copyright 2005-2011 Pierre-Michel Ricordel
Copyright 2007-2011 Thomas Fischer

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
#include "utils.h"

#include <Ogre.h>

#include "RoRPrerequisites.h"
#include "rornet.h"
#include "RoRVersion.h"

#include <fstream>

using namespace Ogre;

String hexdump(void *pAddressIn, long  lSize)
{
	char szBuf[100];
	long lIndent = 1;
	long lOutLen, lIndex, lIndex2, lOutLen2;
	long lRelPos;
	struct { char *pData; unsigned long lSize; } buf;
	unsigned char *pTmp,ucTmp;
	unsigned char *pAddress = (unsigned char *)pAddressIn;

	buf.pData   = (char *)pAddress;
	buf.lSize   = lSize;

	String result = "";
	
	while (buf.lSize > 0)
	{
		pTmp     = (unsigned char *)buf.pData;
		lOutLen  = (int)buf.lSize;
		if (lOutLen > 16)
		  lOutLen = 16;

		// create a 64-character formatted output line:
		sprintf(szBuf, " >                            "
					 "                      "
					 "    %08lX", (long unsigned int)(pTmp-pAddress));
		lOutLen2 = lOutLen;

		for(lIndex = 1+lIndent, lIndex2 = 53-15+lIndent, lRelPos = 0;
		  lOutLen2;
		  lOutLen2--, lIndex += 2, lIndex2++
			)
		{
			ucTmp = *pTmp++;

			sprintf(szBuf + lIndex, "%02X ", (unsigned short)ucTmp);
			if(!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
			szBuf[lIndex2] = ucTmp;

			if (!(++lRelPos & 3))     // extra blank after 4 bytes
			{  lIndex++; szBuf[lIndex+2] = ' '; }
		}

		if (!(lRelPos & 3)) lIndex--;

		szBuf[lIndex  ]   = '<';
		szBuf[lIndex+1]   = ' ';

		result += String(szBuf) + "\n";

		buf.pData   += lOutLen;
		buf.lSize   -= lOutLen;
	}
	return result;
}

UTFString tryConvertUTF(char *buffer)
{
	try
	{
		UTFString s = UTFString(buffer);
		if(s.empty())
			s = UTFString("(conversion error)");
		return s;

	} catch(...)
	{
		return UTFString();
	}
	return UTFString();
}

Ogre::String formatBytes(double bytes)
{
	char tmp[128]="";
	const char *si_prefix[] = { "B", "KB", "MB", "GB", "TB", "EB", "ZB", "YB" };
	int base = 1024;
	int c = std::min((int)(log(bytes)/log(base)), (int)sizeof(si_prefix) - 1);
	sprintf(tmp, "%1.2f %s", bytes / pow(base, c), si_prefix[c]);
	return Ogre::String(tmp);
}

// replace non-ASCII characters with underscores to prevent std::string problems
Ogre::String getASCIIFromCharString(char *str, int maxlen)
{
	char *ptr = str;
	for(int i=0; i < maxlen; i++, ptr++)
	{
		if(*ptr == 0) break;
		if(*ptr < 32 || *ptr > 126)
		{
			*ptr = 95;
		}
	}
	str[maxlen] = 0;
	return std::string(str);
}

// replace non-ASCII characters with underscores to prevent std::string problems
Ogre::String getASCIIFromOgreString(Ogre::String s, int maxlen)
{
	char str[1024] = "";
	strncpy(str, s.c_str(), 1023);
	char *ptr = str;
	for(int i=0; i < maxlen; i++, ptr++)
	{
		if(*ptr == 0) break;
		if(*ptr < 32 || *ptr > 126)
		{
			*ptr = 95;
		}
	}
	str[maxlen] = 0;
	return std::string(str);
}

int getTimeStamp()
{
	return (int)time(NULL); //this will overflow in 2038
}


Ogre::String getVersionString()
{
	char tmp[1024] = "";
	sprintf(tmp, "Rigs of Rods\n"
		" version: %s\n"
		" revision: %s\n"
		" full revision: %s\n"
		" protocol version: %s\n"
		" build time: %s, %s\n"
		, ROR_VERSION_STRING, SVN_REVISION, SVN_ID, RORNET_VERSION, __DATE__, __TIME__);
	return Ogre::String(tmp);
}

bool fileExists(std::string filename)
{
	// be careful about what you use here...
	FILE *f = fopen(filename.c_str(), "r");
	if(!f)
		return false;
	fclose(f);
	return true;
}

int isPowerOfTwo (unsigned int x)
{
  return ((x != 0) && ((x & (~x + 1)) == x));
}

// same as: return preg_replace("/[^A-Za-z0-9-_.]/", "_", $s);
Ogre::String stripNonASCII(Ogre::String s)
{
	char filename[9046] = "";
	sprintf(filename, "%s", s.c_str());
	for(size_t i=0;i<s.size(); i++)
	{
		bool replace = true;
		if     (filename[i] >= 48 && filename[i] <= 57) replace = false; // 0-9
		else if(filename[i] >= 65 && filename[i] <= 90) replace = false; // A-Z
		else if(filename[i] >= 97 && filename[i] <= 122) replace = false; // a-z
		else if(filename[i] == 45 || filename[i] == 95 || filename[i] == 46) replace = false; // -_.
		if(replace)
			filename[i]='_';
	}
	return Ogre::String(filename);
}
	
 
bool compareCaseInsensitive(std::string strFirst, std::string strSecond)
{
	// Convert both strings to upper case by transfrom() before compare.
	transform(strFirst.begin(), strFirst.end(), strFirst.begin(), toupper);
	transform(strSecond.begin(), strSecond.end(), strSecond.begin(), toupper);
	return (strFirst == strSecond);
}


Ogre::AxisAlignedBox getWorldAABB(Ogre::SceneNode* node)
{
	AxisAlignedBox aabb;

	// merge with attached objects
	for (int i=0; i<node->numAttachedObjects(); ++i)
	{
		MovableObject* o = node->getAttachedObject(i);
		aabb.merge(o->getWorldBoundingBox(true));
	}

	// merge with child nodes
	for (int i=0; i<node->numChildren(); ++i)
	{
		SceneNode* child = static_cast<SceneNode*>(node->getChild(i));
		aabb.merge( getWorldAABB(child) );
	}

	return aabb;
}
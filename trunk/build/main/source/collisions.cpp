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
#include "collisions.h"
#include "Settings.h"
#include "Landusemap.h"
#include "approxmath.h"

//these default values are overwritten by the data/ground_models.cfg file!
ground_model_t GROUND_CONCRETE={3.0, 1.20, 0.60, 0.010, 5.0, 2.0, 0.5, 0};
ground_model_t GROUND_ASPHALT={ 3.0, 1.10, 0.55, 0.010, 5.0, 2.0, 0.5, 0};
ground_model_t GROUND_GRAVEL={  3.0, 0.85, 0.50, 0.010, 4.0, 2.0, 0.5, 0};
ground_model_t GROUND_ROCK={    3.0, 0.75, 0.40, 0.010, 5.0, 2.0, 0.5, 0};
ground_model_t GROUND_ICE={     3.0, 0.25, 0.15, 0.0001,4.0, 2.0, 0.5, 0};
ground_model_t GROUND_SNOW={    3.0, 0.55, 0.30, 0.005, 6.0, 3.0, 0.5, 0};
ground_model_t GROUND_METAL={   3.0, 0.40, 0.20, 0.001, 4.0, 2.0, 0.5, 0};
ground_model_t GROUND_GRASS={   3.0, 0.50, 0.25, 0.005, 8.0, 2.0, 0.5, 0};
ground_model_t GROUND_SAND={    3.0, 0.40, 0.20, 0.0001,9.0, 2.0, 0.5, 0};

extern ground_model_t *ground_models[9] = {&GROUND_CONCRETE, &GROUND_ASPHALT, &GROUND_GRAVEL, &GROUND_ROCK, &GROUND_ICE, &GROUND_SNOW, &GROUND_METAL, &GROUND_GRASS, &GROUND_SAND};

//hash function SBOX
//from http://home.comcast.net/~bretm/hash/10.html
unsigned int sbox[] =
    {
    0xF53E1837, 0x5F14C86B, 0x9EE3964C, 0xFA796D53,
    0x32223FC3, 0x4D82BC98, 0xA0C7FA62, 0x63E2C982,
    0x24994A5B, 0x1ECE7BEE, 0x292B38EF, 0xD5CD4E56,
    0x514F4303, 0x7BE12B83, 0x7192F195, 0x82DC7300,
    0x084380B4, 0x480B55D3, 0x5F430471, 0x13F75991,
    0x3F9CF22C, 0x2FE0907A, 0xFD8E1E69, 0x7B1D5DE8,
    0xD575A85C, 0xAD01C50A, 0x7EE00737, 0x3CE981E8,
    0x0E447EFA, 0x23089DD6, 0xB59F149F, 0x13600EC7,
    0xE802C8E6, 0x670921E4, 0x7207EFF0, 0xE74761B0,
    0x69035234, 0xBFA40F19, 0xF63651A0, 0x29E64C26,
    0x1F98CCA7, 0xD957007E, 0xE71DDC75, 0x3E729595,
    0x7580B7CC, 0xD7FAF60B, 0x92484323, 0xA44113EB,
    0xE4CBDE08, 0x346827C9, 0x3CF32AFA, 0x0B29BCF1,
    0x6E29F7DF, 0xB01E71CB, 0x3BFBC0D1, 0x62EDC5B8,
    0xB7DE789A, 0xA4748EC9, 0xE17A4C4F, 0x67E5BD03,
    0xF3B33D1A, 0x97D8D3E9, 0x09121BC0, 0x347B2D2C,
    0x79A1913C, 0x504172DE, 0x7F1F8483, 0x13AC3CF6,
    0x7A2094DB, 0xC778FA12, 0xADF7469F, 0x21786B7B,
    0x71A445D0, 0xA8896C1B, 0x656F62FB, 0x83A059B3,
    0x972DFE6E, 0x4122000C, 0x97D9DA19, 0x17D5947B,
    0xB1AFFD0C, 0x6EF83B97, 0xAF7F780B, 0x4613138A,
    0x7C3E73A6, 0xCF15E03D, 0x41576322, 0x672DF292,
    0xB658588D, 0x33EBEFA9, 0x938CBF06, 0x06B67381,
    0x07F192C6, 0x2BDA5855, 0x348EE0E8, 0x19DBB6E3,
    0x3222184B, 0xB69D5DBA, 0x7E760B88, 0xAF4D8154,
    0x007A51AD, 0x35112500, 0xC9CD2D7D, 0x4F4FB761,
    0x694772E3, 0x694C8351, 0x4A7E3AF5, 0x67D65CE1,
    0x9287DE92, 0x2518DB3C, 0x8CB4EC06, 0xD154D38F,
    0xE19A26BB, 0x295EE439, 0xC50A1104, 0x2153C6A7,
    0x82366656, 0x0713BC2F, 0x6462215A, 0x21D9BFCE,
    0xBA8EACE6, 0xAE2DF4C1, 0x2A8D5E80, 0x3F7E52D1,
    0x29359399, 0xFEA1D19C, 0x18879313, 0x455AFA81,
    0xFADFE838, 0x62609838, 0xD1028839, 0x0736E92F,
    0x3BCA22A3, 0x1485B08A, 0x2DA7900B, 0x852C156D,
    0xE8F24803, 0x00078472, 0x13F0D332, 0x2ACFD0CF,
    0x5F747F5C, 0x87BB1E2F, 0xA7EFCB63, 0x23F432F0,
    0xE6CE7C5C, 0x1F954EF6, 0xB609C91B, 0x3B4571BF,
    0xEED17DC0, 0xE556CDA0, 0xA7846A8D, 0xFF105F94,
    0x52B7CCDE, 0x0E33E801, 0x664455EA, 0xF2C70414,
    0x73E7B486, 0x8F830661, 0x8B59E826, 0xBB8AEDCA,
    0xF3D70AB9, 0xD739F2B9, 0x4A04C34A, 0x88D0F089,
    0xE02191A2, 0xD89D9C78, 0x192C2749, 0xFC43A78F,
    0x0AAC88CB, 0x9438D42D, 0x9E280F7A, 0x36063802,
    0x38E8D018, 0x1C42A9CB, 0x92AAFF6C, 0xA24820C5,
    0x007F077F, 0xCE5BC543, 0x69668D58, 0x10D6FF74,
    0xBE00F621, 0x21300BBE, 0x2E9E8F46, 0x5ACEA629,
    0xFA1F86C7, 0x52F206B8, 0x3EDF1A75, 0x6DA8D843,
    0xCF719928, 0x73E3891F, 0xB4B95DD6, 0xB2A42D27,
    0xEDA20BBF, 0x1A58DBDF, 0xA449AD03, 0x6DDEF22B,
    0x900531E6, 0x3D3BFF35, 0x5B24ABA2, 0x472B3E4C,
    0x387F2D75, 0x4D8DBA36, 0x71CB5641, 0xE3473F3F,
    0xF6CD4B7F, 0xBF7D1428, 0x344B64D0, 0xC5CDFCB6,
    0xFE2E0182, 0x2C37A673, 0xDE4EB7A3, 0x63FDC933,
    0x01DC4063, 0x611F3571, 0xD167BFAF, 0x4496596F,
    0x3DEE0689, 0xD8704910, 0x7052A114, 0x068C9EC5,
    0x75D0E766, 0x4D54CC20, 0xB44ECDE2, 0x4ABC653E,
    0x2C550A21, 0x1A52C0DB, 0xCFED03D0, 0x119BAFE2,
    0x876A6133, 0xBC232088, 0x435BA1B2, 0xAE99BBFA,
    0xBB4F08E4, 0xA62B5F49, 0x1DA4B695, 0x336B84DE,
    0xDC813D31, 0x00C134FB, 0x397A98E6, 0x151F0E64,
    0xD9EB3E69, 0xD3C7DF60, 0xD2F2C336, 0x2DDD067B,
    0xBD122835, 0xB0B3BD3A, 0xB0D54E46, 0x8641F1E4,
    0xA0B38F96, 0x51D39199, 0x37A6AD75, 0xDF84EE41,
    0x3C034CBA, 0xACDA62FC, 0x11923B8B, 0x45EF170A,
    };

Collisions::Collisions(
#ifdef LUASCRIPT
  LuaSystem *mlua,
#endif
  ExampleFrameListener *efl, bool _debugMode) : mefl(efl)
{
	landuse=0;
	debugMode=_debugMode;
	last_used_ground_model=&GROUND_GRAVEL;
#ifdef LUASCRIPT
	lua=mlua;
#endif
	free_collision_box=0;
	free_collision_tri=0;
	free_eventsource=0;
	free_cell=0;
	forcecam=false;
	hashmask=0;
	hfinder=0;
	collision_count=0;
	largest_cellcount=0;
	for (int i=0; i<HASH_SIZE; i++) {hashmask=hashmask<<1; hashmask++;};
	for (int i=0; i<(1<<HASH_SIZE); i++) hashtable[i].cellid=UNUSED_CELLID;
	loadDefaultModels();
}

void Collisions::loadDefaultModels()
{
	FILE *fd;
	char line[1024];
	fd=fopen((SETTINGS.getSetting("Config Root")+"ground_models.cfg").c_str(), "r");
	while (!feof(fd))
	{
		fscanf(fd," %[^\n\r]",line);
		if (line[0]==';')
			continue;
		loadGroundModelLine(line);
	}
	fclose(fd);
}

void Collisions::setupLandUse(char *configfile)
{
	if(landuse) return;
	landuse = new Landusemap(configfile, this, mefl->mapsizex, mefl->mapsizez);
}

ground_model_t *Collisions::getGroundModelByString(char *stdf)
{
	if (!strcmp("concrete", stdf)) return &GROUND_CONCRETE;
	if (!strcmp("asphalt", stdf)) return &GROUND_ASPHALT;
	if (!strcmp("gravel", stdf)) return &GROUND_GRAVEL;
	if (!strcmp("rock", stdf)) return &GROUND_ROCK;
	if (!strcmp("ice", stdf)) return &GROUND_ICE;
	if (!strcmp("snow", stdf)) return &GROUND_SNOW;
	if (!strcmp("metal", stdf)) return &GROUND_METAL;
	if (!strcmp("grass", stdf)) return &GROUND_GRASS;
	if (!strcmp("sand", stdf)) return &GROUND_SAND;
	return &GROUND_GRAVEL; // default
}

int Collisions::getGroundModelNumberByString(char *stdf)
{
	if (!strcmp("concrete", stdf)) return 0;
	if (!strcmp("asphalt", stdf)) return 1;
	if (!strcmp("gravel", stdf)) return 2;
	if (!strcmp("rock", stdf)) return 3;
	if (!strcmp("ice", stdf)) return 4;
	if (!strcmp("snow", stdf)) return 5;
	if (!strcmp("metal", stdf)) return 6;
	if (!strcmp("grass", stdf)) return 7;
	if (!strcmp("sand", stdf)) return 8;
	return 2; // default
}


void Collisions::loadGroundModelLine(char *line)
{
	char stdf[256];
	ground_model_t *gm=0;
	sscanf(line,"%s ",stdf);
	gm = getGroundModelByString(stdf);
	if (gm)
	{
		Ogre::LogManager::getSingleton().logMessage("COLL: parsing default model for '"+String(stdf)+"'");
		parseGroundModel(gm, line+strlen(stdf)+1, stdf);
//		Ogre::LogManager::getSingleton().logMessage("COLL: this model has "+Ogre::StringConverter::toString((unsigned int)gm)+" "+Ogre::StringConverter::toString(gm->fx_type));

	}
}

void Collisions::parseGroundModel(ground_model_t* gm, char* line, char *name)
{
	float va, ms, mc, t2, vs, alpha, strength;
	char fxt[256];
	strcpy(fxt, "none");
	float fxr=0.83;
	float fxg=0.71;
	float fxb=0.64;
	int nbe=sscanf(line,"%f, %f, %f, %f, %f, %f, %f, %s %f, %f, %f",&va, &ms,&mc,&t2,&vs,&alpha,&strength, fxt, &fxr, &fxg, &fxb);
	if (nbe<8) Ogre::LogManager::getSingleton().logMessage("COLL: ERROR too few parameters while parsing ground model");
	gm->va=va;
	gm->ms=ms;
	strncpy(gm->name, name, 255);
	gm->mc=mc;
	gm->t2=t2;
	gm->vs=vs;
	gm->alpha=alpha;
	gm->strength=strength;
	int fxtn=-1;
	if (!strcmp("none", fxt)) fxtn=FX_NONE;
	if (!strcmp("hard", fxt)) fxtn=FX_HARD;
	if (!strcmp("dusty", fxt)) fxtn=FX_DUSTY;
	if (!strcmp("clumpy", fxt)) fxtn=FX_CLUMPY;
	if (fxtn==-1)
	{
		Ogre::LogManager::getSingleton().logMessage("COLL: ERROR bad FX name ('"+String(fxt)+"')");
		fxtn=FX_NONE;
	}
	gm->fx_type=fxtn;
	gm->fx_coulour=ColourValue(fxr, fxg, fxb, 1.0);
}


void Collisions::setHfinder(HeightFinder *hfi)
{
	hfinder=hfi;
}

unsigned int Collisions::hashfunc(unsigned int cellid)
{
	unsigned int hash = 0;
	for (int i=0; i<4; i++)
	{
		hash^=sbox[((unsigned char*)&cellid)[i]];
		hash*=3;
	}
	return hash&hashmask;
}

int Collisions::removeCollisionTri(int number)
{
	if(number>free_collision_tri)
		return -1;

	Vector3 p1 = collision_tris[number].a;
	Vector3 p2 = collision_tris[number].b;
	Vector3 p3 = collision_tris[number].c;
	ground_model_t* gm = collision_tris[number].gm;

	//compute tri AAB
	AxisAlignedBox aab;
	aab.merge(p1);
	aab.merge(p2);
	aab.merge(p3);
	//register this collision tri in the index
	int ilox, ihix, iloz, ihiz;
	ilox=(int)(aab.getMinimum().x/(float)CELL_SIZE);
	if (ilox<0) ilox=0; if (ilox>MAXIMUM_CELL) ilox=MAXIMUM_CELL;
	ihix=(int)(aab.getMaximum().x/(float)CELL_SIZE);
	if (ihix<0) ihix=0; if (ihix>MAXIMUM_CELL) ihix=MAXIMUM_CELL;
	iloz=(int)(aab.getMinimum().z/(float)CELL_SIZE);
	if (iloz<0) iloz=0; if (iloz>MAXIMUM_CELL) iloz=MAXIMUM_CELL;
	ihiz=(int)(aab.getMaximum().z/(float)CELL_SIZE);
	if (ihiz<0) ihiz=0; if (ihiz>MAXIMUM_CELL) ihiz=MAXIMUM_CELL;
	int i,j;
	for (i=ilox; i<=ihix; i++)
	{
		for (j=iloz; j<=ihiz; j++)
		{
			hash_free(i,j,number+MAX_COLLISION_BOXES);
		}
	}
	return 0;
}

void Collisions::hash_free(int cell_x, int cell_z, int value)
{
	unsigned int cellid=(cell_x<<16)+cell_z;
	unsigned int pos=hashfunc(cellid);

	// set cell to free state
	if(hashtable[pos].cellid != UNUSED_CELLID)
	{
		cell_t *cell = hashtable[pos].cell;
		// cell has content, search it
		for(int i=0;i<cell->free;i++)
		{
			if(cell->element[i] == value)
			{
				// remove that element
				cell->element[i] = UNUSED_CELLID;
				break;
			}
		}
	}

}

void Collisions::hash_add(int cell_x, int cell_z, int value)
{
	unsigned int cellid=(cell_x<<16)+cell_z;
	unsigned int hash=hashfunc(cellid);
	//search the spot
	unsigned int pos=hash;
	unsigned int stop=hash-1;
	if (hash==0) stop=(1<<HASH_SIZE)-1;
//Ogre::LogManager::getSingleton().logMessage("COLL: adding "+Ogre::StringConverter::toString(cell_x)+", "+Ogre::StringConverter::toString(cell_z)+" hash="+Ogre::StringConverter::toString(hash));

	while (pos!=stop && hashtable[pos].cellid!=UNUSED_CELLID && hashtable[pos].cellid!=cellid)
	{
		pos++;
		if (pos==(1<<HASH_SIZE)) pos=0;
	}

	if (hashtable[pos].cellid==UNUSED_CELLID)
	{
		if (free_cell<MAX_CELLS)
		{
			//create a new cell
			hashtable[pos].cellid=cellid;
			hashtable[pos].cell=&cells[free_cell];
			cells[free_cell].free=1;
			cells[free_cell].element[0]=value;
			cells[free_cell].next=0;
			if (pos!=hashfunc(cellid)) collision_count++;
			if (cells[free_cell].free>largest_cellcount) largest_cellcount=cells[free_cell].free;
			free_cell++;
		}
		else
		{
			if(debugMode)
				Ogre::LogManager::getSingleton().logMessage("COLL: not enough cells available");
		}
	}
	else if (hashtable[pos].cellid==cellid)
	{
		//there is already a cell ready
		cell_t *cell=hashtable[pos].cell;

		// check for re-usable cellelements
		bool found=false;
hash_add_next_cell:
		for(int i=0;i<CELL_BLOCKSIZE;i++)
		{
			if(cell->element[i] == UNUSED_CELLELEMENT)
			{
				// found free cellelement, using it!
				cell->element[i]=value;
				found=true;
				if(debugMode)
					Ogre::LogManager::getSingleton().logMessage("COLL: reused cell element!");
				break;
			}
		}
		if(!found && cell->next)
		{
			// ugly recursive cell search
			cell = (cell_t*) cell->next;
			goto hash_add_next_cell;
		}
		if(!found)
		{
			// no reusable cellelement found, creating new one
			if (cell->free<CELL_BLOCKSIZE)
			{
				cell->element[cell->free]=value;
				cell->free++;
				//Ogre::LogManager::getSingleton().logMessage("COLL: created new cell element!");
				if (cell->free>largest_cellcount) largest_cellcount=cell->free;
			}
			else if (cell->free >= CELL_BLOCKSIZE && !cell->next)
			{
				// create new linked cell

				// find empty cell
				int pos2=pos;
				while (pos2!=stop && hashtable[pos2].cellid!=UNUSED_CELLID)
					pos2++;

				// allocate that cell and link it to the current cell
				if (hashtable[pos2].cellid==UNUSED_CELLID)
				{
					//create a new cell
					hashtable[pos2].cellid=cellid;
					hashtable[pos2].cell=&cells[free_cell];
					cells[free_cell].free=1;
					cells[free_cell].element[0]=value;
					cells[free_cell].next=0;
					if (pos!=hashfunc(cellid)) collision_count++;
					if (cells[free_cell].free>largest_cellcount) largest_cellcount=cells[free_cell].free;
					free_cell++;
					cell->next = hashtable[pos2].cell;
				}
			}
		}
	}
	else
	{
		//the hash table is full!!!
		Ogre::LogManager::getSingleton().logMessage("COLL: the hashtable is full!!!");
	}
}

cell_t *Collisions::hash_find(int cell_x, int cell_z)
{
	unsigned int cellid=(cell_x<<16)+cell_z;
	unsigned int hash=hashfunc(cellid);
	//search the spot
	unsigned int pos=hash;
	unsigned int stop=hash-1;
	if (hash==0) stop=(1<<HASH_SIZE)-1;

	while (pos!=stop && hashtable[pos].cellid!=UNUSED_CELLID && hashtable[pos].cellid!=cellid)
	{
		pos++;
		if (pos==(1<<HASH_SIZE)) pos=0;
	}
	if (hashtable[pos].cellid==cellid) return hashtable[pos].cell;
	return NULL;
}


void Collisions::addCollisionBox(SceneNode *tenode, bool rotating, bool virt, float px, float py, float pz, float rx, float ry, float rz, float lx,float hx,float ly,float hy,float lz,float hz,float srx,float sry,float srz, char* eventname, char* instancename, bool forcecam, Vector3 campos, float scx, float scy, float scz, float drx, float dry, float drz, int event_filter, int luahandler)
{
	Quaternion 	rotation=Quaternion(Degree(rx), Vector3::UNIT_X)*Quaternion(Degree(ry), Vector3::UNIT_Y)*Quaternion(Degree(rz), Vector3::UNIT_Z);
	Quaternion 	direction=Quaternion(Degree(drx), Vector3::UNIT_X)*Quaternion(Degree(dry), Vector3::UNIT_Y)*Quaternion(Degree(drz), Vector3::UNIT_Z);
	//set refined box anyway
	collision_boxes[free_collision_box].relo_x=lx*scx;
	collision_boxes[free_collision_box].rehi_x=hx*scx;
	collision_boxes[free_collision_box].relo_y=ly*scy;
	collision_boxes[free_collision_box].rehi_y=hy*scy;
	collision_boxes[free_collision_box].relo_z=lz*scz;
	collision_boxes[free_collision_box].rehi_z=hz*scz;
	//calculate selfcenter anyway
	collision_boxes[free_collision_box].selfcenter=Vector3((lx*scx+hx*scx)/2.0, (ly*scy+hy*scy)/2.0,(lz*scz+hz*scz)/2.0);
	//and center too (we need it)
	collision_boxes[free_collision_box].center=Vector3(px, py, pz);
	collision_boxes[free_collision_box].virt=virt;

	collision_boxes[free_collision_box].event_filter=event_filter;
	//camera stuff
	collision_boxes[free_collision_box].camforced=forcecam;
	if (forcecam)
	{
		collision_boxes[free_collision_box].campos=collision_boxes[free_collision_box].center+rotation*campos;
	}

	//first, selfrotate
	if (rotating)
	{
		//we have a self-rotated block
		collision_boxes[free_collision_box].selfrotated=true;
		collision_boxes[free_collision_box].selfrot=Quaternion(Degree(srx), Vector3::UNIT_X)*Quaternion(Degree(sry), Vector3::UNIT_Y)*Quaternion(Degree(srz), Vector3::UNIT_Z);
		collision_boxes[free_collision_box].selfunrot=collision_boxes[free_collision_box].selfrot.Inverse();
	}
	else collision_boxes[free_collision_box].selfrotated=false;

	collision_boxes[free_collision_box].eventsourcenum=-1;
	if (strlen(eventname)>0)
	{
//Ogre::LogManager::getSingleton().logMessage("COLL: adding "+Ogre::StringConverter::toString(free_eventsource)+" "+String(instancename)+" "+String(eventname));
		//this is envent-generating
		strcpy(eventsources[free_eventsource].boxname, eventname);
		strcpy(eventsources[free_eventsource].instancename, instancename);
		eventsources[free_eventsource].luahandler = luahandler;
		collision_boxes[free_collision_box].eventsourcenum=free_eventsource;
		eventsources[free_eventsource].cbox=free_collision_box;
		eventsources[free_eventsource].snode=tenode;
		eventsources[free_eventsource].direction=direction;
		free_eventsource++;
	}

	//next, global rotate
	if (rx==0.0 && ry==0.0 && rz==0.0)
	{
		//unrefined box
		collision_boxes[free_collision_box].refined=false;
	} else
	{
		//refined box
		collision_boxes[free_collision_box].refined=true;
		//build rotation
		collision_boxes[free_collision_box].rot=rotation;
		collision_boxes[free_collision_box].unrot=rotation.Inverse();
	}

	//set raw box
	if (collision_boxes[free_collision_box].selfrotated || collision_boxes[free_collision_box].refined)
	{
		Vector3 p[8];
		p[0]=Vector3(lx*scx, ly*scy, lz*scz);
		p[1]=Vector3(hx*scx, ly*scy, lz*scz);
		p[2]=Vector3(lx*scx, hy*scy, lz*scz);
		p[3]=Vector3(hx*scx, hy*scy, lz*scz);
		p[4]=Vector3(lx*scx, ly*scy, hz*scz);
		p[5]=Vector3(hx*scx, ly*scy, hz*scz);
		p[6]=Vector3(lx*scx, hy*scy, hz*scz);
		p[7]=Vector3(hx*scx, hy*scy, hz*scz);
		int i;
		//rotate box
		if (collision_boxes[free_collision_box].selfrotated)
			for (i=0; i<8; i++)
			{
				p[i]=p[i]-collision_boxes[free_collision_box].selfcenter;
				p[i]=collision_boxes[free_collision_box].selfrot*p[i];
				p[i]=p[i]+collision_boxes[free_collision_box].selfcenter;
			}
			if (collision_boxes[free_collision_box].refined)
			{
				for (i=0; i<8; i++) p[i]=collision_boxes[free_collision_box].rot*p[i];
//					if (collision_boxes[free_collision_box].camerabox) collision_boxes[free_collision_box].camerapos=collision_boxes[free_collision_box].rot*collision_boxes[free_collision_box].camerapos;
			}
			//find min/max
			collision_boxes[free_collision_box].lo_x=p[0].x;
			collision_boxes[free_collision_box].hi_x=p[0].x;
			collision_boxes[free_collision_box].lo_y=p[0].y;
			collision_boxes[free_collision_box].hi_y=p[0].y;
			collision_boxes[free_collision_box].lo_z=p[0].z;
			collision_boxes[free_collision_box].hi_z=p[0].z;
			for (i=1; i<8; i++)
			{
				if (p[i].x<collision_boxes[free_collision_box].lo_x) collision_boxes[free_collision_box].lo_x=p[i].x;
				if (p[i].x>collision_boxes[free_collision_box].hi_x) collision_boxes[free_collision_box].hi_x=p[i].x;
				if (p[i].y<collision_boxes[free_collision_box].lo_y) collision_boxes[free_collision_box].lo_y=p[i].y;
				if (p[i].y>collision_boxes[free_collision_box].hi_y) collision_boxes[free_collision_box].hi_y=p[i].y;
				if (p[i].z<collision_boxes[free_collision_box].lo_z) collision_boxes[free_collision_box].lo_z=p[i].z;
				if (p[i].z>collision_boxes[free_collision_box].hi_z) collision_boxes[free_collision_box].hi_z=p[i].z;
			}
			//set absolute coords
			collision_boxes[free_collision_box].lo_x+=px;
			collision_boxes[free_collision_box].hi_x+=px;
			collision_boxes[free_collision_box].lo_y+=py;
			collision_boxes[free_collision_box].hi_y+=py;
			collision_boxes[free_collision_box].lo_z+=pz;
			collision_boxes[free_collision_box].hi_z+=pz;
	}
	else
	{
		//unrefined box
		collision_boxes[free_collision_box].lo_x=px+lx*scx;
		collision_boxes[free_collision_box].hi_x=px+hx*scx;
		collision_boxes[free_collision_box].lo_y=py+ly*scy;
		collision_boxes[free_collision_box].hi_y=py+hy*scy;
		collision_boxes[free_collision_box].lo_z=pz+lz*scz;
		collision_boxes[free_collision_box].hi_z=pz+hz*scz;
	}
//		collision_boxes[free_collision_box].camerapos+=Vector3(px,py,pz);
	//register this collision box in the index
	int ilox, ihix, iloz, ihiz;
	ilox=(int)(collision_boxes[free_collision_box].lo_x/(float)CELL_SIZE);
	if (ilox<0) ilox=0; if (ilox>MAXIMUM_CELL) ilox=MAXIMUM_CELL;
	ihix=(int)(collision_boxes[free_collision_box].hi_x/(float)CELL_SIZE);
	if (ihix<0) ihix=0; if (ihix>MAXIMUM_CELL) ihix=MAXIMUM_CELL;
	iloz=(int)(collision_boxes[free_collision_box].lo_z/(float)CELL_SIZE);
	if (iloz<0) iloz=0; if (iloz>MAXIMUM_CELL) iloz=MAXIMUM_CELL;
	ihiz=(int)(collision_boxes[free_collision_box].hi_z/(float)CELL_SIZE);
	if (ihiz<0) ihiz=0; if (ihiz>MAXIMUM_CELL) ihiz=MAXIMUM_CELL;
	int i,j;
	for (i=ilox; i<=ihix; i++)
		for (j=iloz; j<=ihiz; j++)
		{
			//LogManager::getSingleton().logMessage("Adding a reference to cell "+StringConverter::toString(i)+" "+StringConverter::toString(j)+" at index "+StringConverter::toString(collision_index_free[i*NUM_COLLISON_CELLS+j]));
			hash_add(i,j,free_collision_box);
		}
		free_collision_box++;
}

int Collisions::addCollisionTri(Vector3 p1, Vector3 p2, Vector3 p3, ground_model_t* gm)
{
	if(free_collision_tri >= MAX_COLLISION_TRIS) return -1;
	collision_tris[free_collision_tri].a=p1;
	collision_tris[free_collision_tri].b=p2;
	collision_tris[free_collision_tri].c=p3;
	collision_tris[free_collision_tri].gm=gm;
	collision_tris[free_collision_tri].enabled=true;
	//compute transformations
	//base construction
	Vector3 bx=p2-p1;
	Vector3 by=p3-p1;
	Vector3 bz=bx.crossProduct(by);
	bz.normalise();
	//coordinates change matrix
	collision_tris[free_collision_tri].reverse.SetColumn(0, bx);
	collision_tris[free_collision_tri].reverse.SetColumn(1, by);
	collision_tris[free_collision_tri].reverse.SetColumn(2, bz);
	collision_tris[free_collision_tri].forward=collision_tris[free_collision_tri].reverse.Inverse();

	//compute tri AAB
	AxisAlignedBox aab;
	aab.merge(p1);
	aab.merge(p2);
	aab.merge(p3);
	//register this collision tri in the index
	int ilox, ihix, iloz, ihiz;
	ilox=(int)(aab.getMinimum().x/(float)CELL_SIZE);
	if (ilox<0) ilox=0; if (ilox>MAXIMUM_CELL) ilox=MAXIMUM_CELL;
	ihix=(int)(aab.getMaximum().x/(float)CELL_SIZE);
	if (ihix<0) ihix=0; if (ihix>MAXIMUM_CELL) ihix=MAXIMUM_CELL;
	iloz=(int)(aab.getMinimum().z/(float)CELL_SIZE);
	if (iloz<0) iloz=0; if (iloz>MAXIMUM_CELL) iloz=MAXIMUM_CELL;
	ihiz=(int)(aab.getMaximum().z/(float)CELL_SIZE);
	if (ihiz<0) ihiz=0; if (ihiz>MAXIMUM_CELL) ihiz=MAXIMUM_CELL;
	int i,j;
	for (i=ilox; i<=ihix; i++)
	{
		for (j=iloz; j<=ihiz; j++)
		{
			hash_add(i,j,free_collision_tri+MAX_COLLISION_BOXES);
		}
	}
	int myid = free_collision_tri;
	free_collision_tri++;
	return myid;
}

void Collisions::printStats()
{
	LogManager::getSingleton().logMessage("COLL: Collision system statistics:");
	LogManager::getSingleton().logMessage("COLL: Cell size: "+StringConverter::toString((float)CELL_SIZE)+" m");
	LogManager::getSingleton().logMessage("COLL: Hashtable occupation: "+StringConverter::toString(free_cell)+"/"+StringConverter::toString(MAX_CELLS)+" ("+StringConverter::toString(100.0f*free_cell/(MAX_CELLS))+"%)");
	LogManager::getSingleton().logMessage("COLL: Hashtable collisions: "+StringConverter::toString(collision_count));
	LogManager::getSingleton().logMessage("COLL: Largest cell: "+StringConverter::toString(largest_cellcount)+"/"+StringConverter::toString(CELL_BLOCKSIZE)+" ("+StringConverter::toString(100.0f*largest_cellcount/CELL_BLOCKSIZE)+"%)");

}


bool Collisions::collisionCorrect(Vector3 *refpos)
{
	//find the correct cell
	bool contacted=false;
	int refx, refz;
	int k;

	if (!(refpos->x>0 && refpos->x<mefl->mapsizex && refpos->z>0 && refpos->z<mefl->mapsizex)) return false;

	refx=(int)(refpos->x/(float)CELL_SIZE);
	refz=(int)(refpos->z/(float)CELL_SIZE);
	cell_t *cell=hash_find(refx, refz);

	collision_tri_t *minctri=0;
	float minctridist=100.0;
	Vector3 minctripoint;

	if (cell)
	{
coll_corr_resume_cell:
		for (k=0; k<cell->free; k++)
		{
			if (cell->element[k] != UNUSED_CELLELEMENT && cell->element[k]<MAX_COLLISION_BOXES)
			{
				collision_box_t *cbox=&collision_boxes[cell->element[k]];
				if (refpos->x>cbox->lo_x && refpos->x<cbox->hi_x && refpos->y>cbox->lo_y && refpos->y<cbox->hi_y && refpos->z>cbox->lo_z && refpos->z<cbox->hi_z)
				{
					if (cbox->refined || cbox->selfrotated)
					{
						//we may have a collision, do a change of repere
						Vector3 Pos=*refpos-cbox->center;
						if (cbox->refined) Pos=cbox->unrot*Pos;
						if (cbox->selfrotated)
						{
							Pos=Pos-cbox->selfcenter;
							Pos=cbox->selfunrot*Pos;
							Pos=Pos+cbox->selfcenter;
						}
						//now test with the inner box
						if (Pos.x>cbox->relo_x && Pos.x<cbox->rehi_x && Pos.y>cbox->relo_y && Pos.y<cbox->rehi_y && Pos.z>cbox->relo_z && Pos.z<cbox->rehi_z)
						{
							if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter))
							{
#ifdef LUASCRIPT
								lua->spawnEvent(cbox->eventsourcenum, &eventsources[cbox->eventsourcenum]);
#endif
							}
							if (cbox->camforced && !forcecam)
							{
								forcecam=true;
								forcecampos=cbox->campos;
							}
							if (!cbox->virt)
							{
								//collision, process as usual
								//we have a collision
								contacted=true;
								//determine which side collided
								//								Vector3 normal;
								float min=Pos.z-cbox->relo_z;
								int mincase=0; //south
								//								normal=Vector3(0,0,-1);
								float t=cbox->rehi_z-Pos.z;
								if (t<min) {min=t; mincase=2;/*normal=Vector3(0,0,1);*/}; //north
								t=Pos.x-cbox->relo_x;
								if (t<min) {min=t; mincase=3;/*normal=Vector3(-1,0,0);*/}; //west
								t=cbox->rehi_x-Pos.x;
								if (t<min) {min=t; mincase=1;/*normal=Vector3(1,0,0);*/}; //east
								t=Pos.y-cbox->relo_y;
								if (t<min) {min=t; mincase=4;/*normal=Vector3(0,-1,0);*/}; //down
								t=cbox->rehi_y-Pos.y;
								if (t<min) {min=t; mincase=5;/*normal=Vector3(0,1,0);*/}; //up
								//okay we are in case mincase
								//fix position
								if (mincase==0) Pos.z=cbox->relo_z;
								if (mincase==1) Pos.x=cbox->rehi_x;
								if (mincase==2) Pos.z=cbox->rehi_z;
								if (mincase==3) Pos.x=cbox->relo_x;
								if (mincase==4) Pos.y=cbox->relo_y;
								if (mincase==5) Pos.y=cbox->rehi_y;
								//resume repere
								if (cbox->selfrotated)
								{
									Pos=Pos-cbox->selfcenter;
									Pos=cbox->selfrot*Pos;
									Pos=Pos+cbox->selfcenter;
								}
								if (cbox->refined) Pos=cbox->rot*Pos;
								*refpos=Pos+cbox->center;
							}
						}

					} else
					{
						if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter))
						{
#ifdef LUASCRIPT
							lua->spawnEvent(cbox->eventsourcenum, &eventsources[cbox->eventsourcenum]);
#endif
						}
						if (cbox->camforced && !forcecam)
						{
							forcecam=true;
							forcecampos=cbox->campos;
						}
						if (!cbox->virt)
						{
							//we have a collision
							contacted=true;
							//determine which side collided
							//							Vector3 normal;
							float min=refpos->z-cbox->lo_z;
							int mincase=0; //south
							//							normal=Vector3(0,0,-1);
							float t=cbox->hi_z-refpos->z;
							if (t<min) {min=t; mincase=2;/*normal=Vector3(0,0,1);*/}; //north
							t=refpos->x-cbox->lo_x;
							if (t<min) {min=t; mincase=3;/*normal=Vector3(-1,0,0);*/}; //west
							t=cbox->hi_x-refpos->x;
							if (t<min) {min=t; mincase=1;/*normal=Vector3(1,0,0);*/}; //east
							t=refpos->y-cbox->lo_y;
							if (t<min) {min=t; mincase=4;/*normal=Vector3(0,-1,0);*/}; //down
							t=cbox->hi_y-refpos->y;
							if (t<min) {min=t; mincase=5;/*normal=Vector3(0,1,0);*/}; //up
							//okay we are in case mincase
							//fix position
							if (mincase==0) refpos->z=cbox->lo_z;
							if (mincase==1) refpos->x=cbox->hi_x;
							if (mincase==2) refpos->z=cbox->hi_z;
							if (mincase==3) refpos->x=cbox->lo_x;
							if (mincase==4) refpos->y=cbox->lo_y;
							if (mincase==5) refpos->y=cbox->hi_y;
						}
					}
				}
			}
			else
			{
				collision_tri_t *ctri=&collision_tris[cell->element[k]-MAX_COLLISION_BOXES];
				if(!ctri->enabled)
					continue;
				//check if this tri is minimal
				//transform
				Vector3 point=ctri->forward*(*refpos-ctri->a);
				//test if within tri collision volume (potential cause of bug!)
				if (point.x>=0 && point.y>=0 && (point.x+point.y)<=1.0 && point.z<0 && point.z>-0.1)
				{
					if (-point.z<minctridist)
					{
 						minctridist=-point.z;
						minctri=ctri;
						minctripoint=point;
					}
				}
			}
		}
		if(cell->next)
		{
			// continue with next cell
			cell=(cell_t*)cell->next;
			goto coll_corr_resume_cell;
		}
	}
	//process minctri collision
	if (minctri)
	{
		//we have a contact
		contacted=true;
		//correct point
		minctripoint.z=0;
		//reverse transform
		*refpos=(minctri->reverse*minctripoint)+minctri->a;
	}
	return contacted;
}

bool Collisions::permitEvent(int filter)
{
	if(filter == EVENT_ALL)
		return true;
	else if(filter == EVENT_AVATAR && mefl->getCurrentTruckNumber() == -1)
		return true;
	else if(filter == EVENT_TRUCK && mefl->getCurrentTruckNumber() != -1 && mefl->getCurrentTruck()->driveable == TRUCK)
		return true;
	else if(filter == EVENT_AIRPLANE && mefl->getCurrentTruckNumber() != -1 && mefl->getCurrentTruck()->driveable == AIRPLANE)
		return true;
	return false;
}

int Collisions::enableCollisionTri(int number, bool enable)
{
	if(number>free_collision_tri)
		return -1;
	collision_tris[number].enabled=enable;
	return 0;
}

bool Collisions::nodeCollision(node_t *node, bool iscinecam, int contacted, float dt, float* nso, ground_model_t** ogm)
{
	bool smoky=false;
	//float corrf=1.0;
	Vector3 oripos=node->AbsPosition;
	int k;
	//find the correct cell
	int refx, refz;
	refx=(int)(node->AbsPosition.x*inverse_CELL_SIZE);
	refz=(int)(node->AbsPosition.z*inverse_CELL_SIZE);
	cell_t *cell=hash_find(refx, refz);
	//LogManager::getSingleton().logMessage("Checking cell "+StringConverter::toString(refx)+" "+StringConverter::toString(refz)+" total indexes: "+StringConverter::toString(num_cboxes_index[refp]));

	collision_tri_t *minctri=0;
	float minctridist=100.0;
	Vector3 minctripoint;

	if (cell)
	{
node_coll_resume_cell:
		for (k=0; k<cell->free; k++)
		{
			if (cell->element[k] != UNUSED_CELLELEMENT && cell->element[k]<MAX_COLLISION_BOXES)
			{
				collision_box_t *cbox=&collision_boxes[cell->element[k]];
				if (node->AbsPosition.x>cbox->lo_x && node->AbsPosition.x<cbox->hi_x && node->AbsPosition.y>cbox->lo_y && node->AbsPosition.y<cbox->hi_y && node->AbsPosition.z>cbox->lo_z && node->AbsPosition.z<cbox->hi_z)
				{
					if (cbox->refined || cbox->selfrotated)
					{
						//we may have a collision, do a change of repere
						Vector3 Pos=node->AbsPosition-cbox->center;
						if (cbox->refined) Pos=cbox->unrot*Pos;
						if (cbox->selfrotated)
						{
							Pos=Pos-cbox->selfcenter;
							Pos=cbox->selfunrot*Pos;
							Pos=Pos+cbox->selfcenter;
						}
						//now test with the inner box
						if (Pos.x>cbox->relo_x && Pos.x<cbox->rehi_x && Pos.y>cbox->relo_y && Pos.y<cbox->rehi_y && Pos.z>cbox->relo_z && Pos.z<cbox->rehi_z)
						{
							if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter))
							{
#ifdef LUASCRIPT
								lua->spawnEvent(cbox->eventsourcenum, &eventsources[cbox->eventsourcenum]);
#endif
							}
							if (cbox->camforced && !forcecam)
							{
								forcecam=true;
								forcecampos=cbox->campos;
							}
							if (!cbox->virt)
							{
								//collision, process as usual
								//we have a collision
								contacted++;
								//setup smoke
								//float ns=node->Velocity.length();
								smoky=true;
								//*nso=ns;
								//determine which side collided
								float min=Pos.z-cbox->relo_z;
								int mincase=0; //south
								Vector3 normal=Vector3(0,0,-1);
								float t=cbox->rehi_z-Pos.z;
								if (t<min) {min=t; mincase=2;normal=Vector3(0,0,1);}; //north
								t=Pos.x-cbox->relo_x;
								if (t<min) {min=t; mincase=3;normal=Vector3(-1,0,0);}; //west
								t=cbox->rehi_x-Pos.x;
								if (t<min) {min=t; mincase=1;normal=Vector3(1,0,0);}; //east
								t=Pos.y-cbox->relo_y;
								if (t<min) {min=t; mincase=4;normal=Vector3(0,-1,0);}; //down
								t=cbox->rehi_y-Pos.y;
								if (t<min) {min=t; mincase=5;normal=Vector3(0,1,0);}; //up

								//we need the normal, and the depth
								//resume repere for the normal
								if (cbox->selfrotated) normal=cbox->selfrot*normal;
								if (cbox->refined) normal=cbox->rot*normal;
								primitiveCollision(node, normal, dt, &GROUND_CONCRETE, nso);
								if (ogm) *ogm=&GROUND_CONCRETE;
								}
							}

					} else
					{
						if (cbox->eventsourcenum!=-1 && permitEvent(cbox->event_filter))
						{
#ifdef LUASCRIPT
							lua->spawnEvent(cbox->eventsourcenum, &eventsources[cbox->eventsourcenum]);
#endif
						}
						if (cbox->camforced && !forcecam)
						{
							forcecam=true;
							forcecampos=cbox->campos;
						}
						if (!cbox->virt)
						{
							//we have a collision
							contacted++;
							//setup smoke
							//float ns=node->Velocity.length();
							smoky=true;
							//*nso=ns;
							//determine which side collided
							float min=node->AbsPosition.z-cbox->lo_z;
							int mincase=0; //south
							Vector3 normal=Vector3(0,0,-1);
							float t=cbox->hi_z-node->AbsPosition.z;
							if (t<min) {min=t; mincase=2;normal=Vector3(0,0,1);}; //north
							t=node->AbsPosition.x-cbox->lo_x;
							if (t<min) {min=t; mincase=3;normal=Vector3(-1,0,0);}; //west
							t=cbox->hi_x-node->AbsPosition.x;
							if (t<min) {min=t; mincase=1;normal=Vector3(1,0,0);}; //east
							t=node->AbsPosition.y-cbox->lo_y;
							if (t<min) {min=t; mincase=4;normal=Vector3(0,-1,0);}; //down
							t=cbox->hi_y-node->AbsPosition.y;
							if (t<min) {min=t; mincase=5;normal=Vector3(0,1,0);}; //up
							//we need the normal
							//resume repere for the normal
							if (cbox->selfrotated) normal=cbox->selfrot*normal;
							if (cbox->refined) normal=cbox->rot*normal;
							primitiveCollision(node, normal, dt, &GROUND_CONCRETE, nso);
							if (ogm) *ogm=&GROUND_CONCRETE;
							/*//fix position
							Vector3 prevPos=node->AbsPosition;
							if (mincase==0) node->AbsPosition.z=cbox->lo_z;
							if (mincase==1) node->AbsPosition.x=cbox->hi_x;
							if (mincase==2) node->AbsPosition.z=cbox->hi_z;
							if (mincase==3) node->AbsPosition.x=cbox->lo_x;
							if (mincase==4) node->AbsPosition.y=cbox->lo_y;
							if (mincase==5) node->AbsPosition.y=cbox->hi_y;
							float depth=(prevPos-node->AbsPosition).length();


							//compute slip velocity vector
							Vector3 slip=node->Velocity-node->Velocity.dotProduct(normal)*normal;
							//remove the normal speed component
							node->Velocity=slip;
							float slipl=slip.length();
							if (slipl<0.5) node->Velocity=Vector3::ZERO;
							else
							{
								float loadfactor=(1.0-fabs(depth/corrf)*120.0);
								if (loadfactor<0) loadfactor=0;
								float slipfactor=(slipl-0.5)/10.0;
								if (slipfactor>1) slipfactor=1;
								node->Velocity*=loadfactor*slipfactor;
							}
							*/
						}
					}
				}
			}
			else
			{
				//tri collision
				collision_tri_t *ctri=&collision_tris[cell->element[k]-MAX_COLLISION_BOXES];
				//check if this tri is minimal
				//transform
				Vector3 point=ctri->forward*(node->AbsPosition-ctri->a);
				//test if within tri collision volume (potential cause of bug!)
				if (point.x>=0 && point.y>=0 && (point.x+point.y)<=1.0 && point.z<0 && point.z>-0.1)
				{
					if (-point.z<minctridist)
					{
						minctridist=-point.z;
						minctri=ctri;
						minctripoint=point;
					}
				}
			}
		}
		if(cell->next)
		{
			// continue with next cell
			cell=(cell_t*)cell->next;
			goto node_coll_resume_cell;
		}
	}
	//process minctri collision
	if (minctri)
	{
		//we have a contact
		contacted++;
		//setup smoke
		//float ns=node->Velocity.length();
		smoky=true;
		//*nso=ns;

		//we need the normal
		//resume repere for the normal
		Vector3 normal=minctri->reverse*Vector3::UNIT_Z;
		primitiveCollision(node, normal, dt, minctri->gm, nso);
		if (ogm) *ogm=minctri->gm;
		/*
		float depth=-minctripoint.z;
		//compute slip velocity vector
		Vector3 slip=node->Velocity-node->Velocity.dotProduct(normal)*normal;
		//remove the normal speed component
		node->Velocity=slip;
		float slipl=slip.length();
		if (slipl<0.5) node->Velocity=Vector3::ZERO;
		else
		{
			float loadfactor=(1.0-fabs(depth/corrf)*120.0);
			if (loadfactor<0) loadfactor=0;
			float slipfactor=(slipl-0.5)/10.0;
			if (slipfactor>1) slipfactor=1;
			node->Velocity*=loadfactor*slipfactor;
		}
		//correct point
		minctripoint.z=0;
		//reverse transform
		node->AbsPosition=(minctri->reverse*minctripoint)+minctri->a;
		//grip
		//node->Velocity=Vector3::ZERO;
		*/
	}
	//correct relative position too
	if (contacted) node->RelPosition=node->RelPosition+(node->AbsPosition-oripos);
	node->contacted=contacted;
	return smoky;
}


Vector3 Collisions::getPosition(char* instance, char* box)
{
	for (int i=0; i<free_eventsource; i++)
	{
		if (!strcmp(instance, eventsources[i].instancename) && !strcmp(box, eventsources[i].boxname))
		{
			return collision_boxes[eventsources[i].cbox].center+collision_boxes[eventsources[i].cbox].rot*collision_boxes[eventsources[i].cbox].selfcenter;
		}
	}
	return Vector3::ZERO;
}

Quaternion Collisions::getDirection(char* instance, char* box)
{
	for (int i=0; i<free_eventsource; i++)
	{
		if (!strcmp(instance, eventsources[i].instancename) && !strcmp(box, eventsources[i].boxname))
		{
			return collision_boxes[eventsources[i].cbox].rot*eventsources[i].direction;
		}
	}
	return Quaternion::ZERO;
}

collision_box_t *Collisions::getBox(char* instance, char* box)
{
	for (int i=0; i<free_eventsource; i++)
	{
		if (!strcmp(instance, eventsources[i].instancename) && !strcmp(box, eventsources[i].boxname))
		{
			return &collision_boxes[eventsources[i].cbox];
		}
	}
	return NULL;
}

bool Collisions::isInside(Vector3 pos, char* instance, char* box, float border)
{

	collision_box_t *cbox=getBox(instance, box);
	return isInside(pos, cbox, border);
}

bool Collisions::isInside(Vector3 pos, collision_box_t *cbox, float border)
{
	if (!cbox) return false;
	if (pos.x+border>cbox->lo_x && pos.x-border<cbox->hi_x && pos.y+border>cbox->lo_y && pos.y-border<cbox->hi_y && pos.z+border>cbox->lo_z && pos.z-border<cbox->hi_z)
	{
		if (cbox->refined || cbox->selfrotated)
		{
			//we may have a collision, do a change of repere
			Vector3 rpos=pos-cbox->center;
			if (cbox->refined) rpos=cbox->unrot*rpos;
			if (cbox->selfrotated)
			{
				rpos=rpos-cbox->selfcenter;
				rpos=cbox->selfunrot*rpos;
				rpos=rpos+cbox->selfcenter;
			}
			//now test with the inner box
			if (rpos.x>cbox->relo_x && rpos.x<cbox->rehi_x && rpos.y>cbox->relo_y && rpos.y<cbox->rehi_y && rpos.z>cbox->relo_z && rpos.z<cbox->rehi_z)
			{
				return true;
			}
		}
		else return true;
	}
	return false;
}

bool Collisions::groundCollision(node_t *node, float dt, ground_model_t** ogm, float *nso)
{
	if (!hfinder) return false;
	ground_model_t *gm=&GROUND_GRAVEL; //to be determined by the landuse map
	if(landuse) gm = landuse->getGroundModelAt(node->AbsPosition.x, node->AbsPosition.z);
	last_used_ground_model = gm;
	if (ogm) *ogm=gm; //write that info to the caller
	//new ground collision code
	Real v=hfinder->getHeightAt(node->AbsPosition.x, node->AbsPosition.z);
	if (v>node->AbsPosition.y)
	{

		//collision!
		Vector3 normal;
		hfinder->getNormalAt(node->AbsPosition.x, v, node->AbsPosition.z, &normal);
		primitiveCollision(node, normal, dt, gm, nso, v-node->AbsPosition.y);
		return true;
	}
	return false;
}

void Collisions::primitiveCollision(node_t *node, Vector3 normal, float dt, ground_model_t* gm, float* nso, float penetration)
{
	//normal velocity
	float nvel=node->Velocity.dotProduct(normal);
	Vector3 slip=node->Velocity-nvel*normal;
	float slipv=slip.squaredLength();
	float invslipv=fast_invSqrt(slipv);
	slipv=slipv*invslipv;

	if (nso) *nso=slipv;
	if (slipv!=0.0) slip=slip*invslipv; else slip=Vector3::ZERO;
	//steady force
	float fns_orig=node->Forces.dotProduct(normal);
	float fnn=-fns_orig;
	float fns=fnn;
	//impact force
	if (nvel<0)
	{
		float tmp=-nvel*node->mass/dt;
		fnn+=tmp*gm->strength; //Newton's second law
		fns+=tmp;
	}
	if (fnn<0) fnn=0;
	if (fns<0) fns=0;

	float ff;
	// If the velocity that we slip is lower than adhesion velocity and
	// we have a downforce and the slip forces are lower than static friction
	// forces then it's time to go into static friction physics mode.
        // This code is a direct translation of textbook static friction physics
	if ( slipv<(gm->va) && fns>0 && fabs(node->Forces.dotProduct(slip))<=fabs((gm->ms)*fns))
	{
		// Static friction model (with a little smoothing to help the integrator deal with it)
		ff=-(gm->ms)*fns*(1-approx_exp(-fabs(slipv/gm->va)));
		node->Forces=(fns_orig+fnn)*normal + ff*slip;
	}
	else
	{
		//Stribek model. It also comes directly from textbooks.
		float g=gm->mc+(gm->ms-gm->mc)*approx_exp(-approx_pow(slipv/gm->vs, gm->alpha));
		ff=-(g+gm->t2*slipv)*fns;
	node->Forces+=fnn*normal+ff*slip;
}
}

int Collisions::createCollisionDebugVisualization()
{
	static int loaded = 0;
	// prevent double calling
	if(loaded != 0)
		return -1;

	SceneManager *mSceneMgr = mefl->getSceneMgr();

	// create materials
	int i = 0;
	char bname[255];
	for(i=0;i<=100;i++)
	{
		//register a material for skeleton view
		sprintf(bname, "mat-coll-dbg-%d", i);
		MaterialPtr mat=(MaterialPtr)(MaterialManager::getSingleton().create(bname, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME));
		float f = fabs(((float)i)/100);
		Pass *p = mat->getTechnique(0)->getPass(0); //
		p->createTextureUnitState()->setColourOperationEx(LBX_MODULATE, LBS_MANUAL, LBS_CURRENT, ColourValue(f*2.0, 2.0*(1.0-f), 0.2, 0.7));
		p->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		p->setLightingEnabled(false);
		p->setDepthWriteEnabled(false);
		p->setDepthBias(3, 3);
		p->setCullingMode(Ogre::CULL_NONE);

		Pass *p2 = mat->getTechnique(0)->createPass();
		p2->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		p2->setLightingEnabled(false);
		p2->setDepthWriteEnabled(false);
		p2->setDepthBias(3, 3);
		p2->setCullingMode(Ogre::CULL_NONE);
		p2->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		TextureUnitState *tus2 = p2->createTextureUnitState();
		tus2->setTextureName("tile.png");


		mat->setLightingEnabled(false);
		mat->setReceiveShadows(false);
	}

	for(int x=0; x<(int)(mefl->mapsizex); x+=(int)CELL_SIZE)
	{
		for(int z=0; z<(int)(mefl->mapsizez); z+=(int)CELL_SIZE)
		{
			int cellx = (int)(x/(float)CELL_SIZE);
			int cellz = (int)(z/(float)CELL_SIZE);
			cell_t *cell=hash_find(cellx, cellz);
			if(cell)
			{
				float groundheight = -9999;
				float x2 = x+CELL_SIZE;
				float z2 = z+CELL_SIZE;

				// find a good ground height for all corners of the cell ...
				Real h1=hfinder->getHeightAt(x, z);
				Real h2=hfinder->getHeightAt(x2, z);
				Real h3=hfinder->getHeightAt(x, z2);
				Real h4=hfinder->getHeightAt(x2, z2);
				if(h1>groundheight)
					groundheight = h1;
				if(h2>groundheight)
					groundheight = h2;
				if(h3>groundheight)
					groundheight = h3;
				if(h4>groundheight)
					groundheight = h4;
				groundheight+=0.1; // 10 cm hover
				// ground height should fit

				int deep = 0, cc=cell->free;
				if(cell->next) { deep++; cc+=((cell_t*)cell->next)->free; };
				if(cell->next && ((cell_t*)cell->next)->next) { deep++; cc+=((cell_t*)((cell_t*)cell->next)->next)->free; };
				if(cell->next && ((cell_t*)cell->next)->next && ((cell_t*)((cell_t*)cell->next)->next)->next) { deep++; cc+=((cell_t*)((cell_t*)((cell_t*)cell->next)->next)->next)->free; };
				float percent = cc / (float)CELL_BLOCKSIZE;

				float percentd = percent;
				if(percentd > 1) percentd = 1;
				String matName = "mat-coll-dbg-"+StringConverter::toString((int)(percentd*100));
				String cell_name="("+StringConverter::toString(cellx)+","+ StringConverter::toString(cellz)+")";

				ManualObject *mo =  mSceneMgr->createManualObject("collisionDebugVisualization"+cell_name);
				SceneNode *mo_node = mSceneMgr->getRootSceneNode()->createChildSceneNode("collisionDebugVisualization_node"+cell_name);

				mo->begin(matName, Ogre::RenderOperation::OT_TRIANGLE_LIST);

				// 1st tri
				mo->position(-CELL_SIZE/(float)2.0, 0, -CELL_SIZE/(float)2.0);
				mo->textureCoord(0,0);

				mo->position(CELL_SIZE/(float)2.0, 0, CELL_SIZE/(float)2.0);
				mo->textureCoord(1,1);

				mo->position(CELL_SIZE/(float)2.0, 0, -CELL_SIZE/(float)2.0);
				mo->textureCoord(1,0);

				// 2nd tri
				mo->position(-CELL_SIZE/(float)2.0, 0, CELL_SIZE/(float)2.0);
				mo->textureCoord(0,1);

				mo->position(CELL_SIZE/(float)2.0, 0, CELL_SIZE/(float)2.0);
				mo->textureCoord(1,1);

				mo->position(-CELL_SIZE/(float)2.0, 0, -CELL_SIZE/(float)2.0);
				mo->textureCoord(0,0);

				mo->end();
				mo->setBoundingBox(AxisAlignedBox(0, 0, 0, CELL_SIZE, 1, CELL_SIZE));
				mo_node->attachObject(mo);



				// setup the label
				String labelName = "label_"+cell_name;
				String labelCaption = cell_name+" "+StringConverter::toString(percent*100,2) + "% usage ("+StringConverter::toString(cc)+"/"+StringConverter::toString(CELL_BLOCKSIZE)+") DEEP: " + StringConverter::toString(deep);
				MovableText *mt = new MovableText(labelName, labelCaption);
				mt->setFontName("highcontrast_black");
				mt->setTextAlignment(MovableText::H_CENTER, MovableText::V_ABOVE);
				mt->setAdditionalHeight(1);
				mt->showOnTop(false);
				mt->setCharacterHeight(0.3);
				mt->setColor(ColourValue::White);
				mo_node->attachObject(mt);

				mo_node->setVisible(true);
				mo_node->setPosition(Vector3(x+CELL_SIZE/(float)2.0, groundheight, z+CELL_SIZE/(float)2.0));
			}
		}
	}

	loaded = 1;
	return 0;
}

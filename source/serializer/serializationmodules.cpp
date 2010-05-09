
#include "serializationmodules.h"
#include "BeamData.h"

#include "OgreLogManager.h"
#include "Ogre.h"


using namespace Ogre;

// format description:
// http://wiki.rigsofrods.com/pages/Truck_Description_File

// the serialization modules
//// GlobalsSerializer
GlobalsSerializer::GlobalsSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = sectionTrigger = "globals";
	s->registerModuleSerializer(this);
}

void GlobalsSerializer::initData(rig_t *rig)
{
	rig->globals = new globals_section_t();
	memset(rig->globals, 0, sizeof(globals_section_t));
	initiated = true;
}

int GlobalsSerializer::deserialize(char *line, rig_t *rig)
{
	// ignore section header
	if(!strcmp(line, "globals"))
		return 1;
	
	if(!initiated) initData(rig);

	// shortcuts	
	globals_section_t *g = rig->globals;

	return checkRes(2, sscanf(line,"%f, %f, %s", &g->truckmass, &g->loadmass, g->texname));
}

int GlobalsSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}

//// NodeSerializer
NodeSerializer::NodeSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = sectionTrigger = "nodes";
	s->registerModuleSerializer(this);
}

void NodeSerializer::initData(rig_t *rig)
{
	rig->nodes = new nodes_section_t();
	memset(rig->nodes, 0, sizeof(nodes_section_t));
	// set some defaults, important!
	rig->nodes->gravitation = -9.81f;
	initiated = true;
}

int NodeSerializer::deserialize(char *line, rig_t *rig)
{
	// ignore section header
	if(!strcmp(line, "nodes"))
		return 1;

	if(!initiated) initData(rig);
	
	// shortcuts
	nodes_section_t *n = rig->nodes;

	if(!rig->globals)
	{
		LogManager::getSingleton().logMessage("Nodes section must come after the globals section");
		return -1;
	}


	// XXX: TOFIX: logging!
	// parse nodes
	int id = 0;
	float x = 0, y = 0, z = 0, mass = 0;
	char options[255] = "n";
	int result = sscanf(line,"%i, %f, %f, %f, %s %f", &id, &x, &y, &z, options, &mass);
	// catch some errors
	if (result < 4 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}
	if (id != n->free_node)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Node) " + String(fname) +" line " + StringConverter::toString(linecounter) + ":");
		//LogManager::getSingleton().logMessage("Error: lost sync in nodes numbers after node " + StringConverter::toString(free_node) + "(got " + StringConverter::toString(id) + " instead)");
		exit(2);
	};

	if(n->free_node >= MAX_NODES)
	{
		//LogManager::getSingleton().logMessage("nodes limit reached ("+StringConverter::toString(MAX_NODES)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}

	Vector3 npos = n->startPos + n->startRot * Vector3(x, y, z);
	init_node(rig, id, npos.x, npos.y, npos.z, NODE_NORMAL, 10, 0, 0, n->free_node, -1, n->default_node_friction, n->default_node_volume, n->default_node_surface, n->default_node_loadweight);
	n->nodes[id].iIsSkin = true;

	if 	(n->default_node_loadweight >= 0.0f)
	{
		n->nodes[id].masstype     = NODE_LOADED;
		n->nodes[id].overrideMass = true;
		n->nodes[id].mass         = n->default_node_loadweight;
	}

	// merge options and default_node_options
	strncpy(options, ((String(n->default_node_options) + String(options)).c_str()), 250);

	// now 'parse' the options
	char *options_pointer = options;
	while (*options_pointer != 0)
	{
		switch (*options_pointer)
		{
			case 'l':	// load node
				if(mass != 0)
				{
					n->nodes[id].masstype     = NODE_LOADED;
					n->nodes[id].overrideMass = true;
					n->nodes[id].mass         = mass;
				}
				else
				{
					n->nodes[id].masstype     = NODE_LOADED;
					n->masscount++;
				}
				break;
			case 'x':	//exhaust
				// XXX: TOFIX
				//if (disable_smoke)
				//	break;
				/*
				if(s->smokeId == 0 && s->smokeRef != 0)
				{
					exhaust_t e;
					e.emitterNode = id;
					e.directionNode = smokeRef;
					e.isOldFormat = true;
					//smokeId=id;
					e.smokeNode = parent->createChildSceneNode();
					//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
					char wname[256];
					sprintf(wname, "exhaust-%zu-%s", exhausts.size(), truckname);
					//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
					e.smoker=manager->createParticleSystem(wname, "tracks/Smoke");
					// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
					if (!e.smoker)
						continue;
					e.smokeNode->attachObject(e.smoker);
					e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
					exhausts.push_back(e);

				}
				*/
				n->nodes[n->smokeId].isHot = true;
				n->nodes[id].isHot      = true;
				n->smokeId = id;
				break;
			case 'y':	//exhaust reference
				// XXX: TOFIX
				//if (disable_smoke)
				//	break;
				/*
				if(smokeId != 0 && smokeRef == 0)
				{
					exhaust_t e;
					e.emitterNode = smokeId;
					e.directionNode = id;
					e.isOldFormat = true;
					//smokeId=id;
					e.smokeNode = parent->createChildSceneNode();
					//ParticleSystemManager *pSysM=ParticleSystemManager::getSingletonPtr();
					char wname[256];
					sprintf(wname, "exhaust-%zu-%s", exhausts.size(), truckname);
					//if (pSysM) smoker=pSysM->createSystem(wname, "tracks/Smoke");
					e.smoker=manager->createParticleSystem(wname, "tracks/Smoke");
					// ParticleSystem* pSys = ParticleSystemManager::getSingleton().createSystem("exhaust", "tracks/Smoke");
					if (!e.smoker)
						continue;
					e.smokeNode->attachObject(e.smoker);
					e.smokeNode->setPosition(nodes[e.emitterNode].AbsPosition);
					exhausts.push_back(e);

					nodes[smokeId].isHot=true;
					nodes[id].isHot=true;
				}
				*/
				n->smokeRef = id;
				break;
			case 'c':	//contactless
				n->nodes[id].contactless = 1;
				break;
			case 'h':	//hook
				// emulate the old behaviour using new fancy hookgroups
				// XXX: TOFIX
				/*
				hook_t h;
				h.hookNode=&nodes[id];
				h.locked=UNLOCKED;
				h.lockNode=0;
				h.lockTruck=0;
				h.lockNodes=true;
				h.group=-1;
				hooks.push_back(h);
				*/
				break;
			case 'e':	//editor
				n->editorId = id;
				break;
			case 'b':	//buoy
				n->nodes[id].buoyancy = 10000.0f;
				break;
			case 'p':	//diasble particles
				n->nodes[id].disable_particles = true;
			break;
			case 'L':	//Log data:
				LogManager::getSingleton().logMessage("Node " + StringConverter::toString(id) + "  settings. Node load mass: " \
					+ StringConverter::toString(n->nodes[id].mass) + ", friction coefficient: " + StringConverter::toString(n->default_node_friction) \
					+ " and buoyancy volume coefficient: " + StringConverter::toString(n->default_node_volume) + " Fluid drag surface coefficient: " \
					+ StringConverter::toString(n->default_node_surface)+ " Particle mode: " + StringConverter::toString(n->nodes[id].disable_particles));
			break;
		}
		options_pointer++;
	}
	n->free_node++;
	return result;
}

int NodeSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}

//// BeamSerializer
BeamSerializer::BeamSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = sectionTrigger = "beams";
	s->registerModuleSerializer(this);
}

void BeamSerializer::initData(rig_t *rig)
{
	rig->beams = new beams_section_t();
	memset(rig->beams, 0, sizeof(beams_section_t));
	initiated = true;
}

int BeamSerializer::deserialize(char *line, rig_t *rig)
{
	// ignore section header
	if(!strcmp(line, "beams"))
		return 1;
	
	if(!initiated) initData(rig);
	
	// shortcuts
	nodes_section_t *n = rig->nodes;
	beams_section_t *b = rig->beams;
	if(!n)
	{
		LogManager::getSingleton().logMessage("Beams section must come after the nodes section");
		return -1;
	}

	// XXX: TOFIX: logging!
	//parse beams
	int id1, id2;
	char options[50] = "v";
	int type = BEAM_NORMAL;
	int result = sscanf(line, "%i, %i, %s", &id1, &id2, options);
	if (result < 2 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Beam) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}

	if (id1 >= n->free_node || id2 >= n->free_node)
	{
		LogManager::getSingleton().logMessage("Error: unknown node number in beams section ("
			+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
		exit(3);
	};

	if(b->free_beam >= MAX_BEAMS)
	{
		//LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}

	//skip if a beam already exists
	int i;
	for (i=0; i < b->free_beam; i++)
	{
		if ((b->beams[i].p1 == &n->nodes[id1] && b->beams[i].p2 == &n->nodes[id2]) \
			|| (b->beams[i].p1 == &n->nodes[id2] && b->beams[i].p2 == &n->nodes[id1]))
		{
			LogManager::getSingleton().logMessage("Skipping duplicate beams: from node "+StringConverter::toString(id1)+" to node "+StringConverter::toString(id2));
			return 0;
		}
	}

	// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
	// this is just ugly:
	char *options_pointer = options;
	while (*options_pointer != 0)
	{
		if(*options_pointer=='i')
		{
			type = BEAM_INVISIBLE;
			break;
		}
		options_pointer++;
	}

	int pos = add_beam(rig, &n->nodes[id1], &n->nodes[id2], type, \
		b->default_break  * b->default_break_scale, \
		b->default_spring * b->default_spring_scale, \
		b->default_damp   * b->default_damp_scale, \
		-1, -1, -1, 1, \
		b->default_beam_diameter);

	// now 'parse' the options
	options_pointer = options;
	while (*options_pointer != 0)
	{
		switch (*options_pointer)
		{
			case 'i':	// invisible
				b->beams[pos].type = BEAM_INVISIBLE;
				break;
			case 'v':	// visible
				b->beams[pos].type = BEAM_NORMAL;
				break;
			case 'r':
				b->beams[pos].bounded = ROPE;
				break;
			case 's':
				b->beams[pos].bounded = SUPPORTBEAM;
				break;
		}
		options_pointer++;
	}

	return result;
}

int BeamSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}


//// FileInfoSerializer
FileInfoSerializer::FileInfoSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = commandTrigger = "fileinfo";
	s->registerModuleSerializer(this);
}

void FileInfoSerializer::initData(rig_t *rig)
{
	rig->fileinfo = new fileinfo_t();
	memset(rig->fileinfo, 0, sizeof(fileinfo_t));
	initiated = true;
}

int FileInfoSerializer::deserialize(char *line, rig_t *rig)
{
	if(!initiated) initData(rig);
	return checkRes(1, sscanf(line, "fileinfo %s, %i, %i", rig->fileinfo->uniquetruckid, &rig->fileinfo->categoryid, &rig->fileinfo->truckversion));
}
int FileInfoSerializer::serialize(char *line, rig_t *rig)
{
	return sprintf(line, "fileinfo %s, %i, %i\n", rig->fileinfo->uniquetruckid, rig->fileinfo->categoryid, rig->fileinfo->truckversion);
}


//// AuthorSerializer
AuthorSerializer::AuthorSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = commandTrigger = "author";
	s->registerModuleSerializer(this);
}

void AuthorSerializer::initData(rig_t *rig)
{
	rig->fileauthors = new fileauthors_t();
	// beware or memsetting std::* !
	rig->fileauthors->authors.clear();
	initiated = true;
}

int AuthorSerializer::deserialize(char *line, rig_t *rig)
{
	if(!initiated) initData(rig);
	fileauthors_t *a = rig->fileauthors;
	
	int authorid;
	char authorname[255], authoremail[255], authortype[255];
	authorinfo_t author;
	author.id = -1;
	strcpy(author.email, "unknown");
	strcpy(author.name,  "unknown");
	strcpy(author.type,  "unknown");

	int result = sscanf(line,"author %s %i %s %s", authortype, &authorid, authorname, authoremail);
	if (result < 1 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (author) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}
	//replace '_' with ' '
	char *tmp = authorname;
	while (*tmp!=0) {if (*tmp=='_') *tmp=' ';tmp++;};
	//fill the struct now
	author.id = authorid;
	if(strnlen(authortype, 250) > 0)
		strncpy(author.type, authortype, 255);
	if(strnlen(authorname, 250) > 0)
		strncpy(author.name, authorname, 255);
	if(strnlen(authoremail, 250) > 0)
		strncpy(author.email, authoremail, 255);

	a->authors.push_back(author);
	return result;
}
int AuthorSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}

//// EngineSerializer
EngineSerializer::EngineSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = sectionTrigger = "engine";
	s->registerModuleSerializer(this);
}

void EngineSerializer::initData(rig_t *rig)
{
	rig->engine = new engine_section_t();
	memset(rig->engine, 0, sizeof(engine_section_t));
	initiated = true;
}

int EngineSerializer::deserialize(char *line, rig_t *rig)
{
	// ignore section header
	if(!strcmp(line, "engine"))
		return 1;

	if(!initiated) initData(rig);

	// shortcuts
	engine_section_t *e = rig->engine;
	
	//parse engine
	int numgears;
	if(rig->driveable == MACHINE)
		// ignore engine section on machines
		return 1;

	rig->driveable = TRUCK;
	int result = sscanf(line, "%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", \
		&e->minrpm, \
		&e->maxrpm, \
		&e->torque, \
		&e->dratio, \
		&e->rear, \
		&e->gears[0],&e->gears[1],&e->gears[2],&e->gears[3],&e->gears[4],&e->gears[5],&e->gears[6],&e->gears[7],&e->gears[8],&e->gears[9],&e->gears[10],&e->gears[11],&e->gears[12],&e->gears[13],&e->gears[14],&e->gears[15]
		);
	
	if (result < 7 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Engine) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}
	for (numgears = 0; numgears < MAX_GEARS; numgears++)
		if (e->gears[numgears] <= 0)
			break;
	if (numgears < 3)
	{
		//LogManager::getSingleton().logMessage("Trucks with less than 3 gears are not supported! " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}
	e->numgears = numgears;
	return result;
}

int EngineSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}


//// CamerasSerializer
CamerasSerializer::CamerasSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = sectionTrigger = "cameras";
	s->registerModuleSerializer(this);
}

void CamerasSerializer::initData(rig_t *rig)
{
	rig->cameras = new cameras_section_t();
	memset(rig->cameras, 0, sizeof(cameras_section_t));
	initiated = true;
}

int CamerasSerializer::deserialize(char *line, rig_t *rig)
{
	// ignore section header
	if(!strcmp(line, "cameras"))
		return 1;

	if(!initiated) initData(rig);

	// shortcuts
	cameras_section_t *c = rig->cameras;
	
	int nodepos, nodedir, dir;
	int result = sscanf(line,"%i, %i, %i",&nodepos,&nodedir,&dir);
	if (result < 3 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Camera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}

	if(c->free_camera > MAX_CAMERAS)
	{
		return -1;
	}

	c->cameras[c->free_camera].nodepos = nodepos;
	c->cameras[c->free_camera].nodedir = nodedir;
	c->cameras[c->free_camera].dir = dir;
	c->free_camera++;
	return result;
}

int CamerasSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}


//// ShocksSerializer
ShocksSerializer::ShocksSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = sectionTrigger = "shocks";
	s->registerModuleSerializer(this);
}

void ShocksSerializer::initData(rig_t *rig)
{
	rig->shocks = new shocks_section_t();
	memset(rig->shocks, 0, sizeof(shocks_section_t));
	initiated = true;
}

int ShocksSerializer::deserialize(char *line, rig_t *rig)
{
	// ignore section header
	if(!strcmp(line, "shocks"))
		return 1;

	if(!rig->beams)
	{
		LogManager::getSingleton().logMessage("Nodes section must come after the beams section");
		return -1;
	}

	if(!initiated) initData(rig);

	// shortcuts
	shocks_section_t *s = rig->shocks;
	beams_section_t  *b = rig->beams;
	nodes_section_t  *n = rig->nodes;

	int id1, id2;
	float sp, d, sbound,lbound,precomp;
	char options[50] = "n";
	int result = sscanf(line, "%i, %i, %f, %f, %f, %f, %f, %s", &id1, &id2, &sp, &d, &sbound, &lbound, &precomp, options);
	if (result < 7 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Shock) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}

	// checks ...
	if(b->free_beam >= MAX_BEAMS)
	{
		//LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}
	if(s->free_shock >= MAX_BEAMS)
	{
		//LogManager::getSingleton().logMessage("shock limit reached ("+StringConverter::toString(MAX_SHOCKS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}
	if (id1 >= n->free_node || id2 >= n->free_node)
	{
		LogManager::getSingleton().logMessage("Error: unknown node number in shocks section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
		exit(4);
	}

	// options
	int htype = BEAM_HYDRO;
	int shockflag = SHOCK_FLAG_NORMAL;

	// now 'parse' the options
	char *options_pointer = options;
	while (*options_pointer != 0)
	{
		switch (*options_pointer)
		{
			case 'i':	// invisible
				htype      = BEAM_INVISIBLE_HYDRO;
				shockflag |= SHOCK_FLAG_INVISIBLE;
				break;
			case 'l':
			case 'L':
				shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
				shockflag |= SHOCK_FLAG_LACTIVE;
				s->free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
				break;
			case 'r':
			case 'R':
				shockflag &= ~SHOCK_FLAG_NORMAL; // not normal anymore
				shockflag |= SHOCK_FLAG_RACTIVE;
				s->free_active_shock++; // this has no array associated with it. its just to determine if there are active shocks!
				break;
			case 'm':
				{
					// metric values: calculate sbound and lbound now
					float beam_length = n->nodes[id1].AbsPosition.distance(n->nodes[id2].AbsPosition);
					sbound = sbound / beam_length;
					lbound = lbound / beam_length;
				}
				break;
		}
		options_pointer++;
	}
	int pos = add_beam(rig, &n->nodes[id1], &n->nodes[id2], htype, b->default_break * 4.0f, sp, d, -1.0f, sbound, lbound, precomp);
	b->beams[pos].shock = &s->shocks[s->free_shock];
	s->shocks[s->free_shock].beamid = pos;
	s->shocks[s->free_shock].flags = shockflag;
	s->free_shock++;

	return result;
}

int ShocksSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}


//// HydrosSerializer
HydrosSerializer::HydrosSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	name = sectionTrigger = "hydros";
	s->registerModuleSerializer(this);
}

void HydrosSerializer::initData(rig_t *rig)
{
	rig->hydros = new hydros_section_t();
	memset(rig->hydros, 0, sizeof(hydros_section_t));
	initiated = true;
}

int HydrosSerializer::deserialize(char *line, rig_t *rig)
{
	// ignore section header
	if(!strcmp(line, "hydros"))
		return 1;

	if(!rig->beams)
	{
		LogManager::getSingleton().logMessage("Hydros section must come after the beams section");
		return -1;
	}

	if(!initiated) initData(rig);

	// shortcuts
	hydros_section_t *h = rig->hydros;
	beams_section_t  *b = rig->beams;
	nodes_section_t  *n = rig->nodes;

	//parse hydros
	int id1, id2;
	float ratio;
	char options[50] = "n";
	float startDelay=0;
	float stopDelay=0;
	char startFunction[50]="";
	char stopFunction[50]="";

	int result = sscanf(line, "%i, %i, %f, %s %f, %f, %s %s",&id1,&id2,&ratio,options,&startDelay,&stopDelay,startFunction,stopFunction);
	if (result < 3 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Hydro) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}

	int htype = BEAM_HYDRO;

	// FIXME: separate init_beam and setup_beam to be able to set all parameters after creation
	// this is just ugly:
	char *options_pointer = options;
	while (*options_pointer != 0)
	{
		if(*options_pointer=='i')
		{
			htype = BEAM_INVISIBLE_HYDRO;
			break;
		}
		options_pointer++;
	}

	if (id1 >= n->free_node || id2 >= n->free_node)
	{
		LogManager::getSingleton().logMessage("Error: unknown node number in hydros section ("+StringConverter::toString(id1)+","+StringConverter::toString(id2)+")");
		exit(6);
	};

	if(b->free_beam >= MAX_BEAMS)
	{
		//LogManager::getSingleton().logMessage("beams limit reached ("+StringConverter::toString(MAX_BEAMS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}
	if(h->free_hydro >= MAX_HYDROS)
	{
		//LogManager::getSingleton().logMessage("hydros limit reached ("+StringConverter::toString(MAX_HYDROS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}

	int pos = add_beam(rig, &n->nodes[id1], &n->nodes[id2], htype, b->default_break * b->default_break_scale, b->default_spring * b->default_spring_scale, b->default_damp * b->default_damp_scale);
	h->hydro[h->free_hydro]=pos;
	h->free_hydro++;
	b->beams[pos].Lhydro = b->beams[pos].L;
	b->beams[pos].hydroRatio = ratio;
	b->beams[pos].animOption = 0;


	// now 'parse' the options
	options_pointer = options;
	while (*options_pointer != 0)
	{
		switch (*options_pointer)
		{
			case 'i':	// invisible
				b->beams[pos].type        = BEAM_INVISIBLE_HYDRO;
				break;
			case 'n':	// normal
				b->beams[pos].type        = BEAM_HYDRO;
				b->beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
				break;
			case 's': // speed changing hydro
				b->beams[pos].hydroFlags |= HYDRO_FLAG_SPEED;
				break;
			case 'a':
				b->beams[pos].hydroFlags |= HYDRO_FLAG_AILERON;
				break;
			case 'r':
				b->beams[pos].hydroFlags |= HYDRO_FLAG_RUDDER;
				break;
			case 'e':
				b->beams[pos].hydroFlags |= HYDRO_FLAG_ELEVATOR;
				break;
			case 'u':
				b->beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_ELEVATOR);
				break;
			case 'v':
				b->beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_ELEVATOR);
				break;
			case 'x':
				b->beams[pos].hydroFlags |= (HYDRO_FLAG_AILERON | HYDRO_FLAG_RUDDER);
				break;
			case 'y':
				b->beams[pos].hydroFlags |= (HYDRO_FLAG_REV_AILERON | HYDRO_FLAG_RUDDER);
				break;
			case 'g':
				b->beams[pos].hydroFlags |= (HYDRO_FLAG_ELEVATOR | HYDRO_FLAG_RUDDER);
				break;
			case 'h':
				b->beams[pos].hydroFlags |= (HYDRO_FLAG_REV_ELEVATOR | HYDRO_FLAG_RUDDER);
				break;
		}
		options_pointer++;
		// if you use the i flag on its own, add the direction to it
		if(b->beams[pos].type == BEAM_INVISIBLE_HYDRO && !b->beams[pos].hydroFlags)
			b->beams[pos].hydroFlags |= HYDRO_FLAG_DIR;
	}
	return result;
}

int HydrosSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}


//// HELPER functions below
int add_beam(rig_t *rig, node_t *p1, node_t *p2, int type, float strength, float spring, float damp, float length, float shortbound, float longbound, float precomp, float diameter)
{
	// shortcuts
	nodes_section_t *n = rig->nodes;
	beams_section_t *b = rig->beams;
	
	int pos = b->free_beam;

	b->beams[pos].p1 = p1;
	b->beams[pos].p2 = p2;
	b->beams[pos].p2truck = 0;
	b->beams[pos].type = type;
	if (length < 0)
	{
		//calculate the length
		Vector3 t = p1->RelPosition - p2->RelPosition;
		b->beams[pos].L = precomp * t.length();
	} else
	{
		b->beams[pos].L = length;
	}
	b->beams[pos].k                 = spring;
	b->beams[pos].d                 = damp;
	b->beams[pos].broken            = 0;
	b->beams[pos].Lhydro            = b->beams[pos].L;
	b->beams[pos].refL              = b->beams[pos].L;
	b->beams[pos].hydroRatio        = 0;
	b->beams[pos].hydroFlags        = 0;
	b->beams[pos].animFlags         = 0;
	b->beams[pos].stress            = 0;
	b->beams[pos].lastforce         = Vector3::ZERO;
	b->beams[pos].iscentering       = false;
	b->beams[pos].isOnePressMode    = 0;
	b->beams[pos].isforcerestricted = false;
	b->beams[pos].autoMovingMode    = 0;
	b->beams[pos].autoMoveLock      = false;
	b->beams[pos].pressedCenterMode = false;
	b->beams[pos].disabled          = false;
	b->beams[pos].shock             = 0;
	if (b->default_deform < b->beam_creak)
		b->default_deform = b->beam_creak;
	b->beams[pos].default_deform    = b->default_deform  * b->default_deform_scale;
	b->beams[pos].minmaxposnegstress = b->default_deform * b->default_deform_scale;
	b->beams[pos].maxposstress      = b->default_deform  * b->default_deform_scale;
	b->beams[pos].maxnegstress      = - b->beams[pos].maxposstress;
	b->beams[pos].plastic_coef      = b->default_plastic_coef;
	b->beams[pos].default_plastic_coef = b->default_plastic_coef;
	b->beams[pos].strength          = strength;
	b->beams[pos].iStrength         = strength;
	b->beams[pos].diameter          = b->default_beam_diameter;
	b->beams[pos].minendmass        = 1;
	b->beams[pos].diameter          = diameter;
	b->beams[pos].scale             = 0;
	if (shortbound != -1.0)
	{
		b->beams[pos].bounded       = SHOCK1;
		b->beams[pos].shortbound    = shortbound;
		b->beams[pos].longbound     = longbound;

	} else
	{
		b->beams[pos].bounded       = NOSHOCK;
	}

	// no visuals here
	b->beams[pos].mSceneNode        = 0;
	b->beams[pos].mEntity           = 0;

	b->free_beam++;
	return pos;
}

void init_node(rig_t *rig, int pos, float x, float y, float z, int type, float m, int iswheel, float friction, int id, int wheelid, float nfriction, float nvolume, float nsurface, float nloadweight)
{
	nodes_section *n = rig->nodes;
	n->nodes[pos].AbsPosition    = Vector3(x, y, z);
	n->nodes[pos].RelPosition    = Vector3(x, y, z) - n->origin;
	n->nodes[pos].smoothpos      = n->nodes[pos].AbsPosition;
	n->nodes[pos].iPosition      = Vector3(x, y, z);
	if(pos != 0)
		n->nodes[pos].iDistance  = (n->nodes[0].AbsPosition - Vector3(x, y, z)).squaredLength();
	else
		n->nodes[pos].iDistance  = 0;
	n->nodes[pos].Velocity       = Vector3::ZERO;
	n->nodes[pos].Forces         = Vector3::ZERO;
	n->nodes[pos].locked         = m < 0.0;
	n->nodes[pos].mass           = m;
	n->nodes[pos].iswheel        = iswheel;
	n->nodes[pos].wheelid        = wheelid;
	n->nodes[pos].friction_coef  = nfriction;
	n->nodes[pos].volume_coef    = nvolume;
	n->nodes[pos].surface_coef   = nsurface;
	if (nloadweight >= 0.0f)
	{
		n->nodes[pos].masstype   = NODE_LOADED;
		n->nodes[pos].overrideMass = true;
		n->nodes[pos].mass       = nloadweight;
	}
	n->nodes[pos].disable_particles = false;
	n->nodes[pos].masstype       = type;
	n->nodes[pos].contactless    = 0;
	n->nodes[pos].contacted      = 0;
	n->nodes[pos].lockednode     = 0;
	n->nodes[pos].buoyanceForce  = Vector3::ZERO;
	n->nodes[pos].buoyancy       = rig->globals->truckmass / 15.0f;//DEFAULT_BUOYANCY;
	n->nodes[pos].lastdrag       = Vector3(0,0,0);
	// XXX: TOFIX
	//n->nodes[pos].gravimass      = Vector3(0, RoRFrameListener::getGravity()*m, 0);
	n->nodes[pos].gravimass      = Vector3(0, n->gravitation * m, 0);
	n->nodes[pos].wetstate       = DRY;
	n->nodes[pos].isHot          = false;
	n->nodes[pos].overrideMass   = false;
	n->nodes[pos].id             = id;
	n->nodes[pos].colltesttimer  = 0;
	n->nodes[pos].iIsSkin        = false;
	n->nodes[pos].isSkin         = n->nodes[pos].iIsSkin;
	n->nodes[pos].pos            = pos;
	if (type == NODE_LOADED)
		n->masscount++;
}
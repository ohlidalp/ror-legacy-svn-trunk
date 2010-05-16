
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
	s->registerModuleSerializer(this);
	s->addSectionHandler("globals", this);
}

void GlobalsSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->globals = new globals_section_t();
	memset(rig->globals, 0, sizeof(globals_section_t));
	initiated = true;
}

int GlobalsSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
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
	return sprintf(line, "globals\n%f, %f, %s\n", rig->globals->truckmass, rig->globals->loadmass, rig->globals->texname);
}

int GlobalsSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}

//// NodeSerializer
NodeSerializer::NodeSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("nodes", this);
}

void NodeSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->nodes = new nodes_section_t();
	memset(rig->nodes, 0, sizeof(nodes_section_t));
	// set some defaults, important!
	rig->nodes->gravitation = -9.81f;
	initiated = true;
}

int NodeSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
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

int NodeSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}


void NodeSerializer::init_node(rig_t *rig, int pos, float x, float y, float z, int type, float m, int iswheel, float friction, int id, int wheelid, float nfriction, float nvolume, float nsurface, float nloadweight)
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


//// BeamSerializer
BeamSerializer::BeamSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("beams", this);
}

void BeamSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->beams = new beams_section_t();
	memset(rig->beams, 0, sizeof(beams_section_t));

	// init basics
	rig->beams->default_spring        = DEFAULT_SPRING;
	rig->beams->default_spring_scale  = 1;
	rig->beams->default_damp          = DEFAULT_DAMP;
	rig->beams->default_damp_scale    = 1;
	rig->beams->default_deform        = BEAM_DEFORM;
	rig->beams->default_deform_scale  = 1;
	rig->beams->default_break         = BEAM_BREAK;
	rig->beams->default_break_scale   = 1;
	rig->beams->default_beam_diameter = DEFAULT_BEAM_DIAMETER;
	strcpy(rig->beams->default_beam_material, "tracks/beam");
	rig->beams->default_plastic_coef  = 0;
	rig->beams->beam_creak            = BEAM_CREAK_DEFAULT;


	// done
	initiated = true;
}

int BeamSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
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

int BeamSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	// create visuals for beams
	// root node
	rig->beams->beamsRoot = node->createChildSceneNode();

	// then all the beams
	for(int i = 0; i < rig->beams->free_beam; i++)
	{
	    beam_t *b = &rig->beams->beams[i];
	    if (b->type != BEAM_VIRTUAL)
	    {
		    //setup visuals
		    //the cube is 100x100x100
		    try
		    {
			    b->mEntity = manager->createEntity("beam.mesh");
		    }catch(...)
		    {
			    LogManager::getSingleton().logMessage("error loading mesh: beam.mesh");
		    }

		    // no materialmapping for beams!
		    //		ec->setCastShadows(false);

		    if (b->mEntity && (b->type == BEAM_HYDRO || b->type == BEAM_MARKED))
			    b->mEntity->setMaterialName("tracks/Chrome");
		    else if(b->mEntity)
			    b->mEntity->setMaterialName(rig->beams->default_beam_material);
		    b->mSceneNode = rig->beams->beamsRoot->createChildSceneNode();
		    b->mSceneNode->setScale(b->diameter, b->L, b->diameter);

		    // colourize beams in simple mode
			/*
		    ColourValue c = ColourValue::Blue;
		    if(b->type == BEAM_HYDRO)
			    c = ColourValue::Red;
		    else if(b->type == BEAM_HYDRO)
			    c = ColourValue::Red;
		    MaterialFunctionMapper::replaceSimpleMeshMaterials(b->mEntity, c);
		    b->mSceneNode->attachObject(b->mEntity);
		    b->mSceneNode->setVisible(true);
			*/

			/*
		    printf("nodes %p, %d, from nodes %d to %d: (%3.3f,%3.3f,%3.3f) to (%3.3f,%3.3f,%3.3f)\n",
			    b, i, b->p1->id, b->p2->id,
			    b->p1->RelPosition.x, b->p1->RelPosition.y, b->p1->RelPosition.z,
			    b->p2->RelPosition.x, b->p2->RelPosition.y, b->p2->RelPosition.z
			    );
			*/

	    }
	}
	return 0;
}

int BeamSerializer::add_beam(rig_t *rig, node_t *p1, node_t *p2, int type, float strength, float spring, float damp, float length, float shortbound, float longbound, float precomp, float diameter)
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


//// FileInfoSerializer
FileInfoSerializer::FileInfoSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addCommandHandler("fileinfo", this);
}

void FileInfoSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->fileinfo = new fileinfo_t();
	memset(rig->fileinfo, 0, sizeof(fileinfo_t));
	initiated = true;
}

int FileInfoSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
{
	if(!initiated) initData(rig);
	return checkRes(1, sscanf(line, "fileinfo %s, %i, %i", rig->fileinfo->uniquetruckid, &rig->fileinfo->categoryid, &rig->fileinfo->truckversion));
}
int FileInfoSerializer::serialize(char *line, rig_t *rig)
{
	return sprintf(line, "fileinfo %s, %i, %i\n", rig->fileinfo->uniquetruckid, rig->fileinfo->categoryid, rig->fileinfo->truckversion);
}

int FileInfoSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}


//// AuthorSerializer
AuthorSerializer::AuthorSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addCommandHandler("author", this);
}

void AuthorSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->fileauthors = new fileauthors_t();
	// beware or memsetting std::* !
	rig->fileauthors->authors.clear();
	initiated = true;
}

int AuthorSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
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

int AuthorSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}

//// EngineSerializer
EngineSerializer::EngineSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("engine", this);
}

void EngineSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->engine_section = new engine_section_t();
	memset(rig->engine_section, 0, sizeof(engine_section_t));
	initiated = true;
}

int EngineSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
{
	// ignore section header
	if(!strcmp(line, "engine"))
		return 1;

	if(!initiated) initData(rig);

	// shortcuts
	engine_section_t *e = rig->engine_section;
	
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

int EngineSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}

//// CamerasSerializer
CamerasSerializer::CamerasSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("cameras", this);
}

void CamerasSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->cameras = new cameras_section_t();
	memset(rig->cameras, 0, sizeof(cameras_section_t));
	initiated = true;
}

int CamerasSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
{
	// ignore section header
	if(!strcmp(line, "cameras"))
		return 1;

	if(!initiated) initData(rig);

	// shortcuts
	cameras_section_t *c = rig->cameras;
	nodes_section_t   *n = rig->nodes;
	
	int node_center, node_back, node_left;
	int result = sscanf(line,"%i, %i, %i", &node_center, &node_back, &node_left);
	if (result < 3 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Camera) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}

	if(c->free_camera >= MAX_CAMERAS)
	{
		return -1;
	}

	c->cameras[c->free_camera].nodepos  = &(n->nodes[node_center]);
	c->cameras[c->free_camera].nodedir  = &(n->nodes[node_back]);
	c->cameras[c->free_camera].noderoll = &(n->nodes[node_left]);
	c->free_camera++;
	return result;
}

int CamerasSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}

int CamerasSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}

//// ShocksSerializer
ShocksSerializer::ShocksSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("shocks", this);
}

void ShocksSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->shocks = new shocks_section_t();
	memset(rig->shocks, 0, sizeof(shocks_section_t));
	initiated = true;
}

int ShocksSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
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
	int pos = BeamSerializer::add_beam(rig, &n->nodes[id1], &n->nodes[id2], htype, b->default_break * 4.0f, sp, d, -1.0f, sbound, lbound, precomp);
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

int ShocksSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}

//// HydrosSerializer
HydrosSerializer::HydrosSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("hydros", this);
}

void HydrosSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->hydros = new hydros_section_t();
	memset(rig->hydros, 0, sizeof(hydros_section_t));
	initiated = true;
}

int HydrosSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
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

	int pos = BeamSerializer::add_beam(rig, &n->nodes[id1], &n->nodes[id2], htype, b->default_break * b->default_break_scale, b->default_spring * b->default_spring_scale, b->default_damp * b->default_damp_scale);
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

int HydrosSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}

//// HydrosSerializer
WheelsSerializer::WheelsSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("wheels", this);
	s->addSectionHandler("wheels2", this);
}

void WheelsSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->wheels = new wheels_section_t();
	memset(rig->wheels, 0, sizeof(wheels_section_t));
	initiated = true;
}

int WheelsSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
{
	if(!initiated) initData(rig);

	if (activeSection == "wheels")
	{
		// ignore section header
		if(!strcmp(line, "wheels"))
			return 1;

		//parse wheels
		float radius, width, mass, spring, damp;
		char texf[256];
		char texb[256];
		int rays, node1, node2, snode, braked, propulsed, torquenode;
		int result = sscanf(line,"%f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %s %s",
			&radius,
			&width,
			&rays,
			&node1,
			&node2,
			&snode,
			&braked,
			&propulsed,
			&torquenode,
			&mass,
			&spring,
			&damp,
			texf,
			texb);
		if (result < 14 || result == EOF)
		{
			//LogManager::getSingleton().logMessage("Error parsing File (Wheel) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
			return 0;
		}
		WheelsSerializer::addWheel(rig, radius, width, rays, node1, node2, snode, braked, propulsed, torquenode, mass, spring, damp, texf, texb);
		return result;
	}
	return -1;
}

int WheelsSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}

int WheelsSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return -2;
}

void WheelsSerializer::addWheel(rig_t *rig, float radius, float width, int rays, int node1, int node2, int snode, int braked, int propulsed, int torquenode, float mass, float wspring, float wdamp, char* texf, char* texb, bool meshwheel, float rimradius, bool rimreverse)
{
	int i;
	int nodebase = rig->nodes->free_node;
	int node3;
	int contacter_wheel=1;
	//ignore the width parameter
	width=(rig->nodes->nodes[node1].RelPosition - rig->nodes->nodes[node2].RelPosition).length();
	//enforce the "second node must have a larger Z coordinate than the first" constraint
	if (rig->nodes->nodes[node1].RelPosition.z > rig->nodes->nodes[node2].RelPosition.z)
	{
		//swap
		node3=node1;
		node1=node2;
		node2=node3;
	}
	//ignore the sign of snode, just do the thing automatically
	//if (snode<0) node3=-snode; else node3=snode;
	if (snode<0) snode=-snode;
	bool closest1=false;
	if (snode!=9999) closest1=(rig->nodes->nodes[snode].RelPosition-rig->nodes->nodes[node1].RelPosition).length()<(rig->nodes->nodes[snode].RelPosition-rig->nodes->nodes[node2].RelPosition).length();

	//unused:
	//Real px=rig->nodes->nodes[node1].Position.x;
	//Real py=rig->nodes->nodes[node1].Position.y;
	//Real pz=rig->nodes->nodes[node1].Position.z;

	Vector3 axis=rig->nodes->nodes[node2].RelPosition-rig->nodes->nodes[node1].RelPosition;
	axis.normalise();
	Vector3 rayvec = axis.perpendicular() * radius;
	// old rayvec:
	//Vector3 rayvec=Vector3(0, radius, 0);
	Quaternion rayrot=Quaternion(Degree(-360.0/(Real)(rays*2)), axis);
	for (i=0; i<rays; i++)
	{
		//with propnodes and variable friction
//			init_node(nodebase+i*2, px+radius*sin((Real)i*6.283185307179/(Real)rays), py+radius*cos((Real)i*6.283185307179/(Real)rays), pz, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width);
		Vector3 raypoint;
		raypoint=rig->nodes->nodes[node1].RelPosition+rayvec;
		rayvec=rayrot*rayvec;
		NodeSerializer::init_node(rig, nodebase+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, rig->wheels->free_wheel, rig->nodes->default_node_friction, rig->nodes->default_node_volume, rig->nodes->default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// outer ring has wheelid%2 != 0
		rig->nodes->nodes[nodebase+i*2].iswheel = rig->wheels->free_wheel*2+1;

		if (contacter_wheel)
		{
			// init contacters if required
			s->getSectionModule(rig, "contacters");

			rig->contacters->contacters[rig->contacters->free_contacter].nodeid      = nodebase + i * 2;
			rig->contacters->contacters[rig->contacters->free_contacter].contacted   = 0;
			rig->contacters->contacters[rig->contacters->free_contacter].opticontact = 0;
			rig->contacters->free_contacter++;
		}
//			init_node(nodebase+i*2+1, px+radius*sin((Real)i*6.283185307179/(Real)rays), py+radius*cos((Real)i*6.283185307179/(Real)rays), pz+width, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width);
		raypoint=rig->nodes->nodes[node2].RelPosition+rayvec;

		rayvec=rayrot*rayvec;
		NodeSerializer::init_node(rig, nodebase+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, rig->wheels->free_wheel, rig->nodes->default_node_friction, rig->nodes->default_node_volume, rig->nodes->default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// inner ring has wheelid%2 == 0
		rig->nodes->nodes[nodebase+i*2+1].iswheel = rig->wheels->free_wheel*2+2;
		if (contacter_wheel)
		{
			rig->contacters->contacters[rig->contacters->free_contacter].nodeid      = nodebase+i*2+1;
			rig->contacters->contacters[rig->contacters->free_contacter].contacted   = 0;
			rig->contacters->contacters[rig->contacters->free_contacter].opticontact = 0;
			rig->contacters->free_contacter++;
		}
		//wheel object
		rig->wheels->wheels[rig->wheels->free_wheel].nodes[i*2]   = &rig->nodes->nodes[nodebase+i*2];
		rig->wheels->wheels[rig->wheels->free_wheel].nodes[i*2+1] = &rig->nodes->nodes[nodebase+i*2+1];
	}
	rig->nodes->free_node += 2 * rays;
	for (i=0; i<rays; i++)
	{
		//bounded
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node1],          &rig->nodes->nodes[nodebase+i*2],              BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		//bounded
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node2],          &rig->nodes->nodes[nodebase+i*2+1],            BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node2],          &rig->nodes->nodes[nodebase+i*2],              BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node1],          &rig->nodes->nodes[nodebase+i*2+1],            BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		//reinforcement
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node1],          &rig->nodes->nodes[nodebase+i*2],              BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2],   &rig->nodes->nodes[nodebase+i*2+1],            BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2],   &rig->nodes->nodes[nodebase+((i+1)%rays)*2],   BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2+1], &rig->nodes->nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2+1], &rig->nodes->nodes[nodebase+((i+1)%rays)*2],   BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		//reinforcement
		//BeamSerializer::add_beam(this, &nodes[nodebase+i*2], &nodes[nodebase+((i+1)%rays)*2+1], manager, parent, BEAM_INVISIBLE, default_break, wspring, wdamp);

		if (snode!=9999)
		{
			//back beams //BEAM_VIRTUAL

			if (closest1)
			{
				BeamSerializer::add_beam(rig, &rig->nodes->nodes[snode], &rig->nodes->nodes[nodebase+i*2],   BEAM_VIRTUAL, rig->beams->default_break, wspring, wdamp);
			} else
			{
				BeamSerializer::add_beam(rig, &rig->nodes->nodes[snode], &rig->nodes->nodes[nodebase+i*2+1], BEAM_VIRTUAL, rig->beams->default_break, wspring, wdamp);
			}

			/* THIS ALMOST WORKS BUT IT IS INSTABLE AT SPEED !!!!
			//rigidifier version
			if(free_rigidifier >= MAX_RIGIDIFIERS)
			{
				LogManager::getSingleton().logMessage("rigidifiers limit reached ...");
			}

			int na=(closest1)?node2:node1;
			int nb=(closest1)?node1:node2;
			int nc=snode;
			rigidifiers[free_rigidifier].a=&nodes[na];
			rigidifiers[free_rigidifier].b=&nodes[nb];
			rigidifiers[free_rigidifier].c=&nodes[nc];
			rigidifiers[free_rigidifier].k=wspring;
			rigidifiers[free_rigidifier].d=wdamp;
			rigidifiers[free_rigidifier].alpha=2.0*acos((nodes[na].RelPosition-nodes[nb].RelPosition).getRotationTo(nodes[nc].RelPosition-nodes[nb].RelPosition).w);
			rigidifiers[free_rigidifier].lastalpha=rigidifiers[free_rigidifier].alpha;
			rigidifiers[free_rigidifier].beama=0;
			rigidifiers[free_rigidifier].beamc=0;
			//searching for associated beams
			for (int i=0; i<free_beam; i++)
			{
				if ((beams[i].p1==&nodes[na] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[na] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beama=&beams[i];
				if ((beams[i].p1==&nodes[nc] && beams[i].p2==&nodes[nb]) || (beams[i].p2==&nodes[nc] && beams[i].p1==&nodes[nb])) rigidifiers[free_rigidifier].beamc=&beams[i];
			}
			free_rigidifier++;
			*/
		}
	}
	//wheel object
	rig->wheels->wheels[rig->wheels->free_wheel].braked           = braked;
	rig->wheels->wheels[rig->wheels->free_wheel].nodebase           = nodebase;
	rig->wheels->wheels[rig->wheels->free_wheel].nrays            = rays;
	rig->wheels->wheels[rig->wheels->free_wheel].propulsed        = propulsed;
	rig->wheels->wheels[rig->wheels->free_wheel].nbnodes          = 2 * rays;
	rig->wheels->wheels[rig->wheels->free_wheel].refnode0         = &rig->nodes->nodes[node1];
	rig->wheels->wheels[rig->wheels->free_wheel].refnode1         = &rig->nodes->nodes[node2];
	rig->wheels->wheels[rig->wheels->free_wheel].radius           = radius;
	rig->wheels->wheels[rig->wheels->free_wheel].speed            = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].rp               = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].rp1              = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].rp2              = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].rp3              = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].width            = width;
	rig->wheels->wheels[rig->wheels->free_wheel].arm              = &rig->nodes->nodes[torquenode];
	rig->wheels->wheels[rig->wheels->free_wheel].lastContactInner = Vector3::ZERO;
	rig->wheels->wheels[rig->wheels->free_wheel].lastContactOuter = Vector3::ZERO;
	rig->wheels->wheels[rig->wheels->free_wheel].rimradius        = rimradius;
	rig->wheels->wheels[rig->wheels->free_wheel].rimreverse       = rimreverse;
	strncpy(rig->wheels->wheels[rig->wheels->free_wheel].texf, texf, 255);
	strncpy(rig->wheels->wheels[rig->wheels->free_wheel].texb, texb, 255);

	if (propulsed>0)
	{
		//for inter-differential locking
		rig->wheels->proppairs[rig->wheels->proped_wheels] = rig->wheels->free_wheel;
		rig->wheels->proped_wheels++;
	}
	if (braked) rig->wheels->braked_wheels++;
	//find near attach
	Real l1 = (rig->nodes->nodes[node1].RelPosition - rig->nodes->nodes[torquenode].RelPosition).length();
	Real l2 = (rig->nodes->nodes[node2].RelPosition - rig->nodes->nodes[torquenode].RelPosition).length();
	if (l1 < l2)
		rig->wheels->wheels[rig->wheels->free_wheel].near_attach = &rig->nodes->nodes[node1];
	else
		rig->wheels->wheels[rig->wheels->free_wheel].near_attach = &rig->nodes->nodes[node2];

	rig->wheels->vwheels[rig->wheels->free_wheel].meshwheel = meshwheel;
	rig->wheels->free_wheel++;
}

void WheelsSerializer::addWheel2(rig_t *rig, float radius, float radius2, float width, int rays, int node1, int node2, int snode, int braked, int propulsed, int torquenode, float mass, float wspring, float wdamp, float wspring2, float wdamp2, char* texf, char* texb)
{
	int i;
	int nodebase=rig->nodes->free_node;
	int node3;
	int contacter_wheel=1;
	//ignore the width parameter
	width=(rig->nodes->nodes[node1].RelPosition - rig->nodes->nodes[node2].RelPosition).length();
	//enforce the "second node must have a larger Z coordinate than the first" constraint
	if (rig->nodes->nodes[node1].RelPosition.z > rig->nodes->nodes[node2].RelPosition.z)
	{
		//swap
		node3=node1;
		node1=node2;
		node2=node3;
	}
	//ignore the sign of snode, just do the thing automatically
	//if (snode<0) node3=-snode; else node3=snode;
	if (snode<0) snode=-snode;
	bool closest1=false;
	if (snode!=9999) closest1=(rig->nodes->nodes[snode].RelPosition - rig->nodes->nodes[node1].RelPosition).length() < (rig->nodes->nodes[snode].RelPosition - rig->nodes->nodes[node2].RelPosition).length();

	//unused:
	//Real px=nodes[node1].Position.x;
	//Real py=nodes[node1].Position.y;
	//Real pz=nodes[node1].Position.z;

	Vector3 axis = rig->nodes->nodes[node2].RelPosition - rig->nodes->nodes[node1].RelPosition;
	axis.normalise();
	Vector3 rayvec=Vector3(0, radius, 0);
	Quaternion rayrot=Quaternion(Degree(-360.0/(Real)rays), axis);
	Quaternion rayrot2=Quaternion(Degree(-180.0/(Real)rays), axis);
	Vector3 rayvec2=Vector3(0, radius2, 0);
	rayvec2=rayrot2*rayvec2;
	//rim nodes
	for (i=0; i<rays; i++)
	{
		//with propnodes
		Vector3 raypoint=rig->nodes->nodes[node1].RelPosition+rayvec;
		NodeSerializer::init_node(rig, nodebase+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, -1, -1, rig->wheels->free_wheel, rig->nodes->default_node_friction, rig->nodes->default_node_volume, rig->nodes->default_node_surface, NODE_LOADWEIGHT_DEFAULT);
		// outer ring has wheelid%2 != 0
		rig->nodes->nodes[nodebase+i*2].iswheel = rig->wheels->free_wheel*2+1;

		raypoint=rig->nodes->nodes[node2].RelPosition+rayvec;
		NodeSerializer::init_node(rig, nodebase+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, mass/(4.0*rays),1, -1, -1, rig->wheels->free_wheel, rig->nodes->default_node_friction, rig->nodes->default_node_volume, rig->nodes->default_node_surface, NODE_LOADWEIGHT_DEFAULT);

		// inner ring has wheelid%2 == 0
		rig->nodes->nodes[nodebase+i*2+1].iswheel = rig->wheels->free_wheel*2+2;
		//wheel object
		rig->wheels->wheels[rig->wheels->free_wheel].nodes[i*2]   = &rig->nodes->nodes[nodebase+i*2];
		rig->wheels->wheels[rig->wheels->free_wheel].nodes[i*2+1] = &rig->nodes->nodes[nodebase+i*2+1];
		rayvec=  rayrot*rayvec;
	}
	//tire nodes
	for (i=0; i<rays; i++)
	{
		//with propnodes and variable friction
		Vector3 raypoint=rig->nodes->nodes[node1].RelPosition+rayvec2;
		NodeSerializer::init_node(rig, nodebase+2*rays+i*2, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, 0.67*mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1, rig->wheels->free_wheel, rig->nodes->default_node_friction, rig->nodes->default_node_volume, rig->nodes->default_node_surface);
		// outer ring has wheelid%2 != 0
		rig->nodes->nodes[nodebase+2*rays+i*2].iswheel = rig->wheels->free_wheel*2+1;
		if (contacter_wheel)
		{
			rig->contacters->contacters[rig->contacters->free_contacter].nodeid      = nodebase+2*rays+i*2;
			rig->contacters->contacters[rig->contacters->free_contacter].contacted   = 0;
			rig->contacters->contacters[rig->contacters->free_contacter].opticontact = 0;
			rig->contacters->free_contacter++;;
		}
		raypoint=rig->nodes->nodes[node2].RelPosition+rayvec2;
		NodeSerializer::init_node(rig, nodebase+2*rays+i*2+1, raypoint.x, raypoint.y, raypoint.z, NODE_NORMAL, 0.33*mass/(2.0*rays),1, WHEEL_FRICTION_COEF*width, -1,  rig->wheels->free_wheel, rig->nodes->default_node_friction, rig->nodes->default_node_volume, rig->nodes->default_node_surface);

		// inner ring has wheelid%2 == 0
		rig->nodes->nodes[nodebase+2*rays+i*2+1].iswheel = rig->wheels->free_wheel*2+2;
		if (contacter_wheel)
		{
			rig->contacters->contacters[rig->contacters->free_contacter].nodeid      = nodebase+2*rays+i*2+1;
			rig->contacters->contacters[rig->contacters->free_contacter].contacted   = 0;
			rig->contacters->contacters[rig->contacters->free_contacter].opticontact = 0;
			rig->contacters->free_contacter++;;
		}
		//wheel object
//			wheels->wheels[free_wheel].nodes[i*2]=&nodes[nodebase+i*2];
//			wheels->wheels[free_wheel].nodes[i*2+1]=&nodes[nodebase+i*2+1];
		rayvec2=rayrot*rayvec2; //this is not a bug
	}
	rig->nodes->free_node += 4 * rays;
	for (i=0; i<rays; i++)
	{
		//rim
		//bounded
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node1], &rig->nodes->nodes[nodebase+i*2], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node2], &rig->nodes->nodes[nodebase+i*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp, -1.0, 0.66, 0.0);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node2], &rig->nodes->nodes[nodebase+i*2], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node1], &rig->nodes->nodes[nodebase+i*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		//reinforcement
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[node1], &rig->nodes->nodes[nodebase+i*2], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2], &rig->nodes->nodes[nodebase+i*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2], &rig->nodes->nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2+1], &rig->nodes->nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2], &rig->nodes->nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		//reinforcement
		BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+i*2+1], &rig->nodes->nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, rig->beams->default_break, wspring, wdamp);
		if (snode!=9999)
		{
			//back beams
			if (closest1)
			{
				BeamSerializer::add_beam(rig, &rig->nodes->nodes[snode], &rig->nodes->nodes[nodebase+i*2], BEAM_VIRTUAL, rig->beams->default_break, wspring, wdamp);
			} else
			{
				BeamSerializer::add_beam(rig, &rig->nodes->nodes[snode], &rig->nodes->nodes[nodebase+i*2+1], BEAM_VIRTUAL, rig->beams->default_break, wspring, wdamp);
			}
		}
		//tire
		//band
		//init_beam(free_beam , &nodes[nodebase+2*rays+i*2], &nodes[nodebase+2*rays+i*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		//pressure_beams[free_pressure_beam]=free_beam-1; free_pressure_beam++;
		int pos;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2], &rig->nodes->nodes[nodebase+2*rays+((i+1)%rays)*2], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2], &rig->nodes->nodes[nodebase+2*rays+((i+1)%rays)*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2+1], &rig->nodes->nodes[nodebase+2*rays+((i+1)%rays)*2], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2+1], &rig->nodes->nodes[nodebase+2*rays+((i+1)%rays)*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		//walls
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2], &rig->nodes->nodes[nodebase+i*2], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2], &rig->nodes->nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2+1], &rig->nodes->nodes[nodebase+i*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2+1], &rig->nodes->nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		//reinforcement
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2], &rig->nodes->nodes[nodebase+i*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2], &rig->nodes->nodes[nodebase+((i+1)%rays)*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2+1], &rig->nodes->nodes[nodebase+i*2], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[nodebase+2*rays+i*2+1], &rig->nodes->nodes[nodebase+((i+1)%rays)*2], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		//backpressure, bounded
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[node1], &rig->nodes->nodes[nodebase+2*rays+i*2], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2, -1.0, radius/radius2, 0.0);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
		pos=BeamSerializer::add_beam(rig, &rig->nodes->nodes[node2], &rig->nodes->nodes[nodebase+2*rays+i*2+1], BEAM_INVISIBLE, rig->beams->default_break, wspring2, wdamp2, -1.0, radius/radius2, 0.0);
		rig->beams->pressure_beams[rig->beams->free_pressure_beam]=pos; rig->beams->free_pressure_beam++;
	}
	//wheel object
	rig->wheels->wheels[rig->wheels->free_wheel].braked     = braked;
	rig->wheels->wheels[rig->wheels->free_wheel].propulsed  = propulsed;
	rig->wheels->wheels[rig->wheels->free_wheel].nbnodes    = 2 * rays;
	rig->wheels->wheels[rig->wheels->free_wheel].refnode0   = &rig->nodes->nodes[node1];
	rig->wheels->wheels[rig->wheels->free_wheel].refnode1   = &rig->nodes->nodes[node2];
	rig->wheels->wheels[rig->wheels->free_wheel].radius     = radius;
	rig->wheels->wheels[rig->wheels->free_wheel].speed      = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].width      = width;
	rig->wheels->wheels[rig->wheels->free_wheel].rp         = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].rp1        = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].rp2        = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].rp3        = 0;
	rig->wheels->wheels[rig->wheels->free_wheel].arm        = &rig->nodes->nodes[torquenode];
	if (propulsed)
	{
		//for inter-differential locking
		rig->wheels->proppairs[rig->wheels->proped_wheels] = rig->wheels->free_wheel;
		rig->wheels->proped_wheels++;
	}
	if (braked) rig->wheels->braked_wheels++;
	
	//find near attach
	Real l1 = (rig->nodes->nodes[node1].RelPosition - rig->nodes->nodes[torquenode].RelPosition).length();
	Real l2 = (rig->nodes->nodes[node2].RelPosition - rig->nodes->nodes[torquenode].RelPosition).length();
	if (l1 < l2)
		rig->wheels->wheels[rig->wheels->free_wheel].near_attach = &rig->nodes->nodes[node1];
	else
		rig->wheels->wheels[rig->wheels->free_wheel].near_attach = &rig->nodes->nodes[node2];

	rig->wheels->free_wheel++;
}


//// ContactersSerializer
ContactersSerializer::ContactersSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("contacters", this);
}

void ContactersSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->contacters = new contacters_section_t();
	memset(rig->contacters, 0, sizeof(contacters_section_t));
	initiated = true;
}

int ContactersSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
{
	// ignore section header
	if(!strcmp(line, "contacters"))
		return 1;

	if(!rig->nodes)
	{
		LogManager::getSingleton().logMessage("Contacters section must come after the nodes section");
		return -1;
	}

	if(!initiated) initData(rig);

	//parse contacters
	int id1;
	int result = sscanf(line,"%i", &id1);
	if (result < 1 || result == EOF)
	{
		//LogManager::getSingleton().logMessage("Error parsing File (Contacters) " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return 0;
	}

	if(rig->contacters->free_contacter >= MAX_CONTACTERS)
	{
		//LogManager::getSingleton().logMessage("contacters limit reached ("+StringConverter::toString(MAX_CONTACTERS)+"): " + String(fname) +" line " + StringConverter::toString(linecounter) + ". trying to continue ...");
		return -1;
	}

	rig->contacters->contacters[rig->contacters->free_contacter].nodeid      = id1;
	rig->contacters->contacters[rig->contacters->free_contacter].contacted   = 0;
	rig->contacters->contacters[rig->contacters->free_contacter].opticontact = 0;
	rig->nodes->nodes[id1].iIsSkin = true;
	rig->contacters->free_contacter++;
	return result;
}

int ContactersSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}

int ContactersSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}


//// BrakesSerializer
BrakesSerializer::BrakesSerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	// setup the base descriptions, etc, then register ourself
	s->registerModuleSerializer(this);
	s->addSectionHandler("brakes", this);
}

void BrakesSerializer::initData(rig_t *rig)
{
	if(initiated) return;
	rig->brakes = new brakes_section_t();
	memset(rig->brakes, 0, sizeof(brakes_section_t));
	initiated = true;
}

int BrakesSerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
{
	// ignore section header
	if(!strcmp(line, "brakes"))
		return 1;

	if(!initiated) initData(rig);

	// parse brakes
	int result = sscanf(line,"%f, %f", &rig->brakes->brakeforce, &rig->brakes->hbrakeforce);
	// Read in footbrake force and handbrake force. If handbrakeforce is not present, set it to the default value 2*footbrake force to preserve older functionality
	if (result == 1)
		rig->brakes->hbrakeforce = 2.0f * rig->brakes->brakeforce;
	return result;
}

int BrakesSerializer::serialize(char *line, rig_t *rig)
{
	// XXX: TODO
	return -1;
}

int BrakesSerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}


//// DummySerializer : dummy for all not yet implemented sections and commands
DummySerializer::DummySerializer(RoRSerializer *s) : RoRSerializationModule(s)
{
	s->registerModuleSerializer(this);
	// unsupported sections
	s->addSectionHandler("cinecam", this);
	s->addSectionHandler("help", this);
	s->addSectionHandler("engoption", this);
	s->addSectionHandler("wheels2", this);
	s->addSectionHandler("meshwheels", this);
	s->addSectionHandler("shocks", this);
	s->addSectionHandler("shocks2", this);
	s->addSectionHandler("hydros", this);
	s->addSectionHandler("animators", this);
	s->addSectionHandler("commands", this);
	s->addSectionHandler("commands2", this);
	s->addSectionHandler("rotators", this);
	s->addSectionHandler("ropes", this);
	s->addSectionHandler("fixes", this);
	s->addSectionHandler("minimass", this);
	s->addSectionHandler("ties", this);
	s->addSectionHandler("ropables", this);
	s->addSectionHandler("hookgroup", this);
	s->addSectionHandler("particles", this);
	s->addSectionHandler("rigidifiers", this);
	s->addSectionHandler("torquecurve", this);
	s->addSectionHandler("axles", this);
	s->addSectionHandler("managedmaterials", this);
	s->addSectionHandler("flares", this);
	s->addSectionHandler("materialflarebindings", this);
	s->addSectionHandler("flares2", this);
	s->addSectionHandler("flexbodies", this);
	s->addSectionHandler("submesh", this);
	s->addSectionHandler("texcoords", this);
	s->addSectionHandler("cab", this);
	s->addSectionHandler("backmesh", this);
	s->addSectionHandler("exhausts", this);
	s->addSectionHandler("guisettings", this);
	s->addSectionHandler("soundsources", this);
	s->addSectionHandler("props", this);
	s->addSectionHandler("wings", this);
	s->addSectionHandler("airbrakes", this);
	s->addSectionHandler("turboprops", this);
	s->addSectionHandler("fusedrag", this);
	s->addSectionHandler("turbojets", this);
	s->addSectionHandler("pistonprops", this);
	s->addSectionHandler("screwprops", this);
	s->addSectionHandler("slidenodes", this);
	s->addSectionHandler("railgroups", this);
	s->addSectionHandler("globeams", this);

	// unsupported commands
	s->addCommandHandler("set_inertia_defaults", this);
	s->addCommandHandler("forwardcommands", this);
	s->addCommandHandler("importcommands", this);
	s->addCommandHandler("set_beam_defaults", this);
	s->addCommandHandler("set_beam_defaults_scale", this);
	s->addCommandHandler("set_node_defaults", this);
	s->addCommandHandler("enable_advanced_deformation", this);
	s->addCommandHandler("rollon", this);
	s->addCommandHandler("rescuer", this);
	s->addCommandHandler("set_managedmaterials_options", this);
	s->addCommandHandler("add_animation", this);
	s->addCommandHandler("set_skeleton_settings", this);
	s->addCommandHandler("disabledefaultsounds", this);
	s->addCommandHandler("slidenode_connect_instantly", this);
	s->addCommandHandler("set_collision_range", this);
}

void DummySerializer::initData(rig_t *rig)
{
	initiated = true;
}

int DummySerializer::deserialize(char *line, rig_t *rig, std::string activeSection)
{
	// ignore everything
	return 0;
}

int DummySerializer::serialize(char *line, rig_t *rig)
{
	return 0;
}

int DummySerializer::initResources(Ogre::SceneManager *manager, Ogre::SceneNode *node, rig_t *rig)
{
	return 0;
}

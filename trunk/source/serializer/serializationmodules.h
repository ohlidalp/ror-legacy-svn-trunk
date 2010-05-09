#include "string.h"
#include "RoRPrerequisites.h"
#include "BeamData.h"
#include "serializer.h"
// format description:
// http://wiki.rigsofrods.com/pages/Truck_Description_File


// global functions
int add_beam(rig_t *rig, node_t *p1, node_t *p2, int type, float strength, float spring, float damp, float length=-1, float shortbound=-1, float longbound=-1, float precomp=1, float diameter=DEFAULT_BEAM_DIAMETER);
void init_node(rig_t *rig, int pos, float x, float y, float z, int type, float m=10, int iswheel=0, float friction=CHASSIS_FRICTION_COEF, int id=-1, int wheelid=-1, float nfriction=NODE_FRICTION_COEF_DEFAULT, float nvolume=NODE_VOLUME_COEF_DEFAULT, float nsurface=NODE_SURFACE_COEF_DEFAULT, float nloadweight=NODE_LOADWEIGHT_DEFAULT);

// the serialization modules
class RoRSerializer;

class GlobalsSerializer : public RoRSerializationModule
{
public:
	GlobalsSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};

class NodeSerializer : public RoRSerializationModule
{
public:
	NodeSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};

class BeamSerializer : public RoRSerializationModule
{
public:
	BeamSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};


class FileInfoSerializer : public RoRSerializationModule
{
public:
	FileInfoSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};


class AuthorSerializer : public RoRSerializationModule
{
public:
	AuthorSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};


class EngineSerializer : public RoRSerializationModule
{
public:
	EngineSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};


class CamerasSerializer : public RoRSerializationModule
{
public:
	CamerasSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};


class ShocksSerializer : public RoRSerializationModule
{
public:
	ShocksSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};


class HydrosSerializer : public RoRSerializationModule
{
public:
	HydrosSerializer(RoRSerializer *s);
	void initData(rig_t *rig);
	int deserialize(char *line, rig_t *rig);
	int serialize(char *line, rig_t *rig);
};

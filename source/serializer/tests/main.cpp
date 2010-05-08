
// example program to test the serializer

#include "serializer.h"
#include "BeamData.h"
#include "Ogre.h"

using namespace std;
using namespace Ogre;

int main(int argc, char **argv)
{
	String filename = "b6b0UID-semi.truck";
	
	// start ogre
	Ogre::Root* root = new Ogre::Root("", "", "ogre.log");
	
	// add resource dir
	ResourceGroupManager::getSingleton().addResourceLocation("streams/dev/dafsemi.zip", "Zip");
	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();	

	//	load the file
	DataStreamPtr ds = ResourceGroupManager::getSingleton().openResource(filename);
	
	// root struct
	rig_t rig;
	
	// create the serializer
	RoRSerializer *s = new RoRSerializer();
	int result = s->loadRig(ds, &rig);
	printf("result: %d\n", result);

	delete root;
}


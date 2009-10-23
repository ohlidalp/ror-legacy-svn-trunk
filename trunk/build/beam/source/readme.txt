brainstorm about new engine layout below, WIP! (please comment!)

== high level description ==
The concept has to be simplified so it is much more easy to provide a new
physics function for the Beam engine. This flexibility should be provided by
the plugin mechanism (more later). For example we will assume that we want to
write a game that is using the Beam engine. The game would create the main
beam engine (Beam) instance. It then adds a BeamRig to this engine and adds
plugins to this instance. Different rigs can have different plugin sets.

For example we write a truck racing game, then we will create a rig
(Beam->createRig()) and add some plugins to our class we want to use:
- rig->addPlugin(BeamPluginGroundFriction)
- rig->addPlugin(BeamPluginShocks)

Please note that every plugin attaches itself to the rig instance and will:
- enhance the truck configuration by adding new keywords or sections to the
  config file
- will be included in the calc() routing of the instance

After all plugin instances are created for the rig instance, we will load a
model into the rig:
- BeamIO::loadModel("myExample.truck", rig);

this will process the truck file "myexample.truck". All configuration except
BeamNodes and BeamBeams are stored inside the plugins instance.

So we got the rig loaded and its now ready to be simulated. The Beam class
managed all calculation jobs and just issues a rig->calc() call. The rig is
then responsible to call the plugins in the right order to simulate the truck
correctly.

== Logical class hierarchy ==
Beam (main engine, top level, syncs the calculations)
- BeamRig (a Beam structure which represents one truck)
 - BeamPlugin (plugin added to the rig which processes data during IO actions
   (loading/saving truck) and which is used for the calculation

== file overview ==
Beam.h:
- main header file

BeamPrerequisites.h:
- forward declaration of main structures

BeamPlugin.h:
- abstract class that should form an interface to all Plugins for the beam
  engine

BeamIO.h:
- parses files and load the data into the provided BeamRig* instance

== naming scheme ==
 - Beam*
 - BeamPlugin*



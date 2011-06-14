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
#include "TruckHUD.h"
#include "InputEngine.h"
#include "engine.h"
#include "turboprop.h"
#include "language.h"
#include "TorqueCurve.h"
#include "RoRFrameListener.h"

using namespace std;
using namespace Ogre;


TruckHUD::TruckHUD() :
	torqueLineStream(0)
{
	width = 450;
	border = 10;

	// init counters
	maxVelos[NOT_DRIVEABLE] = -9999;
	minVelos[NOT_DRIVEABLE] = 9999;
	avVelos[NOT_DRIVEABLE] = 0;

	maxVelos[TRUCK] = -9999;
	minVelos[TRUCK] = 9999;
	avVelos[TRUCK] = 0;

	maxVelos[AIRPLANE] = -9999;
	minVelos[AIRPLANE] = 9999;
	avVelos[AIRPLANE] = 0;

	maxVelos[BOAT] = -9999;
	minVelos[BOAT] = 9999;
	avVelos[BOAT] = 0;

	maxVelos[MACHINE] = -9999;
	minVelos[MACHINE] = 9999;
	avVelos[MACHINE] = 0;

	maxPosVerG[NOT_DRIVEABLE] =  -9999;
	maxPosVerG[TRUCK] =  -9999;
	maxPosVerG[AIRPLANE] = -9999;
	maxPosVerG[BOAT] = -9999;
	maxPosVerG[MACHINE] = -9999;
	maxNegVerG[NOT_DRIVEABLE] =  9999;
	maxNegVerG[TRUCK] =  9999;
	maxNegVerG[AIRPLANE] = 9999;
	maxNegVerG[BOAT] = 9999;
	maxNegVerG[MACHINE] = 9999;

	maxPosSagG[NOT_DRIVEABLE] =  -9999;
	maxPosSagG[TRUCK] =  -9999;
	maxPosSagG[AIRPLANE] = -9999;
	maxPosSagG[BOAT] = -9999;
	maxPosSagG[MACHINE] = -9999;
	maxNegSagG[NOT_DRIVEABLE] =  9999;
	maxNegSagG[TRUCK] =  9999;
	maxNegSagG[AIRPLANE] = 9999;
	maxNegSagG[BOAT] = 9999;
	maxNegSagG[MACHINE] = 9999;

	maxPosLatG[NOT_DRIVEABLE] =  -9999;
	maxPosLatG[TRUCK] =  -9999;
	maxPosLatG[AIRPLANE] = -9999;
	maxPosLatG[BOAT] = -9999;
	maxPosLatG[MACHINE] = -9999;
	maxNegLatG[NOT_DRIVEABLE] =  9999;
	maxNegLatG[TRUCK] =  9999;
	maxNegLatG[AIRPLANE] = 9999;
	maxNegLatG[BOAT] = 9999;
	maxNegLatG[MACHINE] = 9999;

	lastTorqueModel = "";
	lastTorqueRatio = 0;

	truckHUD = OverlayManager::getSingleton().getByName("tracks/TruckInfoBox");


	updatetime=0;
}

TruckHUD::~TruckHUD()
{
}

void TruckHUD::show(bool value)
{
	if(value) {
		truckHUD->show();
	} else {
		truckHUD->hide();
	}
}

bool TruckHUD::isVisible()
{
	return truckHUD->isVisible();
}

void TruckHUD::checkOverflow(Ogre::OverlayElement* e)
{
	int newval = e->getLeft() + e->getWidth() + border;
	if(newval > this->width)
	{
		this->width = newval;
		OverlayElement *panel = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MainPanel");
		panel->setWidth(width);
	}
}

bool TruckHUD::update(float dt, Beam *truck, SceneManager *mSceneMgr, Camera* mCamera, RenderWindow* mWindow, bool visible)
{
	OverlayElement *descl = 0;

	//only update every 300 ms
	if(visible)
	{
		updatetime -= dt;
		if(updatetime <= 0)
		{
			// update now, reset timer
			updatetime = 0.3;
		} else
			// dont update visuals, only count stats
			visible = false;
	}


	if(visible)
	{
		OverlayElement* oTruckname = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Truckname");
		oTruckname->setCaption(truck->getTruckName());
		checkOverflow(oTruckname);

		OverlayElement* oTruckauthor = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Truckauthor");
		std::vector<authorinfo_t> file_authors = truck->getAuthors();
		if(file_authors.size() > 0)
		{
			String author_string = String("");
			for(unsigned int i=0;i<file_authors.size();i++)
				author_string += file_authors[i].name + String(" ");
			oTruckauthor->setCaption(_L("Authors: ") + author_string);
		} else
		{
			oTruckauthor->setCaption(_L("(no author information available)"));
		}
		checkOverflow(oTruckauthor);
		std::vector<std::string> desc = truck->getDescription();

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/DescriptionLine1");
		if(desc.size() > 0)
			descl->setCaption(desc[0]);
		else
			descl->setCaption("");
		checkOverflow(descl);

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/DescriptionLine2");
		if(desc.size() > 1)
			descl->setCaption(desc[1]);
		else
			descl->setCaption("");
		checkOverflow(descl);

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/DescriptionLine3");
		if(desc.size() > 2)
			descl->setCaption(desc[2]);
		else
			descl->setCaption("");
		checkOverflow(descl);

		int beamCount = truck->getBeamCount();
		beam_t *beam = truck->getBeams();
		int beambroken = 0;
		float beamstress = 0;
		int beamdeformed = 0;
		float average_deformation = 0;
		float current_deformation = 0;
		float mass = truck->getTotalMass();

		for(int i=0; i<beamCount; i++, beam++)
		{
			if(beam->broken != 0)
				beambroken++;
			beamstress += beam->stress;
			current_deformation = fabs(beam->L-beam->refL);
			if (fabs(current_deformation) > 0.0001f && beam->type != BEAM_HYDRO && beam->type != BEAM_INVISIBLE_HYDRO) beamdeformed++;
			average_deformation += current_deformation;
		}

		char beamcountstr[256];
		sprintf(beamcountstr, "%d", beamCount);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamTotal");
		descl->setCaption(_L("beam count: ") + String(beamcountstr));
		checkOverflow(descl);

		char beambrokenstr[256];
		sprintf(beambrokenstr, "%0.2f", ((float)beambroken/(float)beamCount)*100);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamBroken");
		descl->setCaption(_L("broken: ") + TOSTRING(beambroken) + string(" (") + beambrokenstr + string("%)"));
		checkOverflow(descl);

		char beamhealthstr[256];
		float health = ((float)beambroken/(float)beamCount) * 10 + ((float)beamdeformed/(float)beamCount);
		if(health<1)
		{
			sprintf(beamhealthstr, "%0.2f", (1-health)*100);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setCaption(_L("health: ") + string(beamhealthstr) + string("%"));
			checkOverflow(descl);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setColour(ColourValue(0.6,0.8,0.4,1));
			checkOverflow(descl);
		}else if(health>1)
		{
			health = ((float)beambroken/(float)beamCount) * 3;
			if(health>1)
				health=1;
			sprintf(beamhealthstr, "%0.2f", (health)*100);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setCaption(_L("destruction: ") + string(beamhealthstr) + string("%"));
			checkOverflow(descl);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setColour(ColourValue(0.8,0.4,0.4,1));
			checkOverflow(descl);
		}
		char beamdeformedstr[256];
		sprintf(beamdeformedstr, "%0.2f", ((float)beamdeformed/(float)beamCount)*100);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamDeformed");
		descl->setCaption(_L("deformed: ") + TOSTRING(beamdeformed) + string(" (") + beamdeformedstr + string("%)"));
		checkOverflow(descl);

		char beamavdeformedstr[256];
		sprintf(beamavdeformedstr, "%0.4f", ((float)average_deformation/(float)beamCount));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageBeamDeformation");
		descl->setCaption(_L("average deformation: ") + string(beamavdeformedstr));
		checkOverflow(descl);

		char beamstressstr[256];
		sprintf(beamstressstr, "%+08.0f", 1-(float)beamstress/(float)beamCount);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageBeamStress");
		descl->setCaption(_L("average stress: ") + string(beamstressstr));
		checkOverflow(descl);

		int nodeCount = truck->getNodeCount();
		int wcount =  truck->getWheelNodeCount();
		char nodecountstr[256];
		sprintf(nodecountstr, "%d (wheels: %d)", nodeCount, wcount);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/NodeCount");
		descl->setCaption(_L("node count: ") + String(nodecountstr));
		checkOverflow(descl);

		char truckmassstr[256];
		String massstr = _L("current mass:");
		sprintf(truckmassstr, "%s %8.2f kg (%.2f tons)", massstr.c_str(), mass, mass/1000);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/TruckWeight");
		descl->setCaption(truckmassstr);
		checkOverflow(descl);

		char geesstr[256];
		Vector3 accv=truck->cameranodeacc/truck->cameranodecount;
		truck->cameranodeacc=Vector3::ZERO;
		truck->cameranodecount=0;
		float longacc=accv.dotProduct((truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranodedir[0]].RelPosition).normalisedCopy());
		float latacc=accv.dotProduct((truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranoderoll[0]].RelPosition).normalisedCopy());
		Ogre::Vector3 upv=(truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranodedir[0]].RelPosition).crossProduct(-(truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranoderoll[0]].RelPosition));
		upv.normalise();
		
		float gravity = DEFAULT_GRAVITY;
		if(RoRFrameListener::eflsingleton)
			gravity = RoRFrameListener::eflsingleton->getGravity();
		float vertacc=(fabs(gravity))-(accv.dotProduct((truck->nodes[truck->cameranodepos[0]].RelPosition-(truck->nodes[truck->cameranodepos[0]].RelPosition + upv)).normalisedCopy()));
		sprintf(geesstr, _L("Gees: Vertical %1.2fg // Saggital %1.2fg // Lateral %1.2fg"), vertacc/(fabs(gravity)), longacc/(fabs(gravity)), latacc/(fabs(gravity)));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Gees");
		descl->setCaption(geesstr);
		checkOverflow(descl);

		//maxGees
		if(truck->driveable == TRUCK || truck->driveable == AIRPLANE || truck->driveable == BOAT)
		{
			if(vertacc > maxPosVerG[truck->driveable])
				maxPosVerG[truck->driveable] = vertacc;
			if(vertacc < maxNegVerG[truck->driveable])
				maxNegVerG[truck->driveable] = vertacc;

			if(longacc > maxPosSagG[truck->driveable])
				maxPosSagG[truck->driveable] = longacc;
			if(longacc < maxNegSagG[truck->driveable])
				maxNegSagG[truck->driveable] = longacc;

			if(latacc > maxPosLatG[truck->driveable])
				maxPosLatG[truck->driveable] = latacc;
			if(latacc < maxNegLatG[truck->driveable])
				maxNegLatG[truck->driveable] = latacc;

			sprintf(geesstr, _L("maxG: V %1.2fg %1.2fg // S %1.2fg %1.2fg // L %1.2fg %1.2fg"), 
					maxPosVerG[truck->driveable]/(fabs(gravity)), 
					maxNegVerG[truck->driveable]/(fabs(gravity)), 
					maxPosSagG[truck->driveable]/(fabs(gravity)), 
					maxNegSagG[truck->driveable]/(fabs(gravity)), 
					maxPosLatG[truck->driveable]/(fabs(gravity)), 
					maxNegLatG[truck->driveable]/(fabs(gravity))
				);

			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Gees2");
			descl->setCaption(geesstr);
			checkOverflow(descl);
		}
	}

	Vector3 hdir = Vector3::ZERO;
	if(truck->cameranodepos[0]>=0)
		hdir = truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranodedir[0]].RelPosition;
	hdir.normalise();
	float g_along_hdir=hdir.dotProduct(truck->ffforce/10000.0);

	//LOG("ffforce: " + TOSTRING(truck->ffforce.x) + ", " + TOSTRING(truck->ffforce.y) + ", " + TOSTRING(truck->ffforce.z) + " / direction: " + TOSTRING(hdir.x) + ", " + TOSTRING(hdir.y) + ", " + TOSTRING(hdir.z));


	// TODO: FIX THIS!
	//char rpmstring[256];
	//sprintf(rpmstring, "current GForces: %2.2f", g_along_hdir);
	//OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM")->setCaption(rpmstring);

	// always update these statistics, also if not visible!
	String rpmsstr = _L("current RPM:");
	if(truck->driveable == TRUCK && truck->engine)
	{
		char rpmstring[256];
		sprintf(rpmstring, "%s %.0f / %.0f", rpmsstr.c_str(), truck->engine->getRPM(), truck->engine->getMaxRPM()*1.25);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		descl->setCaption(rpmstring);
		checkOverflow(descl);

		/*
		// TODO: FIX
		if(visible && truck->engine->getTorqueCurve())
		{
			// torque curve handling
			if(!torqueLineStream) initTorqueOverlay();

			// update
			String model = truck->engine->getTorqueCurve()->getTorqueModel();
			SimpleSpline *usedSpline = truck->engine->getTorqueCurve()->getUsedSpline();
			float ratio = truck->engine->getRPM() / float(truck->engine->getMaxRPM());

			if(lastTorqueModel != model && usedSpline)
			{
				// update the torque curve
				LOG("regenerating torque displayed curve");
				for(int i=0;i<1000;i++)
				{
					float factor = i/1000.0f;
					float res = usedSpline->interpolate(factor).y;
					//LOG(TOSTRING(i)+ " - "+TOSTRING(res)+ " - "+TOSTRING(factor));
					torqueLineStream->setExactValue(0, i, res);
				}
				lastTorqueModel = model;
			}
			// now proceed
			// reset old ratio:
			torqueLineStream->setExactValue(1, lastTorqueRatio * 1000.0f, 0);

			// no overflow
			if(ratio <= 1 && usedSpline)
			{
				// get data for the new ratio
				float res = usedSpline->interpolate(ratio).y;
				float x = ratio * 1000.0f;

				//update the new point
				torqueLineStream->setExactValue(1, x, res);

				lastTorqueRatio = ratio;
			}
		}
		*/
	} else if (truck->driveable == AIRPLANE){
		char rpmstring[256];

		if(truck->aeroengines[0] && truck->aeroengines[1] && truck->aeroengines[2] && truck->aeroengines[3])
			sprintf(rpmstring, "%s %.0f / %.0f / %.0f / %.0f", rpmsstr.c_str(), truck->aeroengines[0]->getRPM(), truck->aeroengines[1]->getRPM(), truck->aeroengines[2]->getRPM(), truck->aeroengines[3]->getRPM());
		else if(truck->aeroengines[0] && truck->aeroengines[1] && truck->aeroengines[2] && !truck->aeroengines[3])
			sprintf(rpmstring, "%s %.0f / %.0f / %.0f", rpmsstr.c_str(), truck->aeroengines[0]->getRPM(), truck->aeroengines[1]->getRPM(), truck->aeroengines[2]->getRPM());
		else if(truck->aeroengines[0] && truck->aeroengines[1] && !truck->aeroengines[2] && !truck->aeroengines[3])
			sprintf(rpmstring, "%s %.0f / %.0f", rpmsstr.c_str(), truck->aeroengines[0]->getRPM(), truck->aeroengines[1]->getRPM());
		else if(truck->aeroengines[0] && !truck->aeroengines[1] && !truck->aeroengines[2] && !truck->aeroengines[3])
			sprintf(rpmstring, "%s %.0f", rpmsstr.c_str(), truck->aeroengines[0]->getRPM());

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		descl->setCaption(rpmstring);
		checkOverflow(descl);
	} else
	{
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		descl->setCaption("");
	}

	String cspeedstr = _L("current Speed:");
	String maxspstr = _L("max Speed:");

	if(truck->driveable == TRUCK)
	{
		char velostring[256];
		float velocityKMH = (truck->WheelSpeed * 3.6);
		if(velocityKMH > maxVelos[truck->driveable])
			maxVelos[truck->driveable] = velocityKMH;
		if(velocityKMH < minVelos[truck->driveable])
			minVelos[truck->driveable] = velocityKMH;

		// round values if zero ==> no flickering +/-
		if (fabs(velocityKMH) < 1)
			velocityKMH = 0;
		float velocityMPH = velocityKMH / 1.609;
			sprintf(velostring, "%s %+.0f km/h (%+.0f mp/h)", cspeedstr.c_str(), ceil(velocityKMH), ceil(velocityMPH));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);

		float velocityMaxMPH = maxVelos[truck->driveable] / 1.609;
		float velocityMinMPH = minVelos[truck->driveable] / 1.609;
		sprintf(velostring, "%s %+.0fkmh/%+.0fmph, min: %+.0fkmh/%+.0fmph", maxspstr.c_str(), ceil(maxVelos[truck->driveable]), ceil(velocityMaxMPH), ceil(minVelos[truck->driveable]), ceil(velocityMinMPH));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity");
		descl->setCaption("");
	}
		else if(truck->driveable == AIRPLANE)
	{
		char velostring[256];
		float velocity = truck->nodes[0].Velocity.length()*1.9438;

		if(velocity > maxVelos[truck->driveable])
			maxVelos[truck->driveable] = velocity;
		if(velocity < minVelos[truck->driveable])
			minVelos[truck->driveable] = velocity;

		// round values if zero ==> no flickering +/-
		if (fabs(velocity) < 1)
			velocity = 0;
		sprintf(velostring, "%s %+.0f kn", cspeedstr.c_str(), ceil(velocity));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);
		sprintf(velostring, "%s %+.0fkn, min: %+.0f kn", maxspstr.c_str(), ceil(maxVelos[truck->driveable]), ceil(minVelos[truck->driveable]));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);

		float alt = truck->nodes[0].AbsPosition.y*1.1811;
		String altstr = _L("Altitude:");
		sprintf(velostring, "%s %+.0f m", altstr.c_str(), ceil(alt));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);
	}
		else if(truck->driveable == BOAT)
	{
		char velostring[256];
		Vector3 hdir=truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranodedir[0]].RelPosition;
		hdir.normalise();
		float velocity=hdir.dotProduct(truck->nodes[truck->cameranodepos[0]].Velocity)*1.9438;

		if(velocity > maxVelos[truck->driveable])
			maxVelos[truck->driveable] = velocity;
		if(velocity < minVelos[truck->driveable])
			minVelos[truck->driveable] = velocity;

		sprintf(velostring, "%s %+.0f kn", cspeedstr.c_str(),ceil(velocity));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);

		sprintf(velostring, "%s %+.0fkn, min: %+.0f kn", maxspstr.c_str(),ceil(maxVelos[truck->driveable]), ceil(minVelos[truck->driveable]));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity");
		descl->setCaption("");
	}
		else if(truck->driveable == MACHINE)
	{
		OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity")->setCaption("");
		OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity")->setCaption("");
		OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity")->setCaption("");
	}
	OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/TimeStats")->setCaption("");

	if(visible)
	{
		// update commands

		//clear them first
		for(int i=1;i<=COMMANDS_VISIBLE;i++)
		{
			char commandOverlayID[256];
			sprintf(commandOverlayID, "tracks/TruckInfoBox/Command%d", i);
			descl = OverlayManager::getSingleton().getOverlayElement(commandOverlayID);
			descl->setCaption(string(""));
		}

		int j = 0;
		for(int i=1,j=0;i<MAX_COMMANDS && j<COMMANDS_VISIBLE;i+=2)
		{
			if (truck->commandkey[i].description.size() == 0)
				continue;

			j++;
			char commandID[256];
			String keyStr="";

			sprintf(commandID, "COMMANDS_%02d", i);
			int eventID = INPUTENGINE.resolveEventName(String(commandID));
			String keya = INPUTENGINE.getEventCommand(eventID);
			sprintf(commandID, "COMMANDS_%02d", i+1);
			eventID = INPUTENGINE.resolveEventName(String(commandID));
			String keyb = INPUTENGINE.getEventCommand(eventID);
			//cut off expl
			if(keya.size()>6 && keya.substr(0,5) == "EXPL+") keya = keya.substr(5);
			if(keyb.size()>6 && keyb.substr(0,5) == "EXPL+") keyb = keyb.substr(5);

			keyStr = keya + "/" + keyb;

			char commandOverlayID[256];
			sprintf(commandOverlayID, "tracks/TruckInfoBox/Command%d", j);
			descl = OverlayManager::getSingleton().getOverlayElement(commandOverlayID);
			descl->setCaption(keyStr + ": " + truck->commandkey[i].description);
			checkOverflow(descl);
		}

		// hide command section title if no commands
		if(j == 0)
		{
			OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CommandsTitleLabel")->setCaption("");
		} else
		{
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CommandsTitleLabel");
			descl->setCaption(_L("Commands:"));
			checkOverflow(descl);
		}


	}
	return true;
}

void TruckHUD::initTorqueOverlay()
{
	// TODO: fix
	return;
	OverlayContainer *lineStreamContainer = (OverlayContainer *) (OverlayManager::getSingleton().createOverlayElement("LineStream", "TorqueCurveLineStream"));
	lineStreamContainer->_setPosition(0.37f,  0.6f);
	lineStreamContainer->_setDimensions(0.62f, 0.2f);
	lineStreamContainer->setMaterialName("Core/StatsBlockCenter");

	torqueLineStream = dynamic_cast<Ogre::LineStreamOverlayElement *>(lineStreamContainer);
	torqueLineStream->setNumberOfSamplesForTrace(1000);
	torqueLineStream->setNumberOfTraces(2);
	torqueLineStream->setMoveMode(0);
	torqueLineStream->createVertexBuffer();
	torqueLineStream->defaultStyle();

	torqueLineStream->setTraceInfo(0, ColourValue::Red, "torque");
	torqueLineStream->setTraceInfo(1, ColourValue::Green, "");

	OverlayContainer *oPanel = (OverlayContainer *) (OverlayManager::getSingleton().createOverlayElement("Panel","TorqueCurveLinePanel"));
	oPanel->_setPosition(0.37f,  0.6f);
	oPanel->_setDimensions(0.62f, 0.2f);
	oPanel->setMaterialName("Core/StatsBlockCenter");

	truckHUD->add2D(oPanel);
	truckHUD->add2D(lineStreamContainer);
}

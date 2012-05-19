/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

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

#include "aeroengine.h"
#include "Beam.h"
#include "BeamEngine.h"
#include "InputEngine.h"
#include "language.h"
#include "Ogre.h"
#include "utils.h"

using namespace Ogre;

TruckHUD::TruckHUD()
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

void TruckHUD::checkOverflow(OverlayElement* e)
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
			descl->setCaption(ANSI_TO_UTF(desc[0]));
		else
			descl->setCaption("");
		checkOverflow(descl);

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/DescriptionLine2");
		if(desc.size() > 1)
			descl->setCaption(ANSI_TO_UTF(desc[1]));
		else
			descl->setCaption("");
		checkOverflow(descl);

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/DescriptionLine3");
		if(desc.size() > 2)
			descl->setCaption(ANSI_TO_UTF(desc[2]));
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

		wchar_t beamcountstr[256];
		swprintf(beamcountstr, 256, L"%d", beamCount);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamTotal");
		descl->setCaption(_L("beam count: ") + UTFString(beamcountstr));
		checkOverflow(descl);

		wchar_t beambrokenstr[256];
		swprintf(beambrokenstr, 256, L"%0.2f", ((float)beambroken/(float)beamCount)*100);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamBroken");
		descl->setCaption(_L("broken: ") + TOUTFSTRING(beambroken) + U(" (") + UTFString(beambrokenstr) + U("%)"));
		checkOverflow(descl);

		wchar_t beamhealthstr[256];
		float health = ((float)beambroken/(float)beamCount) * 10 + ((float)beamdeformed/(float)beamCount);
		if(health<1)
		{
			swprintf(beamhealthstr, 256, L"%0.2f", (1-health)*100);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setCaption(_L("health: ") + UTFString(beamhealthstr) + U("%"));
			checkOverflow(descl);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setColour(ColourValue(0.6,0.8,0.4,1));
			checkOverflow(descl);
		}else if(health>1)
		{
			health = ((float)beambroken/(float)beamCount) * 3;
			if(health>1)
				health=1;
			swprintf(beamhealthstr, 256, L"%0.2f", (health)*100);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setCaption(_L("destruction: ") + UTFString(beamhealthstr) + U("%"));
			checkOverflow(descl);
			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamHealth");
			descl->setColour(ColourValue(0.8,0.4,0.4,1));
			checkOverflow(descl);
		}
		wchar_t beamdeformedstr[256];
		swprintf(beamdeformedstr, 256, L"%0.2f", ((float)beamdeformed/(float)beamCount)*100);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/BeamDeformed");
		descl->setCaption(_L("deformed: ") + TOUTFSTRING(beamdeformed) + U(" (") + UTFString(beamdeformedstr) + U("%)"));
		checkOverflow(descl);

		wchar_t beamavdeformedstr[256];
		swprintf(beamavdeformedstr, 256, L"%0.4f", ((float)average_deformation/(float)beamCount));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageBeamDeformation");
		descl->setCaption(_L("average deformation: ") + UTFString(beamavdeformedstr));
		checkOverflow(descl);

		wchar_t beamstressstr[256];
		swprintf(beamstressstr, 256, L"%+08.0f", 1-(float)beamstress/(float)beamCount);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageBeamStress");
		descl->setCaption(_L("average stress: ") + UTFString(beamstressstr));
		checkOverflow(descl);

		int nodeCount = truck->getNodeCount();
		int wcount =  truck->getWheelNodeCount();
		wchar_t nodecountstr[256];
		swprintf(nodecountstr, 256, L"%d (wheels: %d)", nodeCount, wcount);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/NodeCount");
		descl->setCaption(_L("node count: ") + UTFString(nodecountstr));
		checkOverflow(descl);

		wchar_t truckmassstr[256];
		UTFString massstr = _L("current mass:");
		swprintf(truckmassstr, 256, L"%ls %8.2f kg (%.2f tons)", massstr.asWStr_c_str(), mass, mass/1000);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/TruckWeight");
		descl->setCaption(UTFString(truckmassstr));
		checkOverflow(descl);

		wchar_t geesstr[256];
		Vector3 gees = truck->getGForces();
		UTFString tmp = _L("Gees: Vertical %1.2fg // Saggital %1.2fg // Lateral %1.2fg");
		swprintf(geesstr, 256, tmp.asWStr_c_str(), gees.x, gees.y, gees.z);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Gees");
		descl->setCaption(UTFString(geesstr));
		checkOverflow(descl);

		//maxGees
		if(truck->driveable == TRUCK || truck->driveable == AIRPLANE || truck->driveable == BOAT)
		{
			if(gees.x > maxPosVerG[truck->driveable])
				maxPosVerG[truck->driveable] = gees.x;
			if(gees.x < maxNegVerG[truck->driveable])
				maxNegVerG[truck->driveable] = gees.x;

			if(gees.y > maxPosSagG[truck->driveable])
				maxPosSagG[truck->driveable] = gees.y;
			if(gees.y < maxNegSagG[truck->driveable])
				maxNegSagG[truck->driveable] = gees.y;

			if(gees.z > maxPosLatG[truck->driveable])
				maxPosLatG[truck->driveable] = gees.z;
			if(gees.z < maxNegLatG[truck->driveable])
				maxNegLatG[truck->driveable] = gees.z;

			tmp = _L("maxG: V %1.2fg %1.2fg // S %1.2fg %1.2fg // L %1.2fg %1.2fg");
			swprintf(geesstr, 256, tmp.asWStr_c_str(),
					maxPosVerG[truck->driveable],
					maxNegVerG[truck->driveable],
					maxPosSagG[truck->driveable],
					maxNegSagG[truck->driveable],
					maxPosLatG[truck->driveable],
					maxNegLatG[truck->driveable]
				);

			descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/Gees2");
			descl->setCaption(UTFString(geesstr));
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
	UTFString rpmsstr = _L("current RPM:");
	if(truck->driveable == TRUCK && truck->engine)
	{
		wchar_t rpmstring[256];
		rpmsstr = _L("current RPM:");
		swprintf(rpmstring, 256, L"%ls %.0f / %.0f", rpmsstr.asWStr_c_str(), truck->engine->getRPM(), truck->engine->getMaxRPM()*1.25);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		descl->setCaption(UTFString(rpmstring));
		checkOverflow(descl);

	} else if (truck->driveable == AIRPLANE)
	{
		wchar_t rpmstring[256];

		if(truck->aeroengines[0] && truck->aeroengines[1] && truck->aeroengines[2] && truck->aeroengines[3])
			swprintf(rpmstring, 256, L"%ls %.0f / %.0f / %.0f / %.0f", rpmsstr.asWStr_c_str(), truck->aeroengines[0]->getRPM(), truck->aeroengines[1]->getRPM(), truck->aeroengines[2]->getRPM(), truck->aeroengines[3]->getRPM());
		else if(truck->aeroengines[0] && truck->aeroengines[1] && truck->aeroengines[2] && !truck->aeroengines[3])
			swprintf(rpmstring, 256, L"%ls %.0f / %.0f / %.0f", rpmsstr.asWStr_c_str(), truck->aeroengines[0]->getRPM(), truck->aeroengines[1]->getRPM(), truck->aeroengines[2]->getRPM());
		else if(truck->aeroengines[0] && truck->aeroengines[1] && !truck->aeroengines[2] && !truck->aeroengines[3])
			swprintf(rpmstring, 256, L"%ls %.0f / %.0f", rpmsstr.asWStr_c_str(), truck->aeroengines[0]->getRPM(), truck->aeroengines[1]->getRPM());
		else if(truck->aeroengines[0] && !truck->aeroengines[1] && !truck->aeroengines[2] && !truck->aeroengines[3])
			swprintf(rpmstring, 256, L"%ls %.0f", rpmsstr.asWStr_c_str(), truck->aeroengines[0]->getRPM());

		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		descl->setCaption(UTFString(rpmstring));
		checkOverflow(descl);
	} else
	{
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentRPM");
		descl->setCaption("");
	}

	UTFString cspeedstr = _L("current Speed:");
	UTFString maxspstr = _L("max Speed:");

	if(truck->driveable == TRUCK)
	{
		wchar_t velostring[256];
		float velocityKMH = (truck->WheelSpeed * 3.6);
		if(velocityKMH > maxVelos[truck->driveable])
			maxVelos[truck->driveable] = velocityKMH;
		if(velocityKMH < minVelos[truck->driveable])
			minVelos[truck->driveable] = velocityKMH;

		// round values if zero ==> no flickering +/-
		if (fabs(velocityKMH) < 1)
			velocityKMH = 0;
		float velocityMPH = velocityKMH / 1.609;
			swprintf(velostring, 256, L"%ls %+.0f km/h (%+.0f mp/h)", cspeedstr.asWStr_c_str(), ceil(velocityKMH), ceil(velocityMPH));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		descl->setCaption(UTFString(velostring));
		checkOverflow(descl);

		float velocityMaxMPH = maxVelos[truck->driveable] / 1.609;
		float velocityMinMPH = minVelos[truck->driveable] / 1.609;
		swprintf(velostring, 256, L"%ls %+.0fkmh/%+.0fmph, min: %+.0fkmh/%+.0fmph", maxspstr.asWStr_c_str(), ceil(maxVelos[truck->driveable]), ceil(velocityMaxMPH), ceil(minVelos[truck->driveable]), ceil(velocityMinMPH));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		descl->setCaption(UTFString(velostring));
		checkOverflow(descl);
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity");
		descl->setCaption("");
	}
		else if(truck->driveable == AIRPLANE)
	{
		wchar_t velostring[256];
		float velocity = truck->nodes[0].Velocity.length()*1.9438;

		if(velocity > maxVelos[truck->driveable])
			maxVelos[truck->driveable] = velocity;
		if(velocity < minVelos[truck->driveable])
			minVelos[truck->driveable] = velocity;

		// round values if zero ==> no flickering +/-
		if (fabs(velocity) < 1)
			velocity = 0;
		swprintf(velostring, 256, L"%ls %+.0f kn", cspeedstr.asWStr_c_str(), ceil(velocity));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		descl->setCaption(UTFString(velostring));
		checkOverflow(descl);
		swprintf(velostring, 256, L"%ls %+.0fkn, min: %+.0f kn", maxspstr.asWStr_c_str(), ceil(maxVelos[truck->driveable]), ceil(minVelos[truck->driveable]));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		descl->setCaption(UTFString(velostring));
		checkOverflow(descl);

		float alt = truck->nodes[0].AbsPosition.y*1.1811;
		UTFString altstr = _L("Altitude:");
		swprintf(velostring, 256, L"%ls %+.0f m", altstr.asWStr_c_str(), ceil(alt));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/AverageVelocity");
		descl->setCaption(velostring);
		checkOverflow(descl);
	}
		else if(truck->driveable == BOAT)
	{
		wchar_t velostring[256];
		hdir=truck->nodes[truck->cameranodepos[0]].RelPosition-truck->nodes[truck->cameranodedir[0]].RelPosition;
		hdir.normalise();
		float velocity=hdir.dotProduct(truck->nodes[truck->cameranodepos[0]].Velocity)*1.9438;

		if(velocity > maxVelos[truck->driveable])
			maxVelos[truck->driveable] = velocity;
		if(velocity < minVelos[truck->driveable])
			minVelos[truck->driveable] = velocity;

		swprintf(velostring, 256, L"%ls %+.0f kn", cspeedstr.asWStr_c_str(),ceil(velocity));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/CurrentVelocity");
		descl->setCaption(UTFString(velostring));
		checkOverflow(descl);

		swprintf(velostring, 256, L"%ls %+.0fkn, min: %+.0f kn", maxspstr.asWStr_c_str(),ceil(maxVelos[truck->driveable]), ceil(minVelos[truck->driveable]));
		descl = OverlayManager::getSingleton().getOverlayElement("tracks/TruckInfoBox/MaxVelocity");
		descl->setCaption(UTFString(velostring));
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
		for(int i=1; i<=COMMANDS_VISIBLE; i++)
		{
			char commandOverlayID[256];
			sprintf(commandOverlayID, "tracks/TruckInfoBox/Command%d", i); // no wchar needed
			descl = OverlayManager::getSingleton().getOverlayElement(commandOverlayID);
			descl->setCaption(String(""));
		}

		int j = 0;
		for(int i=1; i<MAX_COMMANDS && j<COMMANDS_VISIBLE; i+=2)
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

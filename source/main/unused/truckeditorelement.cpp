/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

#include "truckeditorelement.h"
TruckEditorElement::TruckEditorElement(SceneManager *scm, float snap)
	{
		sceneman=scm;
		snapgrid=snap;
		guid=0;
		clear(false);
	};

	int TruckEditorElement::genGUID()
	{
		guid++;
		return guid;
	}

	void TruckEditorElement::clear(bool clearVisuals)
	{
		if (clearVisuals)
		{
			int i;
			for (i=0; i<free_beam; i++)
			{
				if (beams[i].hasVisual)
				{
					beams[i].snode->detachAllObjects();
					sceneman->destroySceneNode(beams[i].snode->getName());
					beams[i].dline->clear();
					delete beams[i].dline;
					beams[i].hasVisual=false;
				}
			}
			for (i=0; i<free_node; i++)
			{
				if (nodes[i].hasVisual)
				{
					nodes[i].snode->detachAllObjects();
					sceneman->destroySceneNode(nodes[i].snode->getName());
					sceneman->destroyBillboardSet(nodes[i].bboard);
					sceneman->destroyBillboardSet(nodes[i].selectedbboard);
					nodes[i].hasVisual=false;
				}
			}
		}
		description[0]=0;
		forwardcommands=false;
		importcommands=false;
		rollon=false;
		rescuer=false;
		free_node=0;
		free_beam=0;
		free_tri=0;
		free_wheel=0;
		free_selected_node=0;
		free_selected_beam=0;
		totalmass=10000;
		cargomass=0;
		strcpy(cabmat,"tracks/white");
		has_cam=false;
		has_engine=false;
		helpmat[0]=0;
		has_cinecam=false;
		has_globeam=false;
		free_prop=0;
		dirty=true;
		selectionmode=SELECTMODE_NODE;
	}

	void TruckEditorElement::deleteTri(int id)
	{
		memcpy(&tris[id], &tris[id+1], sizeof(editortri_t)*(free_tri-id-1));
		free_tri--;
	}

	void TruckEditorElement::deleteBeam(int id)
	{
		//take care of the visuals
		if (beams[id].hasVisual)
		{
			beams[id].snode->detachAllObjects();
			sceneman->destroySceneNode(beams[id].snode->getName());
			beams[id].dline->clear();
			delete beams[id].dline;
			beams[id].hasVisual=false;
		}
		memcpy(&beams[id], &beams[id+1], sizeof(editorbeam_t)*(free_beam-id-1));
		free_beam--;
		unselectBeam(id);
		updateVisuals();
	}

	void TruckEditorElement::deleteNode(int id)
	{
		//take care of the visuals
		if (nodes[id].hasVisual)
		{
			nodes[id].snode->detachAllObjects();
			sceneman->destroySceneNode(nodes[id].snode->getName());
			sceneman->destroyBillboardSet(nodes[id].bboard);
			sceneman->destroyBillboardSet(nodes[id].selectedbboard);
			nodes[id].hasVisual=false;
		}
		//okay, now the big shakeup
		int i;
		//nodes
		memcpy(&nodes[id], &nodes[id+1], sizeof(editornode_t)*(free_node-id-1));
		free_node--;
		//beams
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].id1==id || beams[i].id2==id)
			{
				//we must delete this beam
				deleteBeam(i);
				//this will restack the beams
				i--;
				continue;
			} 
			if (beams[i].id1>id) beams[i].id1--;
			if (beams[i].id2>id) beams[i].id2--;
		}
		//wheels
		for (i=0; i<free_wheel; i++)
		{
			if (wheels[i].id1>id) wheels[i].id1--;
			if (wheels[i].id2>id) wheels[i].id2--;
			if (wheels[i].idstab>id) wheels[i].idstab--;
			if (wheels[i].idtorque>id) wheels[i].idtorque--;
		}
		//props
		for (i=0; i<free_prop; i++)
		{
			if (props[i].refid>id) props[i].refid--;
			if (props[i].xid>id) props[i].xid--;
			if (props[i].yid>id) props[i].yid--;
		}
		//selected
		unselect(id);
		for (i=0; i<free_selected_node; i++) if (selected_nodes[i]>id) selected_nodes[i]--;
		//cam
		if (cam_posid>id) cam_posid--;
		if (cam_dirid>id) cam_dirid--;
		if (cam_refid>id) cam_refid--;
		//cinecam
		for (i=0; i<8; i++) if (cinecamIds[i]>id) cinecamIds[i]--;
		//tris
		for (i=0; i<free_tri; i++)
		{
			if (tris[i].id1==id || tris[i].id2==id|| tris[i].id3==id)
			{
				//we must delete this tri
				deleteTri(i);
				//this will restack the tris
				i--;
				continue;
			} 
			if (tris[i].id1>id) tris[i].id1--;
			if (tris[i].id2>id) tris[i].id2--;
			if (tris[i].id3>id) tris[i].id3--;
		}

		updateVisuals();
	}

	int TruckEditorElement::load(char* truckname)
	{
		if (!truckname) return -1;
		clear(true);
		//setHelpMessage(truckname);

		char fname[256];
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		sprintf(fname, "data\\trucks\\%s", truckname);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		sprintf(fname, "data/trucks/%s", truckname);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		sprintf(fname, "data/trucks/%s", truckname);
#endif
		FILE *fd;
		char line[1024];
		int mode=0;
		fd=fopen(fname, "r");
		if (!fd) {return -1;};

		//setup state for tris
		int submeshstart=0;
		Vector3 submeshcoords[500];
		int free_submeshcoord=0;

		//skip first line
		fscanf(fd," %[^\r\n]",line);
		//description
		strcpy(description, line);
		while (!feof(fd))
		{
			fscanf(fd," %[^\r\n]",line);
			if (line[0]==';')
			{
				continue;
			};
			if (!strcmp("end",line)) 
			{
				break;
			};
			if (!strcmp("nodes",line)) {mode=1;continue;};
			if (!strcmp("beams",line)) {mode=2;continue;};
			if (!strcmp("fixes",line)) {mode=3;continue;};
			if (!strcmp("shocks",line)) {mode=4;continue;};
			if (!strcmp("hydros",line)) {mode=5;continue;};
			if (!strcmp("wheels",line)) {mode=6;continue;};
			if (!strcmp("globals",line)) {mode=7;continue;};
			if (!strcmp("cameras",line)) {mode=8;continue;};
			if (!strcmp("engine",line)) {mode=9;continue;};
			if (!strcmp("texcoords",line)) {mode=10;continue;};
			if (!strcmp("cab",line)) {mode=11;continue;};
			if (!strcmp("commands",line)) {mode=12;continue;};
			if (!strcmp("forwardcommands",line)) {forwardcommands=true;continue;};
			if (!strcmp("importcommands",line)) {importcommands=true;continue;};
			if (!strcmp("rollon",line)) {rollon=true;continue;};
			if (!strcmp("rescuer",line)) {rescuer=true;continue;};
			if (!strcmp("contacters",line)) {mode=13;continue;};
			if (!strcmp("ropes",line)) {mode=14;continue;};
			if (!strcmp("ropables",line)) {mode=15;continue;};
			if (!strcmp("ties",line)) {mode=16;continue;};
			if (!strcmp("help",line)) {mode=17;continue;};
			if (!strcmp("cinecam",line)) {mode=18;continue;};
			if (!strcmp("flares",line)) {mode=19;continue;};
			if (!strcmp("props",line)) {mode=20;continue;};
			if (!strcmp("globeams",line)) {mode=21;continue;};
			if (!strcmp("wings",line)) {mode=22;continue;};
			if (!strcmp("backmesh",line)) 
			{
				//mark current mesh as backmesh!!!
				int i;
				for (i=submeshstart; i<free_tri; i++) tris[i].backfaced=true;
				continue;
			};
			if (!strcmp("submesh",line)) 
			{
				free_submeshcoord=0;
				submeshstart=free_tri;
				continue;
			};
			if (mode==1)
			{
				//parse nodes
				int id;
				float x,y,z;
				char mode='n';
				sscanf(line,"%i, %f, %f, %f, %c",&id,&x,&y,&z,&mode);
				nodes[free_node].Position=Vector3(x,y,z);
				nodes[free_node].mode=mode;
				nodes[free_node].fixed=false;
				nodes[free_node].contacter=false;
				nodes[free_node].ropable=false;
				nodes[free_node].hasTie=false;
				nodes[free_node].hasVisual=false;
				nodes[free_node].tieLong=1.0;
				nodes[free_node].tieMaxl=DEFAULT_TIE_LENGTH;
				nodes[free_node].tieRate=DEFAULT_TIE_RATE;
				nodes[free_node].tieShort=DEFAULT_TIE_STOP;
				nodes[free_node].tieMode='n';
				free_node++; 
			}
			if (mode==2)
			{
				//parse beams
				int id1, id2;
				char mode='v';
				sscanf(line,"%i, %i, %c",&id1,&id2,&mode);
				if (id1>=free_node || id2>=free_node) { return -2;};
				//dedupe
				int i;
				for (i=0; i<free_beam; i++)
				{
					if ((id1==beams[i].id1 && id2==beams[i].id2)
						||(id1==beams[i].id2 && id2==beams[i].id1)) continue;
				}
				beams[free_beam].id1=id1;
				beams[free_beam].id2=id2;
				beams[free_beam].mode=mode;
				beams[free_beam].isShock=false;
				beams[free_beam].isHydro=false;
				beams[free_beam].isCommand=false;
				beams[free_beam].isRope=false;
				beams[free_beam].hasVisual=false;
				free_beam++;
			}
			if (mode==4)
			{
				//parse shocks
				int id1, id2;
				float s, d, sbound,lbound,precomp;
				char mode='n';
				sscanf(line,"%i, %i, %f, %f, %f, %f, %f, %c",&id1,&id2, &s, &d, &sbound, &lbound,&precomp,&mode);
				if (id1>=free_node || id2>=free_node) {return -3;};
				beams[free_beam].id1=id1;
				beams[free_beam].id2=id2;
				beams[free_beam].mode=mode;
				beams[free_beam].isShock=true;
				beams[free_beam].isHydro=false;
				beams[free_beam].isCommand=false;
				beams[free_beam].isRope=false;
				beams[free_beam].hasVisual=false;
				beams[free_beam].s=s;
				beams[free_beam].d=d;
				beams[free_beam].sbound=sbound;
				beams[free_beam].lbound=lbound;
				beams[free_beam].precomp=precomp;
				free_beam++;
			}
			if (mode==3)
			{
				//parse fixes
				int id;
				sscanf(line,"%i",&id);
				if (id>=free_node) {return -4;};
				nodes[id].fixed=true;
			}
			if (mode==5)
			{
				//parse hydros
				int id1, id2;
				float ratio;
				char mode='n';
				sscanf(line,"%i, %i, %f, %c",&id1,&id2,&ratio, &mode);
				if (id1>=free_node || id2>=free_node) {return -5;};
				beams[free_beam].id1=id1;
				beams[free_beam].id2=id2;
				beams[free_beam].mode=mode;
				beams[free_beam].isShock=false;
				beams[free_beam].isHydro=true;
				beams[free_beam].isCommand=false;
				beams[free_beam].isRope=false;
				beams[free_beam].hasVisual=false;
				beams[free_beam].hydroRatio=ratio;
				free_beam++;
			}
			if (mode==6)
			{
				//parse wheels
				float radius, width, mass, spring, damp;
				int rays, node1, node2, snode, braked, propulsed, torquenode;
				char texf[256];
				char texb[256];
				sscanf(line,"%f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %s %s",&radius,&width,&rays,&node1,&node2,&snode,&braked,&propulsed,&torquenode,&mass,&spring,&damp, texf, texb);
				wheels[free_wheel].radius=radius;
				wheels[free_wheel].width=width;
				wheels[free_wheel].numrays=rays;
				wheels[free_wheel].id1=node1;
				wheels[free_wheel].id2=node2;
				if (snode==9999) 
				{
					wheels[free_wheel].idstab=-1;
					wheels[free_wheel].stabmode=STAB_NONE;
				}
				else if (snode<0)
				{
					wheels[free_wheel].idstab=-snode;
					wheels[free_wheel].stabmode=STAB_REVERSE;
				} else
				{
					wheels[free_wheel].idstab=snode;
					wheels[free_wheel].stabmode=STAB_DIRECT;
				}
				wheels[free_wheel].braked=braked!=0;
				wheels[free_wheel].propulsed=propulsed!=0;
				wheels[free_wheel].idtorque=torquenode;
				wheels[free_wheel].mass=mass;
				wheels[free_wheel].spring=spring;
				wheels[free_wheel].damp=damp;
				strcpy(wheels[free_wheel].facemat, texf);
				strcpy(wheels[free_wheel].bandmat, texb);
				free_wheel++;
			}
			if (mode==7)
			{
				//parse globals
				sscanf(line,"%f, %f, %s",&totalmass, &cargomass, cabmat);
			}
			if (mode==8)
			{
				//parse cameras
				has_cam=true;
				sscanf(line,"%i, %i, %i",&cam_posid,&cam_dirid,&cam_refid);
			}
			if (mode==9)
			{
				//parse engine
				has_engine=true;
				sscanf(line,"%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f", &minrpm, &maxrpm, &torque, &dratio, &rear, &gears[0],&gears[1],&gears[2],&gears[3],&gears[4],&gears[5],&gears[6],&gears[7],&gears[8],&gears[9],&gears[10],&gears[11],&gears[12],&gears[13],&gears[14],&gears[15]);
			}

			if (mode==10)
			{
				//parse texcoords
				int id;
				float x, y;
				sscanf(line,"%i, %f, %f", &id, &x, &y);
				submeshcoords[free_submeshcoord]=Vector3(id, x, y);
				free_submeshcoord++;
			}

			if (mode==11)
			{
				//parse cab
				char type='n';
				int id1, id2, id3;
				sscanf(line,"%i, %i, %i, %c", &id1, &id2, &id3,&type);
				tris[free_tri].id1=id1;
				tris[free_tri].id2=id2;
				tris[free_tri].id3=id3;
				tris[free_tri].type=type;
				tris[free_tri].backfaced=false;
				int i;
				for (i=0; i<free_submeshcoord; i++) if (submeshcoords[i].x==id1) break;
				tris[free_tri].tcoord1=Vector2(submeshcoords[i].y,submeshcoords[i].z);
				for (i=0; i<free_submeshcoord; i++) if (submeshcoords[i].x==id2) break;
				tris[free_tri].tcoord2=Vector2(submeshcoords[i].y,submeshcoords[i].z);
				for (i=0; i<free_submeshcoord; i++) if (submeshcoords[i].x==id3) break;
				tris[free_tri].tcoord3=Vector2(submeshcoords[i].y,submeshcoords[i].z);
				free_tri++;
			}

			if (mode==12)
			{
				//parse commands
				int id1, id2,keys,keyl;
				float rate, shortl, longl;
				char mode='n';
				sscanf(line,"%i, %i, %f, %f, %f, %i, %i, %c", &id1, &id2, &rate, &shortl, &longl, &keys, &keyl,&mode);
				if (id1>=free_node || id2>=free_node) {return -6;};
				beams[free_beam].id1=id1;
				beams[free_beam].id2=id2;
				beams[free_beam].mode=mode;
				beams[free_beam].isShock=false;
				beams[free_beam].isHydro=false;
				beams[free_beam].isCommand=true;
				beams[free_beam].isRope=false;
				beams[free_beam].hasVisual=false;
				beams[free_beam].commandRate=rate;
				beams[free_beam].commandShort=shortl;
				beams[free_beam].commandLong=longl;
				beams[free_beam].commandKeyS=keys;
				beams[free_beam].commandKeyL=keyl;
				free_beam++;
			}

			if (mode==13)
			{
				//parse contacters
				int id1;
				sscanf(line,"%i", &id1);
				nodes[id1].contacter=true;
			}
			if (mode==14)
			{
				//parse ropes
				int id1, id2;
				sscanf(line,"%i, %i", &id1, &id2);
				beams[free_beam].id1=id1;
				beams[free_beam].id2=id2;
				beams[free_beam].mode='v';
				beams[free_beam].isShock=false;
				beams[free_beam].isHydro=false;
				beams[free_beam].isCommand=false;
				beams[free_beam].isRope=true;
				beams[free_beam].hasVisual=false;
				free_beam++;
			}
			if (mode==15)
			{
				//parse ropables
				int id1;
				sscanf(line,"%i", &id1);
				nodes[id1].ropable=true;
			}
			if (mode==16)
			{
				//parse ties
				int id1;
				float maxl, rate, shortl, longl;
				char mode='n';
				sscanf(line,"%i, %f, %f, %f, %f, %c", &id1, &maxl, &rate, &shortl, &longl, &mode);
				nodes[id1].hasTie=true;
				nodes[id1].tieMaxl=maxl;
				nodes[id1].tieRate=rate;
				nodes[id1].tieShort=shortl;
				nodes[id1].tieLong=longl;
				nodes[id1].tieMode=mode;
			}

			if (mode==17)
			{
				//help material
				strcpy(helpmat,line);
			}
			if (mode==18)
			{
				//cinecam
				float x,y,z;
				int n1, n2, n3, n4, n5, n6, n7, n8;
				has_cinecam=true;
				sscanf(line,"%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %i", &x,&y,&z,&n1,&n2,&n3,&n4,&n5,&n6,&n7,&n8);
				cinecamPos=Vector3(x,y,z);
				cinecamIds[0]=n1;
				cinecamIds[1]=n2;
				cinecamIds[2]=n3;
				cinecamIds[3]=n4;
				cinecamIds[4]=n5;
				cinecamIds[5]=n6;
				cinecamIds[6]=n7;
				cinecamIds[7]=n8;
			}
	    
			if (mode==19)
			{
				//parse flares
				int ref, nx, ny;
				float ox, oy;
				sscanf(line,"%i, %i, %i, %f, %f", &ref,&nx,&ny, &ox, &oy);
				props[free_prop].refid=ref;
				props[free_prop].xid=nx;
				props[free_prop].yid=ny;
				props[free_prop].offsetx=ox;
				props[free_prop].offsety=oy;
				props[free_prop].isProp=false;
				free_prop++;
			}
			if (mode==20)
			{
				//parse props
				int ref, nx, ny;
				float ox, oy, oz;
				float rx, ry, rz;
				char meshname[256];
				sscanf(line,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s", &ref,&nx,&ny, &ox, &oy, &oz, &rx, &ry, &rz, meshname);
				props[free_prop].refid=ref;
				props[free_prop].xid=nx;
				props[free_prop].yid=ny;
				props[free_prop].offsetx=ox;
				props[free_prop].offsety=oy;
				props[free_prop].isProp=true;
				props[free_prop].offsetz=oz;
				props[free_prop].rotx=rx;
				props[free_prop].roty=ry;
				props[free_prop].rotz=rz;
				strcpy(props[free_prop].meshname, meshname);
				free_prop++;
			}
			if (mode==21)
			{
				//parse globeams
				has_globeam=true;
				sscanf(line,"%f, %f, %f, %s", &globeam_deform,&globeam_break,&globeam_beam_diameter, globam_beam_material);
			}
			if (mode==22)
			{
				//parse wings
				int nds[8];
				float txes[8];
				sscanf(line,"%i, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %f, %f, %f, %f, %f", 
					&nds[0],
					&nds[1],
					&nds[2],
					&nds[3],
					&nds[4],
					&nds[5],
					&nds[6],
					&nds[7],
					&txes[0],
					&txes[1],
					&txes[2],
					&txes[3],
					&txes[4],
					&txes[5],
					&txes[6],
					&txes[7]
					);
			};
		};
		fclose(fd);
		//dedupe();
		updateVisuals();
		dirty=true;
		return 0;
	}

	int TruckEditorElement::save(char* truckname)
	{
		//temporary
		strcpy(description,"Edited truck");
		int i;
		if (!truckname) return -1;
		char fname[256];
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		sprintf(fname, "data\\trucks\\%s", truckname);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_LINUX
		sprintf(fname, "data/trucks/%s", truckname);
#endif
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
		sprintf(fname, "data/trucks/%s", truckname);
#endif
		FILE *fd;
		fd=fopen(fname, "w");
		//description
		fprintf(fd, "%s\n;VERSION 1\n;Generated by the Rigs of Rods Truck Editor\n\n", description);
		//globals
		fprintf(fd,"globals\n%f, %f, %s\n\n",totalmass, cargomass, cabmat);
		if (has_globeam) fprintf(fd,"globeams\n%f, %f, %f, %s\n\n", globeam_deform,globeam_break,globeam_beam_diameter, globam_beam_material);
		//help
		if (strlen(helpmat)) fprintf(fd, "help\n%s\n\n", helpmat);
		//various flags
		if (forwardcommands) fprintf(fd, "forwardcommands\n\n");
		if (importcommands) fprintf(fd, "importcommands\n\n");
		if (rollon) fprintf(fd, "rollon\n\n");
		if (rescuer) fprintf(fd, "rescuer\n\n");
		//engine
		if (has_engine) fprintf(fd, "engine\n%f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f\n\n", minrpm, maxrpm, torque, dratio, rear, gears[0],gears[1],gears[2],gears[3],gears[4],gears[5],gears[6],gears[7],gears[8],gears[9],gears[10],gears[11],gears[12],gears[13],gears[14],gears[15]);
		//nodes
		fprintf(fd,"nodes\n");
		for (i=0; i<free_node; i++)
		{
			fprintf(fd,"%i, %f, %f, %f, %c\n",i,nodes[i].Position.x,nodes[i].Position.y,nodes[i].Position.z,nodes[i].mode);
		}
		fprintf(fd, "\n");
		//beams
		fprintf(fd,"beams\n");
		for (i=0; i<free_beam; i++)
		{
			if (!beams[i].isCommand && !beams[i].isHydro && !beams[i].isShock && !beams[i].isRope)
				fprintf(fd,"%i, %i, %c\n",beams[i].id1,beams[i].id2,beams[i].mode);
		}
		fprintf(fd, "\n");
		//cameras
		if (has_cam) fprintf(fd, "cameras\n%i, %i, %i\n\n",cam_posid,cam_dirid,cam_refid);
		//cinecam
		if (has_cinecam) fprintf(fd, "cinecam\n%f, %f, %f, %i, %i, %i, %i, %i, %i, %i, %i\n\n", cinecamPos.x, cinecamPos.y, cinecamPos.z, cinecamIds[0], cinecamIds[1], cinecamIds[2], cinecamIds[3], cinecamIds[4], cinecamIds[5], cinecamIds[6], cinecamIds[7]);
		//wheels
		fprintf(fd,"wheels\n");
		for (i=0; i<free_wheel; i++)
		{
			int sn=wheels[i].idstab;
			if (wheels[i].stabmode==STAB_NONE) sn=9999;
			if (wheels[i].stabmode==STAB_REVERSE) sn=-sn;
			fprintf(fd,"%f, %f, %i, %i, %i, %i, %i, %i, %i, %f, %f, %f, %s %s\n",wheels[i].radius,wheels[i].width,wheels[i].numrays,wheels[i].id1,wheels[i].id2,sn,wheels[i].braked,wheels[i].propulsed,wheels[i].idtorque,wheels[i].mass,wheels[i].spring,wheels[i].damp, wheels[i].facemat, wheels[i].bandmat);
		}
		fprintf(fd, "\n");
		//shocks
		fprintf(fd,"shocks\n");
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].isShock)
				fprintf(fd,"%i, %i, %f, %f, %f, %f, %f, %c\n",beams[i].id1,beams[i].id2, beams[i].s, beams[i].d, beams[i].sbound, beams[i].lbound,beams[i].precomp,beams[i].mode);
		}
		fprintf(fd, "\n");
		//hydro
		fprintf(fd,"hydros\n");
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].isHydro)
				fprintf(fd,"%i, %i, %f, %c\n",beams[i].id1,beams[i].id2,beams[i].hydroRatio, beams[i].mode);
		}
		fprintf(fd, "\n");
		//commands
		fprintf(fd,"commands\n");
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].isCommand)
				fprintf(fd,"%i, %i, %f, %f, %f, %i, %i, %c\n", beams[i].id1, beams[i].id2, beams[i].commandRate, beams[i].commandShort, beams[i].commandLong, beams[i].commandKeyS, beams[i].commandKeyL,beams[i].mode);
		}
		fprintf(fd, "\n");
		//ropes
		fprintf(fd,"ropes\n");
		for (i=0; i<free_beam; i++)
		{
			if (beams[i].isRope)
				fprintf(fd,"%i, %i\n", beams[i].id1, beams[i].id2);
		}
		fprintf(fd, "\n");
		//ties
		fprintf(fd,"ties\n");
		for (i=0; i<free_node; i++)
		{
			if (nodes[i].hasTie)
				fprintf(fd,"%i, %f, %f, %f, %f, %c\n", i, nodes[i].tieMaxl, nodes[i].tieRate, nodes[i].tieShort, nodes[i].tieLong, nodes[i].tieMode);
		}
		fprintf(fd, "\n");
		//fixes
		fprintf(fd,"fixes\n");
		for (i=0; i<free_node; i++)
		{
			if (nodes[i].fixed)
				fprintf(fd,"%i\n", i);
		}
		fprintf(fd, "\n");
		//contacters
		fprintf(fd,"contacters\n");
		for (i=0; i<free_node; i++)
		{
			if (nodes[i].contacter)
				fprintf(fd,"%i\n", i);
		}
		fprintf(fd, "\n");
		//ropables
		fprintf(fd,"ropables\n");
		for (i=0; i<free_node; i++)
		{
			if (nodes[i].ropable)
				fprintf(fd,"%i\n", i);
		}
		fprintf(fd, "\n");
		//flares
		fprintf(fd,"flares\n");
		for (i=0; i<free_prop; i++)
		{
			if (!props[i].isProp)
				fprintf(fd,"%i, %i, %i, %f, %f\n", props[i].refid,props[i].xid,props[i].yid, props[i].offsetx, props[i].offsety);
		}
		fprintf(fd, "\n");
		//props
		fprintf(fd,"props\n");
		for (i=0; i<free_prop; i++)
		{
			if (props[i].isProp)
				fprintf(fd,"%i, %i, %i, %f, %f, %f, %f, %f, %f, %s\n", props[i].refid,props[i].xid,props[i].yid, props[i].offsetx, props[i].offsety, props[i].offsetz, props[i].rotx, props[i].roty, props[i].rotz, props[i].meshname);
		}
		fprintf(fd, "\n");
		//submeshes
		//init
		for (i=0; i<free_tri; i++) tris[i].exported=false;
		while(true)
		{
			//search a non-exported tri
			for (i=0; i<free_tri; i++) if (!tris[i].exported) break;
			//end
			if (i==free_tri) break;
			//okay, start a submesh from tri i
			tris[i].exported=true;
			//find all other tris
			Plane triplane=Plane(nodes[tris[i].id1].Position, nodes[tris[i].id2].Position, nodes[tris[i].id3].Position);
			int submeshtris[500];
			int free_submeshtri=0;
			while (true)
			{
				int j;
				//search a valid tri
				for (j=0; j<free_tri; j++)
				{
					Plane tplane=Plane(nodes[tris[j].id1].Position, nodes[tris[j].id2].Position, nodes[tris[j].id3].Position);
					if (!tris[j].exported
						&& tris[j].backfaced==tris[i].backfaced
						&& triplane==tplane)
					{
						//okay, we found a coplanar tri, check if they share their texcoords
						bool valid=true;
						valid=valid && !(tris[i].id1==tris[j].id1 && !(tris[i].tcoord1==tris[j].tcoord1));
						valid=valid && !(tris[i].id1==tris[j].id2 && !(tris[i].tcoord1==tris[j].tcoord2));
						valid=valid && !(tris[i].id1==tris[j].id3 && !(tris[i].tcoord1==tris[j].tcoord3));
						valid=valid && !(tris[i].id2==tris[j].id1 && !(tris[i].tcoord2==tris[j].tcoord1));
						valid=valid && !(tris[i].id2==tris[j].id2 && !(tris[i].tcoord2==tris[j].tcoord2));
						valid=valid && !(tris[i].id2==tris[j].id3 && !(tris[i].tcoord2==tris[j].tcoord3));
						valid=valid && !(tris[i].id3==tris[j].id1 && !(tris[i].tcoord3==tris[j].tcoord1));
						valid=valid && !(tris[i].id3==tris[j].id2 && !(tris[i].tcoord3==tris[j].tcoord2));
						valid=valid && !(tris[i].id3==tris[j].id3 && !(tris[i].tcoord3==tris[j].tcoord3));
						if (valid)
						{
							//okay, all checks passed
							submeshtris[free_submeshtri]=j;
							free_submeshtri++;
							tris[j].exported=true;
							break;
						}
					}
				}
				//end
				if (j==free_tri) break;
			}
			//okay, we have the tri list
			//compile the node list
			int nodelist[500];
			Vector2 nodecoord[500];
			int free_nodelist=0;
			int j;
			//add the first tri (i)
			nodelist[0]=tris[i].id1;
			nodecoord[0]=tris[i].tcoord1;
			nodelist[1]=tris[i].id2;
			nodecoord[1]=tris[i].tcoord2;
			nodelist[2]=tris[i].id3;
			nodecoord[2]=tris[i].tcoord3;
			free_nodelist=3;
			for (j=0; j<free_submeshtri; j++)
			{
				int c;
				int k;
				c=tris[submeshtris[j]].id1;
				for (k=0; k<free_nodelist; k++) if (nodelist[k]==c) break;
				if (k==free_nodelist)
				{
					//add node
					nodelist[free_nodelist]=c;
					nodecoord[free_nodelist]=tris[submeshtris[j]].tcoord1;
					free_nodelist++;
				}
				c=tris[submeshtris[j]].id2;
				for (k=0; k<free_nodelist; k++) if (nodelist[k]==c) break;
				if (k==free_nodelist)
				{
					//add node
					nodelist[free_nodelist]=c;
					nodecoord[free_nodelist]=tris[submeshtris[j]].tcoord2;
					free_nodelist++;
				}
				c=tris[submeshtris[j]].id3;
				for (k=0; k<free_nodelist; k++) if (nodelist[k]==c) break;
				if (k==free_nodelist)
				{
					//add node
					nodelist[free_nodelist]=c;
					nodecoord[free_nodelist]=tris[submeshtris[j]].tcoord3;
					free_nodelist++;
				}
			}
			//okay, we are set
			fprintf(fd,"submesh\ntexcoords\n");
			for (j=0; j<free_nodelist; j++) fprintf(fd,"%i, %f, %f\n", nodelist[j], nodecoord[j].x, nodecoord[j].y);
			fprintf(fd,"cab\n");
			//first, the i
			fprintf(fd,"%i, %i, %i, %c\n", tris[i].id1, tris[i].id2, tris[i].id3, tris[i].type);
			for (j=0; j<free_submeshtri; j++) fprintf(fd,"%i, %i, %i, %c\n", tris[submeshtris[j]].id1, tris[submeshtris[j]].id2, tris[submeshtris[j]].id3, tris[submeshtris[j]].type);
			if (tris[i].backfaced) fprintf(fd,"backmesh\n\n"); else fprintf(fd, "\n");
		}
		//ouch!

		fprintf(fd, "end\n");
		fclose(fd);
		return 0;
	}

	void TruckEditorElement::updateVisuals()
	{
		int i;
		for (i=0; i<free_beam;i++)
		{
			char material[256];
			strcpy(material, "editor/beamvisible");
			if (beams[i].mode=='i') strcpy(material, "editor/beaminvisible");
			if (beams[i].mode=='r' || beams[i].isRope) strcpy(material, "editor/beamrope");
			if (beams[i].isShock) strcpy(material, "editor/beamshock");
			if (beams[i].isHydro) strcpy(material, "editor/beamhydro");
			if (beams[i].isCommand) strcpy(material, "editor/beamcommand");
			if (isSelectedBeam(i) && selectionmode==SELECTMODE_BEAM) strcpy(material, "editor/beamselected");
			if (!beams[i].hasVisual)
			{
				//create visual
				beams[i].dline=new DynamicLines(material, RenderOperation::OT_LINE_LIST);
				beams[i].dline->addPoint(nodes[beams[i].id1].Position);
				beams[i].dline->addPoint(nodes[beams[i].id2].Position);
				beams[i].dline->update();
				beams[i].snode=sceneman->getRootSceneNode()->createChildSceneNode();
				beams[i].snode->attachObject(beams[i].dline);
				beams[i].snode->setPosition(0,0,0);
				beams[i].hasVisual=true;
			}
			else
			{
				beams[i].dline->setPoint(0,nodes[beams[i].id1].Position);
				beams[i].dline->setPoint(1,nodes[beams[i].id2].Position);
				beams[i].dline->update();
				beams[i].dline->setMaterial(material);
			}
		}
		for (i=0; i<free_node; i++)
		{
			char uids[64];
			char material[256];
			strcpy(material, "editor/nodenormal");
			ColourValue colour=ColourValue::White;
			if (nodes[i].mode=='l') colour=ColourValue(0.5, 1.0, 1.0);
			if (nodes[i].mode=='f') colour=ColourValue(1.0, 0.5, 0.2);
			if (nodes[i].mode=='x') colour=ColourValue(0.1, 0.1, 0.1);
			if (nodes[i].mode=='y') colour=ColourValue(0.3, 0.3, 0.3);
			if (nodes[i].mode=='c') colour=ColourValue(0.7, 0.7, 0.7);
			if (nodes[i].mode=='h') colour=ColourValue(1.0, 1.0, 0.0);
			if (nodes[i].mode=='e') colour=ColourValue(0.0, 1.0, 0.0);
			if (nodes[i].mode=='b') colour=ColourValue(0.0, 0.3, 1.0);
			if (nodes[i].contacter) colour=ColourValue(1.0, 0.0, 0.0);
			if (!nodes[i].hasVisual)
			{
				nodes[i].guid=genGUID();
				sprintf(uids,"EDT%.8x",nodes[i].guid);
				nodes[i].bboard=sceneman->createBillboardSet(uids, 1);
				nodes[i].bboard->createBillboard(nodes[i].Position, colour);
				nodes[i].bboard->setDefaultDimensions(0.1, 0.1);
				nodes[i].bboard->setMaterialName(material);
				sprintf(uids,"EDT%.8x",genGUID());
				nodes[i].selectedbboard=sceneman->createBillboardSet(uids, 1);
				nodes[i].selectedbboard->createBillboard(nodes[i].Position);
				nodes[i].selectedbboard->setDefaultDimensions(0.15, 0.15);
				nodes[i].selectedbboard->setMaterialName("editor/nodeselected");
				nodes[i].selectedbboard->setVisible(isSelected(i)&&selectionmode==SELECTMODE_NODE);
				nodes[i].snode=sceneman->getRootSceneNode()->createChildSceneNode();
				nodes[i].snode->attachObject(nodes[i].bboard);
				nodes[i].snode->attachObject(nodes[i].selectedbboard);
				nodes[i].snode->setPosition(0,0,0);
				nodes[i].hasVisual=true;
			}
			else
			{
				nodes[i].bboard->getBillboard(0)->setPosition(nodes[i].Position);
				nodes[i].bboard->getBillboard(0)->setColour(colour);
				nodes[i].bboard->setMaterialName(material);
				nodes[i].selectedbboard->setVisible(isSelected(i)&&selectionmode==SELECTMODE_NODE);
				nodes[i].selectedbboard->getBillboard(0)->setPosition(nodes[i].Position);
			}
		}
		dirty=true;
	}

	bool TruckEditorElement::isSelected(int id)
	{
		int i;
		for (i=0; i<free_selected_node; i++) if (selected_nodes[i]==id) return true;
		return false;
	}

	bool TruckEditorElement::isSelectedBeam(int id)
	{
		int i;
		for (i=0; i<free_selected_beam; i++) if (selected_beams[i]==id) return true;
		return false;
	}

	void TruckEditorElement::node2beamSelection()
	{
		int i;
		free_selected_beam=0;
		for (i=0; i<free_beam; i++)
		{
			if (isSelected(beams[i].id1)&&isSelected(beams[i].id2))
			{
				selected_beams[free_selected_beam]=i;
				free_selected_beam++;
			}
		}
	}

	void TruckEditorElement::beam2nodeSelection()
	{
		int i;
		free_selected_node=0;
		for (i=0; i<free_beam; i++)
		{
			if (isSelectedBeam(i))
			{
				if (!isSelected(beams[i].id1))
				{
					selected_nodes[free_selected_node]=beams[i].id1;
					free_selected_node++;
				}
				if (!isSelected(beams[i].id2))
				{
					selected_nodes[free_selected_node]=beams[i].id2;
					free_selected_node++;
				}
			}
		}
	}

	void TruckEditorElement::unselect(int id)
	{
		int i;
		for (i=0; i<free_selected_node; i++) 
			if (selected_nodes[i]==id)
			{
				int j;
				for (j=i+1; j<free_selected_node; j++) selected_nodes[j-1]=selected_nodes[j];
				free_selected_node--;
				return;
			}
	}

	void TruckEditorElement::unselectBeam(int id)
	{
		int i;
		for (i=0; i<free_selected_beam; i++) 
			if (selected_beams[i]==id)
			{
				int j;
				for (j=i+1; j<free_selected_beam; j++) selected_beams[j-1]=selected_beams[j];
				free_selected_beam--;
				return;
			}
	}

	void TruckEditorElement::switchSelectionMode(int mode)
	{
		if (mode!=selectionmode)
		{
			selectionmode=mode;
			if (mode==SELECTMODE_BEAM)
			{
				node2beamSelection();
			}
			if (mode==SELECTMODE_NODE)
			{
				beam2nodeSelection();
			}
			updateVisuals();
		}
	}

	//start and end must be ordered and returns how many nodes affected by this operation
	int TruckEditorElement::selectNodes(Vector3 start, Vector3 stop, int selectmode)
	{
		int ret=0;
		int i;
		if (selectmode==SELECT_NEW) free_selected_node=0;
		for (i=0; i<free_node; i++)
		{
			if (nodes[i].Position.x>start.x && nodes[i].Position.x<stop.x &&
			    nodes[i].Position.y>start.y && nodes[i].Position.y<stop.y &&
			    nodes[i].Position.z>start.z && nodes[i].Position.z<stop.z)
			{
				if (selectmode==SELECT_NEW || (selectmode==SELECT_ADD && !isSelected(i)))
				{
					selected_nodes[free_selected_node]=i;
					free_selected_node++;
				}
				if (selectmode==SELECT_DEL && isSelected(i)) unselect(i);
				ret++;
			}
		}
		updateVisuals();
		return ret;
	}

	//start and end must be ordered and returns how many beams affected by this operation
	int TruckEditorElement::selectBeams(Vector3 start, Vector3 stop, int selectmode)
	{
		int ret=0;
		int i;
		if (selectmode==SELECT_NEW) free_selected_beam=0;
		for (i=0; i<free_beam; i++)
		{
			//okay, this is a ray tracing algorithm, strap your belt!
			Vector3 org=nodes[beams[i].id1].Position;
			Vector3 dir=nodes[beams[i].id2].Position-nodes[beams[i].id1].Position;
			float tnear=-5000;
			float tfar=5000;
			//first, the X axis
			if (dir.x==0)
			{
				//parallel case
				if (!(start.x<org.x && org.x<stop.x)) continue;//no way
			}
			else
			{
				//compute intersection with the planes
				float t1=(start.x-org.x)/dir.x;
				float t2=(stop.x-org.x)/dir.x;
				float s;
				//order
				if (t1>t2) {s=t1; t1=t2; t2=s;};
				if (t1>tnear) tnear=t1;
				if (t2<tfar) tfar=t2;
				if (tnear>tfar) continue; //we missed
				if (tfar<0) continue; //its'behind us
			}
			//the Y axis
			if (dir.y==0)
			{
				//parallel case
				if (!(start.y<org.y && org.y<stop.y)) continue;//no way
			}
			else
			{
				//compute intersection with the planes
				float t1=(start.y-org.y)/dir.y;
				float t2=(stop.y-org.y)/dir.y;
				float s;
				//order
				if (t1>t2) {s=t1; t1=t2; t2=s;};
				if (t1>tnear) tnear=t1;
				if (t2<tfar) tfar=t2;
				if (tnear>tfar) continue; //we missed
				if (tfar<0) continue; //its'behind us
			}
			//the Z axis
			if (dir.z==0)
			{
				//parallel case
				if (!(start.z<org.z && org.z<stop.z)) continue;//no way
			}
			else
			{
				//compute intersection with the planes
				float t1=(start.z-org.z)/dir.z;
				float t2=(stop.z-org.z)/dir.z;
				float s;
				//order
				if (t1>t2) {s=t1; t1=t2; t2=s;};
				if (t1>tnear) tnear=t1;
				if (t2<tfar) tfar=t2;
				if (tnear>tfar) continue; //we missed
				if (tfar<0) continue; //its'behind us
			}
			if (tnear>tfar) continue; //we missed
			if (tfar<0) continue; //its'behind us
			//okay, now that we know it intersects, check the lengths
			if (tnear>dir.length()) continue; //too far!
			//yo, now we have a valid candidate
			if (selectmode==SELECT_NEW || (selectmode==SELECT_ADD && !isSelectedBeam(i)))
			{
				selected_beams[free_selected_beam]=i;
				free_selected_beam++;
			}
			if (selectmode==SELECT_DEL && isSelectedBeam(i)) unselectBeam(i);
			ret++;
		}
		updateVisuals();
		return ret;
	}

	int TruckEditorElement::createNode(Vector3 position)
	{
		//first, check if is not already existing
		int i;
		for (i=0; i<free_node; i++) if ((nodes[i].Position-position).length()<snapgrid/2.0) return i;

		nodes[free_node].Position=position;
		nodes[free_node].mode='n';
		nodes[free_node].fixed=false;
		nodes[free_node].contacter=false;
		nodes[free_node].ropable=false;
		nodes[free_node].hasTie=false;
		nodes[free_node].tieLong=1.0;
		nodes[free_node].tieMaxl=DEFAULT_TIE_LENGTH;
		nodes[free_node].tieRate=DEFAULT_TIE_RATE;
		nodes[free_node].tieShort=DEFAULT_TIE_STOP;
		nodes[free_node].tieMode='n';
		nodes[free_node].hasVisual=false;
		free_node++; 
		return free_node-1;
	}

	int TruckEditorElement::create_beam(int id1, int id2)
	{
		//first check if is not already existing
		int i;
		for (i=0; i<free_beam;i++) if ((beams[i].id1==id1 && beams[i].id2==id2) || (beams[i].id1==id2 && beams[i].id2==id1)) return i;

		beams[free_beam].id1=id1;
		beams[free_beam].id2=id2;
		beams[free_beam].mode='v';
		beams[free_beam].isShock=false;
		beams[free_beam].isHydro=false;
		beams[free_beam].isCommand=false;
		beams[free_beam].isRope=false;
		beams[free_beam].hasVisual=false;
		free_beam++;
		return free_beam-1;
	}

	void TruckEditorElement::deleteSelectedNodes()
	{
		while (free_selected_node>0)
		{
			deleteNode(selected_nodes[0]);
		}
	}

	void TruckEditorElement::deleteSelectedBeams()
	{
		while (free_selected_beam>0)
		{
			deleteBeam(selected_beams[0]);
		}
	}

	//classic click-happend
	void TruckEditorElement::appendNode(Vector3 position, int context)
	{
		//single case, 3D cursor oriented
		if (free_selected_node<=1)
		{
			int nodeid=createNode(position);
			//add a beam
			if (free_selected_node==1)
			{
				create_beam(selected_nodes[0],nodeid);
			}
			//select only the new one
			free_selected_node=1;
			selected_nodes[0]=nodeid;
		}
		else
		//multiple case, trusses building
		{
			//selected nodes must be sorted along the context axis
			int i, j;
			//bubble sort baby!
			for (i=free_selected_node-1; i>=0; i--)
			{
				for (j=0; j<i; j++)
				{
					if ((context==0 && nodes[selected_nodes[j]].Position.x > nodes[selected_nodes[j+1]].Position.x)
						||(context==1 && nodes[selected_nodes[j]].Position.y > nodes[selected_nodes[j+1]].Position.y)
						||(context==2 && nodes[selected_nodes[j]].Position.z > nodes[selected_nodes[j+1]].Position.z))
					{
						//swap
						int t;
						t=selected_nodes[j];
						selected_nodes[j]=selected_nodes[j+1];
						selected_nodes[j+1]=t;
					}
				}
			}
			int newset[100];
			for (i=0; i<free_selected_node; i++)
			{
				Vector3 pos=position;
				if (context==0) pos.x=nodes[selected_nodes[i]].Position.x;
				if (context==1) pos.y=nodes[selected_nodes[i]].Position.y;
				if (context==2) pos.z=nodes[selected_nodes[i]].Position.z;
				newset[i]=createNode(pos);
				//trussing
				//axial
				if (i>0) 
				{
					//there is dupes, but they are removed
					create_beam(selected_nodes[i-1], selected_nodes[i]);
					create_beam(newset[i-1], newset[i]);
					//reinforcements
					create_beam(selected_nodes[i-1], newset[i]);
					create_beam(newset[i-1], selected_nodes[i]);
				}
				//longitudinal
				create_beam(selected_nodes[i], newset[i]);
			}
			//update selection
			for (i=0; i<free_selected_node; i++) selected_nodes[i]=newset[i];
		}

		updateVisuals();
	}

	bool TruckEditorElement::isDraggable(Vector3 pos, int c, float tolerance)
	{
		int i;
		for (i=0; i<free_selected_node; i++)
		{
			if (c==0 && fabs(pos.y-nodes[selected_nodes[i]].Position.y)<tolerance && fabs(pos.z-nodes[selected_nodes[i]].Position.z)<tolerance) return true;
			if (c==1 && fabs(pos.x-nodes[selected_nodes[i]].Position.x)<tolerance && fabs(pos.z-nodes[selected_nodes[i]].Position.z)<tolerance) return true;
			if (c==2 && fabs(pos.x-nodes[selected_nodes[i]].Position.x)<tolerance && fabs(pos.y-nodes[selected_nodes[i]].Position.y)<tolerance) return true;
		}
		return false;
	}
	void TruckEditorElement::moveSelected(Vector3 disp)
	{
		int i;
		for (i=0; i<free_selected_node; i++)
		{
			nodes[selected_nodes[i]].Position+=disp;
		}
		updateVisuals();
	}

	void TruckEditorElement::setSelectedNodesMode(char mode)
	{
		int i;
		for (i=0; i<free_selected_node; i++) nodes[selected_nodes[i]].mode=mode;
		updateVisuals();
	}

	void TruckEditorElement::setSelectedNodesFixed(int mode)
	{
		if (mode==SEL_MIXED) return;
		int i;
		for (i=0; i<free_selected_node; i++) nodes[selected_nodes[i]].fixed=(mode!=0);
		updateVisuals();
	}

	int  TruckEditorElement::getSelectedNodesFixed()
	{
		if (free_selected_node==0) return SEL_FALSE;
		int resp=nodes[selected_nodes[0]].fixed;
		int i;
		for (i=1; i<free_selected_node; i++) if ((int)nodes[selected_nodes[i]].fixed!=resp) return SEL_MIXED;
		return resp;
	}

	void TruckEditorElement::setSelectedNodesContacter(int mode)
	{
		if (mode==SEL_MIXED) return;
		int i;
		for (i=0; i<free_selected_node; i++) nodes[selected_nodes[i]].contacter=(mode!=0);
		updateVisuals();
	}

	int  TruckEditorElement::getSelectedNodesContacter()
	{
		if (free_selected_node==0) return SEL_FALSE;
		int resp=nodes[selected_nodes[0]].contacter;
		int i;
		for (i=1; i<free_selected_node; i++) if ((int)nodes[selected_nodes[i]].contacter!=resp) return SEL_MIXED;
		return resp;
	}

	void TruckEditorElement::setSelectedNodesRopable(int mode)
	{
		if (mode==SEL_MIXED) return;
		int i;
		for (i=0; i<free_selected_node; i++) nodes[selected_nodes[i]].ropable=(mode!=0);
		updateVisuals();
	}

	int  TruckEditorElement::getSelectedNodesRopable()
	{
		if (free_selected_node==0) return SEL_FALSE;
		int resp=nodes[selected_nodes[0]].ropable;
		int i;
		for (i=1; i<free_selected_node; i++) if ((int)nodes[selected_nodes[i]].ropable!=resp) return SEL_MIXED;
		return resp;
	}

	void TruckEditorElement::setSelectedNodesTie(int mode)
	{
		if (mode==SEL_MIXED) return;
		int i;
		for (i=0; i<free_selected_node; i++) nodes[selected_nodes[i]].hasTie=(mode!=0);
		updateVisuals();
	}

	int  TruckEditorElement::getSelectedNodesTie()
	{
		if (free_selected_node==0) return SEL_FALSE;
		int resp=nodes[selected_nodes[0]].hasTie;
		int i;
		for (i=1; i<free_selected_node; i++) if ((int)nodes[selected_nodes[i]].hasTie!=resp) return SEL_MIXED;
		return resp;
	}


	int  TruckEditorElement::getSelectedNodesMode(char mode)
	{
		if (free_selected_node==0) return SEL_FALSE;
		int resp=nodes[selected_nodes[0]].mode==mode;
		int i;
		for (i=1; i<free_selected_node; i++) if ((nodes[selected_nodes[i]].mode==mode)!=resp) return SEL_MIXED;
		return resp;
	}

	float TruckEditorElement::getSelectedNodesTieLength() {return nodes[selected_nodes[0]].tieMaxl;}
	void TruckEditorElement::setSelectedNodesTieLength(float v) {nodes[selected_nodes[0]].tieMaxl=v;}
	float TruckEditorElement::getSelectedNodesTieRate() {return nodes[selected_nodes[0]].tieRate;}
	void TruckEditorElement::setSelectedNodesTieRate(float v) {nodes[selected_nodes[0]].tieRate=v;}
	float TruckEditorElement::getSelectedNodesTieStop() {return nodes[selected_nodes[0]].tieShort;}
	void TruckEditorElement::setSelectedNodesTieStop(float v) {nodes[selected_nodes[0]].tieShort=v;}

	void TruckEditorElement::setSelectedBeamsHydroRate(float v) {beams[selected_beams[0]].hydroRatio=v;}
	float TruckEditorElement::getSelectedBeamsHydroRate() {return beams[selected_beams[0]].hydroRatio;}
	void TruckEditorElement::setSelectedBeamsShockK(float v) {beams[selected_beams[0]].s=v;}
	float TruckEditorElement::getSelectedBeamsShockK() {return beams[selected_beams[0]].s;}
	void TruckEditorElement::setSelectedBeamsShockD(float v) {beams[selected_beams[0]].d=v;}
	float TruckEditorElement::getSelectedBeamsShockD() {return beams[selected_beams[0]].d;}
	void TruckEditorElement::setSelectedBeamsShockShort(float v) {beams[selected_beams[0]].sbound=v;}
	float TruckEditorElement::getSelectedBeamsShockShort() {return beams[selected_beams[0]].sbound;}
	void TruckEditorElement::setSelectedBeamsShockLong(float v) {beams[selected_beams[0]].lbound=v;}
	float TruckEditorElement::getSelectedBeamsShockLong() {return beams[selected_beams[0]].lbound;}
	void TruckEditorElement::setSelectedBeamsShockPrecomp(float v) {beams[selected_beams[0]].precomp=v;}
	float TruckEditorElement::getSelectedBeamsShockPrecomp() {return beams[selected_beams[0]].precomp;}
	void TruckEditorElement::setSelectedBeamsCommandRate(float v) {beams[selected_beams[0]].commandRate=v;}
	float TruckEditorElement::getSelectedBeamsCommandRate() {return beams[selected_beams[0]].commandRate;}
	void TruckEditorElement::setSelectedBeamsCommandShortBound(float v) {beams[selected_beams[0]].commandShort=v;}
	float TruckEditorElement::getSelectedBeamsCommandShortBound() {return beams[selected_beams[0]].commandShort;}
	void TruckEditorElement::setSelectedBeamsCommandShortKey(float v) {beams[selected_beams[0]].commandKeyS=(int)v;}
	float TruckEditorElement::getSelectedBeamsCommandShortKey() {return beams[selected_beams[0]].commandKeyS;}
	void TruckEditorElement::setSelectedBeamsCommandLongBound(float v) {beams[selected_beams[0]].commandLong=v;}
	float TruckEditorElement::getSelectedBeamsCommandLongBound() {return beams[selected_beams[0]].commandLong;}
	void TruckEditorElement::setSelectedBeamsCommandLongKey(float v) {beams[selected_beams[0]].commandKeyL=(int)v;}
	float TruckEditorElement::getSelectedBeamsCommandLongKey() {return beams[selected_beams[0]].commandKeyL;}

	int TruckEditorElement::getSelectedBeamsStructuralVisible()
	{
		if (free_selected_beam==0) return SEL_FALSE;
		int resp=beams[selected_beams[0]].mode=='v';
		int i;
		for (i=1; i<free_selected_beam; i++) if ((int)(beams[selected_beams[i]].mode=='v')!=resp) return SEL_MIXED;
		return resp;
	}

	void TruckEditorElement::setSelectedBeamsStructuralVisible(bool v)
	{
		int i;
		for (i=0; i<free_selected_beam; i++)
			if (v) beams[selected_beams[i]].mode='v';
			else beams[selected_beams[i]].mode='i';
	}

	void TruckEditorElement::setSelectedBeamsMode(int mode)
	{
		int i;
		for (i=0; i<free_selected_beam; i++)
		{
			beams[selected_beams[i]].isRope=false;
			beams[selected_beams[i]].isHydro=false;
			beams[selected_beams[i]].isShock=false;
			beams[selected_beams[i]].isCommand=false;
			switch (mode)
			{
			case EBEAM_STRUCTURAL: break;
			case EBEAM_ROPE: beams[selected_beams[i]].isRope=true; break;
			case EBEAM_HYDRO: beams[selected_beams[i]].isHydro=true; break;
			case EBEAM_SHOCK: beams[selected_beams[i]].isShock=true; break;
			case EBEAM_COMMAND: beams[selected_beams[i]].isCommand=true; break;
			}
		}
	}

	int TruckEditorElement::getBeamMode(int id)
	{
		if (beams[id].isRope) return EBEAM_ROPE;
		if (beams[id].isHydro) return EBEAM_HYDRO;
		if (beams[id].isShock) return EBEAM_SHOCK;
		if (beams[id].isCommand) return EBEAM_COMMAND;
		return EBEAM_STRUCTURAL;
	}
	int TruckEditorElement::getSelectedBeamsMode()
	{
		if (free_selected_beam==0) return EBEAM_MIXED;
		int i;
		int ret=getBeamMode(selected_beams[0]);
		for (i=0; i<free_selected_beam; i++) if (getBeamMode(selected_beams[i])!= ret) ret=EBEAM_MIXED;
		return ret;
	}

	int TruckEditorElement::numSelected()
	{
		return free_selected_node;
	}
	int TruckEditorElement::numBeamSelected()
	{
		return free_selected_beam;
	}

	void TruckEditorElement::dedupe()
	{
		int i,j,k;
		bool hasdupe=true;
		while (hasdupe)
		{
			hasdupe=false;
			for (i=0; i<free_node; i++)
			{
				for (j=i+1; j<free_node; j++)
				{
					if ((nodes[i].Position-nodes[j].Position).length()<snapgrid/2.0)
					{
						//we found a dupe
						for (k=0; k<free_beam; k++)
						{
							if (beams[k].id1==j) beams[k].id1=i;
							if (beams[k].id2==j) beams[k].id2=i;
						}
						deleteNode(j);
						i=free_node;
						j=free_node;
						hasdupe=true;
					}
				}
			}
		}
	}

	TruckEditorElement::~TruckEditorElement()
	{
	}

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

// created by Thomas Fischer thomas{AT}thomasfischer{DOT}biz, 7th of September 2009

#include "gui_friction.h"
#include "gui_manager.h"
#include "Ogre.h"
#include "Settings.h"

#include "collisions.h"
#include "Beam.h"

#include "language.h"

using namespace Ogre;

template<> GUI_Friction * Singleton< GUI_Friction >::ms_Singleton = 0;
GUI_Friction* GUI_Friction::getSingletonPtr(void)
{
	return ms_Singleton;
}
GUI_Friction& GUI_Friction::getSingleton(void)
{
	assert( ms_Singleton );  return ( *ms_Singleton );
}

GUI_Friction::GUI_Friction() : col(0)
{
	int x=0, xb=0, y=0;
	win = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 500,  MyGUI::Align::Center, "Back");
	win->setCaption(_L("Friction Settings"));

	// Fluid parameters
	xb = 10; y = 10;

	x=10;
	MyGUI::StaticTextPtr t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("active Ground Type:"));
	t->setTextAlign(MyGUI::Align::Right);

	MyGUI::ComboBoxPtr cb = win->createWidget<MyGUI::ComboBox>("ComboBox", xb + x, y, 115, 20,  MyGUI::Align::Default, "combo_grounds");
	cb->addItem("concrete");
	cb->addItem("asphalt");
	cb->addItem("gravel");
	cb->addItem("rock");
	cb->addItem("ice");
	cb->addItem("snow");
	cb->addItem("metal");
	cb->addItem("grass");
	cb->addItem("sand");
	cb->setEditStatic(true);
	cb->setIndexSelected(0);


	x=0; y+=40; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Fluid Settings"));

	xb += 10;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Fluid Density:"));
	t->setTextAlign(MyGUI::Align::Right);
	MyGUI::EditPtr e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "fluid_density"); x+=55;
	MyGUI::HScrollPtr h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "fluid_density_scroll"); x+=60;


	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("flow consistency index:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "fluid_flowcon"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "fluid_flowcon_scroll"); x+=60;
	
	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("flow_behavior_index:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "fluid_flowbeh"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "fluid_flowbeh_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("solid_ground_level:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "fluid_solidlevel"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "fluid_solidlevel_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("drag_anisotropy:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy_scroll"); x+=60;

	
	xb -= 10;
	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Solid Ground Settings"));
	


	xb += 10;
	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Adhesion Velocity:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "solid_adhension_velo"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "solid_adhension_velo_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("static friction coef:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "solid_static_friction"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "solid_static_friction_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("dynamic friction coef:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "solid_dynamic_friction"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "solid_dynamic_friction_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("hydrodynamic coef:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "solid_hydrodynamic"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "solid_hydrodynamic_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Stribeck velocity:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "solid_stribeck"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "solid_stribeck_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("alpha:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "solid_alpha"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "solid_alpha_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("strength:"));
	t->setTextAlign(MyGUI::Align::Right);
	e = win->createWidget<MyGUI::Edit>("Edit", xb+x, y, 50, 20,  MyGUI::Align::Default, "solid_strength"); x+=55;
	h = win->createWidget<MyGUI::HScroll>("HScroll", xb+x, y, 60, 20,  MyGUI::Align::Default, "solid_strength_scroll"); x+=60;

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("fx_type:"));
	t->setTextAlign(MyGUI::Align::Right);
	cb = win->createWidget<MyGUI::ComboBox>("ComboBox", xb + x, y, 115, 20,  MyGUI::Align::Default, "combo_fx_type");
	cb->addItem("dusty");
	cb->addItem("hard");
	cb->addItem("clumpy");
	cb->setEditStatic(true);
	cb->setIndexSelected(0);

	// TO BE DONE

	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("fx_color:"));
	t->setTextAlign(MyGUI::Align::Right);
	// TO BE DONE


	xb -= 10;
	x=0; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Ground Debug"));
	

	xb += 10;
	x=0; y+=60; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", xb + x, y, 350, 20,  MyGUI::Align::Default, "current_ground"); x+=350;
	t->setCaption(_L("current ground: N/A"));

	win->setVisible(true);
}

GUI_Friction::~GUI_Friction()
{
}

void GUI_Friction::setVisible(bool value)
{
	win->setVisible(value);
	MyGUI::PointerManager::getInstance().setVisible(value);
}

bool GUI_Friction::getVisible()
{
	return win->isVisible();
}

void GUI_Friction::setActiveCol(ground_model_t *gm)
{
	MyGUI::ComboBoxPtr cb = dynamic_cast<MyGUI::ComboBoxPtr>(win->findWidget("combo_grounds"));

	for(int i=0;i<(int)cb->getItemCount();i++)
	{
		if(cb->getItemNameAt(i) == UTFString(gm->name))
		{
			cb->setIndexSelected(i);
			break;
		}
	}
}

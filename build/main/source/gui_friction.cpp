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
	int x=0, y=0, by=0;
	MyGUI::EditPtr e;
	MyGUI::HScrollPtr h;
	MyGUI::ButtonPtr b;

	win = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 500,  MyGUI::Align::Center, "Back");
	win->setCaption(_L("Friction Settings"));

	// Fluid parameters
	x=10, y=10;
	MyGUI::StaticTextPtr t = win->createWidget<MyGUI::StaticText>("StaticText", x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("active Ground Type:"));
	t->setTextAlign(MyGUI::Align::Right);

	MyGUI::ComboBoxPtr cb = win->createWidget<MyGUI::ComboBox>("ComboBox", x, y, 115, 20,  MyGUI::Align::Default, "combo_grounds");
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


	x=10; y+=40; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Solid Ground Settings"));
	
	// adding panel for solid ground
	x=10; by = y + 20; // set box start now
	MyGUI::WidgetPtr p = win->createWidget<MyGUI::Widget>("Panel", x, by, 350, 25,  MyGUI::Align::Default);
	{
		int lx=0, ly=5; // local coordinate system
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Adhesion Velocity:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_adhension_velo"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_adhension_velo_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_adhension_velo_help"); lx+=20;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("static friction coef:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_static_friction"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_static_friction_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_static_friction_help"); lx+=20;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("dynamic friction coef:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_dynamic_friction"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_dynamic_friction_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_dynamic_friction_help"); lx+=20;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("hydrodynamic coef:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_hydrodynamic"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_hydrodynamic_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_hydrodynamic_help"); lx+=25;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Stribeck velocity:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_stribeck"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_stribeck_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_stribeck_help"); lx+=25;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("alpha:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_alpha"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_alpha_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_alpha_help"); lx+=25;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("strength:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_strength"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_strength_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_strength_help"); lx+=25;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("fx_type:"));
		t->setTextAlign(MyGUI::Align::Right);
		cb = p->createWidget<MyGUI::ComboBox>("ComboBox", lx, ly, 115, 20,  MyGUI::Align::Default, "combo_fx_type"); lx+=120;
		cb->addItem("dusty");
		cb->addItem("hard");
		cb->addItem("clumpy");
		cb->setEditStatic(true);
		cb->setIndexSelected(0);
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "combo_fx_help"); lx+=25;
		b->setCaption("?");

		lx=0; ly+=20; 		
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("fx_color:"));
		t->setTextAlign(MyGUI::Align::Right);
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fx_color_help"); lx+=25;
		b->setCaption("?");
		// TO BE DONE

		lx=0; ly+=20; 		
		// add height to global height
		y += ly;
	}
	p->setSize(350, y-by+25);

	x=10; y+=40; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Fluid Settings"));
	
	// adding panel for solid ground
	x=10; by = y + 20; // set box start now
	p = win->createWidget<MyGUI::Widget>("Panel", x, by, 350, 25,  MyGUI::Align::Default);
	{
		int lx=0, ly=5; // local coordinate system

		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Fluid Density:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_density"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_density_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_density_help"); lx+=25;
		b->setCaption("?");


		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("flow consistency index:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_flowcon"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_flowcon_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_flowcon_help"); lx+=25;
		b->setCaption("?");
		
		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("flow_behavior_index:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_flowbeh"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_flowbeh_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_flowbeh_help"); lx+=25;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("solid_ground_level:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_solidlevel"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_solidlevel_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_solidlevel_help"); lx+=25;
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("drag_anisotropy:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy_scroll"); lx+=65;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy_help"); lx+=25;
		b->setCaption("?");
		
		lx=0; ly+=20; 		
		// add height to global height
		y += ly;
	}
	p->setSize(350, y-by+25);
	
	x=10; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText",  x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Ground Debug"));
	
	x=10; y+=60; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", x, y, 350, 20,  MyGUI::Align::Default, "current_ground"); x+=350;
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

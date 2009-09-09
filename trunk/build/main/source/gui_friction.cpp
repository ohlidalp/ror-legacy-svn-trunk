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

GUI_Friction::GUI_Friction() : col(0), active_gm(0), selected_gm(0)
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
	t->setCaption(_L("selected Ground Type:"));
	t->setTextAlign(MyGUI::Align::Right);

	MyGUI::ComboBoxPtr cb = win->createWidget<MyGUI::ComboBox>("ComboBox", x, y, 115, 20,  MyGUI::Align::Default, "combo_grounds");
	cb->eventComboAccept = MyGUI::newDelegate(this, &GUI_Friction::event_combo_grounds_eventComboAccept);
	cb->eventComboChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_combo_grounds_eventComboAccept);
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
	
	
	x=20; y+=20;
	t = win->createWidget<MyGUI::StaticText>("StaticText", x, y, 270, 20,  MyGUI::Align::Default, "text_current_ground"); x+=275;
	t->setCaption("Current active Ground: n/a");
	b = win->createWidget<MyGUI::Button>("Button", x, y, 70, 20,  MyGUI::Align::Default, "select_current_ground"); x+=75;
	b->setCaption("select");
	b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);

	x=10; y+=30; 
	t = win->createWidget<MyGUI::StaticText>("StaticText", x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Solid ground settings"));
	
	// adding panel for solid ground
	x=10; by = y + 20; // set box start now
	MyGUI::WidgetPtr p = win->createWidget<MyGUI::Widget>("Panel", x, by, 350, 25,  MyGUI::Align::Default);
	{
		int lx=0, ly=5; // local coordinate system
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Solid ground level:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_level"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_level_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["solid_level_scroll"] = 0.1f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_level_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Strength:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_strength"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_strength_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["solid_strength_scroll"] = 0.1f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_strength_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Static friction coef:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_static_friction"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_static_friction_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["solid_static_friction_scroll"] = 0.01f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_static_friction_help"); lx+=20;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Adhesion velocity:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_adhension_velo"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_adhension_velo_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["solid_adhension_velo_scroll"] = 0.1f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_adhension_velo_help"); lx+=20;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");


		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Dynamic friction coef:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_dynamic_friction"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_dynamic_friction_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["solid_dynamic_friction_scroll"] = 0.1f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_dynamic_friction_help"); lx+=20;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Hydrodynamic friction coef:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_hydrodynamic"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_hydrodynamic_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["solid_hydrodynamic_scroll"] = 0.01f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_hydrodynamic_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Stribeck velocity:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_stribeck"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_stribeck_scroll"); lx+=65;
		scrollFactors["solid_stribeck_scroll"] = 0.1f;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_stribeck_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("alpha:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "solid_alpha"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "solid_alpha_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["solid_alpha_scroll"] = 0.1f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "solid_alpha_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
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
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 		
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("fx_color:"));
		t->setTextAlign(MyGUI::Align::Right);
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 140, 20,  MyGUI::Align::Default, "fx_color_text"); lx+=145;
		t->setCaption("");
		t->setTextAlign(MyGUI::Align::Left);
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fx_color_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
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
		t->setCaption(_L("Flow behavior index:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_flowbeh"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_flowbeh_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["fluid_flowbeh_scroll"] = 0.1f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_flowbeh_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Flow consistency:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_flowcon"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_flowcon_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["fluid_flowcon_scroll"] = 15.0f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_flowcon_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");

		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Fluid density:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_density"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_density_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["fluid_density_scroll"] = 0.5f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_density_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");


		lx=0; ly+=20; 
		t = p->createWidget<MyGUI::StaticText>("StaticText", lx, ly, 170, 20,  MyGUI::Align::Default); lx+=175;
		t->setCaption(_L("Drag anisotropy:"));
		t->setTextAlign(MyGUI::Align::Right);
		e = p->createWidget<MyGUI::Edit>("Edit", lx, ly, 80, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy"); lx+=85;
		h = p->createWidget<MyGUI::HScroll>("HScroll", lx, ly, 60, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy_scroll"); lx+=65;
		h->eventScrollChangePosition = MyGUI::newDelegate(this, &GUI_Friction::event_scroll_value);
		h->setScrollRange(1000);
		scrollFactors["fluid_drag_anisotropy_scroll"] = 0.1f;
		b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20,  MyGUI::Align::Default, "fluid_drag_anisotropy_help"); lx+=25;
		b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
		b->setCaption("?");
		
		lx=0; ly+=20; 		
		// add height to global height
		y += ly;
	}
	p->setSize(350, y-by+25);
	
	x=10; y+=40; 
	b = win->createWidget<MyGUI::Button>("Button", x, y, 350, 30,  MyGUI::Align::Default, "apply_changes"); x+=25;
	b->eventMouseButtonClick = MyGUI::newDelegate(this, &GUI_Friction::event_btn_MouseButtonClick);
	b->setCaption("Apply Changes");

	/*
	x=10; y+=20; 
	t = win->createWidget<MyGUI::StaticText>("StaticText",  x, y, 170, 20,  MyGUI::Align::Default); x+=175;
	t->setCaption(_L("Ground Debug"));
	*/
	
	win->setVisible(false);

	// set initial values
	ground_model_t *gm = col->getGroundModelByString("gravel");
	if(gm) updateControls(gm);
	selected_gm = gm;
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
	if(!gm) return;
	active_gm = gm;

	MyGUI::StaticTextPtr t = (MyGUI::StaticTextPtr)win->findWidget("text_current_ground");
	if(t) t->setCaption(_L("Current active Ground: ") + String(gm->name));
	if(selected_gm == gm)
		updateControls(gm);
}

void GUI_Friction::updateControls(ground_model_t *gm, bool setCombo)
{
	if(!gm) return;

	if(setCombo)
	{
		MyGUI::ComboBoxPtr cb = (MyGUI::ComboBoxPtr)win->findWidget("combo_grounds");
		if(!cb) return;

		for(int i=0;i<(int)cb->getItemCount();i++)
		{
			if(cb->getItemNameAt(i) == UTFString(gm->name))
			{
				cb->setIndexSelected(i);
				selected_gm = gm;
				break;
			}
		}
	}

	MyGUI::EditPtr e = (MyGUI::EditPtr)win->findWidget("solid_adhension_velo");
	if(e) e->setCaption(StringConverter::toString(gm->va));
	MyGUI::HScrollPtr h = (MyGUI::HScrollPtr)win->findWidget("solid_adhension_velo_scroll");
	if(h) h->setScrollPosition((size_t)(gm->va / scrollFactors["solid_adhension_velo_scroll"]));
	
	e = (MyGUI::EditPtr)win->findWidget("solid_level");
	if(e) e->setCaption(StringConverter::toString(gm->solid_ground_level));
	h = (MyGUI::HScrollPtr)win->findWidget("solid_adhension_velo_scroll");
	if(h) h->setScrollPosition((size_t)(gm->solid_ground_level / scrollFactors["solid_adhension_velo_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("solid_static_friction");
	if(e) e->setCaption(StringConverter::toString(gm->ms));
	h = (MyGUI::HScrollPtr)win->findWidget("solid_static_friction_scroll");
	if(h) h->setScrollPosition((size_t)(gm->ms / scrollFactors["solid_static_friction_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("solid_dynamic_friction");
	if(e) e->setCaption(StringConverter::toString(gm->mc));
	h = (MyGUI::HScrollPtr)win->findWidget("solid_dynamic_friction_scroll");
	if(h) h->setScrollPosition((size_t)(gm->mc / scrollFactors["solid_dynamic_friction_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("solid_hydrodynamic");
	if(e) e->setCaption(StringConverter::toString(gm->t2));
	h = (MyGUI::HScrollPtr)win->findWidget("solid_hydrodynamic_scroll");
	if(h) h->setScrollPosition((size_t)(gm->t2 / scrollFactors["solid_hydrodynamic_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("solid_stribeck");
	if(e) e->setCaption(StringConverter::toString(gm->vs));
	h = (MyGUI::HScrollPtr)win->findWidget("solid_stribeck_scroll");
	if(h) h->setScrollPosition((size_t)(gm->vs / scrollFactors["solid_stribeck_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("solid_alpha");
	if(e) e->setCaption(StringConverter::toString(gm->alpha));
	h = (MyGUI::HScrollPtr)win->findWidget("solid_alpha_scroll");
	if(h) h->setScrollPosition((size_t)(gm->alpha / scrollFactors["solid_alpha_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("solid_strength");
	if(e) e->setCaption(StringConverter::toString(gm->strength));
	h = (MyGUI::HScrollPtr)win->findWidget("solid_strength_scroll");
	if(h) h->setScrollPosition((size_t)(gm->strength / scrollFactors["solid_strength_scroll"]));

	// fluid
	e = (MyGUI::EditPtr)win->findWidget("fluid_flowbeh");
	if(e) e->setCaption(StringConverter::toString(gm->flow_behavior_index));
	h = (MyGUI::HScrollPtr)win->findWidget("fluid_flowbeh_scroll");
	if(h) h->setScrollPosition((size_t)(gm->flow_behavior_index / scrollFactors["fluid_flowbeh_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("fluid_flowcon");
	if(e) e->setCaption(StringConverter::toString(gm->flow_consistency_index));
	h = (MyGUI::HScrollPtr)win->findWidget("fluid_flowcon_scroll");
	if(h) h->setScrollPosition((size_t)(gm->flow_consistency_index / scrollFactors["fluid_flowcon_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("fluid_density");
	if(e) e->setCaption(StringConverter::toString(gm->fluid_density));
	h = (MyGUI::HScrollPtr)win->findWidget("fluid_density_scroll");
	if(h) h->setScrollPosition((size_t)(gm->fluid_density / scrollFactors["fluid_density_scroll"]));

	e = (MyGUI::EditPtr)win->findWidget("fluid_drag_anisotropy");
	if(e) e->setCaption(StringConverter::toString(gm->drag_anisotropy));
	h = (MyGUI::HScrollPtr)win->findWidget("fluid_drag_anisotropy_scroll");
	if(h) h->setScrollPosition((size_t)(gm->drag_anisotropy / scrollFactors["fluid_drag_anisotropy_scroll"]));

	MyGUI::ComboBoxPtr cb = (MyGUI::ComboBoxPtr)win->findWidget("combo_fx_type");
	if(gm->fx_type == FX_DUSTY)  cb->setIndexSelected(0);
	if(gm->fx_type == FX_HARD)  cb->setIndexSelected(1);
	if(gm->fx_type == FX_CLUMPY)  cb->setIndexSelected(2);

	MyGUI::StaticTextPtr t = (MyGUI::StaticTextPtr)win->findWidget("fx_color_text");
	if(t) t->setCaption(StringConverter::toString(gm->fx_coulour));

}

void GUI_Friction::event_combo_grounds_eventComboAccept(MyGUI::WidgetPtr _sender, size_t _index)
{
	if(!col) return;
	if(!win->isVisible()) return;
	MyGUI::ComboBoxPtr cb = (MyGUI::ComboBoxPtr)win->findWidget("combo_grounds");
	if(!cb) return;
	ground_model_t *gm = col->getGroundModelByString(cb->getItemNameAt(_index));
	if(gm)
		updateControls(gm, false);
	selected_gm = gm;
}

void GUI_Friction::event_btn_MouseButtonClick(MyGUI::WidgetPtr _sender)
{
	if(_sender->getName() == "select_current_ground")
	{
		updateControls(active_gm);
	}else if(_sender->getName() == "solid_level_help")
	{
		MyGUI::MessagePtr msg = MyGUI::Message::createMessage(Ogre::String("Help"),Ogre::String("Help on solid_level_help"), true, MyGUI::Message::IconInfo | MyGUI::Message::Ok);
	}else if(_sender->getName() == "foobat")
	{
		MyGUI::MessagePtr msg = MyGUI::Message::createMessage(Ogre::String("Help"),Ogre::String("Help on solid_level_help"), true, MyGUI::Message::IconInfo | MyGUI::Message::Ok);
	}else if(_sender->getName() == "apply_changes")
	{
		applyChanges();
	}
	
	LogManager::getSingleton().logMessage(" Friction GUI button pressed: " + _sender->getCaption());
}

void GUI_Friction::event_scroll_value(MyGUI::WidgetPtr _sender, size_t _value)
{
	String name = _sender->getName();
	if(name.size() > 7  && name.substr(name.size() - 7, 7) == "_scroll")
	{
		MyGUI::EditPtr e = (MyGUI::EditPtr)win->findWidget(name.substr(0, name.size() - 7));
		Real factor = scrollFactors[name];
		if(e && factor > 0.0f) e->setCaption(StringConverter::toString(((float)_value) * factor));
	}

}

void GUI_Friction::applyChanges()
{
	MyGUI::EditPtr e = (MyGUI::EditPtr)win->findWidget("solid_adhension_velo");
	selected_gm->va = StringConverter::parseReal(e->getCaption());
	
	e = (MyGUI::EditPtr)win->findWidget("solid_level");
	selected_gm->solid_ground_level = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("solid_static_friction");
	selected_gm->ms = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("solid_dynamic_friction");
	selected_gm->mc = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("solid_hydrodynamic");
	selected_gm->t2 = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("solid_stribeck");
	selected_gm->vs = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("solid_alpha");
	selected_gm->alpha = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("solid_strength");
	selected_gm->strength = StringConverter::parseReal(e->getCaption());

	// fluid
	e = (MyGUI::EditPtr)win->findWidget("fluid_flowbeh");
	selected_gm->flow_behavior_index = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("fluid_flowcon");
	selected_gm->flow_consistency_index = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("fluid_density");
	selected_gm->fluid_density = StringConverter::parseReal(e->getCaption());

	e = (MyGUI::EditPtr)win->findWidget("fluid_drag_anisotropy");
	selected_gm->drag_anisotropy = StringConverter::parseReal(e->getCaption());

	MyGUI::ComboBoxPtr cb = (MyGUI::ComboBoxPtr)win->findWidget("combo_fx_type");
	if(cb->getIndexSelected() == 0)
		selected_gm->fx_type = FX_DUSTY;
	else if(cb->getIndexSelected() == 1)
		selected_gm->fx_type = FX_HARD;
	else if(cb->getIndexSelected() == 2)
		selected_gm->fx_type = FX_CLUMPY;

}
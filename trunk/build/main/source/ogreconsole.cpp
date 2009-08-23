#include "ogreconsole.h"
#include "ScriptEngine.h"
#include "InputEngine.h"
#include "OgreFont.h"
#include "OgreFontManager.h"

using namespace Ogre;
using namespace std;

template<> OgreConsole *Ogre::Singleton<OgreConsole>::ms_Singleton=0;

OgreConsole::OgreConsole()
{
	start_line=0;
	cursor_position=0;
	blinkdelay=0;
	history_pos=0;
	history.push_back("");
	insertmode=true;
	line_count=15;
	line_lenght=85;
	font_name="VeraMono";
}

OgreConsole::~OgreConsole()
{

}

void OgreConsole::init(Ogre::Root *root, Ogre::RenderWindow *win)
{
	if(!root->getSceneManagerIterator().hasMoreElements())
		OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "No scene manager found!", "init" );

	this->root=root;

	//we have a monospaced font!
	FontPtr font = FontManager::getSingleton().getByName(font_name);

	float char_height_percent = 0.025f;
	float char_width_percent = char_height_percent * font->getGlyphAspectRatio('X');
	float char_height = char_height_percent * win->getHeight();
	float char_width = char_width_percent * win->getWidth();

	//determine the size of the console dynamically
	line_count = ((win->getHeight() * 0.5f) / char_height) - 1; //-1 because of prompt
	line_lenght = 100; //win->getWidth() / char_width;
	LogManager::getSingleton().logMessage("console line count: " + StringConverter::toString(line_count));
	LogManager::getSingleton().logMessage("console line lenght: " + StringConverter::toString(line_lenght));

	scene=root->getSceneManagerIterator().getNext();
	root->addFrameListener(this);

	height=0;

	MaterialPtr mat = MaterialManager::getSingleton().getByName("console/background");
	if(mat.isNull())
	{
		// create one fast
		MaterialPtr mat = MaterialManager::getSingleton().create("console/background", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		mat->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		mat->getTechnique(0)->getPass(0)->setDiffuse(0, 0, 0, 0.9f);
		//mat->getTechnique(0)->getPass(0)->setLightingEnabled(false);
	}
	// Create background rectangle covering the whole screen
	rect = new Rectangle2D(true);
	rect->setCorners(-1, 1, 1, 1-height);
	rect->setMaterial("console/background");
	rect->setRenderQueueGroup(RENDER_QUEUE_OVERLAY);
	rect->setBoundingBox(AxisAlignedBox(-100000.0*Vector3::UNIT_SCALE, 100000.0*Vector3::UNIT_SCALE));
	node = scene->getRootSceneNode()->createChildSceneNode("#Console");
	node->attachObject(rect);

	// BE WARNED: you have to use a monospaced font, otherwise the cursor is not going to work correctly!
	textbox=OverlayManager::getSingleton().createOverlayElement("TextArea","ConsoleText");
	textbox->setCaption("_");
	textbox->setMetricsMode(GMM_RELATIVE);
	textbox->setPosition(0,0);
	textbox->setParameter("font_name", font_name);
	textbox->setParameter("colour_top","1 1 1");
	textbox->setParameter("colour_bottom","1 1 1");
	textbox->setParameter("char_height", StringConverter::toString(char_height_percent));
	textbox->hide();

	promptbox=OverlayManager::getSingleton().createOverlayElement("TextArea","ConsoleTextPrompt");
	promptbox->setCaption("_");
	promptbox->setMetricsMode(GMM_RELATIVE);
	promptbox->setPosition(0, 0);
	promptbox->setParameter("font_name", font_name);
	promptbox->setParameter("colour_top","1 1 1");
	promptbox->setParameter("colour_bottom","1 1 1");
	promptbox->setParameter("char_height", StringConverter::toString(char_height_percent));
	promptbox->hide();

	overlay=OverlayManager::getSingleton().create("Console");
	overlay->add2D((OverlayContainer*)textbox);
	overlay->add2D((OverlayContainer*)promptbox);
	overlay->show();
	LogManager::getSingleton().getDefaultLog()->addListener(this);
}

void OgreConsole::shutdown()
{
	if(!initialized) return;
	delete rect;
	delete node;
	delete textbox;
	delete promptbox;
	delete overlay;
}

void OgreConsole::onKeyPressed(const OIS::KeyEvent &arg)
{
	if(!visible)
		return;

	// hide key pressed?
	if(arg.key == INPUTENGINE.getKeboardKeyForCommand(EV_COMMON_CONSOLEDISPLAY))
	{
		OgreConsole::getSingleton().setVisible(false);
		return;
	}

	switch(arg.key)
	{
	case OIS::KC_ESCAPE:
		OgreConsole::getSingleton().setVisible(false);
		return;
		break;
	case OIS::KC_RETURN:
		// add some specials
		if(history[history_pos] == "hide")
		{
			this->setVisible(false);
			cursor_position=0;
			history.push_back(""); // new, empty last entry
			history_pos = history.size() - 1; // switch to the new line
			return;
		}

		// normal from now on
		print(history[history_pos]);
#ifdef ANGELSCRIPT
		ScriptEngine::getSingleton().executeString(history[history_pos]);
#endif //ANGELSCRIPT
		cursor_position=0;
		history.push_back(""); // new, empty last entry
		history_pos = history.size() - 1; // switch to the new line
		break;

	case OIS::KC_BACK:
		// if you try to edit the history, rather make a new copy and edit that then :)
		if((unsigned int)history_pos != history.size() - 1)
		{
			// add the edited element to the back
			history.push_back(history[history_pos]);
			history_pos = history.size() - 1;
		}
		// combine what is before and after the cursor
		if(cursor_position>0)
		{
			history[history_pos] = history[history_pos].substr(0,cursor_position-1) + history[history_pos].substr(cursor_position);
			cursor_position--;
		}
		break;

	case OIS::KC_DELETE:
		// if you try to edit the history, rather make a new copy and edit that then :)
		if((unsigned int)history_pos != history.size() - 1)
		{
			// add the edited element to the back
			history.push_back(history[history_pos]);
			history_pos = history.size() - 1;
		}
		// combine what is before and after the cursor
		if(cursor_position<(int)history[history_pos].size())
		{
			history[history_pos] = history[history_pos].substr(0,cursor_position) + history[history_pos].substr(cursor_position+1);
			cursor_position++;
		}
		break;

	case OIS::KC_INSERT:
		insertmode = !insertmode;
		break;

	case OIS::KC_UP:
		if(history_pos > 0)
		{
			history_pos--;
			cursor_position=history[history_pos].size()-1;
		}
		break;

	case OIS::KC_DOWN:
		if(history_pos < (int)history.size() - 1)
		{
			history_pos++;
			cursor_position=history[history_pos].size()-1;
		}
		break;

	case OIS::KC_PGUP:
		if(start_line>0)
			start_line--;
		break;

	case OIS::KC_LEFT:
		if(cursor_position>0)
			cursor_position--;
		break;

	case OIS::KC_RIGHT:
		if(cursor_position<(int)history[history_pos].size())
			cursor_position++;
		break;

	case OIS::KC_PGDOWN:
		if(start_line<(int)lines.size())
			start_line++;
		break;

	default:
		char legalchars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890+!\"#'%&/()=?[]\\*-_.:,;<> ";
		for(unsigned int c=0;c<sizeof(legalchars)-1;c++)
		{
			if(legalchars[c]==(char)arg.text)
			{
				// if you try to edit the history, rather make a new copy and edit that then :)
				if((unsigned int)history_pos != history.size() - 1)
				{
					// add the edited element to the back
					history.push_back(history[history_pos]);
					history_pos = history.size() - 1;
				}
				if((unsigned int)cursor_position == history[history_pos].size())
					history[history_pos] += arg.text;
				else if(insertmode)
					history[history_pos] = history[history_pos].substr(0,cursor_position) + arg.text + history[history_pos].substr(cursor_position);
				else if(!insertmode)
					history[history_pos] = history[history_pos].substr(0,cursor_position) + arg.text + history[history_pos].substr(cursor_position+1);

				cursor_position++;
				break;
			}
		}
	}
	update_overlay=true;
}

bool OgreConsole::frameStarted(const Ogre::FrameEvent &evt)
{
	if(!visible && !promptbox->isVisible()) return true;

	blinkdelay += evt.timeSinceLastFrame;
	if(visible && blinkdelay > 0.2f)
	{
		if(promptbox->isVisible())
			promptbox->hide();
		else
			promptbox->show();

		blinkdelay = 0;
	}

	if(visible&&height<1)
	{
		height+=evt.timeSinceLastFrame*2;
		textbox->show();
		promptbox->show();
		if(height>=1)
		{
			height=1;
		}
	} else if(!visible&&height>0)
	{
		height-=evt.timeSinceLastFrame*2;
		if(height<=0)
		{
			height=0;
			textbox->hide();
			promptbox->hide();
		}
	}

	textbox->setPosition(0,(height-1)*0.5);
	promptbox->setPosition(0,(height-1)*0.5);
	rect->setCorners(-1,1+height,1,1-height);

	if(update_overlay)
	{
		String text, text_prompt;
		list<String>::iterator i,start,end;

		//make sure is in range
		if(start_line>(int)lines.size())
			start_line=lines.size();

		int lcount=0;
		start=lines.begin();
		for(int c=0;c<start_line;c++)
			start++;
		end=start;
		for(int c=0;c<line_count;c++)
		{
			if(end==lines.end())
				break;
			end++;
		}
		for(i=start;i!=end;i++)
		{
			text += *i + "\n";
			text_prompt += "\n";
		}

		// browsing in history
		String entry = history[history_pos];
		text += "] " + entry;
		if(cursor_position > (int)entry.size())
			cursor_position = entry.size();

		String blank = "";
		for(int i=0;i<cursor_position;i++)
			blank+=" ";

		text_prompt += "  " + blank + (insertmode?"_":"_");

		textbox->setCaption(text);
		promptbox->setCaption(text_prompt);
		update_overlay=false;
	}
	return true;
}

void OgreConsole::print(const String &text)
{
	//subdivide it into lines
	const char *str=text.c_str();
	int start=0,count=0;
	int len=text.length();
	String line;
	for(int c=0;c<len;c++)
	{
		if(str[c]=='\n' || (int)line.length()>=line_lenght)
		{
			lines.push_back(line);
			line="";
		}
		if(str[c]!='\n')
			line+=str[c];
	}
	if(line.length())
		lines.push_back(line);
	if((int)lines.size() > line_count)
		start_line=lines.size()-line_count;
	else
		start_line=0;
	update_overlay=true;
}

bool OgreConsole::frameEnded(const Ogre::FrameEvent &evt)
{
	return true;
}

void OgreConsole::setVisible(bool visible)
{
	this->visible=visible;
}

void OgreConsole::messageLogged( const Ogre::String& message, LogMessageLevel lml, bool maskDebug, const String &logName )
{
	String msg = message;
	//this->print(logName+": "+message);
	// strip script engine things
	if(message.substr(0,4) == "SE| ")
		msg = message.substr(4);
	this->print(msg);
}

#include "ogreconsole.h"

using namespace Ogre;
using namespace std;

template<> OgreConsole *Ogre::Singleton<OgreConsole>::ms_Singleton=0;

#define CONSOLE_LINE_LENGTH 85
#define CONSOLE_LINE_COUNT 15

OgreConsole::OgreConsole()
{
	start_line=0;
}

OgreConsole::~OgreConsole()
{
   
}

void OgreConsole::init(Ogre::Root *root)
{
	if(!root->getSceneManagerIterator().hasMoreElements())
		OGRE_EXCEPT( Exception::ERR_INTERNAL_ERROR, "No scene manager found!", "init" );

	this->root=root;
	scene=root->getSceneManagerIterator().getNext();
	root->addFrameListener(this);

	height=1;

	MaterialPtr mat = MaterialManager::getSingleton().getByName("console/background");
	if(mat.isNull())
	{
		// create one fast
		MaterialPtr mat = MaterialManager::getSingleton().create("console/background", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
		mat->getTechnique(0)->getPass(0)->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
		mat->getTechnique(0)->getPass(0)->setDiffuse(0, 0, 1, 0.6);
	}
	// Create background rectangle covering the whole screen
	rect = new Rectangle2D(true);
	rect->setCorners(-1, 1, 1, 1-height);
	rect->setMaterial("console/background");
	rect->setRenderQueueGroup(RENDER_QUEUE_OVERLAY);
	rect->setBoundingBox(AxisAlignedBox(-100000.0*Vector3::UNIT_SCALE, 100000.0*Vector3::UNIT_SCALE));
	node = scene->getRootSceneNode()->createChildSceneNode("#Console");
	node->attachObject(rect);

	textbox=OverlayManager::getSingleton().createOverlayElement("TextArea","ConsoleText");
	textbox->setCaption("hello");
	textbox->setMetricsMode(GMM_RELATIVE);
	textbox->setPosition(0,0);
	textbox->setParameter("font_name","VeraMono");
	textbox->setParameter("colour_top","1 1 1");
	textbox->setParameter("colour_bottom","1 1 1");
	textbox->setParameter("char_height","0.02");

	overlay=OverlayManager::getSingleton().create("Console");   
	overlay->add2D((OverlayContainer*)textbox);
	overlay->show();
	LogManager::getSingleton().getDefaultLog()->addListener(this);
}

void OgreConsole::shutdown(){
	if(!initialized) return;
	delete rect;
	delete node;
	delete textbox;
	delete overlay;
}

void OgreConsole::onKeyPressed(const OIS::KeyEvent &arg)
{
	if(!visible)
		return;
	if (arg.key == OIS::KC_RETURN)
	{
		// execute stuff here later
		print(prompt);
		prompt="";
	}
	if (arg.key == OIS::KC_BACK)
		prompt=prompt.substr(0,prompt.length()-1);
	if (arg.key == OIS::KC_PGUP)
	{
		if(start_line>0)
			start_line--;
	}
	if (arg.key == OIS::KC_PGDOWN)
	{
		if(start_line<(int)lines.size())
			start_line++;
	}
	else
	{
		char legalchars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890+!\"#'%&/()=?[]\\*-_.:,; ";
		for(int c=0;c<sizeof(legalchars);c++)
		{
			if(legalchars[c]==arg.text)
			{
				prompt+=arg.text;
				break;
			}
		}
	}
	update_overlay=true;
}

bool OgreConsole::frameStarted(const Ogre::FrameEvent &evt)
{
	if(visible&&height<1)
	{
		height+=evt.timeSinceLastFrame*2;
		textbox->show();
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
		}
	}

	textbox->setPosition(0,(height-1)*0.5);
	rect->setCorners(-1,1+height,1,1-height);

	if(update_overlay)
	{
		String text;
		list<String>::iterator i,start,end;

		//make sure is in range
		if(start_line>(int)lines.size())
			start_line=lines.size();

		int lcount=0;
		start=lines.begin();
		for(int c=0;c<start_line;c++)
			start++;
		end=start;
		for(int c=0;c<CONSOLE_LINE_COUNT;c++)
		{
			if(end==lines.end())
				break;
			end++;
		}
		for(i=start;i!=end;i++)
			text+=(*i)+"\n";

		//add the prompt
		text+="] "+prompt;

		textbox->setCaption(text);
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
		if(str[c]=='\n'||line.length()>=CONSOLE_LINE_LENGTH)
		{
			lines.push_back(line);
			line="";
		}
		if(str[c]!='\n')
			line+=str[c];
	}
	if(line.length())
		lines.push_back(line);
	if(lines.size()>CONSOLE_LINE_COUNT)
		start_line=lines.size()-CONSOLE_LINE_COUNT;
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
	this->print(logName+": "+message);
}

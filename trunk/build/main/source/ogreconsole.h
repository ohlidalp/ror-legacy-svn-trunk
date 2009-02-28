#ifndef OGRECONSOLE_H__
#define OGRECONSOLE_H__

#include <OgreFrameListener.h>
#include <Ogre.h>
#include <OIS.h>
#include <list>
#include <vector>

class OgreConsole: public Ogre::Singleton<OgreConsole>, Ogre::FrameListener, Ogre::LogListener
{
public:
	OgreConsole();
	~OgreConsole();

	void init(Ogre::Root *root, Ogre::RenderWindow *win);
	void shutdown();

	void setVisible(bool visible);
	bool getVisible(){return visible;}

	void print(const Ogre::String &text);

	virtual bool frameStarted(const Ogre::FrameEvent &evt);
	virtual bool frameEnded(const Ogre::FrameEvent &evt);

	void onKeyPressed(const OIS::KeyEvent &arg);

	void messageLogged( const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName );

protected:
	bool visible;
	bool initialized;
	Ogre::Root *root;
	Ogre::SceneManager *scene;
	Ogre::Rectangle2D *rect;
	Ogre::SceneNode *node;
	Ogre::OverlayElement *textbox;
	Ogre::OverlayElement *promptbox;
	Ogre::Overlay *overlay;

	float height;
	float blinkdelay;
	bool update_overlay;
	bool insertmode;
	int start_line;
	int history_pos;
	int line_lenght;
	int line_count;
	int cursor_position;
	std::list<Ogre::String> lines;
	std::vector<Ogre::String> history;
};

#endif //OGRECONSOLE_H__

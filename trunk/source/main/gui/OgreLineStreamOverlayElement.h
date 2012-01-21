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

#ifndef __LineStreamOverlayElement_H__
#define __LineStreamOverlayElement_H__

#include "RoRPrerequisites.h"

#include "OgreOverlayContainer.h"
#include "OgreOverlayElementFactory.h"

namespace Ogre {


    /** OverlayElement representing a flat, single-material (or transparent) panel which can contain other elements.
    @remarks
        This class subclasses OverlayContainer because it can contain other elements. Like other
        containers, if hidden it's contents are also hidden, if moved it's contents also move etc. 
        The panel itself is a 2D rectangle which is either completely transparent, or is rendered 
        with a single material. The texture(s) on the panel can be tiled depending on your requirements.
    @par
        This component is suitable for backgrounds and grouping other elements. Note that because
        it has a single repeating material it cannot have a discrete border (unless the texture has one and
        the texture is tiled only once). For a bordered panel, see it's subclass BorderLineStreamOverlayElement.
    @par
        Note that the material can have all the usual effects applied to it like multiple texture
        layers, scrolling / animated textures etc. For multiple texture layers, you have to set 
        the tiling level for each layer.
    */
    class LineStreamOverlayElement : public OverlayContainer
    {
    public:
        /** Constructor. */
        LineStreamOverlayElement(const String& name);
        virtual ~LineStreamOverlayElement();

        /** Initialise */
        virtual void initialise(void);

        /** See OverlayElement. */
        virtual const String& getTypeName(void) const;
        /** See Renderable. */
        void getRenderOperation(RenderOperation& op);
        /** Overridden from OverlayElement */
        void setMaterialName(const String& matName);
        /** Overridden from OverlayContainer */
        void _updateRenderQueue(RenderQueue* queue);

		void defaultStyle();

		// data members set\get
		uint32 getNumberOfTraces() const;
		void setNumberOfTraces(const uint32 val);
		uint32 getNumberOfSamplesForTrace() const;
		void setNumberOfSamplesForTrace(const uint32 val);

		void setTraceValue(const uint32 traceIndex, const Real traceValue);
		void setExactValue(const uint32 traceIndex, const uint32 fieldIndex, const Real traceValue);
		void setTraceInfo(const uint32 traceIndex, const ColourValue & traceValue, const String &name);
		void updateVtxBuffer();
		void moveForward();
		void createVertexBuffer();

		void setMoveMode(int value);
		int getMoveMode();
		void setTitle(ColourValue colour, String title);
	protected:

		typedef struct trace_info
		{
			ColourValue colour;
			String name;
			TextAreaOverlayElement *legendText;
		} trace_info_t;
			
		RenderOperation mRenderOp;
        
        /// internal method for setting up geometry, called by OverlayElement::update
        virtual void updatePositionGeometry(void);

		// Internal method which is triggered when the UVs of the element get updated
		virtual void updateTextureGeometry(void);

        static String msTypeName;

		int moveMode;

		uint32 mNumberOfTraces;
		uint32 mNumberOfSamplesForTrace;

		uint32 mPosInStream;
		Ogre::String myTitle;
		Ogre::ColourValue titleColour;
		TextAreaOverlayElement *legendTop, *legendBottom, *title;


		std::vector<Real> mTraceSamples;
		std::vector<trace_info_t> mTraceInfo;

		HardwareVertexBufferSharedPtr mCurrentVtxBuffer;
		bool mDataChanged;



    };

	/** Factory for creating LineStreamOverlayElementFactory instances. */
	class LineStreamOverlayElementFactory: public OverlayElementFactory
	{
	public:
		/** See OverlayElementFactory */
		OverlayElement* createOverlayElement(const String& instanceName);

		virtual void destroyOverlayElement(OverlayElement* pElement);;

		/** See OverlayElementFactory */
		const String& getTypeName(void) const;
	};


}

#endif

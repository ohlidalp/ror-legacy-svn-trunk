/*
This file is part of Rigs of Rods
Copyright 2007 Pierre-Michel Ricordel
This source code must not be used or published without author's consent.
Contact: pierre-michel@ricordel.org
*/

/*#include "OgreStableHeaders.h"
*/
#include "Ogre.h"
#include "spincontrol.h"
#include "OgreMaterial.h"
#include "OgreTechnique.h"
#include "OgrePass.h"
#include "OgreStringConverter.h"
#include "OgreHardwareBufferManager.h"
#include "OgreRoot.h"
#include "OgreRenderSystem.h"

namespace Ogre {
    //---------------------------------------------------------------------
    String SpinControl::msTypeName = "SpinControl";
    SpinControl::CmdTiling SpinControl::msCmdTiling;
    SpinControl::CmdTransparent SpinControl::msCmdTransparent;
    SpinControl::CmdUVCoords SpinControl::msCmdUVCoords;
    SpinControl::CmdMinValue SpinControl::msCmdMinValue;
    SpinControl::CmdMaxValue SpinControl::msCmdMaxValue;
    SpinControl::CmdPostDot SpinControl::msCmdPostDot;
    SpinControl::CmdUnit SpinControl::msCmdUnit;
    //---------------------------------------------------------------------
    // vertex buffer bindings, set at compile time (we could look these up but no point)
    #define POSITION_BINDING 0
    #define TEXCOORD_BINDING 1

    //---------------------------------------------------------------------
    SpinControl::SpinControl(const String& name)
        : OverlayContainer(name)
        , mTransparent(false)
        // Defer creation of texcoord buffer until we know how big it needs to be
        , mNumTexCoordsInBuffer(0)
        , mU1(0.0)
        , mV1(0.0)
        , mU2(1.0)
        , mV2(1.0)

    {
		preminvalue=premaxvalue=prepostdot=0;
		digitpositive=true;
        // Init tiling
        for (ushort i = 0; i < OGRE_MAX_TEXTURE_COORD_SETS; ++i)
        {
            mTileX[i] = 1.0f;
            mTileY[i] = 1.0f;
        }

        // No normals or colours
        if (createParamDictionary("SpinControl"))
        {
            addBaseParameters();
        }

    }
    //---------------------------------------------------------------------
    SpinControl::~SpinControl()
    {
        delete mRenderOp.vertexData;
    }
    //---------------------------------------------------------------------
    void SpinControl::initialise(void)
    {
		bool init = !mInitialised;

		OverlayContainer::initialise();
		if (init)
		{
			// Setup render op in advance
			mRenderOp.vertexData = new VertexData();
			// Vertex declaration: 1 position, add texcoords later depending on #layers
			// Create as separate buffers so we can lock & discard separately
			VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
			decl->addElement(POSITION_BINDING, 0, VET_FLOAT3, VES_POSITION);

			// Basic vertex data
			mRenderOp.vertexData->vertexStart = 0;
			mRenderOp.vertexData->vertexCount = 4;

			// Vertex buffer #1
			HardwareVertexBufferSharedPtr vbuf =
				HardwareBufferManager::getSingleton().createVertexBuffer(
				decl->getVertexSize(POSITION_BINDING), mRenderOp.vertexData->vertexCount,
				HardwareBuffer::HBU_STATIC_WRITE_ONLY// mostly static except during resizing
				);
			// Bind buffer
			mRenderOp.vertexData->vertexBufferBinding->setBinding(POSITION_BINDING, vbuf);

			// No indexes & issue as a strip
			mRenderOp.useIndexes = false;
			mRenderOp.operationType = RenderOperation::OT_TRIANGLE_STRIP;

			mInitialised = true;

			spinEnabled=false;
			//let's build the control
			//maxvalue=StringConverter::parseReal(getParameter("max_value"));
			maxvalue=premaxvalue;
			//minvalue=StringConverter::parseReal(getParameter("min_value"));
			minvalue=preminvalue;
			//int postdot=StringConverter::parseInt(getParameter("post_dot"));
			int postdot=(int)prepostdot;
			//determine the number of digits
			dotpos=(int)(log10(maxvalue))+1;
			numdigits=dotpos+postdot;
			//okay
			int i;
			float dotoffset=0;
			if (minvalue<0)
			{
				//the sign digit
				sign=static_cast<TextAreaOverlayElement*>(OverlayManager::getSingleton().createOverlayElement ("TextArea", mName+"-signtext"));
				sign->setCaption("+");
				sign->setColour(ColourValue::Black);
				sign->setFontName("Cyberbit");
				sign->setCharHeight(0.018);
				sign->setPosition(0.002,0.012);
				sign->setWidth(0.01);
				sign->setHeight(0.02);
				addChild(sign);
			}
			for (i=0; i<numdigits; i++)
			{
				digits[i]=0;
				if (i>=dotpos) dotoffset=0.002;
				//the digit background
				bpoe[i]=static_cast<BorderPanelOverlayElement*>(OverlayManager::getSingleton().createOverlayElement ("BorderPanel", mName+"-digitbg"+StringConverter::toString(i)));
				bpoe[i]->setMaterialName("gui/active");
				bpoe[i]->setBorderMaterialName("gui/borders");
				bpoe[i]->setPosition(0.01+(float)i*0.01+dotoffset,0.01);
				bpoe[i]->setWidth(0.01);
				bpoe[i]->setHeight(0.019);
				bpoe[i]->setBorderSize(0.001/1.33,0.001/1.33,0.001,0.001);
				addChild(bpoe[i]);
				//the digit text
				taoe[i]=static_cast<TextAreaOverlayElement*>(OverlayManager::getSingleton().createOverlayElement ("TextArea", mName+"-digittext"+StringConverter::toString(i)));
				taoe[i]->setCaption("-");
				taoe[i]->setColour(ColourValue::Black);
				taoe[i]->setFontName("Cyberbit");
				taoe[i]->setCharHeight(0.018);
				taoe[i]->setPosition(0.012+(float)i*0.01+dotoffset,0.012);
				taoe[i]->setWidth(0.01);
				taoe[i]->setHeight(0.02);
				//NewPanel->setParameter("start_angle", StringConverter::toString(Math::fDeg2Rad * 0.0f));
				addChild(taoe[i]);
				//up
				bpoeup[i]=static_cast<BorderPanelOverlayElement*>(OverlayManager::getSingleton().createOverlayElement ("BorderPanel", mName+"-digitU"+StringConverter::toString(i)));
				bpoeup[i]->setMaterialName("gui/up");
				bpoeup[i]->setBorderMaterialName("gui/borders");
				bpoeup[i]->setPosition(0.01+(float)i*0.01+dotoffset,0.00);
				bpoeup[i]->setWidth(0.01);
				bpoeup[i]->setHeight(0.01);
				bpoeup[i]->setBorderSize(0.001/1.33,0.001/1.33,0.001,0.001);
				addChild(bpoeup[i]);
				//down
				bpoedown[i]=static_cast<BorderPanelOverlayElement*>(OverlayManager::getSingleton().createOverlayElement ("BorderPanel", mName+"-digitD"+StringConverter::toString(i)));
				bpoedown[i]->setMaterialName("gui/down");
				bpoedown[i]->setBorderMaterialName("gui/borders");
				bpoedown[i]->setPosition(0.01+(float)i*0.01+dotoffset,0.029);
				bpoedown[i]->setWidth(0.01);
				bpoedown[i]->setHeight(0.01);
				bpoedown[i]->setBorderSize(0.001/1.33,0.001/1.33,0.001,0.001);
				addChild(bpoedown[i]);
			}
			//the unit
			if (preunit!="")
			{
				//the unit
				unit=static_cast<TextAreaOverlayElement*>(OverlayManager::getSingleton().createOverlayElement ("TextArea", mName+"-unittext"));
				unit->setCaption(preunit);
				unit->setColour(ColourValue::Black);
				unit->setFontName("Cyberbit");
				unit->setCharHeight(0.018);
				unit->setPosition(0.012+(float)numdigits*0.01+dotoffset,0.012);
				unit->setWidth(0.01);
				unit->setHeight(0.02);
				addChild(unit);
			}
			spinRedraw();
			//set default value
			//setSpinValue(1234.56);
		}
    }

	void SpinControl::spinRedraw()
	{
		int i;
		for (i=0; i<numdigits; i++)
		{
			if (spinEnabled)
			{
				char dtext[2];
				dtext[0]=digits[i]+'0';
				dtext[1]=0;
				taoe[i]->setCaption(dtext);
				bpoe[i]->setMaterialName("gui/active");
			}
			else
			{
				taoe[i]->setCaption("-");
				bpoe[i]->setMaterialName("gui/background");
			}
		}
	}

	void SpinControl::setSpinEnable(bool e)
	{
		spinEnabled=e;
		spinRedraw();
	}
	bool SpinControl::getSpinEnable()
	{
		return spinEnabled;
	}

	float SpinControl::getSpinValue()
	{
		char convert[20];
		int i;
		int pos=1;
		if (digitpositive) convert[0]='+'; else convert[0]='-';
		for (i=0; i<numdigits; i++)
		{
			convert[pos]='0'+digits[i];
			pos++;
			if (i==dotpos-1) {convert[pos]='.';pos++;};
		}
		convert[pos]=0;
		return StringConverter::parseReal(convert);
	}

	void SpinControl::setSpinValue(float val)
	{
		if (val>maxvalue||val<minvalue) return;
		if (val<0)
		{
			val=-val;
			digitpositive=false;
		}
		else digitpositive=true;
		int i;
		int ival=(int)val;
		for (i=dotpos-1; i>=0; i--)
		{
			digits[i]=ival%10;
			ival=ival/10;
		}
		for (i=dotpos; i<numdigits; i++)
		{
			val=val*10.0;
			digits[i]=((int)(val))%10;
		}
		spinRedraw();
	}

	void SpinControl::clickEvent(char* objname)
	{
		if (!spinEnabled) return;
		if (objname[strlen(objname)-2]=='U')
		{
			int id=objname[strlen(objname)-1]-'0';
			if (id<numdigits && id>=0)
			{
				setSpinValue(getSpinValue()+pow((float)10, (float)(dotpos-1-id)));
			}
		}
		if (objname[strlen(objname)-2]=='D')
		{
			int id=objname[strlen(objname)-1]-'0';
			if (id<numdigits && id>=0)
			{
				setSpinValue(getSpinValue()-pow((float)10, (float)(dotpos-1-id)));
			}
		}
	}


    //---------------------------------------------------------------------
    void SpinControl::setTiling(Real x, Real y, ushort layer)
    {
        assert (layer < OGRE_MAX_TEXTURE_COORD_SETS);
        assert (x != 0 && y != 0);

        mTileX[layer] = x;
        mTileY[layer] = y;

        mGeomUVsOutOfDate = true;

    }
    //---------------------------------------------------------------------
    Real SpinControl::getTileX(ushort layer) const
    {
        return mTileX[layer];
    }
    //---------------------------------------------------------------------
    Real SpinControl::getTileY(ushort layer) const
    {
        return mTileY[layer];
    }
    //---------------------------------------------------------------------
    void SpinControl::setTransparent(bool isTransparent)
    {
        mTransparent = isTransparent;
    }
    //---------------------------------------------------------------------
    bool SpinControl::isTransparent(void) const
    {
        return mTransparent;
    }
    //---------------------------------------------------------------------
    void SpinControl::setUV(Real u1, Real v1, Real u2, Real v2)
    {
		mU1 = u1;
		mU2 = u2;
		mV1 = v1;
		mV2 = v2;
		mGeomUVsOutOfDate = true;
    }
    void SpinControl::getUV(Real& u1, Real& v1, Real& u2, Real& v2) const
    {
		u1 = mU1;
		u2 = mU2;
		v1 = mV1;
		v2 = mV2;
    }
    //---------------------------------------------------------------------
    const String& SpinControl::getTypeName(void) const
    {
        return msTypeName;
    }
    //---------------------------------------------------------------------
    void SpinControl::getRenderOperation(RenderOperation& op)
    {
        op = mRenderOp;
    }
    //---------------------------------------------------------------------
    void SpinControl::setMaterialName(const String& matName)
    {
        OverlayContainer::setMaterialName(matName);
    }
    //---------------------------------------------------------------------
    void SpinControl::_updateRenderQueue(RenderQueue* queue)
    {
        if (mVisible)
        {

            if (!mTransparent && !mpMaterial.isNull())
            {
                OverlayElement::_updateRenderQueue(queue);
            }

            // Also add children
            ChildIterator it = getChildIterator();
            while (it.hasMoreElements())
            {
                // Give children ZOrder 1 higher than this
                it.getNext()->_updateRenderQueue(queue);
            }
        }
    }
    //---------------------------------------------------------------------
    void SpinControl::updatePositionGeometry(void)
    {
		/*
			0-----2
			|    /|
			|  /  |
			|/    |
			1-----3
		*/
		Real left, right, top, bottom;

		/* Convert positions into -1, 1 coordinate space (homogenous clip space).
			- Left / right is simple range conversion
			- Top / bottom also need inverting since y is upside down - this means
			that top will end up greater than bottom and when computing texture
			coordinates, we have to flip the v-axis (ie. subtract the value from
			1.0 to get the actual correct value).
		*/
		left = _getDerivedLeft() * 2 - 1;
		right = left + (mWidth * 2);
		top = -((_getDerivedTop() * 2) - 1);
		bottom =  top -  (mHeight * 2);

		HardwareVertexBufferSharedPtr vbuf =
			mRenderOp.vertexData->vertexBufferBinding->getBuffer(POSITION_BINDING);
		float* pPos = static_cast<float*>(
			//vbuf.);
			vbuf->lock(HardwareBuffer::HBL_NORMAL) );

		// Use the furthest away depth value, since materials should have depth-check off
		// This initialised the depth buffer for any 3D objects in front
		Real zValue = Root::getSingleton().getRenderSystem()->getMaximumDepthInputValue();
		*pPos++ = left;
		*pPos++ = top;
		*pPos++ = zValue;

		*pPos++ = left;
		*pPos++ = bottom;
		*pPos++ = zValue;

		*pPos++ = right;
		*pPos++ = top;
		*pPos++ = zValue;

		*pPos++ = right;
		*pPos++ = bottom;
		*pPos++ = zValue;

		vbuf->unlock();
    }
    //---------------------------------------------------------------------
    void SpinControl::updateTextureGeometry(void)
    {
        // Generate for as many texture layers as there are in material
        if (!mpMaterial.isNull() && mInitialised)
        {
            // Assume one technique and pass for the moment
            size_t numLayers = mpMaterial->getTechnique(0)->getPass(0)->getNumTextureUnitStates();

            VertexDeclaration* decl = mRenderOp.vertexData->vertexDeclaration;
            // Check the number of texcoords we have in our buffer now
            if (mNumTexCoordsInBuffer > numLayers)
            {
                // remove extras
                for (size_t i = mNumTexCoordsInBuffer; i > numLayers; --i)
                {
                    decl->removeElement(VES_TEXTURE_COORDINATES, (unsigned short)i);
                }
            }
            else if (mNumTexCoordsInBuffer < numLayers)
            {
                // Add extra texcoord elements
                size_t offset = VertexElement::getTypeSize(VET_FLOAT2) * mNumTexCoordsInBuffer;
                for (size_t i = mNumTexCoordsInBuffer; i < numLayers; ++i)
                {
                    decl->addElement(TEXCOORD_BINDING,
                        offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, (unsigned short)i);
                    offset += VertexElement::getTypeSize(VET_FLOAT2);

                }
            }

            // if number of layers changed at all, we'll need to reallocate buffer
            if (mNumTexCoordsInBuffer != numLayers)
            {
                // NB reference counting will take care of the old one if it exists
                HardwareVertexBufferSharedPtr newbuf =
                    HardwareBufferManager::getSingleton().createVertexBuffer(
                    decl->getVertexSize(TEXCOORD_BINDING), mRenderOp.vertexData->vertexCount,
                    HardwareBuffer::HBU_STATIC_WRITE_ONLY // mostly static except during resizing
                    );
                // Bind buffer, note this will unbind the old one and destroy the buffer it had
                mRenderOp.vertexData->vertexBufferBinding->setBinding(TEXCOORD_BINDING, newbuf);
                // Set num tex coords in use now
                mNumTexCoordsInBuffer = numLayers;
            }

            // Get the tcoord buffer & lock
			if (mNumTexCoordsInBuffer)
			{
				HardwareVertexBufferSharedPtr vbuf =
					mRenderOp.vertexData->vertexBufferBinding->getBuffer(TEXCOORD_BINDING);
				float* pVBStart = static_cast<float*>(
		//											  vbuf);
					vbuf->lock(HardwareBuffer::HBL_NORMAL) );

				size_t uvSize = VertexElement::getTypeSize(VET_FLOAT2) / sizeof(float);
				size_t vertexSize = decl->getVertexSize(TEXCOORD_BINDING) / sizeof(float);
				for (ushort i = 0; i < numLayers; ++i)
				{
				    // Calc upper tex coords
                    Real upperX = mU2 * mTileX[i];
                    Real upperY = mV2 * mTileY[i];


				    /*
					    0-----2
					    |    /|
					    |  /  |
					    |/    |
					    1-----3
				    */
				    // Find start offset for this set
				    float* pTex = pVBStart + (i * uvSize);

                    pTex[0] = mU1;
                    pTex[1] = mV1;

                    pTex += vertexSize; // jump by 1 vertex stride
                    pTex[0] = mU1;
                    pTex[1] = upperY;

                    pTex += vertexSize;
                    pTex[0] = upperX;
                    pTex[1] = mV1;

                    pTex += vertexSize;
                    pTex[0] = upperX;
                    pTex[1] = upperY;
				}
				vbuf->unlock();
			}
        }
    }
    //-----------------------------------------------------------------------
    void SpinControl::addBaseParameters(void)
    {
        OverlayContainer::addBaseParameters();
        ParamDictionary* dict = getParamDictionary();

        dict->addParameter(ParameterDef("uv_coords",
           "The texture coordinates for the texture. 1 set of uv values."
           , PT_STRING),
           &msCmdUVCoords);

        dict->addParameter(ParameterDef("tiling",
            "The number of times to repeat the background texture."
            , PT_STRING),
            &msCmdTiling);

        dict->addParameter(ParameterDef("transparent",
            "Sets whether the panel is transparent, i.e. invisible itself "
            "but it's contents are still displayed."
            , PT_BOOL),
            &msCmdTransparent);
		dict->addParameter(ParameterDef("min_value",
			"Minimum value", PT_REAL),&msCmdMinValue);
		dict->addParameter(ParameterDef("max_value",
			"Maximum value", PT_REAL),&msCmdMaxValue);
		dict->addParameter(ParameterDef("post_dot",
			"Digits post dot", PT_REAL),&msCmdPostDot);
		dict->addParameter(ParameterDef("unit",
			"Digits unit", PT_STRING),&msCmdUnit);
    }
    //-----------------------------------------------------------------------
    // Command objects
    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    String SpinControl::CmdMinValue::doGet(const void* target) const
    {
        return StringConverter::toString(
			static_cast<const SpinControl*>(target)->preminvalue);
    }
    void SpinControl::CmdMinValue::doSet(void* target, const String& val)
    {
        static_cast<SpinControl*>(target)->preminvalue=StringConverter::parseReal(val);
    }
    //-----------------------------------------------------------------------
    String SpinControl::CmdMaxValue::doGet(const void* target) const
    {
        return StringConverter::toString(
			static_cast<const SpinControl*>(target)->premaxvalue);
    }
    void SpinControl::CmdMaxValue::doSet(void* target, const String& val)
    {
        static_cast<SpinControl*>(target)->premaxvalue=StringConverter::parseReal(val);
    }
    //-----------------------------------------------------------------------
    String SpinControl::CmdPostDot::doGet(const void* target) const
    {
        return StringConverter::toString(
			static_cast<const SpinControl*>(target)->prepostdot);
    }
    void SpinControl::CmdPostDot::doSet(void* target, const String& val)
    {
        static_cast<SpinControl*>(target)->prepostdot=StringConverter::parseReal(val);
    }

    //-----------------------------------------------------------------------
    String SpinControl::CmdUnit::doGet(const void* target) const
    {
        return
			static_cast<const SpinControl*>(target)->preunit;
    }
    void SpinControl::CmdUnit::doSet(void* target, const String& val)
    {
        static_cast<SpinControl*>(target)->preunit=val;
    }

	String SpinControl::CmdTiling::doGet(const void* target) const
    {
        // NB only returns 1st layer tiling
        String ret = "0 " + StringConverter::toString(
            static_cast<const SpinControl*>(target)->getTileX() );
        ret += " " + StringConverter::toString(
            static_cast<const SpinControl*>(target)->getTileY() );
        return ret;
    }
    void SpinControl::CmdTiling::doSet(void* target, const String& val)
    {
        // 3 params: <layer> <x_tile> <y_tile>
        // Param count is validated higher up
        std::vector<String> vec = StringUtil::split(val);
        ushort layer = (ushort)StringConverter::parseUnsignedInt(vec[0]);
        Real x_tile = StringConverter::parseReal(vec[1]);
        Real y_tile = StringConverter::parseReal(vec[2]);

        static_cast<SpinControl*>(target)->setTiling(x_tile, y_tile, layer);
    }
    //-----------------------------------------------------------------------
    String SpinControl::CmdTransparent::doGet(const void* target) const
    {
        return StringConverter::toString(
            static_cast<const SpinControl*>(target)->isTransparent() );
    }
    void SpinControl::CmdTransparent::doSet(void* target, const String& val)
    {
        static_cast<SpinControl*>(target)->setTransparent(
            StringConverter::parseBool(val));
    }
    //-----------------------------------------------------------------------
    String SpinControl::CmdUVCoords::doGet(const void* target) const
    {
        Real u1, v1, u2, v2;

        static_cast<const SpinControl*>(target)->getUV(u1, v1, u2, v2);
        String ret = " " + StringConverter::toString(u1) + " "
             + StringConverter::toString(v1) + " " + StringConverter::toString(u2) + " "
             + StringConverter::toString(v2);

        return ret;
    }
    void SpinControl::CmdUVCoords::doSet(void* target, const String& val)
    {
        std::vector<String> vec = StringUtil::split(val);

        static_cast<SpinControl*>(target)->setUV(
            StringConverter::parseReal(vec[0]),
            StringConverter::parseReal(vec[1]),
            StringConverter::parseReal(vec[2]),
            StringConverter::parseReal(vec[3])
            );
    }

/** See OverlayElementFactory */
Ogre::OverlayElement *SpinControlOverlayElementFactory::createOverlayElement(const Ogre::String &instanceName)
{
   return new SpinControl(instanceName);
}

/** See OverlayElementFactory */
const Ogre::String &SpinControlOverlayElementFactory::getTypeName() const
{
   static Ogre::String name = "SpinControl";
   return name;
}

}




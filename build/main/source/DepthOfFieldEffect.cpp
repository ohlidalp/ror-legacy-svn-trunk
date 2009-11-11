// "Depth of Field" demo for Ogre
// Copyright (C) 2006  Christian Lindequist Larsen
//
// This code is in the public domain. You may do whatever you want with it.

#include "DepthOfFieldEffect.h"
#include "Ogre.h"

using namespace Ogre;

const int DepthOfFieldEffect::BLUR_DIVISOR = 4;

DepthOfFieldEffect::DepthOfFieldEffect(Viewport* viewport)
	: mViewport(viewport)
	, mNearDepth(10.0)
	, mFocalDepth(100.0)
	, mFarDepth(190.0)
	, mFarBlurCutoff(1.0)
{
	createDepthRenderTexture();
	createCompositor();
	addCompositor();
}

DepthOfFieldEffect::~DepthOfFieldEffect()
{
	removeCompositor();
	destroyCompositor();
	destroyDepthRenderTexture();
}

void DepthOfFieldEffect::setFocalDepths(float nearDepth, float focalDepth, float farDepth)
{
	mNearDepth = nearDepth;
	mFocalDepth = focalDepth;
	mFarDepth = farDepth;
}

void DepthOfFieldEffect::setFarBlurCutoff(float cutoff)
{
	mFarBlurCutoff = cutoff;
}

bool DepthOfFieldEffect::getEnabled() const
{
	return mCompositor->getEnabled();
}

void DepthOfFieldEffect::setEnabled(bool value)
{
	mCompositor->setEnabled(value);
}

void DepthOfFieldEffect::createDepthRenderTexture()
{
	// Create the depth render texture
	TexturePtr depthTexture = TextureManager::getSingleton().createManual(
		"DoF_Depth", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, mViewport->getActualWidth(), mViewport->getActualHeight(),
		0, PF_L8, TU_RENDERTARGET);

	// Get its render target and add a viewport to it
	RenderTexture* depthTarget = depthTexture->getBuffer()->getRenderTarget();
	Viewport* depthViewport = depthTarget->addViewport(mViewport->getCamera());

	// Register 'this' as a render target listener
	depthTarget->addListener(this);

	// Get the technique to use when rendering the depth render texture
	mDepthMaterial = MaterialManager::getSingleton().getByName("DoF_Depth");
	mDepthMaterial->load(); // needs to be loaded manually
	mDepthTechnique = mDepthMaterial->getBestTechnique();

	// Create a custom render queue invocation sequence for the depth render texture
    RenderQueueInvocationSequence* invocationSequence =
		Root::getSingleton().createRenderQueueInvocationSequence("DoF_Depth");

	// Add a render queue invocation to the sequence, and disable shadows for it
    RenderQueueInvocation* invocation = invocationSequence->add(RENDER_QUEUE_MAIN, "main");
    invocation->setSuppressShadows(true);

	// Set the render queue invocation sequence for the depth render texture viewport
    depthViewport->setRenderQueueInvocationSequenceName("DoF_Depth");

	// update debug texture
	//MaterialPtr mat = MaterialManager::getSingleton().getByName("DoF_DepthDebug");
	//mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureName("DoF_Depth");

}

void DepthOfFieldEffect::destroyDepthRenderTexture()
{
	// TODO
}

void DepthOfFieldEffect::createCompositor()
{
	// Create compositor
	CompositorPtr compositor = CompositorManager::getSingleton().create(
		"DoF_Compositor", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	{
		// Create technique
		CompositionTechnique* technique = compositor->createTechnique();
		{
			// Create texture definition 'scene'
			CompositionTechnique::TextureDefinition* definition = technique->createTextureDefinition("scene");
			definition->width = 0;  // target width
			definition->height = 0; // target height
			definition->format = PF_X8R8G8B8;
		}
		{
			// Create texture definition 'blur'
			CompositionTechnique::TextureDefinition* definition = technique->createTextureDefinition("blur");
			definition->width = mViewport->getActualWidth() / BLUR_DIVISOR;
			definition->height = mViewport->getActualHeight() / BLUR_DIVISOR;
			definition->format = PF_X8R8G8B8;
		}
		{
			// Create target pass 'scene'
			CompositionTargetPass* target = technique->createTargetPass();
			target->setOutputName("scene");
			target->setInputMode(CompositionTargetPass::IM_PREVIOUS);
		}
		{
			// Create target pass 'blur'
			CompositionTargetPass* target = technique->createTargetPass();
			target->setOutputName("blur");
			{
				// Create a composition pass
				CompositionPass* pass = target->createPass();
				pass->setIdentifier(BlurPass);
				pass->setMaterialName("DoF_Gaussian3x3");
				pass->setInput(0, "scene");
			}
		}
		{
			// Build the output target pass
			CompositionTargetPass* output = technique->getOutputTargetPass();
			{
				// Create a composition pass
				CompositionPass* pass = output->createPass();
				pass->setIdentifier(OutputPass);
				pass->setMaterialName("DoF_DepthOfField");
				pass->setInput(0, "scene");
				//pass->setInput(1, "depth"); // render texture
				pass->setInput(2, "blur");
			}
		}
	}
}

void DepthOfFieldEffect::destroyCompositor()
{
	// TODO
}

void DepthOfFieldEffect::addCompositor()
{
	// Add the compositor to the viewport
	mCompositor = CompositorManager::getSingleton().addCompositor(
		mViewport, "DoF_Compositor_test");

	// Register 'this' as a listener
	mCompositor->addListener(this);
}

void DepthOfFieldEffect::removeCompositor()
{
	// TODO
}

void DepthOfFieldEffect::notifyMaterialSetup(uint32 passId, MaterialPtr& material)
{
	switch (passId)
	{
	case BlurPass:
		{
			//float pixelSize[2] = {
			//	1.0f / (mViewport->getActualWidth() / BLUR_DIVISOR),
			//	1.0f / (mViewport->getActualHeight() / BLUR_DIVISOR)};

			// Adjust fragment program parameters
			Ogre::Vector3 pixelSize = Ogre::Vector3(1.0f / (mViewport->getActualWidth() / BLUR_DIVISOR),1.0f / (mViewport->getActualHeight() / BLUR_DIVISOR), 1.0f);
			GpuProgramParametersSharedPtr fragParams = material->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
			fragParams->setNamedConstant("pixelSize", pixelSize);

			break;
		}

	case OutputPass:
		{
			float pixelSizeScene[2] = {
				1.0f / mViewport->getActualWidth(),
				1.0f / mViewport->getActualHeight()};

			float pixelSizeBlur[2] = {
				1.0f / (mViewport->getActualWidth() / BLUR_DIVISOR),
				1.0f / (mViewport->getActualHeight() / BLUR_DIVISOR)};

			// Adjust fragment program parameters
			GpuProgramParametersSharedPtr fragParams =
				material->getBestTechnique()->getPass(0)->getFragmentProgramParameters();
			fragParams->setNamedConstant("pixelSizeScene", pixelSizeScene,
				sizeof(pixelSizeScene)/sizeof(pixelSizeScene[0]));
			fragParams->setNamedConstant("pixelSizeBlur", pixelSizeBlur,
				sizeof(pixelSizeBlur)/sizeof(pixelSizeBlur[0]));

			break;
		}
	}
}

void DepthOfFieldEffect::preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	float dofParams[4] = {mNearDepth, mFocalDepth, mFarDepth, mFarBlurCutoff};

	// Adjust fragment program parameters for depth pass
	GpuProgramParametersSharedPtr fragParams =
		mDepthTechnique->getPass(0)->getFragmentProgramParameters();
	fragParams->setNamedConstant("dofParams", dofParams,
		sizeof(dofParams)/sizeof(dofParams[0]));

	// Add 'this' as a RenderableListener to replace the technique for all renderables
	RenderQueue* queue = evt.source->getCamera()->getSceneManager()->getRenderQueue();
	queue->setRenderableListener(this);
}

void DepthOfFieldEffect::postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	// Reset the RenderableListener
	RenderQueue* queue = evt.source->getCamera()->getSceneManager()->getRenderQueue();
	queue->setRenderableListener(0);
}

bool DepthOfFieldEffect::renderableQueued(Renderable* rend, Ogre::uint8 groupID, Ogre::ushort priority, Technique** ppTech)
{
	// Replace the technique of all renderables
	*ppTech = mDepthTechnique;
	return true;
}

#ifdef OPENCL
#pragma once
#ifndef OPENCL_BEAM_H
#define OPENCL_BEAM_H

#include <Ogre.h>
//#include <oclUtils.h>

class OpenCL_Beam
{
	public:
		OpenCL_Beam();
		~OpenCL_Beam();

		void checkForDevices();

};

#endif //OPENCL_BEAM_H
#endif //OPENCL
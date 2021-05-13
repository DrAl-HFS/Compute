// SimpleOCL.hpp - C++ classes for simple OpenCL usage.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#ifndef SIMPLE_OCL_HPP
#define SIMPLE_OCL_HPP

// For Debian-flavoured Linux on x86, Jetson-Nano, RPi3,4 (64bit OS required).
// POCL (CPU SIMD only acceleration) setup:
// > sudo apt install *opencl-* clinfo
// Note that POCL relies on the "clang" compiler driver i.e. "llvm" compiler tools.
// If clang-6.0 (or whatever) is installed without a generic <clang> link, then it
// may be necessary to create one e.g:
// > sudo ln -s /usr/bin/clang-6.0 /usr/bin/clang
// or just give the more specific name in the Makefile...

#include <CL/cl.h>

// Minimal information required to use a device
class CSimpleOCL
{
public:
   cl_context        ctx;
   cl_command_queue  q;

   CSimpleOCL (cl_device_id id=0) : ctx{0},q{0} { if (0 != id) { create(id); } }

   ~CSimpleOCL () { release(); }

   bool create (cl_device_id id)
   {
      cl_int r;
      ctx= clCreateContext(NULL, 1, &id, NULL, NULL, &r);
      if (r >= 0)
      {
#ifdef OPENCL_LIB_200 // OpenCL 2.0+
         q= clCreateCommandQueueWithProperties(ctx, id, NULL, &r);
#else
         q= clCreateCommandQueue(ctx, id, 0, &r);
#endif
      }
      return(r >= 0);
   } // create

   bool release (void)
   {
      cl_int r= clReleaseCommandQueue(q);
      q= 0;
      r= clReleaseContext(ctx);
      ctx= 0;
      return(r >= 0);
   }
}; // CSimpleOCL

// Build a simple kernel
class CBuildOCL : public CSimpleOCL
{
public:
   cl_program idProg;
   cl_kernel  idKern;

   CBuildOCL (cl_device_id id=0) : CSimpleOCL(id) { ; }
   ~CBuildOCL () { release(true); }

   bool defaultBuild (const char src[], const char name[])
   {
      cl_int r;
      idProg= clCreateProgramWithSource(ctx, 1, &src, NULL, &r);
      //std::cout << "clCreateProgramWithSource() - r=%d" << r);
      if (r >= 0)
      {  // simple build for default device
         r= clBuildProgram(idProg, 0, NULL, NULL, NULL, NULL);
         //std::cout << "clBuildProgram() - r=%d" << r);
         if (r >= 0)
         {  // Create the compute kernel in the program we wish to run
            idKern= clCreateKernel(idProg, name, &r);
            //std::cout << "clCreateKernel() - r=%d" << r);
         }
      }
      return(r >= 0);
   } // defaultBuild

   bool release (bool all=true) // override
   {
      cl_int r= clReleaseProgram(idProg);
      idProg= 0;
      r= clReleaseKernel(idKern);
      idKern= 0;
      if (all) { return((r>=0) && CSimpleOCL::release()); }
      //else
      return(r >= 0);
   } // release
}; // CBuildOCL

#endif // SIMPLE_OCL_HPP

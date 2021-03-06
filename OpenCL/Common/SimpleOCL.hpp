// SimpleOCL.hpp - C++ classes for simple OpenCL usage.
// https://github.com/DrAl-HFS/Compute.git
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
//idDev{0},
   CSimpleOCL (cl_device_id id=0) : ctx{0},q{0} { if (0 != id) { create(id); } }

   ~CSimpleOCL () { release(); }

   bool create (cl_device_id id)
   {
      cl_int r;
      if (0 == ctx)
      {
         ctx= clCreateContext(NULL, 1, &id, NULL, NULL, &r);
         if (r >= 0)
         {
            //idDev= id;
#ifdef OPENCL_LIB_200 // OpenCL 2.0+ on NVidia / Ubuntu despite requesting V1.2 ...
            q= clCreateCommandQueueWithProperties(ctx, id, NULL, &r);
#else
            q= clCreateCommandQueue(ctx, id, 0, &r);
#endif
            return(r >= 0);
         }
      }
      return(false);
   } // create

   cl_device_id getDevice (void)
   {
      cl_int r;
      if (0 != ctx)
      {
         cl_device_id id[1];
         size_t bytes= sizeof(id[0]);
         r= clGetContextInfo(ctx, CL_CONTEXT_DEVICES, bytes, id, &bytes);
         if ((r >= 0) && (sizeof(id[0]) == bytes)) { return(id[0]); }
      }
      return(0);
   } // getDevice

   bool release (void)
   {
      cl_int r=0;
      if (0 != q)
      {
         r= clReleaseCommandQueue(q);
         //std::cout << "clReleaseCommandQueue() - r=" << r << std::endl;
         q= 0;
      }
      if (0 != ctx)
      {
         r= clReleaseContext(ctx); // mysterious segmentation fault here, depends where class declared (?)
         //std::cout << "clReleaseContext() - r=" << r << std::endl;
         ctx= 0;
      }
      //idDev= 0;
      return(r >= 0);
   }
}; // CSimpleOCL

// Build a simple kernel
class CBuildOCL : public CSimpleOCL
{
public:
   cl_program idProg;
   cl_kernel  idKern;

   CBuildOCL (cl_device_id id=0) : idProg{0},idKern{0},CSimpleOCL(id) { ; }
   ~CBuildOCL () { release(true); }

   // Wrapper (overload) for single source
   bool defaultBuild (const char src[], const char entryPoint[]) { return defaultBuild(&src, 1, entryPoint); }

   bool defaultBuild (const char *srcTab[], const int nSrc, const char entryPoint[])
   {
      cl_int r;
      idProg= clCreateProgramWithSource(ctx, nSrc, srcTab, NULL, &r);
      //std::cout << "clCreateProgramWithSource() - r=%d" << r);
      if (r >= 0)
      {  // simple build for default device
         r= clBuildProgram(idProg, 0, NULL, NULL, NULL, NULL);
         //std::cout << "clBuildProgram() - r=%d" << r);
         if (r >= 0)
         {  // Create the compute kernel in the program we wish to run
            idKern= clCreateKernel(idProg, entryPoint, &r);
            //std::cout << "clCreateKernel() - r=%d" << r);
         }
      }
      return(r >= 0);
   } // defaultBuild

   size_t getBuildLog (char log[], size_t max)
   {
      size_t n=0;
      cl_device_id id= getDevice();
      if (0 != id)
      {
         cl_int r= clGetProgramBuildInfo(idProg, id, CL_PROGRAM_BUILD_LOG, max, log, &n);
         //if (r >= 0) {
      }
      return(n);
   } // getBuildLog

   void reportBuildLog (void)
   {
      size_t maxLog= 1<<12; // 4k
      char *log= new char[maxLog];
      if (log && (getBuildLog(log, maxLog) > 1))
      {
         std::cout << "Build Log:" << std::endl;
         std::cout << log << std::endl;
      }
      delete [] log;
   } // reportBuildLog


   bool release (bool all=true) // override
   {
      cl_int r;

      if (0 != idKern)
      {//std::cout << "clReleaseKernel()" << std::endl;
         r= clReleaseKernel(idKern);
         idKern= 0;
      }
      if (0 != idProg)
      {//std::cout << "clReleaseProgram()" << std::endl;
         r= clReleaseProgram(idProg);
         idProg= 0;
      }
      if (all) { return((r>=0) && CSimpleOCL::release()); }
      //else
      return(r >= 0);
   } // release

}; // CBuildOCL

#endif // SIMPLE_OCL_HPP

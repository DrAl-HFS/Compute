// ocl2.cpp -
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#include <iostream>
#include <cmath>

#include "Common/Timing.hpp"
#include "Common/SimpleOCL.hpp"
#include "Common/QueryOCL.hpp"
#include "Common/Image.hpp"


/***/

#define MAX_PF_ID    2
#define MAX_DEV_ID   4


/***/


/***/

// OpenCL kernel source
const char imageSrc[]=
"__kernel void image(__global uint *pI, const ushort2 def)\n" \
"{ size_t iw= get_global_id(0); if (iw < def.x)" \
"   { size_t ih= get_global_id(1); if (ih < def.y)" \
"      {   size_t j= ih * def.x + iw;" // Compute 1D index using row stride <def.x>
"          pI[j]= j; } } }";


struct HostArgs : public CMapImage2D
{
public:
   size_t n;

   HostArgs (void) : CMapImage2D() { ; }

   size_t allocate (size_t w, size_t h)
   {
      n= CMapImage2D::allocate(w,h);
      return(n * sizeof(*pI));
   } // allocate

   bool release (void) { n=0; return CMapImage2D::release(); }

   size_t nwg (size_t n, size_t l) const { return((n + l - 1) / l); }

   // set work groups for local and global sizes on each axis
   void setGWS (size_t gws[], const size_t l[]) const
   {
      for (int i=0; i<2; i++)
      {
         gws[i]= l[i] * nwg(def.s[i], l[i]);
      }
   }
}; // CMapImage

struct DeviceArgs
{
   cl_mem hI; // device buffer "handle" identifiers
   size_t bytes;  // size of each buffer (on both host and device)

   DeviceArgs (void) : hI{0}, bytes{0} { ; }

   bool allocate (size_t buffBytes, cl_context ctx)
   {
      cl_int r;
      if (buffBytes > 0)
      {
         hI= clCreateBuffer(ctx, CL_MEM_WRITE_ONLY|CL_MEM_HOST_READ_ONLY, buffBytes, NULL, &r);
         if (r >= 0)
         {
            bytes= buffBytes;
            return(true);
         }
      }
      return(false);
   } // allocate

   bool release (void)
   {
      cl_int r;
      r= clReleaseMemObject(hI);
      hI= 0;
      std::cout << "DeviceArgs::release() - r=" << r << std::endl;
      return(r >= 0);
   } // release

}; // DeviceArgs

class CImageOCL : public CBuildOCL, public CElapsedTime
{
protected:
   // return number of work groups for local & problem size
   size_t nwg (size_t l, size_t n) { return((n + l - 1) / l); }

   HostArgs    host;
   DeviceArgs  device;

public:
   bool createArgs (size_t w, size_t h)
   {
      return device.allocate( host.allocate(w,h), CSimpleOCL::ctx );
   } // createArgs

   CImageOCL () { ; }
   ~CImageOCL () { release(); }

   //defaultBuild

   bool execute (size_t lws[2], TimeValF *pDT=NULL)
   {
      size_t gws[2];
      cl_event evt[2];
      cl_int r, wr[2], ar[2];

      host.setGWS(gws, lws);
      //std::cout << "lws: " << lws[0] << ", " << lws[1] << std::endl;
      //std::cout << "gws: " << gws[0] << ", " << gws[1] << std::endl;

      // Set args on device
      ar[0]= clSetKernelArg(CBuildOCL::idKern, 0, sizeof(device.hI), &(device.hI));
      ar[1]= clSetKernelArg(CBuildOCL::idKern, 1, sizeof(host.def), &(host.def));
      if (pDT) { pDT[0]= elapsed(); }
      //std::cout << "ar: " << ar[0] << ", " << ar[1] << std::endl;

      try
      {  // Submit kernel job
         r= clEnqueueNDRangeKernel(CSimpleOCL::q, CBuildOCL::idKern, 2, NULL, gws, lws, 0, NULL, evt+0);

         if (r >= 0)
         {
            std::cout << "kernel enqueued" << std::endl;
            r= clFinish(CSimpleOCL::q); // Global sync

            if (pDT) { pDT[2]= elapsed(); }
            std::cout << "kernel completion= " << r << std::endl;

            // Read the results (sync.) from the device
            r= clEnqueueReadBuffer(CSimpleOCL::q, device.hI, CL_BLOCKING, 0, device.bytes, host.pI, 0, NULL, evt+1);
            if (pDT) { pDT[3]= elapsed(); }
            //std::cout << "buffer read complete" << std::endl;
         }
         else { std::cout << "enqueue r=" << r << std::endl; }
      } catch (const std::exception& e) { std::cout << "EXCEPT: " << e.what() << std::endl; }

      return(r >= 0);
   } // execute

   bool release (bool all=true)
   {
      bool r= host.release() && device.release();
      if (all) { r&= CBuildOCL::release(all); }
      return(r);
   }

   int verify (void)
   {
      int r= -1;
      size_t n= host.numElem();
      if (host.pI && n)
      {
         uint32_t v=0;
         r= 0;
         for (int i=0; i < n; i++) { r+= (v == host.pI[i]); v+= 1; }
      }
      return(r);
   } // verify

   size_t save (const char fileName[]) { return host.save(fileName); }
}; // CImageOCL


/***/

CImageOCL img; // global to avoid segment violation

int main (int argc, char *argv[])
{
   cl_platform_id idPfm[MAX_PF_ID]={0,};
   cl_device_id   idDev[MAX_DEV_ID]={0,};
   cl_uint        nDev= queryDevPfm(idDev, MAX_DEV_ID, idPfm, MAX_PF_ID);
   int r=-1;

   if (nDev > 0)
   {
      //CImageOCL img; // destruction causes segment violation inside clReleaseContext()
      TimeValF t[4];
      size_t lws[2]={32,32};

      if (img.create(idDev[0]) && img.createArgs(256,256))
      {
         t[0]= img.elapsed();
         std::cout << "context created: " << t[0] << "sec" << std::endl;
         if (img.defaultBuild(imageSrc,"image"))
         {
            t[1]= img.elapsed();
            std::cout << "build OK: " << t[1] << "sec" << std::endl;

            if (img.execute(lws, t+1))
            {
               r= img.verify();
               std::cout << "execution: r=" << r << std::endl;
               /*std::cout << "\targs:       " << t[3] << "sec"  << std::endl;
               std::cout << "\tbuffers-in: " << t[4] << "sec"  << std::endl;
               std::cout << "\tkernel:     " << t[5] << "sec"  << std::endl;
               std::cout << "\tbuffer-out: " << t[6] << "sec"  << std::endl;
               Scalar s= va.sumR();
               Scalar e= va.getN();
               Scalar re= 2 * fabs(e-s) / (e + s);
               std::cout << "result: sum=" << s << " expected=" << e << std::endl;
               std::cout << "relative error=" << re << std::endl;
               if (re <= 1E-6) { r= 0; }

               std::cout << "unaccelerated host: " << va.hostTest() << "sec"  << std::endl;*/
               img.save("img.raw"); // convert -size 256x256 -depth 32 img.raw img.rgb
            }
         }
         else { img.reportBuildLog(); }
      }
      //std::cout << "Destructing..." << std::endl;
   }
   //std::cout << "Exiting..." << std::endl;
   return(r);
} // main

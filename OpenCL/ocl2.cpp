// ocl2.cpp -
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#include <iostream>
#include <cmath>

#include "Common/Timing.hpp"
#include "Common/SimpleOCL.hpp"
#include "Common/QueryOCL.hpp"


/***/

#define MAX_PF_ID    2
#define MAX_DEV_ID   4

typedef uint32_t Index;
typedef uint32_t Defn;


/***/


/***/

// OpenCL kernel source
const char imageSrc[]=
"__kernel void image(__global uint *pI, const uint w, const uint h)\n" \
"{ size_t iw= get_global_id(0); if (iw < w)" \
"   { size_t ih= get_global_id(1); if (ih < h)" \
"      {   size_t j= ih * w + iw;" // Compute 1D index using row stride <w>
"          pI[j]= j; } } }";

struct HostArgs
{
   Index *pI;   // host memory buffers
   Defn  w, h; // elements in each buffer (on both host and device)

   HostArgs (void) : pI{NULL}, w{0}, h{0} { ; }

   size_t allocate (size_t nW, size_t nH)
   {
      if (NULL == pI)
      {
         size_t n= nW * nH;
         pI= new Index[n];
         if (pI) { w= nW; h= nH; return(n); }
      }
      //else
      return(0);
   } // allocate

   bool release (void)
   {  // std::cout << "HostArgs::release()" << std::endl;
      if (pI)
      {
         delete [] pI;
         pI= NULL;
         w= h= 0;
      }
      return(true);
   }// release

}; // HostArgs

struct DeviceArgs
{
   cl_mem hI; // device buffer "handle" identifiers
   size_t bytes;  // size of each buffer (on both host and device)

   DeviceArgs (void) : hI{0}, bytes{0} { ; }
   size_t allocate (size_t nElem, cl_context ctx)
   {
      cl_int r;
      bytes= sizeof(Index) * nElem;
      hI= clCreateBuffer(ctx, CL_MEM_WRITE_ONLY|CL_MEM_HOST_READ_ONLY, bytes, NULL, &r);
      if (r >= 0) { return(bytes); }
      //else
      return(0);
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
      if ((w > 0) && (h > 0))
      {
         size_t nElem= host.allocate(w,h);
         //std::cout << "createArgs(" << w << ", " << h << ") -\n\tnElem=" << nElem << std::endl;
         if (nElem > 0)
         {
            size_t bytes= device.allocate(nElem, CSimpleOCL::ctx);
            //std::cout << "\tbytes=" << bytes << std::endl;
            return(bytes > 0);
         }
      }
      //else
      return(false);
   } // createArgs

   CImageOCL () { ; }
   ~CImageOCL () { release(); }

   //defaultBuild

   bool execute (size_t lws[2], TimeValF *pDT=NULL)
   {
      size_t gws[2];
      cl_event evt[2];
      cl_int r, wr[2], ar[4];

      gws[0]= lws[0] * nwg(lws[0], host.w);
      gws[1]= lws[1] * nwg(lws[1], host.h);
      //std::cout << "lws: " << lws[0] << ", " << lws[1] << std::endl;
      //std::cout << "gws: " << gws[0] << ", " << gws[1] << std::endl;

      // Set args on device
      ar[0]= clSetKernelArg(CBuildOCL::idKern, 0, sizeof(device.hI), &(device.hI));
      ar[1]= clSetKernelArg(CBuildOCL::idKern, 1, sizeof(host.w), &(host.w));
      ar[2]= clSetKernelArg(CBuildOCL::idKern, 2, sizeof(host.h), &(host.h));
      if (pDT) { pDT[0]= elapsed(); }
      //std::cout << "ar: " << ar[0] << ", " << ar[1] << ", " << ar[2] << std::endl;

      // Submit kernel job
      r= clEnqueueNDRangeKernel(CSimpleOCL::q, CBuildOCL::idKern, 2, NULL, gws, lws, 0, NULL, evt+0);

      if (r >= 0)
      {
         //std::cout << "kernel enqueued" << std::endl;
         clFinish(CSimpleOCL::q); // Global sync
         if (pDT) { pDT[2]= elapsed(); }
         //std::cout << "kernel complete" << std::endl;

         // Read the results (sync.) from the device
         r= clEnqueueReadBuffer(CSimpleOCL::q, device.hI, CL_BLOCKING, 0, device.bytes, host.pI, 0, NULL, evt+1);
         if (pDT) { pDT[3]= elapsed(); }
         //std::cout << "buffer read complete" << std::endl;
      }
      else { std::cout << "enqueue r=" << r << std::endl; }
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
      size_t n= host.w * host.h;
      if (host.pI && n)
      {
         uint32_t v=0;
         r= 0;
         for (int i=0; i < n; i++) { r+= (v == host.pI[i]); v+= 1; }
      }
      return(r);
   } // verify

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
            }
         }
         else { img.reportBuildLog(); }
      }
      //std::cout << "Destructing..." << std::endl;
   }
   //std::cout << "Exiting..." << std::endl;
   return(r);
} // main
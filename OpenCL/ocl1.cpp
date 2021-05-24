// ocl1.cpp - Minimal OpenCL device test, migrated to C++.
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors Apr - May 2021

#include <iostream>
#include <cmath>

#include "Common/Timing.hpp"
#include "Common/SimpleOCL.hpp"
#include "Common/QueryOCL.hpp"


/***/

#define MAX_PF_ID    2
#define MAX_DEV_ID   4

typedef float Scalar;


/***/

// Vector addition, unaccelerated host routine
void vecAdd (Scalar r[], const Scalar a[], const Scalar b[], const size_t n)
{
   for (size_t i= 0; i<n; i++) { r[i]= a[i] + b[i]; }
} // vecAdd


/***/

// OpenCL kernel source
const char vecAddSrc[]=
"kernel void vecAdd(__global float *pR, __global float *pA, __global float *pB, const size_t n)\n" \
"{ int id= get_global_id(0); if (id < n) { pR[id]= pA[id] + pB[id]; } }";

struct HostArgs
{
   Scalar *pR, *pA, *pB;   // host memory buffers
   size_t n; // elements in each buffer (on both host and device)

   HostArgs (void) : pR{NULL}, pA{NULL}, pB{NULL}, n{0} { ; }

   size_t allocate (size_t nElem)
   {
      if ((nElem > 0) && (NULL == pR) && (NULL == pA) && (NULL == pB))
      {
         pR= new Scalar[nElem]; pA= new Scalar[nElem]; pB= new Scalar[nElem];
         if (pR && pA && pB)
         {
            n= nElem;
            return(n * sizeof(Scalar));
         }
      }
      //else
      return(0);
   } // allocate

   bool release (void)
   {
      if (pR) { delete [] pR; }
      if (pA) { delete [] pA; }
      if (pB) { delete [] pB; }
      pR= pA= pB= NULL;
      return(true);
   } // release

   // return number of work groups for local size l and global size n
   size_t nwg (size_t l) const { return((n + l - 1) / l); }
}; // HostArgs

struct DeviceArgs
{
   cl_mem hR, hA, hB; // device buffer "handle" identifiers
   size_t bytes;  // size of each buffer (on both host and device)

   DeviceArgs (void) : hR{0}, hA{0}, hB{0}, bytes{0} { ; }
   bool allocate (size_t buffBytes, cl_context ctx)
   {
      cl_int r[3];
      if ((buffBytes > 0) && (0 == hR) && (0 == hA) && (0 == hB))
      {
         hR= clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, buffBytes, NULL, r+0);
         hA= clCreateBuffer(ctx, CL_MEM_READ_ONLY, buffBytes, NULL, r+1);
         hB= clCreateBuffer(ctx, CL_MEM_READ_ONLY, buffBytes, NULL, r+2);
         if ((r[0] >= 0) && (r[1] >= 0) && (r[2] >= 0))
         {
            bytes= buffBytes;
            return(true);
         }
      }
      return(false);
   } // allocate

   bool release (void)
   {
      cl_int r[3];
      r[0]= clReleaseMemObject(hR);
      r[1]= clReleaseMemObject(hA);
      r[2]= clReleaseMemObject(hB);
      hR= hA= hB= 0;
      return((r[0] >= 0) && (r[1] >= 0) && (r[2] >= 0));
   } // release

}; // DeviceArgs

void initData (const HostArgs& h)
{
   for (size_t i= 0; i < h.n; i++)
   {
      double theta= i * M_PI / (1<<8);
      h.pA[i] = sinf(theta)*sinf(theta);
      h.pB[i] = cos(theta)*cos(theta);
      h.pR[i]= -1;
   }
} // initData

Scalar sum (const Scalar v[], const size_t n)
{
   Scalar s= v[0]; // hacky assumption n>=1
   for (size_t i=1; i<n; i++) { s+= v[i]; }
   return(s);
} // sum

class CVecAddOCL : public CBuildOCL, public CElapsedTime
{
protected:
   //size_t nwg (size_t l, size_t n) { return((n + l - 1) / l); }

   HostArgs    host;
   DeviceArgs  device;

public:
   bool createArgs (size_t nElem)
   {
      if (nElem > 0) { return device.allocate( host.allocate(nElem), CSimpleOCL::ctx ); }
      //else
      return(false);
   }

   CVecAddOCL (size_t nElem=0) { createArgs(nElem); }
   ~CVecAddOCL () { release(); }

   //defaultBuild

   bool execute (size_t lws, TimeValF *pDT=NULL)
   {
      size_t gws= lws * host.nwg(lws);
      cl_event evt[4];
      cl_int r, wr[2], ar[4];

      //std::cout << "%zub, ws: %zu %zu" << bytes, lws, gws);

      // Set args on device
      ar[0]= clSetKernelArg(CBuildOCL::idKern, 0, sizeof(device.hR), &(device.hR));
      ar[1]= clSetKernelArg(CBuildOCL::idKern, 1, sizeof(device.hA), &(device.hA));
      ar[2]= clSetKernelArg(CBuildOCL::idKern, 2, sizeof(device.hB), &(device.hB));
      ar[3]= clSetKernelArg(CBuildOCL::idKern, 3, sizeof(host.n), &(host.n));
      if (pDT) { pDT[0]= elapsed(); }
      //std::cout << "ar: %d %d %d %d" << ar[0], ar[1], ar[2], ar[3]);

      // Copy (sync.) input buffers
      wr[0]= clEnqueueWriteBuffer(CSimpleOCL::q, device.hA, CL_BLOCKING, 0, device.bytes, host.pA, 0, NULL, evt+0);
      wr[1]= clEnqueueWriteBuffer(CSimpleOCL::q, device.hB, CL_BLOCKING, 0, device.bytes, host.pB, 0, NULL, evt+1);
      if (pDT) { pDT[1]= elapsed(); }
      //std::cout << "wr: %d %d" << wr[0], wr[1]);

      // Submit kernel job
      r= clEnqueueNDRangeKernel(CSimpleOCL::q, CBuildOCL::idKern, 1, NULL, &gws, &lws, 0, NULL, evt+2);

      if (r >= 0)
      {
         std::cout << "kernel enqueued" << std::endl;
         clFinish(CSimpleOCL::q); // Global sync
         if (pDT) { pDT[2]= elapsed(); }

         // Read the results (sync.) from the device
         r= clEnqueueReadBuffer(CSimpleOCL::q, device.hR, CL_BLOCKING, 0, device.bytes, host.pR, 0, NULL, evt+3);
         if (pDT) { pDT[3]= elapsed(); }
      }
      else { std::cout << "enqueue r=" << r << std::endl; }
      return(r >= 0);
   } // execute

   void initHostData (void) { initData(host); }

   Scalar sumR (void) { return sum(host.pR, host.n); }

   size_t getN (void) { return(host.n); }

   bool release (bool all=true)
   {
      bool r= host.release() && device.release();
      if (all) { r&= CBuildOCL::release(all); }
      return(r);
   }

   TimeValF hostTest (void)
   {
      elapsed();
      vecAdd(host.pR, host.pA, host.pB, host.n);
      return elapsed();
   } // hostTest
}; // CVecAddOCL


/***/

int main (int argc, char *argv[])
{
   cl_platform_id idPfm[MAX_PF_ID]={0,};
   cl_device_id   idDev[MAX_DEV_ID]={0,};
   cl_uint        nDev= queryDevPfm(idDev, MAX_DEV_ID, idPfm, MAX_PF_ID);
   int r=-1;

   if (nDev > 0)
   {
      CVecAddOCL va;
      TimeValF t[7];

      if (va.create(idDev[0]) && va.createArgs(1<<20))
      {
         t[0]= va.elapsed();
         std::cout << "context created: " << t[0] << "sec" << std::endl;
         if (va.defaultBuild(vecAddSrc,"vecAdd"))
         {
            t[1]= va.elapsed();
            std::cout << "build OK: " << t[1] << "sec" << std::endl;

            va.initHostData();

            t[2]= va.elapsed();
            std::cout << "Data init: " << t[2] << "sec" << std::endl;

            if (va.execute(32, t+3))
            {
               std::cout << "execution:" << std::endl;
               std::cout << "\targs:       " << t[3] << "sec"  << std::endl;
               std::cout << "\tbuffers-in: " << t[4] << "sec"  << std::endl;
               std::cout << "\tkernel:     " << t[5] << "sec"  << std::endl;
               std::cout << "\tbuffer-out: " << t[6] << "sec"  << std::endl;
               Scalar s= va.sumR();
               Scalar e= va.getN();
               Scalar re= 2 * fabs(e-s) / (e + s);
               std::cout << "result: sum=" << s << " expected=" << e << std::endl;
               std::cout << "relative error=" << re << std::endl;
               if (re <= 1E-6) { r= 0; }

               std::cout << "unaccelerated host: " << va.hostTest() << "sec"  << std::endl;
            }
         }
         else { va.reportBuildLog(); }
      }
      //std::cout << "Destructing..." << std::endl;
   }
   //std::cout << "Exiting..." << std::endl;
   return(r);
} // main

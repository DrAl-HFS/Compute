// ocl1.cpp - Minimal OpenCL device test, migrated to C++.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors Apr - May 2021

#include <iostream>
#include <cmath>

#include "SimpleOCL.hpp"
#include "QueryOCL.hpp"


/***/

typedef float Scalar;


/***/

// Finally, the actual application: vector addition

const char vecAddSrc[]=
"__kernel void vecAdd(  __global float *pR, __global float *pA, __global float *pB, const size_t n)\n" \
"{ int id= get_global_id(0); if (id < n) { pR[id]= pA[id] + pB[id]; } }";

struct HostArgs
{
   Scalar *pR, *pA, *pB;   // host memory buffers
   size_t n; // elements in each buffer (on both host and device)

   HostArgs (void) : pR{NULL}, pA{NULL}, pB{NULL}, n{0} { ; }

   bool allocate (size_t nElem)
   {
      if ((NULL == pR) && (NULL == pA) && (NULL == pB))
      {
         n= nElem;
         pR= new Scalar[n]; pA= new Scalar[n]; pB= new Scalar[n];
         return(pR && pA && pB);
      }
      //else
      return(false);
   }
   bool release (void)
   {
      delete [] pR; delete [] pA; delete [] pB;
      pR= pA= pB= NULL;
      return(true);
   }
}; // HostArgs

struct DeviceArgs
{
   cl_mem hR, hA, hB; // device buffer "handle" identifiers
   size_t bytes;  // size of each buffer (on both host and device)

   DeviceArgs (void) : hR{0}, hA{0}, hB{0}, bytes{0} { ; }
   bool allocate (size_t nElem, cl_context ctx)
   {
      cl_int r[3];
      bytes= sizeof(Scalar) * nElem;
      hR= clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, bytes, NULL, r+0);
      hA= clCreateBuffer(ctx, CL_MEM_READ_ONLY, bytes, NULL, r+1);
      hB= clCreateBuffer(ctx, CL_MEM_READ_ONLY, bytes, NULL, r+2);
      return((r[0] >= 0) && (r[1] >= 0) && (r[2] >= 0));
   }
   bool release (void)
   {
      cl_int r[3];
      r[0]= clReleaseMemObject(hR);
      r[1]= clReleaseMemObject(hA);
      r[2]= clReleaseMemObject(hB);
      hR= hA= hB= 0;
      return((r[0] >= 0) && (r[1] >= 0) && (r[2] >= 0));
   }
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

class CVecAddOCL : public CBuildOCL
{
protected:
   // return number of work groups for local & problem size
   size_t nwg (size_t l, size_t n) { return((n + l - 1) / l); }

   HostArgs    host;
   DeviceArgs  device;

public:
   bool createArgs (size_t nElem)
   {
      if (nElem > 0) { return( host.allocate(nElem) && device.allocate(nElem, CSimpleOCL::ctx) ); }
      //else
      return(false);
   }

   CVecAddOCL (size_t nElem=0) { createArgs(nElem); }
   ~CVecAddOCL () { release(); }

   //defaultBuild

   bool execute (size_t lws)
   {
      size_t gws= lws * nwg(lws, host.n);
      cl_event evt[4];
      cl_int r, wr[2], ar[4];

      //std::cout << "%zub, ws: %zu %zu" << bytes, lws, gws);

      // Set args on device
      ar[0]= clSetKernelArg(CBuildOCL::idKern, 0, sizeof(device.hR), &(device.hR));
      ar[1]= clSetKernelArg(CBuildOCL::idKern, 1, sizeof(device.hA), &(device.hA));
      ar[2]= clSetKernelArg(CBuildOCL::idKern, 2, sizeof(device.hB), &(device.hB));
      ar[3]= clSetKernelArg(CBuildOCL::idKern, 3, sizeof(host.n), &(host.n));
      //std::cout << "ar: %d %d %d %d" << ar[0], ar[1], ar[2], ar[3]);

      // Copy (sync.) input buffers
      wr[0]= clEnqueueWriteBuffer(CSimpleOCL::q, device.hA, CL_TRUE, 0, device.bytes, host.pA, 0, NULL, evt+0);
      wr[1]= clEnqueueWriteBuffer(CSimpleOCL::q, device.hB, CL_TRUE, 0, device.bytes, host.pB, 0, NULL, evt+1);
      //std::cout << "wr: %d %d" << wr[0], wr[1]);

      // Submit kernel job
      r= clEnqueueNDRangeKernel(CSimpleOCL::q, CBuildOCL::idKern, 1, NULL, &gws, &lws, 0, NULL, evt+2);

      if (r >= 0)
      {
         std::cout << "job enqueued" << std::endl;
         clFinish(CSimpleOCL::q); // Global sync

         // Read the results (sync.) from the device
         r= clEnqueueReadBuffer(CSimpleOCL::q, device.hR, CL_TRUE, 0, device.bytes, host.pR, 0, NULL, evt+3);
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
}; // CVecAddOCL




/***/

int main (int argc, char *argv[])
{
   cl_device_id   idDev[MAX_DEV_ID]={0,};
   cl_uint        nDev= queryDev(idDev, MAX_DEV_ID);
   int r=-1;

   //std::cout << sizeof(cl_device_info) << std::endl;
   //std::cout << sizeof(cl_platform_info) << std::endl;

   if (nDev > 0)
   {
      CVecAddOCL va;
      if (va.create(idDev[0]) && va.createArgs(1<<20))
      {
         std::cout << "context created" << std::endl;
         if (va.defaultBuild(vecAddSrc,"vecAdd"))
         {
            std::cout << "build OK" << std::endl;
            va.initHostData();

            if (va.execute(32))
            {
               Scalar s= va.sumR();
               Scalar e= va.getN();
               Scalar re= 2 * fabs(e-s) / (e + s);
               std::cout << "result: sum=" << s << " expected=" << e << std::endl;
               std::cout << "relative error=" << re << std::endl;
               if (re <= 1E-6) { r= 0; }
            }
         }
      }
   }
   return(r);
} // main

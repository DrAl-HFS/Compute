// MapImageOCL.hpp -
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#ifndef MAP_IMAGE_OCL_HPP
#define MAP_IMAGE_OCL_HPP

//#include <iostream>
//#include <cmath>

#include "Timing.hpp"
#include "SimpleOCL.hpp"
#include "MapImage.hpp"

typedef float Scalar;

// Abstract base class
class GeomArgs
{
public:
   //GeomArgs (void) { ; }

   virtual uint8_t nArgs (void) const = 0;

   virtual const Scalar *get (size_t& bytes, uint8_t i) const = 0;
}; // GeomArgs

class EmptyGeomArgs : public GeomArgs
{
public:
   EmptyGeomArgs (void) { ; }

   uint8_t nArgs (void) const override { return(0); }

   const Scalar *get (size_t& bytes, uint8_t i) const override { bytes=0; return(NULL); }
}; // EmptyGeomArgs

struct KernInfo
{
   const char *src;
   const char *entryPoint;
   const GeomArgs *pA;

   KernInfo (const char *s, const GeomArgs *p=NULL, const char *e="image") { src= s; entryPoint= e; pA= p; }
}; // KernInfo


/***/

struct HostArgs : public CMapImage2D
{
public:
   HostArgs (void) : CMapImage2D() { ; }

   size_t allocate (size_t w, size_t h)
   {
      return(CMapImage2D::allocate(w,h) * sizeof(*pI));
   } // allocate

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

class CMapImageOCL : public CBuildOCL, public CElapsedTime
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

   CMapImageOCL () { ; }
   ~CMapImageOCL () { release(); }

   //defaultBuild

   bool execute (size_t lws[2], const GeomArgs& ga, TimeValF *pDT=NULL)
   {
      size_t gws[2];
      cl_event evt[2];
      cl_int r, wr[2], ar[4];

      host.setGWS(gws, lws);
      //std::cout << "lws: " << lws[0] << ", " << lws[1] << std::endl;
      //std::cout << "gws: " << gws[0] << ", " << gws[1] << std::endl;

      // Set args on device
      ar[0]= clSetKernelArg(CBuildOCL::idKern, 0, sizeof(device.hI), &(device.hI));
      ar[1]= clSetKernelArg(CBuildOCL::idKern, 1, sizeof(host.def), &(host.def));
      const uint8_t n= ga.nArgs();
      if (n > 0)
      {
         size_t b=0;
         const Scalar *p= ga.get(b,0);
         ar[2]= clSetKernelArg(CBuildOCL::idKern, 2, b, p);
         if (n > 1)
         {
            p= ga.get(b,1);
            ar[3]= clSetKernelArg(CBuildOCL::idKern, 3, b, p);
         }
      }
      if (pDT) { pDT[0]= elapsed(); }
      //std::cout << "ar: " << ar[0] << ", " << ar[1] << std::endl;

      try
      {  // Submit kernel job
         r= clEnqueueNDRangeKernel(CSimpleOCL::q, CBuildOCL::idKern, 2, NULL, gws, lws, 0, NULL, evt+0);

         if (r >= 0)
         {
            std::cout << "kernel enqueued" << std::endl;
            r= clFinish(CSimpleOCL::q); // Global sync

            if (pDT) { pDT[1]= elapsed(); }
            std::cout << "kernel completion= " << r << std::endl;

            // Read the results (sync.) from the device
            r= clEnqueueReadBuffer(CSimpleOCL::q, device.hI, CL_BLOCKING, 0, device.bytes, host.pI, 0, NULL, evt+1);
            if (pDT) { pDT[2]= elapsed(); }
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

   friend int verify (const CMapImageOCL&);

   size_t save (const char fileName[]) { return host.save(fileName); }
}; // CMapImageOCL

#endif // MAP_IMAGE_OCL_HPP

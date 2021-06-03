// ocl2.cpp - Synthesise a map image.
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#include <iostream>

#include "Common/SimpleOCL.hpp" // still needed - include hierarchy issue?
#include "Common/QueryOCL.hpp"
#include "Common/MapImageOCL.hpp"


/***/

// Query parameters
#define MAX_PF_ID    2
#define MAX_DEV_ID   4


/* OpenCL kernel sources */

// Generate a simple map of element indices - easily verified
const char idxKernSrc[]=
"kernel void image (__global int *pI, const ushort2 def)\n" \
"{ size_t x= get_global_id(0); if (x < def.x)" \
"   { size_t y= get_global_id(1); if (y < def.y)" \
"      {   size_t i= y * def.x + x;" // Compute 1D index using row stride <def.x>
"          pI[i]= i; } } }";

// Generate distance map of a circle - visually verifiable
const char dmapKernSrc[]=
"kernel void image (__global int *pI, const ushort2 def, const float2 c, const float r)\n" \
"{ ushort2 u;"\
"  float2 f;"\
"  u.x= get_global_id(0);"\
"  u.y= get_global_id(1);"\
"  if ((u.x < def.x) && (u.y < def.y)) {" \
"    f.x= u.x; f.y= u.y; "\
"    int s= distance(f,c) - r;"\
"    pI[(size_t)u.y * def.x + u.x]= s; } }";

struct Coord2D
{
   union { struct { Scalar x,y; }; Scalar s[2]; }; // anon

   Coord2D (Scalar kx=0, Scalar ky=0) { x= kx; y= ky; }
}; // Coord2D

class DMapGeomArgs : public GeomArgs
{
public:
   Scalar v[3];
   //uint8_t

   DMapGeomArgs (const Coord2D& c, Scalar r)
   {
      v[0]= c.x; v[1]= c.y; v[2]= r;
   }

   uint8_t nArgs (void) const override { return(2); }

   const Scalar *get (size_t& bytes, uint8_t i, Scalar *pR=NULL, const Def2D *pD=NULL) const override
   {
      switch(i)
      {
         case 0 :    bytes= 2 * sizeof(v[0]); return(v+0); // break;
         case 1 :    bytes= sizeof(v[2]); return(v+2); // break;
         default :   bytes= 0; return(NULL);
      }
   }
}; // DMapGeomArgs

int verify (const CMapImageOCL& m)
{
   int r= -1;
   size_t n= m.host.numElem();
   if (m.host.pI && n)
   {
      uint32_t v=0;
      r= 0;
      for (int i=0; i < n; i++) { r+= (v == m.host.pI[i]); v+= 1; }
   }
   return(r);
} // verify

/***/
Def2D gDef={512,512};

EmptyGeomArgs idxGA;
KernInfo idxKI(idxKernSrc, &idxGA);

DMapGeomArgs dmapGA(Coord2D(0.5*gDef.x,0.5*gDef.y),0.125*(gDef.x+gDef.y));
KernInfo dmapKI(dmapKernSrc, &dmapGA);

CMapImageOCL img; // global to avoid segment violation

int main (int argc, char *argv[])
{
   cl_platform_id idPfm[MAX_PF_ID]={0,};
   cl_device_id   idDev[MAX_DEV_ID]={0,};
   cl_uint        nDev= queryDevPfm(idDev, MAX_DEV_ID, idPfm, MAX_PF_ID);
   int r=-1;

   if (nDev > 0)
   {
      //CImageOCL img; // destruction causes segment violation inside clReleaseContext()
      TimeValF t[5];
      size_t lws[2]={32,32};

      if (img.create(idDev[0]) && img.createArgs(gDef.x,gDef.y))
      {
         KernInfo *pKI= &dmapKI;
         t[0]= img.elapsed();
         std::cout << "context created: " << t[0] << "sec" << std::endl;
         if (img.defaultBuild(pKI->src,pKI->entryPoint))//
         {
            t[1]= img.elapsed();
            std::cout << "build OK: " << t[1] << "sec" << std::endl;

            if (img.execute(lws, *(pKI->pA), t+2))
            {
               if (0 == pKI->pA->nArgs()) { r= verify(img); } else { r= 0; }
               std::cout << "execution: r=" << r << std::endl;
               std::cout << "\targs:       " << t[2] << "sec"  << std::endl;
               std::cout << "\tkernel:     " << t[3] << "sec"  << std::endl;
               std::cout << "\tbuffer-out: " << t[4] << "sec"  << std::endl;
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

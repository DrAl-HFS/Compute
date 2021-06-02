// ocl3.cpp - Mandelbrot set.
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#include <iostream>
//#include <cmath>

#include "Common/SimpleOCL.hpp"
#include "Common/QueryOCL.hpp"
#include "Common/MapImageOCL.hpp"

/***/

// Query parameters
#define MAX_PF_ID    2
#define MAX_DEV_ID   4

//#include "mandKern.hpp"
const char mandelKernSrc[]=
"float cm2 (const float2 *pV) { return(pV->x * pV->x - pV->y * pV->y); }\n\n" \
"void csq1 (float2 *pV) { float ty= 2 * pV->x * pV->y; pV->x= cm2(pV); pV->y= ty; }\n\n" \
"float cad1m2 (float2 *pR, const float2 *pV) { pR->x+= pV->x; pR->y+= pV->y; return cm2(pR); }\n\n" \
"float csqad1m2 (float2 *pV, const float2 *pC) { csq1(pV); return cad1m2(pV, pC); }\n" \
"\n" \
"int mandel (const float2 *pC, int maxI, float maxM2)\n" \
"{ int i=1; float m2; float2 x= *pC;\n" \
"  do { m2= csqad1m2(&x, pC); } while ((++i < maxI) && (m2 < maxM2));\n" \
"  return(i); }\n" \
"\n" \
"kernel void image (__global int *pI, const ushort2 def, const float2 c0, const float2 dc)\n" \
"{ ushort2 u; float2 c;\n"\
"  u.x= get_global_id(0); u.y= get_global_id(1);\n" \
"  if ((u.x < def.x) && (u.y < def.y)) {\n" \
"    c.x= c0.x + dc.x * u.x;\n" \
"    c.y= c0.y + dc.y * u.y;\n" \
"    pI[(size_t)u.y * def.x + u.x]= mandel(&c,255,1000); } }\n";
//127;\n\n } }\n"; //
struct Complex2D
{
   union { struct { Scalar r,i; }; Scalar s[2]; }; // anon

   Complex2D (Scalar kr=0, Scalar ki=0) { r= kr; i= ki; }
}; // Complex2D

class MandelGeomArgs : public GeomArgs
{
public:
   Scalar v[4];

   MandelGeomArgs (const Complex2D& c, const Complex2D& sr, const Def2D& def) // complex plane origin+resolution specified using centre, semi-radii and pixel definition
   {
      v[0]= c.r-sr.r; v[1]= c.i-sr.i; // convert to lower bound
      v[2]= 2 * sr.r; v[3]= 2 * sr.i; // convert to width (semi-diameters)
      if (def.x > 0) { v[2]/= def.x; } // convert to resolution
      if (def.y > 0) { v[3]/= def.y; } // convert to resolution
   } // MandelGeomArgs

   uint8_t nArgs (void) const override { return(2); }

   const Scalar *get (size_t& bytes, uint8_t i) const override
   {
      switch(i)
      {
         case 0 :
         case 1 :
            bytes= 2 * sizeof(v[0]); return(v+2*i); // break;
         default :   bytes= 0; return(NULL);
      }
   } // get
}; // MandelGeomArgs


int verify (const CMapImageOCL& m) { return(0); }

CMapImageOCL img; // global to avoid segment violation
Def2D gDef={256,256};
//const ExtArgs dmapEA(Coord2D(128,128));
const MandelGeomArgs mandelGA(Complex2D(-0.909, -0.275), Complex2D(0.005,0.005), gDef);
//const MandelGeomArgs mandelGA(Complex2D(), Complex2D(2.5,2.5));


//const KernInfo idx(idxImgSrc);
//const KernInfo dmap(dmapImgSrc, &dmapEA);
const KernInfo mandel(mandelKernSrc, &mandelGA);

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
         const KernInfo *pK= &mandel;

         t[0]= img.elapsed();
         std::cout << "context created: " << t[0] << "sec" << std::endl;

         if (img.defaultBuild(pK->src,pK->entryPoint))
         {
            t[1]= img.elapsed();
            std::cout << "build OK: " << t[1] << "sec" << std::endl;

            if (img.execute(lws, *(pK->pA), t+2))
            {
               r= verify(img);
               std::cout << "execution: r=" << r << std::endl;
               std::cout << "\targs:       " << t[2] << "sec"  << std::endl;
               std::cout << "\tkernel:     " << t[3] << "sec"  << std::endl;
               std::cout << "\tbuffer-out: " << t[4] << "sec"  << std::endl;
               img.save("img.raw");
            }
         }
         else { img.reportBuildLog(); std::cout << pK->src; }
      }
      //std::cout << "Destructing..." << std::endl;
   }
   //std::cout << "Exiting..." << std::endl;
   return(r);
} // main
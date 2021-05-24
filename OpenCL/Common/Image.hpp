// Table.hpp - C++ classes for image/map handling.
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#ifndef MAP_IMAGE_HPP
#define MAP_IMAGE_HPP

#include <fstream>
//#include <iostream>

#ifdef DEBUG
#include <cstdlib>
#endif

typedef cl_ushort Def1D;
typedef cl_ushort2 Def2D; // union members={ s[2]; (x,y); (s0,s1); (lo,hi); }
typedef cl_uint MapElement;

class CMapImage2D
{
public:
   MapElement *pI;
   Def2D       def;

   CMapImage2D (void) : pI{NULL}, def{0,0} { ; }

   size_t numElem (void) const { return((size_t)def.s0 * def.s1); }

   size_t allocate (Def1D w, Def1D h)
   {
      if (NULL == pI)
      {
         size_t n= (size_t)w * h;
         pI= new MapElement[n];
         if (pI) { def.x= w; def.y= h; return(n); }
      }
      return(0);
   } // allocate

   bool release (void)
   {  // std::cout << "CMapImage::release()" << std::endl;
      if (pI)
      {
         delete [] pI;
         pI= NULL;
         def.x= def.y= 0;
      }
      return(true);
   }// release

   size_t save (const char fileName[], uint8_t outByteDepth=1) const
   {
      size_t bytes= 0;
      const size_t n= numElem();
      if (n > 0)
      {
         auto outFile= std::fstream(fileName, std::ios::out | std::ios::binary);
         if (1 == outByteDepth)
         {  // convert -size 256x256 -depth 8 gray:img.raw img.png
            uint8_t hb[256];
            uint32_t o=0;
            for (int l=0; l<def.y; l++)
            {
               for (int i=0; i<def.x; i++) { hb[i]= pI[o+i]; }
               outFile.write((const char *)hb, sizeof(hb));
               bytes+= sizeof(hb);
               o+= def.x;
            }
         }
         else
         {
            bytes= n * sizeof(*pI);
            outFile.write((const char *)pI, bytes);
         }
         //if error bytes= 0;?
         outFile.close();
      }
      return(bytes);
   }
}; // CMapImage2D

#endif // MAP_IMAGE_HPP

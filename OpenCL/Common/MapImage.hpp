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
typedef cl_int MapElement;

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

   void i2u8Hack (uint8_t u[], const int lineI[], const int nI) const
   {
      for (int i=0; i<nI; i++)
      {
         int v= lineI[i];
         if (v < 0) { u[i]= -v; } else { u[i]= v; }
      }
   } // i2u8Hack

   void i2rgbHack (uint8_t rgb[], const int lineI[], const int nI) const
   {
      int j=0;
      for (int i=0; i<nI; i++)
      {
         int v= lineI[i];
         if ((v > 255) || (0 == v)) { rgb[j+0]= rgb[j+1]= rgb[j+2]= 0; }
         else if (v > 0)
         {  // blue -> magenta -> red +ve
            rgb[j+0]= std::min(0xFF,0x20+v);
            rgb[j+1]= 0x20;
            rgb[j+2]= 0xC0 - std::min(0xC0,v);
         }
         else
         {  // grey/green -> white -ve
            rgb[j+0]= rgb[j+2]= 0x20 - v/2;
            rgb[j+1]= 0x40-v;
         }
         j+= 3;
      }
   } // i2rgbHack

   size_t save (const char fileName[], uint8_t outFmt=3) const
   {
      size_t bytes= 0;
      const size_t n= numElem();
      if (n > 0)
      {
         auto outFile= std::fstream(fileName, std::ios::out | std::ios::binary);
         if (outFmt > 0)
         {  // Example commandline ImageMagick conversions:
            // convert -size 512x512 -depth 8 gray:img.raw img.png
            // convert -size 512x512 -depth 8 RGB:img.raw img.png
            if ((outFmt > 1) && (3 != outFmt)) { outFmt= 1; }
            const size_t lineBytes= def.x * outFmt;
            uint8_t *pB = new uint8_t[lineBytes];
            if (pB)
            {
               size_t o= 0;
               for (int l=0; l<def.y; l++)
               {
                  if (3 == outFmt) { i2rgbHack(pB, pI+o, def.x); }
                  else { i2u8Hack(pB, pI+o, def.x); }
                  o+= def.x;
                  outFile.write((const char *)pB, lineBytes);
                  bytes+= lineBytes;
               }
               delete [] pB;
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
   } // save

}; // CMapImage2D

#endif // MAP_IMAGE_HPP

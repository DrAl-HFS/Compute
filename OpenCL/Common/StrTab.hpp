// StrTab.hpp - C++ classes for string table.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors May 2021

// A basic string table is useful for many hacks:
// development using a complex API, simple parsing etc.
// This example uses indices to eliminate the overhead
// of numerous pointers (commonly 64bits under most OS,
// at the time of writing) and maximise portability to
// embedded applications where resources are limited.

#ifndef STR_TAB_HPP
#define STR_TAB_HPP

#ifdef DEBUG
#include <cstdlib>
#endif

typedef uint16_t StrTabIdx;
typedef signed char StrTabElem;


class CStrTabBase
{
static const StrTabElem _nul;

protected:
   StrTabIdx   *pI;
   StrTabElem  *pE;
   StrTabIdx   maxI, maxE;
   StrTabIdx   nI, nE;

   CStrTabBase (int mI=0, int mE=0) : pI{NULL}, pE{NULL}, nI{0}, nE{0} { allocate(mI,mE); }
   ~CStrTabBase () { release(); }

   bool allocate (int mI, int mE)
   {
      if ((mI > 0) && (mE > 0))
      {
         if (NULL == pI)
         {
            pI= new StrTabIdx[mI];
            if (pI) { maxI= mI; }
         }
         if (NULL == pE)
         {
            pE= new StrTabElem[mE];
            if (pE) { maxE= mE-1; }
         }
      }
      return setup();
   } // alloc

   bool release (void)
   {
      if (pI) { delete [] pI; pI= NULL; }
      nI= maxI= 0;
      if (pE) { delete [] pE; pE= NULL; }
      nE= maxE= 0;
      return(true);
   } // release

   bool commitE (int nElem)
   {  //else
      if ((nElem > 0) && (nElem <= elemAvail()))
      {
         nE+= nElem;
         if (0x0 != pE[nE-1]) // ensure nul termination
         {
            if (++nE < maxE) { pE[nE]= 0x0; }
         }
         return(true);
      }
      //else
      return(false);
   } // commitE

   bool commitI (void)
   {  // get ready for next
      if (nE < maxE) { pE[nE]= 0x0; }
      if (nI < maxI) { pI[++nI]= nE; return(true); }
      //else
      return(false);
   } // commitI


   StrTabElem *operator [] (int i) const { return(pE + pI[i]); }

public:
   bool setup (void) // aka reset
   {
      nI= nE= 0;
      if (maxE > 0)
      {  // Set terminating NUL characters for all strings
         if (pE) { pE[maxE]= pE[0]= 0x0; }
      }
      if (pI && (maxI > 0))
      {
         pI[0]= 0;   // First string ready
         for (StrTabIdx i=1; i<maxI; i++) { pI[i]= maxE; }
      }
      return(pI && pE);
   } // setup

   bool valid (void) const { return((NULL != pI) && (NULL != pE)); }
   bool full (void) const { return((nI >= maxI) || (nE >= maxE)); }
   int elemAvail (void) const { return(maxE - nE); }
   const StrTabElem *nul (void) const { return &_nul; }

}; // CStrTabBase

const StrTabElem CStrTabBase::_nul=0x0; // singleton ?


class CStrTabASCII : public CStrTabBase
{
public:
   CStrTabASCII (int maxS=32, int expectLenS=29) : CStrTabBase(maxS,maxS*(expectLenS+1)) { ; }  // 1kbyte default

   char *next (void)
   {
      char *p=NULL;
      if (valid() && !full()) { p= (char *) CStrTabBase::operator[](nI); }
      return(p);
   } // next

   bool commit (int nElem) { return commitE(nElem) && commitI(); }

   const char *operator [] (int i) const
   {
      const char *p= (const char*) nul();
      if ((i >= 0) && (i < (int)maxI) && valid()) { p= (const char *) CStrTabBase::operator[](i); }
      //else
      return(p);
   }
}; // CStrTabBase

#endif // STR_TAB_HPP

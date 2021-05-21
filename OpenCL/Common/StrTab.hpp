// StrTab.hpp - C++ classes for string table.
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

// A simple string table is useful for many hacks: development (test/debug)
// using a complex API, crude parsing etc. Here the C++ string class is
// deliberately avoided due to its "heavyweight" nature that limits portability
// to embedded applications where resources are limited.
// Instead a "low-level" array-of-char/asciz/cstring representation is used
// with indices to reduce overheads associated with pointer handling.

#ifndef STR_TAB_HPP
#define STR_TAB_HPP

#ifdef DEBUG
#include <cstdlib>
#endif

// CONSIDER: better way of defining 'nul' singleton?
// struct StrTabElem : public signed char { static const StrTabElem _nul; }

// CONSIDER: perhaps better to split into separate classes for element storage and for indices?

template <typename TElem, typename TCount> class CTable
{
protected:
   TElem  *pE;
   TCount n, max;

   CTable (int m=0) : pE{NULL}, n{0}, max{0} { allocate(m); }
   ~CTable () { release(); }

   bool allocate (int m)
   {
      if ((m > 0) && (NULL == pE))
      {
         n= max= 0;
         pE= new TElem[m];
         if (pE) { max= m-1; }
      }
   } // allocate

   bool release ()
   {
      n= max= 0;
      if (pE) { delete [] pE; pE= NULL; }
      return(true);
   } // release

}; // CTable

// Templated base class allows tuning for different applications
template <typename TStrTabElem, typename TStrTabIdx> class CStrTabBase
{
static const TStrTabElem _nul; // singleton

protected:
   TStrTabIdx   *pI;
   TStrTabElem  *pE;
   TStrTabIdx   maxI, maxE;
   TStrTabIdx   nI, nE;

   CStrTabBase (int mI=0, int mE=0) : pI{NULL}, pE{NULL}, nI{0}, nE{0} { allocate(mI,mE); }
   ~CStrTabBase () { release(); }

   bool allocate (int mI, int mE)
   {
      if ((mI > 0) && (mE > 0))
      {
         if (NULL == pI)
         {
            pI= new TStrTabIdx[mI];
            if (pI) { maxI= mI; }
         }
         if (NULL == pE)
         {
            pE= new TStrTabElem[mE];
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


   TStrTabElem *operator [] (int i) const { return(pE + pI[i]); }

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
         for (TStrTabIdx i=1; i<maxI; i++) { pI[i]= maxE; }
      }
      return(pI && pE);
   } // setup

   bool valid (void) const { return((NULL != pI) && (NULL != pE)); }
   bool full (void) const { return((nI >= maxI) || (nE >= maxE)); }
   int elemAvail (void) const { return(maxE - nE); }
   const TStrTabElem *nul (void) const { return &_nul; }

}; // CStrTabBase

// Singleton member must be defined globally
template<typename TStrTabElem, typename TStrTabIdx> const TStrTabElem CStrTabBase<TStrTabElem,TStrTabIdx>::_nul=0x0;


class CStrTabASCII : public CStrTabBase<signed char,uint16_t>
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

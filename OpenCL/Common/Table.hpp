// Table.hpp - C++ classes for tables.
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#ifndef TABLE_HPP
#define TABLE_HPP

#ifdef DEBUG
#include <cstdlib>
#endif

// CONSIDER: better way of defining 'nul' singleton?
// struct StrTabElem : public signed char { static const StrTabElem _nul; }

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
         if (pE) { max= m-1; return(true); }
      }
      return(false);
   } // allocate

   bool release ()
   {
      n= max= 0;
      if (pE) { delete [] pE; pE= NULL; }
      return(true);
   } // release

   bool valid (void) const { return(NULL != pE); }
   bool full (void) const { return(n >= max); }

   int avail (void) const { return(max-n); }

   int commit (int m)
   {
      if ((m > 0) && (m < avail()))
      {
         n+= m;
         return(m);
      }
      return(0);
   } // commit

   TElem *end (void) const
   {
      if (!valid()) { return(NULL); } // paranoid mode
      return(pE + n); // assumes n <= max !
   } // end
   
   int setN (int i)
   { 
      if (i < 0) { i= 0; } 
      if (i > max) { i= max; }
      n= i;
      return(n);
   } // setN
   
   void endMark (TElem em) { TElem *p= end(); if (p) { *p= em; } }
   
   void fillFrom (TElem e, TCount i) { for ( ; i<max; i++) { pE[i]= e; } }
   void setLimits (TElem e) { pE[max]= pE[0]= e; }
}; // CTable

template <typename TElem, typename TCount> class CIndexedTable
{
protected:
   CTable<TElem,TCount> elem;
   CTable<TCount,TCount> idx;

   CIndexedTable (int mI=0, int mE=0) : elem{mE}, idx{mI} { ; }
   ~CIndexedTable () { release(); }

   bool allocate (int mI, int mE) { return(elem.allocate(mE) || idx.allocate(mI)); }
   bool release (void) { return(elem.release() && idx.release()); }
   bool valid (void) const { return(elem.valid() && idx.valid()); }
   bool full (void) const { return(elem.full() || idx.full()); }

   TElem *operator [] (int i) const { return(elem.pE + idx.pE[i]); }
}; // CIndexedTable

/* EXPERIMENTAL: Specialisation for asciz strings */
// This monstrosity illustrates the unholy consequence of mixing templates and inheritance:
// although some useful factorisation can be achieved, the cumbersome notation encourages
// programmer fatigue and error, resulting in poor maintainability...
template <typename TElem, typename TCount> class CStrTabB2 : public CIndexedTable<TElem, TCount>
{
static const TElem _nul; // singleton

   CStrTabB2 (int mI=0, int mE=0) { allocate(mI,mE); }
   ~CStrTabB2 () { release(); }

   bool allocate (int mI, int mE)
   {
      if (CIndexedTable<TElem, TCount>::allocate(mI,mE)) { return setup(); }
      //else
      return(false);
   } // alloc

   bool release (void) { CIndexedTable<TElem, TCount>::release(); } 

   bool commitE (int nElem)
   {
      bool r= CIndexedTable<TElem, TCount>::elem.commit(nElem);
      if (r) { CIndexedTable<TElem, TCount>::elem.endMark(_nul); }
      return(r);
   } // commitE

   bool commitI (void)
   {  // get ready for next

      bool r= CIndexedTable<TElem, TCount>::idx.commit(1);
      if (r)
      { 
         CIndexedTable<TElem, TCount>::elem.endMark(_nul); 
         *(CIndexedTable<TElem, TCount>::idx.end())= CIndexedTable<TElem, TCount>::elem.n;
      }
      return(r);
   } // commitI

   // inherited *operator [] (int i) const { return(pE + pI[i]); }

public:
   bool setup (void) // aka reset
   {
      if (CIndexedTable<TElem, TCount>::elem.valid())
      {  // Set terminating NUL characters for all strings
         CIndexedTable<TElem, TCount>::elem.setN(0);
         CIndexedTable<TElem, TCount>::elem.setLimits(_nul);
      }
      if (CIndexedTable<TElem, TCount>::idx.valid())
      {
         CIndexedTable<TElem, TCount>::idx.setN(0);
         CIndexedTable<TElem, TCount>::idx.fillFrom(CIndexedTable<TElem, TCount>::idx.maxE,1);
      }
   } // setup

   int elemAvail (void) const { return CIndexedTable<TElem, TCount>::elem.avail(); }
   const TElem *nul (void) const { return &_nul; }

}; // CStrTabB2

template<typename TElem, typename TCount> const TElem CStrTabB2<TElem,TCount>::_nul=0x0;

#endif // TABLE_HPP

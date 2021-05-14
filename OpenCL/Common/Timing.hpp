// Timing.hpp - C++ classes for performance timing.
// https://github.com/DrAl-HFS/Compute.git
// Licence: AGPL3
// (c) Project Contributors May 2021

#ifndef TIMING_HPP
#define TIMING_HPP

typedef double TimeValF;

#if (_WIN32 || _WIN64)

// CAVEAT: untested...
class CTimestamp
{
protected:
   TimeValF tick;

public:
   CTimestamp (void)// : tick{0}
   {
      LARGE_INTEGER f;
      if (QueryPerformanceFrequency(&f)) { tick= 1.0 / f; }
      else { tick= 0; }
   }

   TimeValF get (void) const
   {
      LARGE_INTEGER t;
      if (QueryPerformanceCounter(&t)) { return(t * tick); }
      else return(0);
   } // get

}; // CTimestamp

#else

class CTimestamp
{
public:
   CTimestamp (void) { ; }

   TimeValF get (void) const
   {
      struct timespec t;
      if (clock_gettime(CLOCK_REALTIME, &t) >= 0)
      {
         return(t.tv_sec + (TimeValF)1E-9 * t.tv_nsec);
      }
      return(0);
   } // get

}; // CTimestamp

#endif

class CElapsedTime : public CTimestamp
{
public:
   TimeValF last;

   CElapsedTime (void) { last= get(); }

   TimeValF elapsed (void)
   {
      TimeValF diff= 0;
      TimeValF now= get();
      if ((last > 0) && (now > 0)) { diff= now - last; }
      last= now;
      return(diff);
   } // elapsed

}; // CElapsedTime

#endif // TIMING_HPP

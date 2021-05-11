// QueryOCL.hpp - OpenCl interface text query tools.
// https://github.com/DrAl-HFS/Hacks.git
// Licence: AGPL3
// (c) Project Contributors May 2021


#include "StrTab.hpp"

#ifndef QUERY_OCL_HPP
#define QUERY_OCL_HPP


#define MAX_PF_ID    2
#define MAX_DEV_ID   4

// Function-encapsulation "functor" base class
class InfoFunc
{
public:
   //InfoFunc (void) {;}

   virtual size_t operator () (char *p, const size_t max, const int tok) const =0;
}; // InfoFunc

class PlatformInfo : public InfoFunc
{
public:
   cl_platform_id pfmID;

   PlatformInfo (const cl_platform_id id) : pfmID{id} { ; }

   virtual size_t operator () (char *p, const size_t max, const int tok) const override
   {
      size_t b= 0;
      cl_int r= clGetPlatformInfo(pfmID, tok, max, p, &b);
      return(b);
   }
}; // DevInfo

class DevInfo : public InfoFunc
{
public:
   cl_device_id devID;

   DevInfo (const cl_device_id id) : devID{id} { ; }

   virtual size_t operator () (char *p, const size_t max, const int tok) const override
   {
      size_t b= 0;
      cl_int r= clGetDeviceInfo(devID, tok, max, p, &b);
      return(b);
   }
}; // DevInfo

// Replacement for strncpy() which seems to be missing...
int copyStrN (char *d, const char *s, int n)
{
   int i= 0;
   if (d && s && (n > 0))
   {
      do
      {
         d[i]= s[i];
      } while (s[i] && (++i < n));
   }
   return(i);
} // copyStrN

bool addStr (CStrTabASCII& t, const InfoFunc& f, const cl_device_info tok[], const int n)
{
   if (!t.valid()) return(false);
   for (int i=0; i<n; i++)
   {
      int a= t.elemAvail(); //printf("a=%d\n", a);
      if (a > 1)
      {
         char *p= t.next();
         size_t b= f(p, a, tok[i]);
         if (b <= 1) // nul -> marker
         {
            a= copyStrN(p,"?",a);
            if (a > 0) { b= a; }
         }
         t.commit(b); //printf("%d : %s (%zu)\n", i, p, b);
      }
   }
   return(true);
} // addStr

int queryDev (cl_device_id idDev[], int maxD)
{
   CStrTabASCII st;
   cl_platform_id idPfm[MAX_PF_ID]={0,};
   cl_uint nPfm, n;
   int nDev=0;

   std::cout << "Build target : OpenCl V" << CL_TARGET_OPENCL_VERSION << std::endl;
   if (clGetPlatformIDs(MAX_PF_ID, idPfm, &nPfm) >= 0)
   {
      std::cout << nPfm << "platform(s):" << std::endl;

      for (int iP= 0; iP < nPfm; iP++)
      {
         const cl_platform_info tokPfm[]={ CL_PLATFORM_NAME, CL_PLATFORM_PROFILE, CL_PLATFORM_EXTENSIONS };
         st.setup();
         addStr(st, PlatformInfo(idPfm[iP]), tokPfm, 3);
         std::cout << "platform id= " << idPfm[iP] << "->" << st[0] << " (" << st[1] << ", " << st[2] << ") " << std::endl;

         int remD= maxD-nDev;
         if (remD < 0) { remD= 0; }
         if (clGetDeviceIDs(idPfm[iP], CL_DEVICE_TYPE_ALL, remD, idDev+nDev, &n) >= 0)
         {
            std::cout << n << " device(s):" << std::endl;
            for (int iD= nDev; iD < nDev+n; iD++)
            {
               const cl_device_info ditok[]={ CL_DEVICE_NAME, CL_DEVICE_VENDOR, CL_DEVICE_VERSION, CL_DRIVER_VERSION };
               st.setup();
               addStr(st, DevInfo(idDev[iD]), ditok, 4);
               std::cout << "\t" << st[0] << " (" << st[1] << ") " << st[2] << " (Driver V" << st[3] << ")" << std::endl;
            }
            nDev+= n;
         }
      }
   }
   return(nDev);
} // queryDev

#endif // QUERY_OCL_HPP

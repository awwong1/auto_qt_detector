/*

lib.cpp - ECG annotation library based on continuous (CWT) and fast (FWT) wavelet transforms.  
Copyright (C) 2006 YURIY V. CHESNOKOV

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


You may contact the author by e-mail (chesnokov.yuriy@gmail.com or chesnokov_yuriy@mail.ru)
or postal mail (Unilever Centre for Molecular Sciences Informatics,
University Chemical Laboratory, Cambridge University, Lensfield Road, Cambridge, CB2 1EW, UK)

*/
















#include "stdafx.h"
#include "LIB.h"






//----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
////////////CLASS SIGNAL//////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Signal::Signal(): data(0), fp(0),fpmap(0),lpMap(0),
                           SR(0.0),Bits(0),UmV(0),Lead(0),Len(0),
                           hh(0),mm(0),ss(0)
    
{
    wcscpy(fname,L"");
}
Signal::~Signal()
{
   if(vdata.size())
   {
     for(int i=0; i<(int)vdata.size(); i++)
     {
       data = vdata[i];
       delete[] data;
     }
   }
}

//----------------------------------------------------------------------------
long double * Signal::ReadFile(char *name)
{
   wchar_t ustr[256]=L"";
   for(int i=0; i<(int)strlen(name); i++)
    mbtowc(ustr+i,name+i, MB_CUR_MAX);
   return ReadFile(ustr);
}
long double * Signal::ReadFile(wchar_t *name)
{
    wcscpy(fname,name);

     if(!GetInfo(name)) return 0;

     if(dat)
     {
       if(!ReadDat()) return 0;
     }
     else
     {
       if(!ReadTxt())  //read text file
       {
         if(!ReadMit())  //read mit-bih file
           return 0;
       }
     }

    return data;
}
//----------------------------------------------------------------------------

bool Signal::GetInfo(wchar_t *name)
{
     fp = CreateFileW(name,GENERIC_WRITE | GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
      if(fp == INVALID_HANDLE_VALUE)
       return false;
     fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,sizeof(DATAHDR),0);
     lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,sizeof(DATAHDR));
     
     phdr = (PDATAHDR)lpMap;

       if(!memcmp(phdr->hdr,"DATA",4))
       {
         Len = phdr->size;
         SR = phdr->sr;
         Bits = phdr->bits;
         Lead = phdr->lead;
         UmV = phdr->umv;
         hh = phdr->hh;
         mm = phdr->mm;
         ss = phdr->ss;
         dat = true;
       }
       else
         dat = false;

     CloseFile();
     return true;
}

//----------------------------------------------------------------------------
bool Signal::ReadDat()
{
    short tmp;

     fp = CreateFileW(fname,GENERIC_WRITE|GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
      if(fp == INVALID_HANDLE_VALUE)
       return false;
     fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,0,0);
     lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,0);

      phdr = (PDATAHDR)lpMap;
      hdrs.push_back(*phdr);

       lpf = (float *)lpMap;
       lps = (short *)lpMap;
       lpu = (unsigned short *)lpMap;
       lpc = (char *)lpMap;

	 Len = phdr->size;
	  SR = phdr->sr;
	Bits = phdr->bits;
	Lead = phdr->lead;
	 UmV = phdr->umv;

        lpf += sizeof(DATAHDR)/sizeof(float);
        lps += sizeof(DATAHDR)/sizeof(short);
        lpc += sizeof(DATAHDR);

      data = new long double[Len];
      vdata.push_back(data);

      for(int i=0; i<Len; i++)
       switch(Bits)
        {
         case 12:                                             //212 format   12bit
          if(i%2 == 0)
           {
            tmp = MAKEWORD(lpc[0],lpc[1]&0x0f);
             if(tmp > 0x7ff)
              tmp |= 0xf000;
            data[i] = (long double)tmp/(long double)UmV;
           }
          else
           {
            tmp = MAKEWORD(lpc[2],(lpc[1]&0xf0)>>4);
             if(tmp > 0x7ff)
              tmp |= 0xf000;
            data[i] = (long double)tmp/(long double)UmV;
            lpc += 3;
           }
         break;

         case 16:                                             //16format
          data[i] = (long double)lps[i]/(long double)UmV;
         break;

          case 0:
         case 32:                                             //32bit float
          data[i] = (long double)lpf[i];
         break;
        }


     CloseFile();
     return true;
}
//----------------------------------------------------------------------------
bool Signal::ReadTxt()
{
   vector<long double> vec1;
    long double tmp;
    int res;

     if((in = _wfopen(fname,L"rt")) == 0)
      return false;

        for(;;)
        {
         res = fscanf(in,"%Lf",&tmp);
          if(res == EOF || res == 0)
           break;
          else
           vec1.push_back(tmp);
        }
       fclose(in);
       Len = vec1.size();
      if(Len < 2)
       return false;

      data = new long double[Len];
       for(int i=0; i<Len; i++)
        data[i] = vec1[i];
      vdata.push_back(data);

      DATAHDR hdr;
      memset(&hdr,0,sizeof(DATAHDR));
       hdr.size = Len;
       hdr.umv = 1;
       hdr.bits = 32;
      hdrs.push_back(hdr);

   return true;
}
//-----------------------------------------------------------------------------
bool Signal::ReadMit()
{
   wchar_t hdrfile[256];
   wcscpy(hdrfile,fname);

   changeext(hdrfile,L".hea");
   FILE *fh = _wfopen(hdrfile,L"rt");
   if(!fh) return false;

   if(parseHdr(fh))
   {
     fclose(fh);

     phdr = &hdrs[0];
     int lNum = (int)hdrs.size();
     int size = phdr->size;

      fp = CreateFileW(fname,GENERIC_WRITE|GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
      if(fp==INVALID_HANDLE_VALUE || (GetFileSize(fp,0)!=(lNum*phdr->size*phdr->bits)/8))
       return false;
      fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,0,0);
      lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,0);

       lps = (short *)lpMap;
       lpc = (char *)lpMap;

      for(int i=0; i<lNum; i++)
      {
        data = new long double[phdr->size];
        vdata.push_back(data);
      }

       short tmp;
      for(int s=0; s<size; s++)
      {
        for(int n=0; n<lNum; n++)
        {
          data = vdata[n];
          phdr = &hdrs[n];

          switch(phdr->bits)
          {
           case 12:                                             //212 format   12bit
            if((s*lNum+n)%2 == 0)
            {
             tmp = MAKEWORD(lpc[0],lpc[1]&0x0f);
              if(tmp > 0x7ff)
               tmp |= 0xf000;
             tmp -= phdr->bline;
             data[s] = (long double)tmp/(long double)phdr->umv;
            }
            else
            {
             tmp = MAKEWORD(lpc[2],(lpc[1]&0xf0)>>4);
              if(tmp > 0x7ff)
               tmp |= 0xf000;
             tmp -= phdr->bline;  
             data[s] = (long double)tmp/(long double)phdr->umv;
             lpc += 3;
            }
           break;

           case 16:                                      //16format
             data[s] = (long double)(*lps++ - phdr->bline)/(long double)phdr->umv;
           break;
          }
        }
      }

      GetData(0);
      CloseFile();
      return true;
   }
   else
   {
     fclose(fh);
     return false;
   }
}

int Signal::GetData(int index)
{
   if(!vdata.size()) return 0;
   if(index > (int)vdata.size()-1) index=(int)vdata.size()-1;
   else if(index < 0) index = 0;

   phdr = &hdrs[index];
   data = vdata[index];
   SR = phdr->sr;
   Lead = phdr->lead;
   UmV = phdr->umv;
   Bits = phdr->bits;
   Len = phdr->size;
   hh = phdr->hh;
   mm = phdr->mm;
   ss = phdr->ss;

   return index;
}

int Signal::parseHdr(FILE *fh)
{
   char leads[18][6] =  {"I","II","III","aVR","aVL","aVF","v1","v2",
                         "v3","v4","v5","v6","MLI","MLII","MLIII","vX","vY","vZ"};
   char str[10][256];

   if(readline(fh, str[0]) <= 0)
    return false;

   int sNum,size;
   float sr;
   int res = sscanf(str[0],"%s %s %s %s",str[1],str[2],str[3],str[4]);
   if(res != 4)
    return 0;

   sNum = atoi(str[2]);
   sr = atof(str[3]);
   size = atoi(str[4]);

    int eNum=0;
   for(int i=0; i<sNum; i++)
   {
     if(readline(fh, str[0]) <= 0)
      return 0;

     int umv,bits,bline;
     memset(str[9],0,256);

     res = sscanf(str[0],"%s %s %s %s %s %s %s %s %s",str[1],str[2],str[3],str[4],str[5],str[6],str[7],str[8],str[9]);
     if(res < 5) return 0;

     bits = atoi(str[2]);
     umv = atoi(str[3]);
     bline = atoi(str[5]);

     int offs=strlen(str[1]),j=0;
      for(j=0; j<offs; j++)
      {
        if(fname[(wcslen(fname)-offs)+j] != str[1][j])   //wctomb
         break;
      }
      if(j == offs)
      {
        eNum++;
        DATAHDR hdr;
         memset(&hdr,0,sizeof(DATAHDR));
         hdr.sr = sr;
         hdr.bits = (bits == 212)? 12: bits;
         hdr.umv = (umv == 0)? 200: umv;
         hdr.bline = bline;
         hdr.size = size;
         for(int l=0; l<18; l++)
         {
           if(!stricmp(leads[l],str[9]))
           {
             hdr.lead = l+1;
             break;
           }
         }
        hdrs.push_back(hdr);
      }
   }
   return eNum;
}

//-----------------------------------------------------------------------------
bool Signal::SaveFile(char *name, long double *buff, PDATAHDR hdr)
{
   wchar_t ustr[256]=L"";
   for(int i=0; i<(int)strlen(name); i++)
    mbtowc(ustr+i,name+i, MB_CUR_MAX);
   return SaveFile(ustr, buff, hdr);
}
bool Signal::SaveFile(wchar_t *name, long double *buff, PDATAHDR hdr)
{
     int filesize;
      int tmp;

      switch(hdr->bits)
      {
          case 12:
	   if(hdr->size%2 != 0)
            filesize = int((long double)(hdr->size+1)*1.5);
           else
            filesize = int((long double)(hdr->size)*1.5);
          break;
          case 16:
            filesize = hdr->size*2;
          break;
          case 0:
          case 32:
            filesize = hdr->size*4;
          break;
      }

   
      fp = CreateFileW(name,GENERIC_WRITE|GENERIC_READ,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
      if(fp == INVALID_HANDLE_VALUE)
        return false;
      fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,filesize+sizeof(DATAHDR),0);
      lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,filesize+sizeof(DATAHDR));
	    
        lpf = (float *)lpMap;
        lps = (short *)lpMap;
        lpu = (unsigned short *)lpMap;
        lpc = (char *)lpMap;

        memset(lpMap,0,filesize+sizeof(DATAHDR));
	memcpy(lpMap,hdr,sizeof(DATAHDR));
           
        lpf += sizeof(DATAHDR)/sizeof(float);
        lps += sizeof(DATAHDR)/sizeof(short);
        lpc += sizeof(DATAHDR);


        for(unsigned int i=0; i<hdr->size; i++)
        {
          switch(hdr->bits)
          {
           case 12:                                               //212 format   12bit
            tmp = int(buff[i]*(long double)hdr->umv);
             if(tmp > 2047) tmp = 2047;
             if(tmp < -2048) tmp = -2048;

             if(i%2 == 0)
              {
               lpc[0] = LOBYTE((short)tmp);
                lpc[1] = 0;
               lpc[1] = HIBYTE((short)tmp) & 0x0f;
              }
             else
              {
                lpc[2] = LOBYTE((short)tmp);
                lpc[1] |= HIBYTE((short)tmp) << 4;
               lpc += 3;
              }
           break;

           case 16:                                               //16format
              tmp = int(buff[i]*(long double)hdr->umv);
               if(tmp > 32767) tmp = 32767;
               if(tmp < -32768) tmp = -32768;
             *lps++ = (short)tmp;
           break;

           case 0:
           case 32:                                               //32bit float
             *lpf++ = (float)buff[i];
           break;
          }
	}

     CloseFile();
     return true;
}
//-----------------------------------------------------------------------------
bool Signal::ToTxt(wchar_t *name, long double *buff, int size)
{
   in = _wfopen(name,L"wt"); 

     if(in)
     {
       for(int i=0; i<size; i++)
        fwprintf(in,L"%f\n",(float)buff[i]);

       fclose(in);
       return true;
     }
     else
      return false;
}

void Signal::getfname(wchar_t *path, wchar_t *name)
{
    int sl=0,dot=(int)wcslen(path);
    int i;
   for(i=0; i<(int)wcslen(path); i++)
   {
     if(path[i] == '.') break;
     if(path[i] == '\\') break;
   }
   if(i >= (int)wcslen(path))
   {
     wcscpy(name,path);
      return;
   }

   for(i=(int)wcslen(path)-1; i>=0; i--)
   {
     if(path[i] == '.')
       dot = i;
     if(path[i] == '\\')
     {
       sl = i+1;
       break;
     }
   }

   memcpy(name,&path[sl],(dot-sl)*2);
   name[dot-sl]=0;
}

void Signal::changeext(wchar_t *path, wchar_t *ext)
{
   for(int i=(int)wcslen(path)-1; i>0; i--)
   {
     if(path[i] == '.')
     {
       path[i] = 0;
       wcscat(path, ext);
       return;
     }
   }

   wcscat(path, ext);
}

int Signal::readline(FILE *f, char *buff)
{
   int res=0;
   char *pbuff = buff;

   while((short)res != EOF)
   {
      res = fgetc(f);
      if(res == 0xD || res == 0xA)
      {
        if(pbuff == buff) continue;

       	*pbuff = 0;
       	 return 1;
      }
      if((short)res != EOF)
      {
        *pbuff++ = (char)res;
      }
   }

   return (short)res;
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void  Signal::mSecToTime(int msec, int &h, int &m, int &s, int &ms)
{
     ms = msec%1000;
      msec /= 1000;

      if(msec < 60)
       {
        h = 0;
        m = 0;
        s = msec;                 //sec to final
       }
      else
       {
         long double tmp;
          tmp = (long double)(msec%60) / 60;
          tmp *= 60;
          s = int(tmp);
         msec /= 60;

           if(msec < 60)
            {
              h = 0;
              m = msec;
            }
           else
            {
              h = msec/60;
              tmp = (long double)(msec%60)/60;
              tmp *= 60;
              m = int(tmp);
            }
       }
}
//-----------------------------------------------------------------------------
void  Signal::MinMax(long double *buff, int size, long double &min, long double &max)
{
    max=buff[0];
    min=buff[0];
   for(int i=1; i<size; i++)
   {
     if(buff[i] > max)max=buff[i];
     if(buff[i] < min)min=buff[i];
   }   
}
//-----------------------------------------------------------------------------
void  Signal::CloseFile()
{
  if(lpMap)
  {
    UnmapViewOfFile(lpMap);
     if(fpmap)
     {
      CloseHandle(fpmap);
       if(fp && fp != INVALID_HANDLE_VALUE)
        CloseHandle(fp);
     }
  }
}
///////////////////////////////////////////////////////////////////////////////





//-------MATH ROUTINES--------------------------------------------------------
long double  Signal::Mean(long double *mass, int size)
{
  long double mean = 0;

   for(int i=0; i<size; i++)
    mean += mass[i];

  return mean /(long double)size;
}

long double  Signal::Disp(long double *mass, int size)
{
   long double mean,disp=0;

    mean = Mean(mass,size);

     for(int i=0; i<size; i++)
      disp += (mass[i] - mean)*(mass[i] - mean);

   return (sqrtl(disp/(long double)(size-1)));
}
//----------------------------------------------------------------------------
void  Signal::nMinMax(long double *buff, int len, long double a, long double b)
{
    long double min,max;
    MinMax(buff,len,min,max);

    for(int i=0; i<len; i++)
    {
      if(max-min)
       buff[i] = (buff[i]-min)*((b-a)/(max-min)) + a;
      else
       buff[i] = a;
    }
}
void Signal::nMean(long double *buff, int len)
{
   long double mean = Mean(buff,len);

   for(int i=0; i<len; i++)
    buff[i] = buff[i]-mean;
}
void Signal::nZscore(long double *buff, int len)
{
   long double mean = Mean(buff,len);
   long double disp = Disp(buff,len);

   if(disp==0.0) disp=1.0;
   for(int i=0; i<len; i++)
    buff[i] = (buff[i]-mean)/disp;
}
void Signal::nSoftmax(long double *buff, int len)
{
   long double mean = Mean(buff,len);
   long double disp = Disp(buff,len);

   if(disp==0.0) disp=1.0;
   for(int i=0; i<len; i++)
     buff[i] = 1.0/( 1 + expl(-((buff[i]-mean)/disp)) );
}
void Signal::nEnergy(long double *buff, int len, int L)
{
   long double enrg=0.0;
   for(int i=0; i<len; i++)
     enrg += powl(fabsl(buff[i]), (long double)L);

   enrg = powl(enrg, 1.0/(long double)L);
   if(enrg==0.0) enrg=1.0;

   for(int i=0; i<len; i++)
     buff[i] /= enrg;
}



//----------------------------------------------------------------------------
long double  Signal::MINIMAX(long double *mass, int size)
{
   return Disp(mass,size)*(0.3936 + 0.1829*logl((long double)size));
}
long double  Signal::FIXTHRES(long double *mass, int size)
{
   return Disp(mass,size)*sqrtl(2.0*logl((long double)size));
}
long double  Signal::SURE(long double *mass, int size)
{
   return Disp(mass,size)*sqrtl(2.0*logl((long double)size*logl((long double)size)));
}

void  Signal::Denoise(long double *mass, int size, int window, int type, bool soft)
{
   long double TH;

    for(int i=0; i<size/window; i++)
    {
       switch(type)
       {
         case 0:
           TH = MINIMAX(mass,window);
         break;
         case 1:
           TH = FIXTHRES(mass,window);
         break;
         case 2:
           TH = SURE(mass,window);
         break;
       }
       if(soft)
         SoftTH(mass, window, TH);
       else
         HardTH(mass, window, TH);

       mass += window;
    }

    if(size%window > 5)  //skip len=1
    {
       switch(type)
       {
         case 0:
           TH = MINIMAX(mass,size%window);
         break;
         case 1:
           TH = FIXTHRES(mass,size%window);
         break;
         case 2:
           TH = SURE(mass,size%window);
         break;
       }
       if(soft)
         SoftTH(mass, size%window, TH);
       else
         HardTH(mass, size%window, TH);
    }
}

void  Signal::HardTH(long double *mass, int size, long double TH, long double l)
{
  for(int i=0; i<size; i++)
   if(fabsl(mass[i]) <= TH)
     mass[i] *= l;
}
void  Signal::SoftTH(long double *mass, int size, long double TH, long double l)
{
  for(int i=0; i<size; i++)
   {
     if(fabsl(mass[i]) <= TH)
      {
        mass[i] *= l;
      }
     else
      {
        if(mass[i] > 0)
         {
           mass[i] -= TH*(1-l);
         }
        else
         {
           mass[i] += TH*(1-l);
         }
      }
   }
}
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
bool  Signal::AutoCov(long double *mass, int size)
{
   long double *rk, mu;
    int t;
  rk = new long double[size];

     mu = Mean(mass,size);

   for(int k=0; k<size; k++)
    {
     rk[k] = 0;

      t=0;
     while(t+k != size)
      {
       rk[k] += (mass[t]-mu)*(mass[t+k]-mu);
       t++;
      }

     rk[k] /= (long double)t;                       // rk[k] /= t ?  autocovariance
    }

   for(int i=0; i<size; i++)
    mass[i] = rk[i];

  
   delete[] rk;
   return true;
}
//-----------------------------------------------------------------------------









































//////////////////////////////////////////////////////////////////////////////
////////////////////////CLASS CWT/////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

CWT::CWT(): type(0), cwtBuff(0),ReW(0),ImW(0)
{
}
CWT::~CWT()
{
   if(ReW) free(ReW);
   if(ImW) free(ImW);
   if(cwtBuff) free(cwtBuff);
}
//----------------------------------------------------------------------------
float * CWT::CWTCreateFileHeader(wchar_t *name, PCWTHDR hdr, int index, long double w)
{
     wchar_t tmp[256];

     int filesize;
     
      switch(index)
       {
        case 0:
         wcscat(name,L"(mHat).w");
        break;
        case 1:
         wcscat(name,L"(Inv).w");
        break;
        case 2:
         wcscat(name,L"(Morl).w");
        break;
        case 3:
         wcscat(name,L"(MPow).w");
        break;
        case 4:
         wcscat(name,L"(MComp");
	 swprintf(tmp,L"%d",(int)w);
	 wcscat(name,tmp);
	 wcscat(name,L").w");
        break;

        case 5:
         wcscat(name,L"(Gaussian).w");
        break;
        case 6:
         wcscat(name,L"(1Gauss).w");
        break;
        case 7:
         wcscat(name,L"(2Gauss).w");
        break;
        case 8:
         wcscat(name,L"(3Gauss).w");
        break;
        case 9:
         wcscat(name,L"(4Gauss).w");
        break;
        case 10:
         wcscat(name,L"(5Gauss).w");
        break;
        case 11:
         wcscat(name,L"(6Gauss).w");
        break;
        case 12:
         wcscat(name,L"(7Gauss).w");
        break;
       }

       if(hdr->type)
       	 filesize = hdr->size * (int)ceill( (logl(hdr->fmax)+hdr->fstep-logl(hdr->fmin))/hdr->fstep );
       else
         filesize = hdr->size * (int)ceill( (hdr->fmax+hdr->fstep-hdr->fmin)/hdr->fstep );


      filesize = sizeof(float)*filesize+sizeof(CWTHDR);

       fp = CreateFileW(name,GENERIC_WRITE | GENERIC_READ,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
        if(fp == INVALID_HANDLE_VALUE)
         return 0;
       fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,filesize,0);
       lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,filesize);

         lpf = (float *)lpMap;

        memset(lpMap,0,filesize);
      	memcpy(lpMap,hdr,sizeof(CWTHDR));

     return (lpf+sizeof(CWTHDR)/sizeof(float));
}
//----------------------------------------------------------------------------
float * CWT::CWTReadFile(wchar_t *name)
{
    fp = CreateFileW(name,GENERIC_WRITE | GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
     if(fp == INVALID_HANDLE_VALUE)
      return 0;
    fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,0,0);
    lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,0);

      phdr = (PCWTHDR)lpMap;
      lpf = (float *)lpMap;


     if(memcmp(phdr->hdr,"WLET",4))
      return 0;

     fmin = phdr->fmin;
     fmax = phdr->fmax;
    fstep = phdr->fstep;
      Len = phdr->size;
       SR = phdr->sr;
     type = phdr->type;

    return (lpf+sizeof(CWTHDR)/sizeof(float));
}
//----------------------------------------------------------------------------
long double  CWT::HzToScale(long double f, long double sr, int index, long double w)
{
   long double k;

      switch(index)
        {
         case 0:
          k = 0.22222 * sr;
         break;
         case 1:
          k = 0.15833 * sr;
         break;
         case 2:
         case 3:
          k = sr;
         break;
         case 4:
          k = sr * w * 0.1589;
         break;


         case 5:                                 //Gauss
          k = 0.2 * sr;
         break;
         case 6:                                 //1Gauss
          k = 0.16 * sr;
         break;
         case 7:                                 //2Gauss
          k = 0.224 * sr;
         break;
         case 8:                                 //3Gauss
          k = 0.272 * sr;
         break;
         case 9:                                 //4Gauss
          k = 0.316 * sr;
         break;
         case 10:                                 //5Gauss
          k = 0.354 * sr;
         break;
         case 11:                                //6Gauss
          k = 0.388 * sr;
         break;
         case 12:                                //7Gauss
          k = 0.42 * sr;
         break;

         default:
          k = 0;
        }

     return (k/f);
}
int CWT::GetFreqRange()
{
    if(type == 0)
     return (int)((fmax+fstep-fmin)/fstep);
    else
     return (int)((logl(fmax)+fstep-logl(fmin))/fstep);
}
void  CWT::ConvertName(wchar_t *name, int index, long double w)
{
    wchar_t tmp[256];

       switch(index)
       {
        case 0:
         wcscat(name,L"(mHat).w");
        break;
        case 1:
         wcscat(name,L"(Inv).w");
        break;
        case 2:
         wcscat(name,L"(Morl).w");
        break;
        case 3:
         wcscat(name,L"(MPow).w");
        break;
        case 4:
         wcscat(name,L"(MComp");
      	 swprintf(tmp,L"%d",(int)w);
      	 wcscat(name,tmp);
      	 wcscat(name,L").w");
        break;

        case 5:
         wcscat(name,L"(Gaussian).w");
        break;
        case 6:
         wcscat(name,L"(1Gauss).w");
        break;
        case 7:
         wcscat(name,L"(2Gauss).w");
        break;
        case 8:
         wcscat(name,L"(3Gauss).w");
        break;
        case 9:
         wcscat(name,L"(4Gauss).w");
        break;
        case 10:
         wcscat(name,L"(5Gauss).w");
        break;
        case 11:
         wcscat(name,L"(6Gauss).w");
        break;
        case 12:
         wcscat(name,L"(7Gauss).w");
        break;
       }
}

//----------------------------------------------------------------------------
void  CWT::InitCWT(int sz, int index, long double w, long double sr)
{
     cwtSize = sz;

     if(sr)
      SR = sr;

     w0 = w;
    ReW = (long double *)malloc(sizeof(long double)*(2*cwtSize-1));
    ImW = (long double *)malloc(sizeof(long double)*(2*cwtSize-1));
   cwtBuff = (long double *)malloc(sizeof(long double)*(cwtSize));
    cwIndex = index;

    for(int i=0; i<2*cwtSize-1; i++)
    {
      ReW[i]=0;
      ImW[i]=0;
    }
}
//----------------------------------------------------------------------------
void  CWT::CloseCWT()
{
     if(ReW) { free(ReW); ReW=0; };
     if(ImW) { free(ImW); ImW=0; };
     if(cwtBuff) {free(cwtBuff); cwtBuff=0; };
}

//----------------------------------------------------------------------------

long double *CWT::CWTrans(long double *buff, long double freq, bool mr,long double lv,long double rv)
{
       mirror = mr;
       lval=lv;
       rval=rv;

       prec = false;
       precision=0;                                        //0,0000001 float prsision
      long double t,sn,cs,scale;

      scale = HzToScale(freq,SR,cwIndex,w0);

///////////wavelet calculation//////////////////////////////////////////////////////////////////
///////// center = csigLen-1 in wavelet mass////////////////////////////////////////////////////

        for(int i=0; i<cwtSize; i++)                            //positive side
         {
          t = ((long double)i)/scale;

          if(cwIndex > 1 && cwIndex < 4)
	   {
            sn = sinl(6.28*t);
	    cs = cosl(6.28*t);
	   }
          if(cwIndex == 4)
	   {
            sn = sinl(w0*t);
	    cs = cosl(w0*t);
	   }

           switch(cwIndex)
            {
             case 0:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)*(-t*t+1);
             break;
             case 1:
              ReW[(cwtSize-1)+i] = t*expl(-t*t/2);
             break;
             case 2:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)*(cs-sn);
             break;
             case 3:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)* cs;
              ImW[(cwtSize-1)+i] = expl(-t*t/2)* sn;
             break;
             case 4:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)* (cs - expl(-w0*w0/2));
              ImW[(cwtSize-1)+i] = expl(-t*t/2)* (sn - expl(-w0*w0/2));
             break;


             case 5:
              ReW[(cwtSize-1)+i] = expl(-t*t/2);
             break;
             case 6:
              ReW[(cwtSize-1)+i] = -t*expl(-t*t/2);
             break;
             case 7:
              ReW[(cwtSize-1)+i] = (t*t-1)*expl(-t*t/2);
             break;
             case 8:
              ReW[(cwtSize-1)+i] = (2*t+t-t*t*t)*expl(-t*t/2);
             break;
             case 9:
              ReW[(cwtSize-1)+i] = (3-6*t*t+t*t*t*t)*expl(-t*t/2);
             break;
             case 10:
              ReW[(cwtSize-1)+i] = (-15*t+10*t*t*t-t*t*t*t*t)*expl(-t*t/2);
             break;
             case 11:
              ReW[(cwtSize-1)+i] = (-15+45*t*t-15*t*t*t*t+t*t*t*t*t*t)*expl(-t*t/2);
             break;
             case 12:
              ReW[(cwtSize-1)+i] = (105*t-105*t*t*t+21*t*t*t*t*t-t*t*t*t*t*t*t)*expl(-t*t/2);
             break;
            }

           if(fabsl(ReW[(cwtSize-1)+i]) < 0.0000001)
             precision++;

           if(precision > 15)
            {
             precision = i;
             prec = true;
             break;
            }
         }
        if(prec == false)
         precision = cwtSize;

         for(int i=-(precision-1); i<0; i++)                        //negative side
          {
           t = ((long double)i)/scale;

           if(cwIndex > 1 && cwIndex < 4)
	   {
             sn = sinl(6.28*t);
	     cs = cosl(6.28*t);
	   }
           if(cwIndex == 4)
	   {
             sn = sinl(w0*t);
	     cs = cosl(w0*t);
	   }

           switch(cwIndex)
            {
             case 0:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)*(-t*t+1);
             break;
             case 1:
              ReW[(cwtSize-1)+i] = t*expl(-t*t/2);
             break;
             case 2:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)*(cs-sn);
             break;
             case 3:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)* cs;
              ImW[(cwtSize-1)+i] = expl(-t*t/2)* sn;
             break;
             case 4:
              ReW[(cwtSize-1)+i] = expl(-t*t/2)* (cs - expl(-w0*w0/2));
              ImW[(cwtSize-1)+i] = expl(-t*t/2)* (sn - expl(-w0*w0/2));
             break;


             case 5:
              ReW[(cwtSize-1)+i] = expl(-t*t/2);    //gauss
             break;
             case 6:
              ReW[(cwtSize-1)+i] = -t*expl(-t*t/2);   //gauss1
             break;
             case 7:
              ReW[(cwtSize-1)+i] = (t*t-1)*expl(-t*t/2);     //gauss2
             break;
             case 8:
              ReW[(cwtSize-1)+i] = (2*t+t-t*t*t)*expl(-t*t/2);    //gauss3
             break;
             case 9:
              ReW[(cwtSize-1)+i] = (3-6*t*t+t*t*t*t)*expl(-t*t/2);    //gauss4
             break;
             case 10:
              ReW[(cwtSize-1)+i] = (-15*t+10*t*t*t-t*t*t*t*t)*expl(-t*t/2);    //gauss5
             break;
             case 11:
              ReW[(cwtSize-1)+i] = (-15+45*t*t-15*t*t*t*t+t*t*t*t*t*t)*expl(-t*t/2);   //gauss6
             break;
             case 12:
              ReW[(cwtSize-1)+i] = (105*t-105*t*t*t+21*t*t*t*t*t-t*t*t*t*t*t*t)*expl(-t*t/2);    //gauss7
             break;
            }
          }
///////end wavelet calculations////////////////////////////////////////////

        csig = buff;
       for (int x = 0; x < cwtSize; x++)
        cwtBuff[x] = cWave(x,scale);

    return cwtBuff;
}
//----------------------------------------------------------------------------

 long double CWT::cWave(int x, long double scale)
{
    long double res=0, Re=0, Im=0;

      for(int t = 0; t < cwtSize; t++)                      //main
       {
          if(prec==true)
          {
           if(t < x-precision)t=x-(precision-1);//continue;
           if(t >= precision+x)break;
          }

          Re += ReW[((cwtSize-1)-x) + t] * csig[t];
         if(cwIndex == 3 || cwIndex == 4)
          Im += ImW[((cwtSize-1)-x) + t] * csig[t];
       }


    ////////////////////boundaries///////////////////////////////////////////////
        int p=0;
      for(int i=(cwtSize-precision); i<(cwtSize-1)-x; i++)          // Left edge calculations
      {
        if(mirror)
        {
         Re += ReW[i]*csig[(cwtSize-1)-i-x];    //mirror
        }
        else
        {
         if(lval)
          Re += ReW[i]*lval;
         else
          Re += ReW[i]*csig[0];
        }

        if(cwIndex == 3 || cwIndex == 4)  //Im part for complex wavelet
        {
         if(mirror)
         {
          Im += ImW[i]*csig[(cwtSize-1)-i-x];
         }                                  
         else
         {
           if(lval)
            Im += ImW[i]*lval;
           else
            Im += ImW[i]*csig[0];
         }
        }
      }
        int q=0;
      for(int i=2*cwtSize-(x+1); i<cwtSize+precision-1; i++)       // Right edge calculations
      {
        if(mirror)
         Re += ReW[i]*csig[(cwtSize-2)-q];   //mirror
        else
        {
         if(rval)
          Re += ReW[i]*rval;
         else
          Re += ReW[i]*csig[cwtSize-1];
        }

        if(cwIndex == 3 || cwIndex == 4)
        {
          if(mirror)
          {
           Im += ImW[i]*csig[(cwtSize-2)-q];
          }
          else
          {
            if(rval)
             Im += ImW[i]*rval;
            else
             Im += ImW[i]*csig[cwtSize-1];
          }
        }
         q++;
      }
    ////////////////////boundaries///////////////////////////////////////////////



      switch(cwIndex)
      {
       case 2:
        res = (1/sqrtl(6.28)) * Re;
       break;
       case 3:
        res = sqrtl(Re*Re + Im*Im);
        res *= (1/sqrtl(6.28));
       break;
       case 4:
        res = sqrtl(Re*Re + Im*Im);
        res *= (1/powl(3.14,0.25));
       break;

       default:
        res = Re;
      }

      res = (1/sqrtl(scale)) * res;

    return res;
}
//----------------------------------------------------------------------------*/



















//////////////////////////////////////////////////////////////////////////////
////////////////////////CLASS FWT/////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

FWT::FWT(): tH(0),tG(0),H(0),G(0),
            thL(0),tgL(0),hL(0),gL(0), thZ(0),tgZ(0),hZ(0),gZ(0),
            fwtBuff(0),tempBuff(0),
            J(0),Jnumbs(0)
{
}
FWT::~FWT()
{
    if(tH) delete[] tH;
    if(tG) delete[] tG;
    if(H) delete[] H;
    if(G) delete[] G;

   if(fwtBuff) free(fwtBuff);
   if(tempBuff) free(tempBuff);

   if(Jnumbs) delete[] Jnumbs;
}

//----------------------------------------------------------------------------
//////////////////// filters init ///////////////////////////////////////////////////
bool FWT::InitFWT(wchar_t *fltname, long double *sig, int len)
{
   filter = _wfopen(fltname,L"rt");
    if(filter)
    {
       tH = loadfilter(thL, thZ);
       tG = loadfilter(tgL, tgZ);
       H = loadfilter(hL, hZ);
       G = loadfilter(gL, gZ);
       fclose(filter);

       size = len;
       sigLen = len;
       fwtBuff = (long double *)malloc(sizeof(long double)*len);
       tempBuff = (long double *)malloc(sizeof(long double)*len);
       lo = tempBuff;
       hi = tempBuff+len;

       for(int i=0; i<len; i++)
        fwtBuff[i] = sig[i];
       memset(tempBuff,0,sizeof(long double)*len);

        J = 0;

       return true;
    }
    else
     return false;
}

long double *FWT::loadfilter(int &L, int &Z)
{
   fscanf(filter,"%d",&L);
   fscanf(filter,"%d",&Z);

    long double *flt = new long double[L];

   for(int i=0; i<L; i++)
     fscanf(filter,"%Lf",&flt[i]);

   return flt;
}

void  FWT::CloseFWT()
{
    if(tH) {delete[] tH; tH=0;}
    if(tG) {delete[] tG; tG=0;}
    if(H) {delete[] H; H=0;}
    if(G) {delete[] G; G=0;}

    if(fwtBuff) {free(fwtBuff); fwtBuff=0;}
    if(tempBuff) {free(tempBuff); tempBuff=0;}

    if(Jnumbs) {delete[] Jnumbs; Jnumbs = 0;}
}
//////////////////// filters init ///////////////////////////////////////////////////




//----------------------------------------------------------------------------
void FWT::fwtHiLoTrans()
{
   int n;
   long double s,d;

    for(int k=0; k<size/2; k++)
    {
      s = 0;
      d = 0;

       for(int m=-thZ; m<thL-thZ; m++)
       {
         n = 2*k + m;
       	 if(n < 0) n = 0 - n;
       	 if(n >= size) n -= (2 + n-size);
       	  s += tH[m+thZ] * fwtBuff[n];
       }

       for(int m=-tgZ; m<tgL-tgZ; m++)
       {
         n = 2*k + m;
       	 if(n < 0) n = 0 - n;
       	 if(n >= size) n -= (2 + n-size);
       	  d += tG[m+tgZ] * fwtBuff[n];
       }

       lo[k] = s;
       hi[k] = d;
    }

    for(int i=0; i<sigLen; i++)
     fwtBuff[i] = tempBuff[i];
}

void FWT::fwtTrans(int scales)
{
    for(int j=0; j<scales; j++)
    {
       hi -= size/2;
       fwtHiLoTrans();

       size /= 2;
       J++;
    }
}
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
void FWT::fwtHiLoSynth()
{
     int n;
    long double s2k, s2k1;


    for(int i=0; i<sigLen; i++)
     tempBuff[i] = fwtBuff[i];

    for(int k=0; k<size; k++)
    {
      s2k = 0;
      s2k1 = 0;

      for(int m=-hZ; m<hL-hZ; m++)                //s2k and s2k1 for H[]
      {
        n = k-m;
      	if(n < 0) n = 0 - n;
      	if(n >= size) n -= (2 + n-size);

        if(2*m >= -hZ && 2*m < hL-hZ)
         s2k += H[(2*m)+hZ]*lo[n];
        if((2*m+1) >= -hZ && (2*m+1) < hL-hZ)
         s2k1 += H[(2*m+1)+hZ]*lo[n];
      }

      for(int m=-gZ; m<gL-gZ; m++)               //s2k and s2k1 for G[]
      {
        n = k-m;
        if(n < 0) n = 0 - n;
	if(n >= size) n -= (2 + n-size);

        if(2*m >= -gZ && 2*m < gL-gZ)
         s2k += G[(2*m)+gZ]*hi[n];
        if((2*m+1) >= -gZ && (2*m+1) < gL-gZ)
         s2k1 += G[(2*m+1)+gZ]*hi[n];
      }

       fwtBuff[2*k] = 2.0*s2k;
       fwtBuff[2*k+1] = 2.0*s2k1;
    }

}
void FWT::fwtSynth(int scales)
{
    for(int j=0; j<scales; j++)
    {
       fwtHiLoSynth();
       hi += Jnumbs[j];

       size *= 2;
       J--;
    }
}
//-----------------------------------------------------------------------------







//-----------------------------------------------------------------------------
int *FWT::GetJnumbs(int j, int len)
{
   if(Jnumbs) delete[] Jnumbs;

   Jnumbs = new int[j];

     for(int i=0; i<j; i++)
      Jnumbs[i] = len/(int)powl(2,(long double)(j-i));

   return Jnumbs;
}

//-----------------------------------------------------------------------------
void FWT::HiLoNumbs(int j, int len, int &hinum, int &lonum)
{
   lonum = 0;
   hinum = 0;

   for(int i=0; i<j; i++)
    {
     len /= 2;
     hinum += len;
    }
   lonum = len;
}


//-----------------------------------------------------------------------------
bool  FWT::fwtSaveFile(wchar_t *name, long double *hipass, long double *lopass, PFWTHDR hdr)
{
      int filesize;
       short tmp;

      HiLoNumbs(hdr->J,hdr->size,hiNum,loNum);

       switch(hdr->bits)
       {
        case 12:
         if((hiNum+loNum)%2 != 0)
          filesize = (long double)((hiNum+loNum)+1)*1.5;
         else
          filesize = (long double)(hiNum+loNum)*1.5;
        break;
        case 16:
          filesize = (hiNum+loNum)*2;
        break;
        case 0:
        case 32:
          filesize = (hiNum+loNum)*4;
        break;
       }

      fp = CreateFileW(name,GENERIC_WRITE | GENERIC_READ,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
       if(fp == INVALID_HANDLE_VALUE)
       {
         fp = 0;
         return false;
       }
      fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,filesize+sizeof(FWTHDR),0);
      lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,filesize+sizeof(FWTHDR));

        lpf = (float *)lpMap;
        lps = (short *)lpMap;
        lpc = (char *)lpMap;

        memset(lpMap,0,filesize+sizeof(FWTHDR));
	memcpy(lpMap,hdr,sizeof(FWTHDR));

        lpf += sizeof(FWTHDR)/sizeof(float);
        lps += sizeof(FWTHDR)/sizeof(short);
        lpc += sizeof(FWTHDR);

      for(int i=0; i<hiNum+loNum; i++)
       {
        if(i < hiNum)
         tmp = short(  hipass[i]*(long double)hdr->umv  );
        else
         tmp = short(  lopass[i-hiNum]*(long double)hdr->umv  );

          switch(hdr->bits)
          {
           case 12:
             if(i%2 == 0)
              {
               lpc[0] = LOBYTE(tmp);
                lpc[1] = 0;
               lpc[1] = HIBYTE(tmp) & 0x0f;
              }
             else
              {
                lpc[2] = LOBYTE(tmp);
                lpc[1] |= HIBYTE(tmp) << 4;
               lpc += 3;
              }
           break;

           case 16:                                               //16format
             *lps++ = tmp;
           break;

           case 0:
           case 32:
            if(i < hiNum)                                         //32bit float
             *lpf++ = (float)hipass[i];
            else
             *lpf++ = (float)lopass[i-hiNum];
           break;
          }
       }

     CloseFile();
     return true;
}
//----------------------------------------------------------------------------
bool  FWT::fwtReadFile(wchar_t *name, char *appdir)
{
     short tmp;

     fp = CreateFileW(name,GENERIC_WRITE | GENERIC_READ,0,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
      if(fp == INVALID_HANDLE_VALUE)
      {
        fp = 0;
        return false;
      }
     fpmap = CreateFileMapping(fp,0,PAGE_READWRITE,0,0,0);
     lpMap = MapViewOfFile(fpmap,FILE_MAP_WRITE,0,0,0);

      phdr = (PFWTHDR)lpMap;
       lpf = (float *)lpMap;
       lps = (short *)lpMap;
       lpc = (char *)lpMap;

           Len = phdr->size;
            SR = phdr->sr;
          Bits = phdr->bits;
          Lead = phdr->lead;
           UmV = phdr->umv;
          strcpy(fname,phdr->wlet);
            J = phdr->J;


        lpf += sizeof(FWTHDR)/sizeof(float);
        lps += sizeof(FWTHDR)/sizeof(short);
        lpc += sizeof(FWTHDR);

       HiLoNumbs(J,Len,hiNum,loNum);
        sigLen = loNum+hiNum;
        size = loNum;

       fwtBuff = new long double[sigLen];
       tempBuff = new long double[sigLen];
       memset(tempBuff,0,sizeof(long double)*sigLen);
        lo = fwtBuff;
        hi = fwtBuff+loNum;

     for(int i=0; i<sigLen; i++)
      {
       switch(Bits)
        {
         case 12:                                             //212 format   12bit
          if(i%2 == 0)
           {
            tmp = MAKEWORD(lpc[0],lpc[1]&0x0f);
             if(tmp > 0x7ff)
              tmp |= 0xf000;
           }
          else
           {
            tmp = MAKEWORD(lpc[2],(lpc[1]&0xf0)>>4);
             if(tmp > 0x7ff)
              tmp |= 0xf000;
            lpc += 3;
           }

           if(i<hiNum)
            {hi[i] = (long double)tmp/(long double)UmV;}
           else
            {lo[i-hiNum] = (long double)tmp/(long double)UmV;}
         break;

         case 16:                                             //16format
          if(i<hiNum)
           {hi[i] = (long double)lps[i]/(long double)UmV;}
          else
           {lo[i-hiNum] = (long double)lps[i]/(long double)UmV;}
         break;

          case 0:
         case 32:                                             //32bit float
          if(i<hiNum)
           {hi[i] = (long double)lpf[i];}
          else
           {lo[i-hiNum] = (long double)lpf[i];}
         break;
        }

      }

       lo = tempBuff;
       hi = tempBuff+loNum;


      if(appdir)  //load filter for synthesis
      {
        char flt[256];
         strcpy(flt,appdir);
         strcat(flt,"\\filters\\");
         strcat(flt,fname);
         strcat(flt,".flt");

        filter = fopen(flt,"rt");
         if(filter)
         {
           tH = loadfilter(thL, thZ);
           tG = loadfilter(tgL, tgZ);
           H = loadfilter(hL, hZ);
           G = loadfilter(gL, gZ);
           fclose(filter);
         }
         else
         {
           CloseFile();
           return false;
         }
      }


    CloseFile();
    return true;
}
//-----------------------------------------------------------------------------
























//////////////////////////////////////////////////////////////////////////////
////////////////////////CLASS ANN/////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

Annotation::Annotation(PANNHDR p): qrsNum(0),annNum(0),auxNum(0),
                                   ANN(0),qrsANN(0),AUX(0)
{
     if(p)
     {
       memcpy(&ahdr, p, sizeof(ANNHDR));
     }
     else  //defaults
     {
       ahdr.minbpm = 40;
       ahdr.maxbpm = 200;
       ahdr.minQRS = 0.04;   //min QRS duration
       ahdr.maxQRS = 0.2;    //max QRS duration
       ahdr.minUmV = 0.2;    //min UmV of R,S peaks
       ahdr.minPQ = 0.07;    //min PQ duration
       ahdr.maxPQ = 0.20;    //max PQ duration
       ahdr.minQT = 0.21;    //min QT duration
       ahdr.maxQT = 0.48;    //max QT duration
       ahdr.pFreq = 9.0;     //cwt Hz for P wave
       ahdr.tFreq = 3.0;     //cwt Hz for T wave
     }
}
Annotation::~Annotation()
{
   if(ANN)
    {
     for (int i = 0; i < annNum;  i++)
      delete[] ANN[i];
      delete[] ANN;
    }
   if(qrsANN)
    {
     for (int i = 0; i < qrsNum;  i++)
      delete[] qrsANN[i];
      delete[] qrsANN;
    }
   if(AUX)
    {
     for (int i = 0; i < auxNum;  i++)
      delete[] AUX[i];
      delete[] AUX;
    }
}

//-----------------------------------------------------------------------------
// 10Hz cwt trans of signal
// spectrum in cwt class
// create qrsANN array   with qrsNum records  num of heart beats = qrsNum/2
//
int ** Annotation::GetQRS(long double *mass, int len, long double sr, wchar_t *fltdir, int stype)
{

   long double *pmass = (long double *)malloc(len*sizeof(long double));
   for(int i=0; i<len; i++)
    pmass[i] = mass[i];



    if(fltdir)
    {
      if(flt30Hz(pmass,len,sr,fltdir,stype) == false)   //pmass filed with filterd signal
      {
        delete[] pmass; 
        return 0;
      }

      // 2 continuous passes of CWT filtering
      /*if(flt30Hz(pmass,len,sr,fltdir,qNORM) == false)   //pmass filed with filterd signal
       return 0;
      if(stype == qNOISE)
       if(flt30Hz(pmass,len,sr,fltdir,qNORM) == false)
        return 0;*/
    }


    long double eCycle = (60.0/(long double)ahdr.maxbpm)-ahdr.maxQRS;  //secs
     if(int(eCycle*sr) <= 0)
     {
       eCycle = 0.1;
       ahdr.maxbpm = int(60.0/(ahdr.maxQRS+eCycle));
     }
   //////////////////////////////////////////////////////////////////////////////

     int lqNum=0;
    vector <int> QRS;    //clean QRS detected
     int add=0;

     while(pmass[add]) add+=int(0.1*sr);                       //skip QRS in begining
     while(pmass[add]==0) add++;                               //get  1st QRS

      QRS.push_back(add-1);
   /////////////////////////////////////////////////////////////////////////////
     for(int m=add; m<len; m++)                         //MAX 200bpm  [0,3 sec  min cario len]
      {
        m += int(ahdr.maxQRS*sr);                       //smpl + 0,20sec    [0,20 max QRS length]


         if(m>=len) m=len-1;
          add=0;

        //noise checking///////////////////////////////////////
        if(m + int(eCycle*sr) >= len)  //near end of signal
        {
           QRS.pop_back();
           break;
        }
        if(chkNoise(&pmass[m],eCycle*sr))      //smpl(0.10sec)+0,20sec in noise
        {
          if(lqNum != (int)QRS.size()-1)
           MA.push_back(QRS[QRS.size()-1]);     //push MA noise location

          QRS.pop_back();
          lqNum = QRS.size();

          //find for next possible QRS start
           while(chkNoise(&pmass[m],eCycle*sr))
           {
             m += int(eCycle*sr);
             if(m >= len-int(eCycle*sr)) break;   //end of signal
           }

           if(m >= len-int(eCycle*sr)) break;
           else
           {
              while(pmass[m]==0)
              {
                m++;
                if(m >= len) break;
              }
              if(m >= len) break;
              else
              {
                QRS.push_back(m-1);
                continue;
              }
           }
        }
        ////////////////////////////////////////////////////////


        while(pmass[m-add]==0.0) add++;                  //find back for QRS end


        if((m-add+1) - QRS[QRS.size()-1] > ahdr.minQRS*sr)       //QRS size > 0.04 sec
         QRS.push_back(m-add+2); //QRS end
        else
         QRS.pop_back();


        m += int(eCycle*sr);                                  //smpl + [0,20+0,10]sec    200bpm MAX
         if(len-m < int(sr/2)) break;



         while(pmass[m]==0 && (len-m)>=int(sr/2))            //find nearest QRS
          m++;

        if(len-m<int(sr/2)) break;  //end of data

         QRS.push_back(m-1);    //QRS begin
      }
   /////////////////////////////////////////////////////////////////////////////
    delete[] pmass;



    qrsNum = QRS.size()/2;

     if(qrsNum > 0)                               //         46: ?
     {                                           //          1: N    -1: nodata in **AUX
      qrsANN = new int*[2*qrsNum];                    // [samps] [type] [?aux data]
       for(int i=0; i<2*qrsNum; i++)
        qrsANN[i] = new int[3];

       for(int i=0; i<2*qrsNum; i++)
        {
         qrsANN[i][0] = QRS[i];                                     //samp

          if(i%2 == 0)
           qrsANN[i][1] = 1;                                        //type N
          else
           {
            qrsANN[i][1] = 40;                                      //type QRS)
             //if( (qrsANN[i][0]-qrsANN[i-1][0]) >= int(sr*0.12) || (qrsANN[i][0]-qrsANN[i-1][0]) <= int(sr*0.03) )
             // qrsANN[i-1][1] = 46;                                  //N or ? beat  (0.03?-0.12secs)
           }

         qrsANN[i][2] = -1;                                         //no aux data
        }

       return qrsANN;
     }
     else
      return 0;
}

 bool Annotation::chkNoise(long double *mass, int window)
{
    for(int i=0; i<window; i++)
     if(mass[i]) return true;

    return false;
}

bool Annotation::flt30Hz(long double *mass, int len, long double sr, wchar_t *fltdir, int stype)
{

     CWT cwt;
    ///////////CWT 10Hz transform//////////////////////////////////////////
     cwt.InitCWT(len,6,0,sr);           //gauss1 wavelet 6-index
     long double *pspec = cwt.CWTrans(mass,13);      //10-13 Hz transform?
      for(int i=0; i<len; i++)
       mass[i] = pspec[i];
     cwt.CloseCWT();

     //debug
      //ToTxt(L"10hz.txt",mass,len);



     wchar_t flt[256]=L"";
      wcscpy(flt,fltdir);

     switch(stype)
     {
      case qNORM:
       wcscat(flt,L"\\inter1.flt");
      break;

      case qNOISE:
       for(int i=0; i<len; i++)  //ridges
        mass[i] *= (fabsl(mass[i])/2.0);
       wcscat(flt,L"\\bior13.flt");
      break;
     }

     FWT fwt;
    ////////////FWT 0-30Hz removal//////////////////////////////////////////
     if(fwt.InitFWT(flt,mass,len) == false)
      return false;

      int J = ceill(log2(sr/23.0))-2;
      //trans///////////////////////////////////////////////////
       fwt.fwtTrans(J);

         int *Jnumbs = fwt.GetJnumbs(J,len);
         int hinum,lonum;
          fwt.HiLoNumbs(J,len,hinum,lonum);
         long double *lo = fwt.getSpec();
         long double *hi = fwt.getSpec() + (len-hinum);


         int window;   //2.0sec window
        for(int j=J; j>0; j--)
        {
          window = (2.0*sr)/powl(2.0,(long double)j);    //2.0sec interval

          Denoise(hi,Jnumbs[J-j],window,0,false);      //hard,MINIMAX denoise [30-...Hz]
           hi += Jnumbs[J-j];
        }
        for(int i=0; i<lonum; i++)                    //remove [0-30Hz]
         lo[i] = 0.0;

     //synth/////////////////////////////
       fwt.fwtSynth(J);

       for(int i=0; i<fwt.getSpecSize(); i++)
        mass[i] = lo[i];
       for(int i=len-(len-fwt.getSpecSize()); i<len; i++)
        mass[i] = 0.0;

     fwt.CloseFWT();

    //debug
      //ToTxt(L"10hz(intr1).txt",mass,len);
     return true;
}
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
// find ectopic beats in HRV data
void Annotation::GetECT(int **ann, int qrsnum, long double sr)
{
    if(qrsnum < 3)
     return;

    vector<long double> RRs;
    for(int n=0; n<qrsnum-1; n++)
     RRs.push_back((long double)(ann[n*2+2][0] - ann[n*2][0])/sr);   //qrsNum-1 rr's
    RRs.push_back(RRs[RRs.size()-1]);

  //  [RR1  RR2  RR3]   RR2 beat classification
     long double rr1,rr2,rr3;
    for(int n=-2; n<(int)RRs.size()-2; n++)
    {
      if(n == -2)
      {
        rr1 = RRs[1];  //2
        rr2 = RRs[0];  //1
        rr3 = RRs[0];  //0
      }
      else if(n == -1)
      {
        rr1 = RRs[1];
        rr2 = RRs[0];
        rr3 = RRs[1];
      }
      else
      {
        rr1 = RRs[n];
        rr2 = RRs[n+1];
        rr3 = RRs[n+2];
      }

       if(60.0/rr1 < ahdr.minbpm || 60.0/rr1 > ahdr.maxbpm) //if RR's within 40-200bpm
         continue;
       if(60.0/rr2 < ahdr.minbpm || 60.0/rr2 > ahdr.maxbpm) //if RR's within 40-200bpm
         continue;
       if(60.0/rr3 < ahdr.minbpm || 60.0/rr3 > ahdr.maxbpm) //if RR's within 40-200bpm
         continue;

       if(1.15*rr2 < rr1 && 1.15*rr2 < rr3)
       {
         ann[n*2+4][1] = 46;
         continue;
       }
       if( fabsl(rr1-rr2)<0.3 && rr1<0.8 && rr2<0.8 && rr3>2.4*(rr1+rr2) )
       {
         ann[n*2+4][1] = 46;
         continue;
       }
       if( fabsl(rr1-rr2)<0.3 && rr1<0.8 && rr2<0.8 && rr3>2.4*(rr2+rr3) )
       {
         ann[n*2+4][1] = 46;
         continue;
       }
    }
}
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
//out united annotation with QRS PT////////////////////////////////////////////
// **ann [PQ,JP] pairs
int ** Annotation::GetPTU(long double *mass, int len, long double sr, wchar_t *fltdir, int **ann, int qrsnum)
{
    int size, annPos;
    int T1=-1,T=-1,T2=-1, Twaves=0;
    int P1=-1,P=-1,P2=-1, Pwaves=0;

     CWT cwt;
    vector <int> Pwave;
    vector <int> Twave;                             //Twave [ ( , T , ) ]
     long double *buff;                             //      [ 0 , 0 , 0 ]  0 no Twave
     long double min,max;                           //min,max for gaussian1 wave, center is zero crossing

     long double rr;   //rr interval

     bool sign;
     bool twave,pwave;


     int add = 0;//int(sr*0.04);  //prevent imprecise QRS end detection

      int maNum=0;
     for(int n=0; n<qrsnum-1; n++)
     {
        annPos = ann[n*2+1][0];                //i
        size = ann[n*2+2][0]-ann[n*2+1][0];    //i   size of  (QRS) <----> (QRS)

        bool maNs=false;
        for(int i=maNum; i<(int)MA.size(); i++)
        {
           if(MA[i]>ann[n*2+1][0] && MA[i]<ann[n*2+2][0])
           {
             Pwave.push_back(0);
             Pwave.push_back(0);
             Pwave.push_back(0);
             Twave.push_back(0);
             Twave.push_back(0);
             Twave.push_back(0);
             maNum++;
             maNs=true;
             break;
           }
        }
        if(maNs) continue;


        rr = (long double)(ann[n*2+2][0]-ann[n*2][0])/sr;
        if(60.0/rr < ahdr.minbpm || 60.0/rr > ahdr.maxbpm-20)  //check if normal RR interval (40bpm - 190bpm)
        {
          Pwave.push_back(0);
          Pwave.push_back(0);
          Pwave.push_back(0);
          Twave.push_back(0);
          Twave.push_back(0);
          Twave.push_back(0);
          continue;
        }


        ///////////////search for TWAVE///////////////////////////////////////////////////////////

          if(sr*ahdr.maxQT-(ann[n*2+1][0]-ann[n*2+0][0]) > size-add)
           size = size - add;
          else
           size = sr*ahdr.maxQT-(ann[n*2+1][0]-ann[n*2+0][0]) - add;


          //long double avg = Mean(mass+annPos+add,size);         //avrg extension on boundaries
          //long double lvl,rvl;
          //lvl = mass[annPos+add];
          //rvl = mass[annPos+add+size-1];
          cwt.InitCWT(size,6,0,sr);                              //6-Gauss1 wlet
          buff = cwt.CWTrans(mass+annPos+add,ahdr.tFreq);//,false,lvl,rvl);   //3Hz transform  buff = size-2*add

           //cwt.ToTxt(L"debugS.txt",mass+annPos+add,size);    //T wave
           //cwt.ToTxt(L"debugC.txt",buff,size);               //T wave spectrum

          MinMax(buff,size,min,max);
          for(int i=0; i<size; i++)
          {
             if(buff[i]==min) T1 = i+ annPos + add;
             if(buff[i]==max) T2 = i+ annPos + add;
          }
          if(T1>T2)swap(T1,T2);

          //additional constraints on [T1 T T2] duration, symmetry, QT interval
            twave = false;
           if((buff[T1-annPos-add]<0 && buff[T2-annPos-add]>0) || (buff[T1-annPos-add]>0 && buff[T2-annPos-add]<0))
            twave = true;
           if(twave)
           {
              if((long double)(T2-T1)>=0.09*sr)// && (long double)(T2-T1)<=0.24*sr)   //check for T wave duration
              {
                twave = true;                  //QT interval = .4*sqrt(RR)
                if((long double)(T2-ann[n*2+0][0])>=ahdr.minQT*sr && (long double)(T2-ann[n*2+0][0])<=ahdr.maxQT*sr)
                 twave = true;
                else
                 twave = false;
              }
              else
               twave = false;
           }

          if(twave)
          {
             if(buff[T1-annPos-add] > 0) sign = true;
             else sign = false;

             for(int i=T1-annPos-add; i<T2-annPos-add; i++)
             {
               if(sign)
               { if(buff[i] > 0) continue; }
               else
               { if(buff[i] < 0) continue; }

                T = i+annPos + add;
                break;
             }

              //check for T wave symetry//////////////////////////
              long double ratio;
               if(T2-T < T-T1) ratio = (long double)(T2-T)/(long double)(T-T1);
               else ratio = (long double)(T-T1)/(long double)(T2-T);
              ////////////////////////////////////////////////////

              if(ratio < 0.4) //not a T wave
              {
                Twave.push_back(0);
                Twave.push_back(0);
                Twave.push_back(0);
                twave = false;
              }
              else
              {
                //adjust center of T wave
                //smooth it with gaussian, find max ?
                //cwt.ToTxt(L"debugS.txt",mass+annPos+add,size);
                int Tcntr = findTmax(mass+T1, T2-T1);
                if(Tcntr != -1)
                {
                  Tcntr += T1;
                 if(abs((Tcntr-T1)-((T2-T1)/2))<abs((T-T1)-((T2-T1)/2)))  //which is close to center 0-cross or T max
                  T = Tcntr;
                }

                Twaves++;
                Twave.push_back(T1);
                Twave.push_back(T);
                Twave.push_back(T2);
              }
          }
          else  //no T wave???    empty (QRS) <-------> (QRS)
          {
             Twave.push_back(0);
             Twave.push_back(0);
             Twave.push_back(0);
          }
          T=-1;
   ///////////////search for TWAVE///////////////////////////////////////////////////////////
          cwt.CloseCWT();





   ///////////////search for PWAVE///////////////////////////////////////////////////////////

         size = ann[n*2+2][0]-ann[n*2+1][0];  //n   size of  (QRS) <----> (QRS)

         if(sr*ahdr.maxPQ < size)
          size = sr*ahdr.maxPQ;

         if(twave)
         {
           if(T2 > ann[n*2+2][0]-size - int(0.04*sr) )     // pwave wnd far from Twave at least on 0.02sec
            size -= T2 - (ann[n*2+2][0]-size - int(0.04*sr));
         }
         int size23 = (ann[n*2+2][0]-ann[n*2+1][0])-size;

         //size -= 0.02*sr;   //impresize QRS begin detection
         if(size <= 0.03*sr)
         {
           Pwave.push_back(0);
           Pwave.push_back(0);
           Pwave.push_back(0);
           continue;
         }


          //avg = Mean(mass+annPos+size23,size);                     //avrg extension on boundaries
          //lvl = mass[annPos+size23];
          //rvl = mass[annPos+size23+size-1];
          cwt.InitCWT(size,6,0,sr);                                           //6-Gauss1 wlet
          buff = cwt.CWTrans(mass+annPos+size23,ahdr.pFreq);//,false,lvl,rvl);    //9Hz transform  buff = size-2/3size

           //cwt.ToTxt(L"debugS.txt",mass+annPos+size23,size);
           //cwt.ToTxt(L"debugC.txt",buff,size);

          MinMax(buff,size,min,max);
          for(int i=0; i<size; i++)
          {
             if(buff[i]==min) P1 = i+ annPos + size23;
             if(buff[i]==max) P2 = i+ annPos + size23;
          }
          if(P1>P2)swap(P1,P2);

          //additional constraints on [P1 P P2] duration, symmetry, PQ interval
            pwave = false;
           if((buff[P1-annPos-size23]<0 && buff[P2-annPos-size23]>0) || (buff[P1-annPos-size23]>0 && buff[P2-annPos-size23]<0))
            pwave = true;
           if(pwave)
           {
              if((long double)(P2-P1)>=0.03*sr && (long double)(P2-P1)<=0.15*sr)   //check for P wave duration  9Hz0.03 5Hz0.05
              {
                pwave = true;                //PQ interval = [0.07 - 0.12,0.20]
                if((long double)(ann[n*2+2][0]-P1)>=ahdr.minPQ*sr && (long double)(ann[n*2+2][0]-P1)<=ahdr.maxPQ*sr)
                 pwave = true;
                else
                 pwave = false;
              }
              else
               pwave = false;
           }

          if(pwave)
          {
             if(buff[P1-annPos-size23] > 0) sign = true;
             else sign = false;

             for(int i=P1-annPos-size23; i<P2-annPos-size23; i++)
             {
               if(sign)
               { if(buff[i] > 0) continue; }
               else
               { if(buff[i] < 0) continue; }

                P = i+ annPos + size23;
                break;
             }

              //check for T wave symetry//////////////////////////
              long double ratio;
               if(P2-P < P-P1) ratio = (long double)(P2-P)/(long double)(P-P1);
               else ratio = (long double)(P-P1)/(long double)(P2-P);
              ////////////////////////////////////////////////////

              if(ratio < 0.4f) //not a P wave
              {
                Pwave.push_back(0);
                Pwave.push_back(0);
                Pwave.push_back(0);
              }
              else
              {
                Pwaves++;
                Pwave.push_back(P1);
                Pwave.push_back(P);
                Pwave.push_back(P2);
              }
          }
          else
          {
            Pwave.push_back(0);
            Pwave.push_back(0);
            Pwave.push_back(0);
          }
          P1=-1;P=-1;P2=-1;
    ///////////////search for PWAVE///////////////////////////////////////////////////////////
          cwt.CloseCWT();

     }






    /////////////////get q,r,s peaks//////////////////////////////////////////////////////////
    // on a denoised signal

     int peaksnum=0;
      int Q,R,S;
     vector <int> qrsPeaks;      //q,r,s peaks [ q , r , s ]
                                 //            [ 0,  R , 0 ]  zero if not defined
     vector <char> qrsTypes;     //[q,r,s] or [_,R,s], etc...


     for(int n=0; n<qrsnum; n++) //fill with zeros
     {
       for(int i=0; i<3; i++)
       {
         qrsPeaks.push_back(0);
         qrsTypes.push_back(' ');
       }
     }


     buff = (long double *)malloc(len*sizeof(long double));
     for(int i=0; i<len; i++)
      buff[i] = mass[i];

      ECGDenoise enoise;
      enoise.InitDenoise(fltdir,buff,len,sr);
      if(enoise.LFDenoise())
      {
         //ToTxt(L"lf.txt",buff,len);
         long double *pbuff;
         for(int n=0; n<qrsnum; n++)
         {
           annPos = ann[n*2][0];   //PQ
           size = ann[n*2+1][0]-ann[n*2][0] + 1; //PQ-Jpnt, including Jpnt

           pbuff = &buff[annPos];

           Q = -1;
           findRS(pbuff,size,R,S,ahdr.minUmV);
           if(R != -1) R += annPos;
           if(S != -1) S += annPos;

           if(R!=-1 && S!=-1)    // Rpeak > 0mV Speak < 0mV
           {
             if(S<R)  //check for S
             {
               if(buff[R]>-buff[S])
               {
                Q = S;
                S = -1;

                size = ann[n*2+1][0]-R + 1;  //including Jpnt
                pbuff = &buff[R];
                S = finds(pbuff,size,0.05);
                 if(S != -1) S += R;
               }
             }
             else      //check for Q
             {
               size = R-annPos + 1;  //including R peak
               Q = findq(pbuff,size,0.05);
                if(Q != -1) Q += annPos;
             }
           }
           //else if only S
           else if(S!=-1)  //find small r if only S detected  in rS large T lead
           {
              size = S-annPos + 1; //including S peak
              pbuff = &buff[annPos];
              R = findr(pbuff,size,0.05);
               if(R != -1) R += annPos;
           }
           //else if only R
           else if(R!=-1)  //only R find small q,s
           {
              size = R-annPos + 1;   //including R peak
              Q = findq(pbuff,size,0.05);
               if(Q != -1) Q += annPos;
              size = ann[n*2+1][0]-R + 1;  //including Jpnt
              pbuff = &buff[R];
              S = finds(pbuff,size,0.05);
               if(S != -1) S += R;
           }


           //put peaks to qrsPeaks vector
           if(R==-1 && S==-1)   //no peaks
           {
             ann[n*2][1] = 16;   //ARTEFACT
            //remove P,T
             if(n != 0)
             {
               if(Pwave[3*(n-1)])
               {
                 Pwaves--;
                 Pwave[3*(n-1)] = 0;
                 Pwave[3*(n-1)+1] = 0;
                 Pwave[3*(n-1)+2] = 0;
               }
             }
             if(n != qrsnum-1)
             {
               if(Twave[3*n])
               {
                 Twaves--;
                 Twave[3*n] = 0;
                 Twave[3*n+1] = 0;
                 Twave[3*n+2] = 0;
               }
             }
           }
           if(Q != -1)
           {
             peaksnum++;
             qrsPeaks[n*3] = Q;
             if(fabsl(buff[Q]) > 0.5)
              qrsTypes[n*3] = 17; //'Q';
             else
              qrsTypes[n*3] = 15; //'q';
           }
           if(R != -1)
           {
             peaksnum++;
             qrsPeaks[n*3+1] = R;
             if(fabsl(buff[R]) > 0.5)
              qrsTypes[n*3+1] = 48; //'R';
             else
              qrsTypes[n*3+1] = 47; //'r';
           }
           if(S != -1)
           {
             peaksnum++;
             qrsPeaks[n*3+2] = S;
             if(fabsl(buff[S]) > 0.5)
              qrsTypes[n*3+2] = 50; //'S';
             else
              qrsTypes[n*3+2] = 49; //'s';
           }
         }
      }

     free(buff);
    /////////////////get q,r,s peaks//////////////////////////////////////////////////////////






    ///////////////////////// complete annotation array///////////////////////////////////////
     maNum = 0;

    //Pwave vec size = Twave vec size
    annNum = Pwaves*3 + qrsnum*2+peaksnum + Twaves*3 + (int)MA.size();   //P1 P P2 [QRS] T1 T T2  noise annotation
     if(annNum > qrsnum)                         //42-(p 43-p) 24-Pwave
     {                                           //44-(t 45-t) 27-Twave
      ANN = new int*[annNum];                    // [samps] [type] [?aux data]
       for(int i=0; i<annNum; i++)
        ANN[i] = new int[3];

         int index=0;   //index to ANN
         int qindex=0;  //index to qrsANN

         for(int i=0; i<(int)Twave.size(); i+=3)     //Twave=Pwaves=qrsPeaks size
         {
            //QRS complex
            ANN[index][0] = ann[qindex][0];           //(QRS
            ANN[index][1] = ann[qindex][1];           //
            ANN[index++][2] = ann[qindex++][2];       //aux
             if(qrsPeaks[i])   //q
             {
               ANN[index][0] = qrsPeaks[i];
               ANN[index][1] = qrsTypes[i];
               ANN[index++][2] = -1;              //no aux
             }
             if(qrsPeaks[i+1]) //r
             {
               ANN[index][0] = qrsPeaks[i+1];
               ANN[index][1] = qrsTypes[i+1];
               ANN[index++][2] = -1;              //no aux
             }
             if(qrsPeaks[i+2]) //s
             {
               ANN[index][0] = qrsPeaks[i+2];
               ANN[index][1] = qrsTypes[i+2];
               ANN[index++][2] = -1;              //no aux
             }
            ANN[index][0] = ann[qindex][0];           //QRS)
            ANN[index][1] = ann[qindex][1];           //
            ANN[index++][2] = ann[qindex++][2];       //aux

            //T wave
            if(Twave[i])
            {
             ANN[index][0] = Twave[i];                //(t
             ANN[index][1] = 44;
             ANN[index++][2] = -1;                    //no aux
             ANN[index][0] = Twave[i+1];              //T
             ANN[index][1] = 27;
             ANN[index++][2] = -1;                    //no aux
             ANN[index][0] = Twave[i+2];              //t)
             ANN[index][1] = 45;
             ANN[index++][2] = -1;                    //no aux
            }
            //P wave
            if(Pwave[i])
            {
             ANN[index][0] = Pwave[i];                //(t
             ANN[index][1] = 42;
             ANN[index++][2] = -1;                    //no aux
             ANN[index][0] = Pwave[i+1];              //T
             ANN[index][1] = 24;
             ANN[index++][2] = -1;                    //no aux
             ANN[index][0] = Pwave[i+2];              //t)
             ANN[index][1] = 43;
             ANN[index++][2] = -1;                    //no aux
            }

            if(!Twave[i] && !Pwave[i])             //check for MA noise
            {
              for(int m=maNum; m<(int)MA.size(); m++)
              {
                if(MA[m]>ann[qindex-1][0] && MA[m]<ann[qindex][0])
                {
                  ANN[index][0] = MA[m];      //Noise
                  ANN[index][1] = 14;
                  ANN[index++][2] = -1;       //no aux
                  maNum++;
                  break;
                }
              }
            }
         }

        //last QRS complex
         int ii=3*(qrsnum-1);
         ANN[index][0] = ann[qindex][0];           //(QRS
         ANN[index][1] = ann[qindex][1];           //
         ANN[index++][2] = ann[qindex++][2];       //aux
          if(qrsPeaks[ii])   //q
          {
            ANN[index][0] = qrsPeaks[ii];
            ANN[index][1] = qrsTypes[ii];
            ANN[index++][2] = -1;              //no aux
          }
          if(qrsPeaks[ii+1]) //r
          {
            ANN[index][0] = qrsPeaks[ii+1];
            ANN[index][1] = qrsTypes[ii+1];
            ANN[index++][2] = -1;              //no aux
          }
          if(qrsPeaks[ii+2]) //s
          {
            ANN[index][0] = qrsPeaks[ii+2];
            ANN[index][1] = qrsTypes[ii+2];
            ANN[index++][2] = -1;              //no aux
          }
         ANN[index][0] = ann[qindex][0];           //QRS)
         ANN[index][1] = ann[qindex][1];           //
         ANN[index++][2] = ann[qindex++][2];       //aux

        //check if noise after last qrs
         if(maNum < (int)MA.size())
         {
           if(MA[maNum]>ann[qindex-1][0])     
           {
             ANN[index][0] = MA[maNum];      //Noise
             ANN[index][1] = 14;
             ANN[index++][2] = -1;
           }
         }

        return ANN;
     }
    else
     return 0;
}

 void Annotation::findRS(long double *mass, int len, int &R, int &S, long double err)  //find RS or QR
{
   long double tmp,min,max;
   MinMax(mass,len, min,max);

   R = -1; S = -1;
   if(!( max<0.0 || max==mass[0] || max==mass[len-1] || max<err ))//(fabsl(max-mass[0])<err && fabsl(max-mass[len-1])<err) ))
   {
     for(int i=1; i<len-1; i++)
      if(mass[i] == max)
      {
        R=i;
        break;
      }
   }
   if(!( min>0.0 || min==mass[0] || min==mass[len-1] || -min<err ))//(fabsl(min-mass[0])<err && fabsl(min-mass[len-1])<err) ))
   {
     for(int i=1; i<len-1; i++)
      if(mass[i] == min)
      {
        S=i;
        break;
      }
   }
}

int Annotation::findTmax(long double *mass, int len)   //find T max/min peak position
{
   long double min,max;
   MinMax(mass,len, min,max);

   int tmin=-1,tmax=-1;
   for(int i=0; i<len; i++)
   {
    if(mass[i] == max)
    {
      tmax = i;
      break;
    }
   }
   for(int i=0; i<len; i++)
   {
    if(mass[i] == min)
    {
      tmin = i;
      break;
    }
   }

   //max closest to the center
   if(tmin==-1 || tmax==-1) //??no max min found
    return -1;
   else
   {
     if(abs(tmax-(len/2))<abs(tmin-(len/2)))
      return tmax;
     else
      return tmin;
   }
}

 int Annotation::findr(long double *mass, int len, long double err)  //find small r in PQ-S
{
   long double tmp,min,max;
   MinMax(mass,len, min,max);

   if(max<0.0 || max==mass[0] || max==mass[len-1] || fabsl(max-mass[0])<err) return -1;
   else tmp = max;

   for(int i=1; i<len-1; i++)
   {
    if(mass[i] == tmp)
     return i;
   }
   return -1;
}
 int Annotation::findq(long double *mass, int len, long double err)  //find small q in PQ-R
{
   long double tmp,min,max;
   MinMax(mass,len, min,max);

   if(min>0.0 || min==mass[0] || min==mass[len-1] || fabsl(min-mass[0])<err) return -1;
   else tmp = min;

   for(int i=1; i<len-1; i++)
   {
    if(mass[i] == tmp)
     return i;
   }
   return -1;
}
 int Annotation::finds(long double *mass, int len, long double err)  //find small s in R-Jpnt
{
   long double tmp,min,max;
   MinMax(mass,len, min,max);

   if(min>0.0 || min==mass[0] || min==mass[len-1] || fabsl(min-mass[len-1])<err) return -1;
   else tmp = min;

   for(int i=1; i<len-1; i++)
   {
    if(mass[i] == tmp)
     return i;
   }
   return -1;
}

//-----------------------------------------------------------------------------





//-----------------------------------------------------------------------------
void Annotation::AddANN(int add)
{
   for(int i=0; i<qrsNum; i++)
    qrsANN[i][0] += add;

   for(int i=0; i<annNum; i++)
    ANN[i][0] += add;
}
//-----------------------------------------------------------------------------


// SaveANN (**aux)   aux data
//-----------------------------------------------------------------------------
bool Annotation::SaveANN(wchar_t *name, int **ann, int nums)
{
     int samps;
    unsigned short anncode=0;
    unsigned short type;
     DWORD bytes;
     char buff[6];


  fp = CreateFileW(name,GENERIC_WRITE | GENERIC_READ,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);

     samps = ann[0][0];
      sprintf(buff,"%c%c%c%c%c%c",0,0xEC,HIWORD(LOBYTE(samps)),HIWORD(HIBYTE(samps)),LOWORD(LOBYTE(samps)),LOWORD(HIBYTE(samps)));
      WriteFile(fp,buff,6,&bytes,0);
     type = ann[0][1];
     anncode |= (type<<10);
      sprintf(buff,"%c%c",(char)LOBYTE(anncode),(char)HIBYTE(anncode));
      WriteFile(fp,buff,2,&bytes,0);


      for(int i=1; i<nums; i++)
       {
        samps = ann[i][0] - ann[i-1][0];
         anncode = 0;
        type = ann[i][1];


         if(samps > 1023)
          {
           sprintf(buff,"%c%c%c%c%c%c",0,0xEC,HIWORD(LOBYTE(samps)),HIWORD(HIBYTE(samps)),LOWORD(LOBYTE(samps)),LOWORD(HIBYTE(samps)));
           WriteFile(fp,buff,6,&bytes,0);
          }
         else
           anncode = samps;


          anncode |= (type<<10);

        sprintf(buff,"%c%c",(char)LOBYTE(anncode),(char)HIBYTE(anncode));
        WriteFile(fp,buff,2,&bytes,0);
       }
     sprintf(buff,"%c%c",0,0);
     WriteFile(fp,buff,2,&bytes,0);


   CloseHandle(fp);
   return true;
}

bool Annotation::SaveQTseq(wchar_t *name, int **ann, int annsize, long double sr, int len)
{
   vector <long double> QT;
   int q=0,t=0;


     for(int i=0; i<annsize; i++)
     {
        switch(ann[i][1])
        {
         case 14:            //noise
         case 15:            //q
         case 16:            //artifact
         case 17:            //Q
         case 18:            //ST change
         case 19:            //T change
         case 20:            //systole
         case 21:            //diastole
         case 22:
         case 23:
         case 24:            //P
         case 25:
         case 26:
         case 27:
         case 28:
         case 29:
         case 30:
         case 31:
         case 32:
         case 33:
         case 36:
         case 37:
         case 39:
         case 40:
         case 42:  //(p
         case 43:  //p)
         case 44:  //(t
         case 47:  //r
         case 48:  //R
         case 49:  //s
         case 50:  //S
          continue;
        }

          if(ann[i][1] == 45)    //45 - t)
          {
             t = ann[i][0];
             if(q < t)
               QT.push_back((long double)(t-q)/sr);
          }
          else
          {
            /*if(i+1<annsize && (ann[i+1][1]==47 || ann[i+1][1]==48))  //r only
             q = ann[i+1][0];
            else if(i+2<annsize && (ann[i+2][1]==47 || ann[i+2][1]==48))  //q,r
             q = ann[i+2][0];
            else*/
             q = ann[i][0];
          }
     }


    if(QT.size())
    {
       DATAHDR hdr;
        memset(&hdr,0,sizeof(DATAHDR));

        memcpy(hdr.hdr,"DATA",4);
        hdr.size = QT.size();
        hdr.sr = float( (long double)QT.size() / ((long double)len/sr) );
        hdr.bits = 32;
        hdr.umv = 1;

      SaveFile(name, &QT[0], &hdr);
      
      return true;
    }
    else
     return false;
}

bool Annotation::SavePQseq(wchar_t *name, int **ann, int annsize, long double sr, int len)
{
   vector <long double> PQ;
   int p=len,q=0;


     for(int i=0; i<annsize; i++)
     {
        switch(ann[i][1])
        {
         case 14:            //noise
         case 15:            //q
         case 16:            //artifact
         case 17:            //Q
         case 18:            //ST change
         case 19:            //T change
         case 20:            //systole
         case 21:            //diastole
         case 22:
         case 23:
         case 24:            //P
         case 25:
         case 26:
         case 27:
         case 28:
         case 29:
         case 30:
         case 31:
         case 32:
         case 33:
         case 36:
         case 37:
         case 39:
         case 40:
         case 43:  //p)
         case 44:  //(t
         case 45:  //t)
         case 47:  //r
         case 48:  //R
         case 49:  //s
         case 50:  //S
          continue;
        }

          if(ann[i][1] == 42)    //42 - (p
             p = ann[i][0];
          else
          {
             q = ann[i][0];
             if(p < q)
             {
               PQ.push_back((long double)(q-p)/sr);
               p=len;
             }
          }
     }

    if(PQ.size())
    {
     DATAHDR hdr;
      memset(&hdr,0,sizeof(DATAHDR));

      memcpy(hdr.hdr,"DATA",4);
      hdr.size = PQ.size();
      hdr.sr = float( (long double)PQ.size() / ((long double)len/sr) );
      hdr.bits = 32;
      hdr.umv = 1;

      SaveFile(name,&PQ[0],&hdr);

      return true;
    }
    else
     return false;
}

bool Annotation::SavePPseq(wchar_t *name, int **ann, int annsize, long double sr, int len)
{
   vector <long double> PP;
    int p1,p2;

    for(int i=0; i<annsize; i++)
    {
        if(ann[i][1] == 42)       //42 - (p
          p1 = ann[i][0];
        else if(ann[i][1] == 43)    //43 - p)
        {
          p2 = ann[i][0];
          PP.push_back((long double)(p2-p1)/sr);
        }
    }

    if(PP.size())
    {
     DATAHDR hdr;
      memset(&hdr,0,sizeof(DATAHDR));

      memcpy(hdr.hdr,"DATA",4);
      hdr.size = PP.size();
      hdr.sr = float( (long double)PP.size() / ((long double)len/sr) );
      hdr.bits = 32;
      hdr.umv = 1;

      SaveFile(name,&PP[0],&hdr);

      return true;
    }
    else
     return false;
}

bool Annotation::GetRRseq(int **ann, int nums, long double sr, vector<long double> *RR, vector<int> *RRpos)
{
   int add=-1;
   long double rr, r1,r2;

   RR->clear();
   RRpos->clear();
   
   //R peak or S peak annotation
   bool rrs=true;
   int rNum=0,sNum=0;
   for(int i=0; i<nums; i++)
   {
     if(ann[i][1]==47 || ann[i][1]==48) rNum++;
     else if(ann[i][1]==49 || ann[i][1]==50) sNum++;
   }
   if(int(1.2f*(float)rNum) < sNum)
     rrs=false;  //R peaks less than S ones


    for(int i=0; i<nums; i++)
     {
       switch(ann[i][1])
        {
         case 0:    //non beats
         case 15:   //q
         case 17:   //Q
         case 18:   //ST change
         case 19:   //T change
         case 20:   //systole
         case 21:   //diastole
         case 22:
         case 23:
         case 24:
         case 25:
         case 26:
         case 27:
         case 28:
         case 29:
         case 30:
         case 31:
         case 32:
         case 33:
         case 36:
         case 37:
         case 40:   //')' J point
         case 42:   //(p
         case 43:   //p)
         case 44:   //(t
         case 45:   //t)
         case 47:   //r
         case 48:   //R
         case 49:   //s
         case 50:   //S
          continue;

         case 14:   //noise
         case 16:   //artifact
          add = -1;
          continue;
        }

       if(add != -1)
       {
        //annotation on RRs peaks
        if(rrs)
        {
         if(i+1<nums && (ann[i+1][1]==47 || ann[i+1][1]==48))  //r only
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==47 || ann[i+2][1]==48))  //q,r
          r2 = ann[i+2][0];
         else //(ann[i][1]==N,ECT,...)  //no detected R only S
          r2 = ann[i][0];

         if(add+1<nums && (ann[add+1][1]==47 || ann[add+1][1]==48))
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==47 || ann[add+2][1]==48))
          r1 = ann[add+2][0];
         else //(ann[add][1]==N,ECT,...) //no detected R only S
          r1 = ann[add][0];
        }
        //annotation on S peaks
        else
        {
         if(i+1<nums && (ann[i+1][1]==40))  //N)
          r2 = ann[i][0];
         else if(i+1<nums && (ann[i+1][1]==49 || ann[i+1][1]==50))  //Sr
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==49 || ann[i+2][1]==50))  //rS
          r2 = ann[i+2][0];
         else if(i+3<nums && (ann[i+3][1]==49 || ann[i+3][1]==50))  //errQ rS
          r2 = ann[i+3][0];
         else if(i+1<nums && (ann[i+1][1]==47 || ann[i+1][1]==48))  //no S
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==47 || ann[i+2][1]==48))  //no S
          r2 = ann[i+2][0];

         if(add+1<nums && (ann[add+1][1]==40))  //N)
          r1 = ann[add][0];
         else if(add+1<nums && (ann[add+1][1]==49 || ann[add+1][1]==50))
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==49 || ann[add+2][1]==50))
          r1 = ann[add+2][0];
         else if(add+3<nums && (ann[add+3][1]==49 || ann[add+3][1]==50))
          r1 = ann[add+3][0];
         else if(add+1<nums && (ann[add+1][1]==47 || ann[add+1][1]==48))  //no S
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==47 || ann[add+2][1]==48))  //no S
          r1 = ann[add+2][0];
        }

         rr = 60.0 / ((r2-r1)/sr);
         if(rr >= ahdr.minbpm && rr <= ahdr.maxbpm)
         {
           RR->push_back( rr );       //in bpm
           RRpos->push_back( r1 );
         }
       }
       add = i;
     }

    if(RR->size())
     return true;
    else
     return false;
}

bool Annotation::SaveRRseq(wchar_t *name, int **ann, int nums, long double sr, int len)
{
  vector <long double> RR;
    int add=-1;
   long double rr, r1,r2;

   //R peak or S peak annotation////////////////////////////
   bool rrs=true;
   int rNum=0,sNum=0;
   for(int i=0; i<nums; i++)
   {
     if(ann[i][1]==47 || ann[i][1]==48) rNum++;
     else if(ann[i][1]==49 || ann[i][1]==50) sNum++;
   }
   if(int(1.1f*(float)rNum) < sNum)
   {
     rrs=false;  //R peaks less than S ones
     wcscat(name, L"_SS.dat");
   }
   else
    wcscat(name, L"_RR.dat");

   ////////////////////////////////////////////////////////


    for(int i=0; i<nums; i++)
     {
       switch(ann[i][1])
        {
         case 0:    //non beats
         case 15:   //q
         case 17:   //Q
         case 18:   //ST change
         case 19:   //T change
         case 20:   //systole
         case 21:   //diastole
         case 22:
         case 23:
         case 24:
         case 25:
         case 26:
         case 27:
         case 28:
         case 29:
         case 30:
         case 31:
         case 32:
         case 33:
         case 36:
         case 37:
         case 40:   //')' J point
         case 42:   //(p
         case 43:   //p)
         case 44:   //(t
         case 45:   //t)
         case 47:   //r
         case 48:   //R
         case 49:   //s
         case 50:   //S
          continue;

         case 14:   //noise
         case 16:   //artifact
          add = -1;
          continue;
        }

       if(add != -1)
       {
        //annotation on RRs peaks
        if(rrs)
        {
         if(i+1<nums && (ann[i+1][1]==47 || ann[i+1][1]==48))  //r only
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==47 || ann[i+2][1]==48))  //q,r
          r2 = ann[i+2][0];
         else //(ann[i][1]==N,ECT,...)  //no detected R only S
          r2 = ann[i][0];

         if(add+1<nums && (ann[add+1][1]==47 || ann[add+1][1]==48))
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==47 || ann[add+2][1]==48))
          r1 = ann[add+2][0];
         else //(ann[add][1]==N,ECT,...) //no detected R only S
          r1 = ann[add][0];
        }
        //annotation on S peaks
        else
        {
         if(i+1<nums && (ann[i+1][1]==40))  //N)
          r2 = ann[i][0];
         else if(i+1<nums && (ann[i+1][1]==49 || ann[i+1][1]==50))  //Sr
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==49 || ann[i+2][1]==50))  //rS
          r2 = ann[i+2][0];
         else if(i+3<nums && (ann[i+3][1]==49 || ann[i+3][1]==50))  //errQ rS
          r2 = ann[i+3][0];
         else if(i+1<nums && (ann[i+1][1]==47 || ann[i+1][1]==48))  //no S
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==47 || ann[i+2][1]==48))  //no S
          r2 = ann[i+2][0];

         if(add+1<nums && (ann[add+1][1]==40))  //N)
          r1 = ann[add][0];
         else if(add+1<nums && (ann[add+1][1]==49 || ann[add+1][1]==50))
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==49 || ann[add+2][1]==50))
          r1 = ann[add+2][0];
         else if(add+3<nums && (ann[add+3][1]==49 || ann[add+3][1]==50))
          r1 = ann[add+3][0];
         else if(add+1<nums && (ann[add+1][1]==47 || ann[add+1][1]==48))  //no S
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==47 || ann[add+2][1]==48))  //no S
          r1 = ann[add+2][0];
        }

         rr = 60.0 / ((r2-r1)/sr);
         if(rr >= ahdr.minbpm && rr <= ahdr.maxbpm)
          RR.push_back( rr );       //in bpm
       }
       add = i;
     }

    if(RR.size())
    {
      DATAHDR hdr;
      memset(&hdr,0,sizeof(DATAHDR));

       memcpy(hdr.hdr,"DATA",4);
       hdr.size = RR.size();
       hdr.sr = float( (long double)RR.size() / ((long double)len/sr) );
       hdr.bits = 32;
       hdr.umv = 1;

      SaveFile(name,&RR[0],&hdr);
      return true;
    }
    else
      return false;
}


bool Annotation::SaveRRnseq(wchar_t *name, int **ann, int nums, long double sr, int len)
{
   vector <long double> RR;
    int add=-1;
   long double rr,r1,r2;

   //R peak or S peak annotation////////////////////////////
   bool rrs=true;
   int rNum=0,sNum=0;
   for(int i=0; i<nums; i++)
   {
     if(ann[i][1]==47 || ann[i][1]==48) rNum++;
     else if(ann[i][1]==49 || ann[i][1]==50) sNum++;
   }
   if(int(1.1f*(float)rNum) < sNum)
   {
     rrs=false;  //R peaks less than S ones
     wcscat(name, L"_SSn.dat");
   }
   else
    wcscat(name, L"_RRn.dat");
   ////////////////////////////////////////////////////////


    for(int i=0; i<nums; i++)
     {
       switch(ann[i][1])
        {
         case 1:             //N
          if(add == -1)
          {
            add = i;
            continue;
          }
         break;

         case 0:     //ectopic beats
         case 4:
         case 5:
         case 6:
         case 7:
         case 8:
         case 9:
         case 10:
         case 11:
         case 12:
         case 13:
         case 14:    //noise
         case 16:    //artefact
         case 34:
         case 35:
         case 38:
         case 46:
          add = -1;   //reset counter
          continue;

         default:     //other types
          continue;
        }

        //annotation on RRs peaks
        if(rrs)
        {
         if(i+1<nums && (ann[i+1][1]==47 || ann[i+1][1]==48))  //r only
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==47 || ann[i+2][1]==48))  //q,r
          r2 = ann[i+2][0];
         else //(ann[i][1]==N,ECT,...)  //no detected R only S
          r2 = ann[i][0];

         if(add+1<nums && (ann[add+1][1]==47 || ann[add+1][1]==48))
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==47 || ann[add+2][1]==48))
          r1 = ann[add+2][0];
         else //(ann[add][1]==N,ECT,...) //no detected R only S
          r1 = ann[add][0];
        }
        //annotation on S peaks
        else
        {
         if(i+1<nums && (ann[i+1][1]==40))  //N)
          r2 = ann[i][0];
         else if(i+1<nums && (ann[i+1][1]==49 || ann[i+1][1]==50))  //Sr
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==49 || ann[i+2][1]==50))  //rS
          r2 = ann[i+2][0];
         else if(i+3<nums && (ann[i+3][1]==49 || ann[i+3][1]==50))  //errQ rS
          r2 = ann[i+3][0];
         else if(i+1<nums && (ann[i+1][1]==47 || ann[i+1][1]==48))  //no S
          r2 = ann[i+1][0];
         else if(i+2<nums && (ann[i+2][1]==47 || ann[i+2][1]==48))  //no S
          r2 = ann[i+2][0];

         if(add+1<nums && (ann[add+1][1]==40))  //N)
          r1 = ann[add][0];
         else if(add+1<nums && (ann[add+1][1]==49 || ann[add+1][1]==50))
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==49 || ann[add+2][1]==50))
          r1 = ann[add+2][0];
         else if(add+3<nums && (ann[add+3][1]==49 || ann[add+3][1]==50))
          r1 = ann[add+3][0];
         else if(add+1<nums && (ann[add+1][1]==47 || ann[add+1][1]==48))  //no S
          r1 = ann[add+1][0];
         else if(add+2<nums && (ann[add+2][1]==47 || ann[add+2][1]==48))  //no S
          r1 = ann[add+2][0];
        }

       rr = 60.0 / ((r2-r1)/sr);
       if(rr >= ahdr.minbpm && rr <= ahdr.maxbpm)
        RR.push_back( rr );       //in bpm

       add = i;
     }


    if(RR.size())
    {
      DATAHDR hdr;
      memset(&hdr,0,sizeof(DATAHDR));

       memcpy(hdr.hdr,"DATA",4);
       hdr.size = RR.size();
       hdr.sr = float( (long double)RR.size() / ((long double)len/sr) );
       hdr.bits = 32;
       hdr.umv = 1;

      SaveFile(name,&RR[0],&hdr);
      return true;
    }
    else
      return false;
}


//////////////////////////////////////////////////////////////////////////////




















////////////////////////////////////////////////////////////////////////////////
////////////////////////CLASS DENOISE///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
ECGDenoise::ECGDenoise(): tempBuff(0)
{
}
ECGDenoise::~ECGDenoise()
{
   if(tempBuff) free(tempBuff);
}
//-----------------------------------------------------------------------------



void ECGDenoise::InitDenoise(wchar_t *fltdir, long double *data, int size, long double sr, bool mirror)
{
    wcscpy(this->fltdir, fltdir);

    ecgBuff = data;
    SR = sr;
    Len = size;

   if(tempBuff) free(tempBuff);
   tempBuff = (long double *)malloc(sizeof(long double)*(Len+2*SR));    // [SR add] [sig] [SR add]

   for(int i=0; i<Len; i++)                //signal
     tempBuff[i+(int)SR] = ecgBuff[i];

   if(Len < SR) mirror = false;

   if(mirror)  //mirror extension of signal
   {
    for(int i=0; i<(int)SR; i++)               //left side
      tempBuff[i] = ecgBuff[(int)SR-i];
    for(int i=Len+SR; i<Len+2*SR; i++)         //right side
      tempBuff[i] = ecgBuff[(Len-2) - (i-(Len+(int)SR))];  
   }
   else
   {
    for(int i=0; i<(int)SR; i++)               //left side
      tempBuff[i] = ecgBuff[0];
    for(int i=Len+SR; i<Len+2*SR; i++)         //right side
      tempBuff[i] = ecgBuff[Len-1];
   }
}
void ECGDenoise::CloseDenoise()
{
   if(tempBuff)
   {
     free(tempBuff);
     tempBuff = 0;
   }
}

//-----------------------------------------------------------------------------
bool  ECGDenoise::LFDenoise()
{
    wchar_t filter[256];


    //get base line J///////
    int J = ceill(log2(SR/0.8)) - 1;

     wcscpy(filter,fltdir);
     wcscat(filter,L"\\daub2.flt");
    if(InitFWT(filter,tempBuff,Len+2*SR) == false)
     return false;


    //transform////////////////////////
      fwtTrans(J);
    ///////////////////////////////////


      int *Jnumbs = GetJnumbs(J,Len+2*SR);
      long double *lo = getSpec();
     for(int i=0; i<Jnumbs[0]; i++)
       lo[i] = 0.0;


    //synth/////////////////////////////
       fwtSynth(J);
    ////////////////////////////////////

      for(int i=0; i<Len; i++)
       ecgBuff[i] = lo[i+(int)SR];


    CloseFWT();
    return true;
}

bool ECGDenoise::HFDenoise()
{
    wchar_t filter[256];


    //get HF scale J///////
    int J = ceill(log2(SR/23.0)) - 2;       //[30Hz - ...] hf denoising

     wcscpy(filter,fltdir);
     wcscat(filter,L"\\bior97.flt");
    if(InitFWT(filter,tempBuff,Len+2*SR) == false)
     return false;


    //transform////////////////////////
      fwtTrans(J);
    ///////////////////////////////////


      int *Jnumbs = GetJnumbs(J,Len+2*SR);
      int hinum,lonum;
      HiLoNumbs(J,Len+2*SR,hinum,lonum);
      long double *lo = getSpec();
      long double *hi = getSpec() + (int(Len+2*SR)-hinum);

      int window;   //3sec window
     for(int j=J; j>0; j--)
     {
       window = 3.0*SR/powl(2.0,(long double)j);

       Denoise(hi,Jnumbs[J-j],window);
        hi += Jnumbs[J-j];
     }



    //synth/////////////////////////////
       fwtSynth(J);
    ////////////////////////////////////

      for(int i=0; i<Len; i++)
       ecgBuff[i] = lo[i+(int)SR];


    CloseFWT();
    return true;
}

bool ECGDenoise::LFHFDenoise()
{
    wchar_t filter[256];
    int J,*Jnumbs;
    long double *lo,*hi;


    //get base line J///////
     J = ceill(log2(SR/0.8)) - 1;

     wcscpy(filter,fltdir);
     wcscat(filter,L"\\daub2.flt");
    if(InitFWT(filter,tempBuff,Len+2*SR) == false)
     return false;


    //transform////////////////////////
      fwtTrans(J);
    ///////////////////////////////////

      Jnumbs = GetJnumbs(J,Len+2*SR);
      lo = getSpec();
     for(int i=0; i<Jnumbs[0]; i++)
       lo[i] = 0.0;

    //synth/////////////////////////////
       fwtSynth(J);
    ////////////////////////////////////

      for(int i=0; i<Len+2*SR; i++)
       tempBuff[i] = lo[i];

    ////////get min max///////////////////
    long double min,max;
    MinMax(&lo[(int)SR],Len,min,max);

    CloseFWT();



    //get HF scale J///////
     J = ceill(log2(SR/23.0)) - 2;       //[30Hz - ...] hf denoising

     wcscpy(filter,fltdir);
     wcscat(filter,L"\\bior97.flt");
    if(InitFWT(filter,tempBuff,Len+2*SR) == false)
     return false;


    //transform////////////////////////
      fwtTrans(J);
    ///////////////////////////////////

      Jnumbs = GetJnumbs(J,Len+2*SR);
      int hinum,lonum;
      HiLoNumbs(J,Len+2*SR,hinum,lonum);
      lo = getSpec();
      hi = getSpec() + (int(Len+2*SR)-hinum);

      int window;   //3sec window
     for(int j=J; j>0; j--)
     {
       window = 3.0*SR/powl(2.0,(long double)j);

       Denoise(hi,Jnumbs[J-j],window);
        hi += Jnumbs[J-j];
     }

    //synth/////////////////////////////
       fwtSynth(J);
    ////////////////////////////////////

      for(int i=0; i<Len; i++)
       ecgBuff[i] = lo[i+(int)SR];

     //renormalize
     nMinMax(ecgBuff,Len,min,max);

     CloseFWT();
     return true;
}






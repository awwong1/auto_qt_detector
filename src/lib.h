/*

lib.h - ECG annotation library based on continuous (CWT) and fast (FWT) wavelet transforms.  
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






#pragma once


//////////////////////////////////////////////////////////////////////////////
typedef struct _datahdr
{
   char hdr[4];
   unsigned int size;
   float sr;
   unsigned char bits;
   unsigned char lead;
   unsigned short umv;
   unsigned short bline;
   unsigned char hh;
   unsigned char mm;
   unsigned char ss;
   char rsrv[19];
} DATAHDR, *PDATAHDR;

class Signal
{
   PDATAHDR phdr;                   //data header

   vector<DATAHDR> hdrs;           //arrays of headers
   vector<long double *> vdata;     //arrays of signals

    bool dat;                       //bin/txt file
    wchar_t fname[256];             //file name

   bool ReadDat();                  //read custom data file
   bool ReadTxt();                  //read text file data
   bool ReadMit();                  //read mit-bih dat file
   int parseHdr(FILE *fh);          //parse mit-bih header

   void getfname(wchar_t *path, wchar_t *name);
   void changeext(wchar_t *path, wchar_t *ext);
   int readline(FILE *f, char *buff);



  protected:
    long double *data,SR;
    int Lead,UmV,Bits,Len;
    int hh,mm,ss;               //start time of the record

      HANDLE fp, fpmap;
         LPVOID lpMap;

            FILE *in;
          float *lpf;
          short *lps;
 unsigned short *lpu;
           char *lpc;



  public:
           Signal();
          ~Signal();

  long double *ReadFile(wchar_t *name);
  long double *ReadFile(char *name);
          bool SaveFile(wchar_t *name, long double *buff, PDATAHDR hdr);
          bool SaveFile(char *name, long double *buff, PDATAHDR hdr);
          bool ToTxt(wchar_t *name, long double *buff, int size);
          bool GetInfo(wchar_t *name);
	  void CloseFile();

         int GetData(int index);
 long double *GetData(){ return data; };
         int GetLength(){ return Len; };
         int GetBits(){ return Bits; };
         int GetUmV(){ return UmV; };
         int GetLead(){ return Lead; };
         int GetLeadsNum(){return (int)vdata.size(); };
 long double GetSR(){ return SR; };
         int GetH(){ return hh; };
         int GetM(){ return mm; };
         int GetS(){ return ss; };

        void  mSecToTime(int , int &,int &,int &,int &);
        void  MinMax(long double *, int , long double &, long double &);

      //normalization
        void nMinMax(long double *buff, int len, long double a, long double b);
        void nMean(long double *buff, int len);
	void nZscore(long double *buff, int len);
       	void nSoftmax(long double *buff, int len);
       	void nEnergy(long double *buff, int len, int L=2);
        

  //math routines////////////////////////////////////////////////////////////
     long double log2(long double x){ return logl(x)/logl(2.0); };

            bool  AutoCov(long double *, int);
     long double  Disp(long double *, int);
     long double  Mean(long double *, int);

     long double  MINIMAX(long double *, int);
     long double  FIXTHRES(long double *, int);
     long double  SURE(long double *, int);
           void  Denoise(long double *mass, int size, int window, int type=0, bool soft=true);
            void  HardTH(long double *mass, int size, long double TH, long double l=0.0);
            void  SoftTH(long double *mass, int size, long double TH, long double l=0.0);

};
//----------------------------------------------------------------------------
/*////////info////////////////////////////////
 to OPEN file:
        long double *sig


          sig = s1->ReadFile("filename");              //error sig=0
         iSize = s1->GetLength();
            SR = s1->GetSR();
           UmV = s1->GetUmV();
          bits = s1->GetBits();


  to SAVE file:
     s1->SaveFile("fname", sig, SR,Len,Bits,Umv)


         s1->mSecToTime(msecs, &h,&m,&s,&ms)
         s1->MinMax(*buff, Len,&min,&max)
////////////////////////////////////////////*/
























/////////////////////////////////////////////////////////////////////////////////
typedef struct _cwthdr
{
   char hdr[4];
   float fmin;
   float fmax;
   float fstep;
   unsigned int size;
   float sr;
   unsigned char type;   //1-log; 0-norm scale

   char rsrv[15];
} CWTHDR, *PCWTHDR;

class CWT : public Signal
{  
     PCWTHDR phdr;

    long double fmin,fmax,fstep, w0;

     int cwtSize,type,cwIndex;
     long double *csig;                //pointer to original signal
     long double *cwtBuff;             //buffer with spectra
     long double *ReW,*ImW;            //Re,Im parts of wavelet
       bool prec;
      int precision;

      bool mirror;   //mirror boundary extension
      long double lval,rval;   //left/right value for signal ext at boundaries

      long double cWave(int, long double);


  public:
          CWT();
         ~CWT();

        float * CWTCreateFileHeader(wchar_t *, PCWTHDR, int, long double);
        float * CWTReadFile(wchar_t *);
  long double   HzToScale(long double, long double, int, long double w=5);
         void   ConvertName(wchar_t *, int, long double);

          void  InitCWT(int, int, long double w=5, long double sr=0);
          void  CloseCWT();
  long double * CWTrans(long double *buff, long double freq, bool mr=true, long double lv=0, long double rv=0);

      long double GetFmin(){return fmin;};
      long double GetFmax(){return fmax;};
      long double GetFstep(){return fstep;};
              int GetType(){return type;};
              int GetFreqRange();
};
/*//////////////////////////////////////////////
    CWT cwt;

     cwt->ReadFile(L"filename");

      cwt.InitCWT(size, wletindex, w0, SR);
       *buff = cwt.CWT(sig, freq);
      cwt.loseCWT();

//////////////////////////////////////////////*/










/////////////////////////////////////////////////////////////
typedef struct _fwthdr
{
   char hdr[4];
   unsigned int size;
   float sr;
   unsigned char bits;
   unsigned char lead;
   unsigned short umv;

   char wlet[8];
   unsigned short J;

   char rsrv[14];
} FWTHDR, *PFWTHDR;

class FWT : public Signal
{
    PFWTHDR phdr;
    char fname[256];

    long double *tH, *tG;     //analysis filters
    long double *H, *G;       //synth filters
    int thL, tgL, hL, gL;     //filters lenghts
    int thZ, tgZ, hZ, gZ;     //filter centers

     int J;                //scales
     int *Jnumbs;          //hi values per scale
     int sigLen;           //signal len
     int size;             //divided signal size


   //filters inits
    FILE *filter;
    long double *loadfilter(int &L, int &Z);

   //spectra
     long double *fwtBuff;     //buffer with fwt spectra
     long double *tempBuff;    //temporary
     long double *hi,*lo;      //pointers to tempBuff
     int hiNum, loNum;         //number of lo and hi coeffs

     

     void fwtHiLoTrans();
     void fwtHiLoSynth();

   public:
       FWT();
       ~FWT();

      bool InitFWT(wchar_t *fltname, long double *sig, int len);
      void CloseFWT();

      void fwtTrans(int scales);                      //wavelet transform
      void fwtSynth(int scales);                      //wavelet synthesis


  long double *getSpec(){ return fwtBuff; };
         int getSpecSize(){ return size; };
         int getJ() {return J;};
       char *getFltName(){ return fname; };


      int *GetJnumbs(int j, int len);
      void HiLoNumbs(int j, int len, int &hinum, int &lonum);

      bool  fwtSaveFile(wchar_t *name, long double *hipass, long double *lopass, PFWTHDR hdr);
      bool  fwtReadFile(wchar_t *name, char *appdir=0);

};







////////////////////////////////////////////////////////////////////////////////
#define qNORM 0
#define qNOISE 1

typedef struct _annrec
{
   unsigned int pos;    //offset
   unsigned int type;   //beat type
   unsigned int aux;    //index to aux array
} ANNREC, *PANNREC;

typedef struct _annhdr
{
   int minbpm;
   int maxbpm;
   long double minUmV;  //min R,S amplitude
   long double minQRS;  //min QRS duration  
   long double maxQRS;  //max QRS duration
   long double minPQ;
   long double maxPQ;
   long double minQT;
   long double maxQT;
   long double pFreq;   //p wave freq for CWT
   long double tFreq;   //t wave freq for CWT
} ANNHDR, *PANNHDR;

class Annotation : public Signal
{
     ANNHDR ahdr;             //annotation ECG params

     int **ANN, annNum;           //annotation after getPTU (RR's + PT)
     int **qrsANN, qrsNum;        //annotation after getQRS (RR's+MAnoise)  +ECT after getECT()
      vector <int> MA;       //MA noise
     int auxNum;
     char **AUX;


    bool chkNoise(long double *mass, int window);        //check for noise in window len
    bool flt30Hz(long double *mass, int len, long double sr, wchar_t *fltdir, int stype);    //0-30Hz removal

     void findRS(long double *mass, int len, int &R, int &S, long double err=0.0);  //find RS or QR  
     int findr(long double *mass, int len, long double err=0.0);  //find small r in PQ-S
     int findq(long double *mass, int len, long double err=0.0);  //find small q in PQ-R
     int finds(long double *mass, int len, long double err=0.0);  //find small s in R-Jpnt
     int findTmax(long double *mass, int len); 


   public:
       Annotation(PANNHDR p=0);
       ~Annotation();


   int **GetQRS(long double *mass, int len, long double sr, wchar_t *fltdir=0, int stype=qNORM);     //get RR's classification
    void GetECT(int **ann, int qrsnum, long double sr);                                              //classify ectopic beats
   int **GetPTU(long double *mass, int len, long double sr, wchar_t *fltdir, int **ann, int qrsnum);
     int GetQRSnum(){ return qrsNum; };
     int GetANNnum(){ return annNum; };

     void AddANN(int add);    //add if annotated within fromX-toX  


     bool  SaveANN(wchar_t *, int **, int);
     //bool  ReadANN(wchar_t *);
      int ** GetANNdata(){return ANN;};
      int ** GetQRSdata(){return qrsANN;};
      char ** GetAUXdata(){return AUX;};

     PANNHDR GetANNhdr(){return &ahdr;}; 

     bool GetRRseq(int **ann, int nums, long double sr, vector<long double> *RR, vector<int> *RRpos);
     bool SaveRRseq(wchar_t *, int **, int, long double, int);
     bool SaveRRnseq(wchar_t *, int **, int, long double, int);
     bool SaveQTseq(wchar_t *, int **, int, long double, int);
     bool SavePQseq(wchar_t *, int **, int, long double, int);
     bool SavePPseq(wchar_t *, int **, int, long double, int);
};

/*                   0           1             2
    int **ANN [x][samples][annotation type][aux data index]

    wchar_t *dir = "c:\\dir_for_fwt_filters\\filters";
    long double *sig;   //signal massive
    long double SR;     //sampling rate of the signal
    int size;   //size of the signal

    Annotation ann;

     int **qrsAnn;  //qrs annotation massive
     qrsAnn = ann.GetQRS(sig,size,SR,dir);       //get QRS complexes
     //qrsAnn = ann->GetQRS(psig,size,SR,umess,qNOISE);    //get QRS complexes if signal is quite noisy

     int **ANN; //QRS + PT annotation
     if(qrsAnn)
     {
        ann.GetECT(qrsAnn,ann->GetQRSnum(),SR);     //label Ectopic beats
        ANN = ann.GetPTU(sig,size,SR,dir,qrsAnn,ann.GetQRSnum());   //find P,T waves
     }
*/



  //annotation codes types
      /*
         skip=59
         num=60
         sub=61
         chn=62
         aux=63

      char anncodes [51][10] =  {"notQRS", "N",       "LBBB",    "RBBB",     "ABERR", "PVC",
                                 "FUSION", "NPC",     "APC",     "SVPB",     "VESC",  "NESC",
                                 "PACE",   "UNKNOWN", "NOISE",   "q",        "ARFCT", "Q",
                                 "STCH",   "TCH",     "SYSTOLE", "DIASTOLE", "NOTE",  "MEASURE",
                                 "P",      "BBB",     "PACESP",  "T",        "RTM",   "U",
                                 "LEARN",  "FLWAV",   "VFON",    "VFOFF",    "AESC",  "SVESC",
                                 "LINK",   "NAPC",    "PFUSE",   "(",        ")",     "RONT",

          //user defined beats//
                                 "(p",     "p)",      "(t",      "t)",       "ECT",
                                 "r",      "R",       "s",       "S"};

                     /*
                        [16] - ARFCT
                        [15] - q
                        [17] - Q
                        [24] - P
                        [27] - T 
                        [39, 40] - '(' QRS ')'  PQ, J point
                        42 - (p Pwave onset
                        43 - p) Pwave offset
                        44 - (t Twave onset
                        45 - t) Twave offset
                        46 - ect Ectopic of any origin beat
                        47 - r
                        48 - R
                        49 - s
                        50 - S
                                                     */





     



class ECGDenoise : public FWT
{
     wchar_t fltdir[256];             //filters dir

     long double *ecgBuff;            //pointer to [original sig]
     long double *tempBuff;           //[SRadd][original sig][SRadd]

     //void  Denoise(long double *, int);


   public:
       ECGDenoise();
       ~ECGDenoise();

       void  InitDenoise(wchar_t *fltdir, long double *data, int size, long double sr, bool mirror=true);
       void  CloseDenoise();

       bool LFDenoise();         //baseline wander removal
       bool HFDenoise();         //hf denoising
       bool LFHFDenoise();       //baseline and hf denoising

};

/*
    wchar_t *dir = "c:\\dir_for_fwt_filters\\filters";
    long double *sig;   //signal massive
    long double SR;     //sampling rate of the signal
    int size;   //size of the signal

     ECGDenoise enoise;

      enoise.InitDenoise(dir, sig,size,SR);
      enoise.LFDenoise();     //baseline wander removal
      //or  enoise.HFDenoise();    //high frequency noise removal
      //or  enoise.LFHFDenoise();  //both noises removal
      enoise.CloseDenoise();

*/

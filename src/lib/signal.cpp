

#include "../stdafx.h"
#include "signal.h"
#include <string.h>
#include "ishne.h"

Signal::Signal(): pData(0), fp(0), fpmap(0), lpMap(0),
                  SR(0.0), Bits(0), UmV(0), Lead(0), Length(0),
                  hh(0), mm(0), ss(0)

{
        wcscpy(EcgFileName, L"");
}
Signal::~Signal()
{
        if (EcgSignals.size()) {
                for (int i = 0; i < (int)EcgSignals.size(); i++) {
                        pData = EcgSignals[i];
                        delete[] pData;
                }
        }
}

double* Signal::ReadFile(const char* name)
{
  wchar_t ustr[_MAX_PATH] = L"";
  for (int i = 0; i < (int)strlen(name); i++) {
    mbtowc(ustr + i, name + i, MB_CUR_MAX);
  }
  return ReadFile(ustr);
}

double* Signal::ReadFile(const wchar_t* name)
{
  wcscpy(EcgFileName, name);
  
  if (!IsFileValid(name)) {
    fprintf(stderr, "Invalid file.\n");
    return 0;
  }
  
  //wprintf(L"file is valid.\n");  // debugging
  
  if (IsDat) {
    if (!ReadDatFile()) { return 0; }  // TODO: need to fix ReadDatFile() before using
  }
  else if (IsISHNE) {
    if (!ReadIshneFile()) { return 0; }
  }
  else {
    if (!ReadTxtFile())  // try to read as text file
      {
	if (!ReadMitbihFile())  // else, read as mit-bih file
	  { return 0; }
      }
  }
  
  return GetData();
}

bool Signal::IsFileValid(const wchar_t* name)
{
  // fp = CreateFileW(name, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  // if (fp == INVALID_HANDLE_VALUE)
  //         return false;
  
  IsISHNE = false;
  IsDat = false;
  
  // Convert name to char* for fopen():
  char buffer[PATH_MAX];
  wcstombs(buffer, name, sizeof(buffer) );
  
  fp = fopen( buffer, "r" );  // TODO: use open() instead?
  if(fp == NULL) { return false; }
  
  // fpmap = CreateFileMapping(fp, 0, PAGE_READONLY, 0, sizeof(DATAHDR), 0);
  // lpMap = MapViewOfFile(fpmap, FILE_MAP_READ, 0, 0, sizeof(DATAHDR));
  int fp_int = fileno(fp);
  lpMap = mmap(NULL, sizeof(DATAHDR), PROT_READ, MAP_SHARED, fp_int, 0);
  if(lpMap == 0) { return false; }
  
  pEcgHeader = (PDATAHDR)lpMap;
  
  //printf("lpmap = %i\n", lpMap);  // debugging
  
  //printf("header = %s\n", pEcgHeader->hdr);  // debugging
  
  if(!memcmp(pEcgHeader->hdr,"ISHNE1.0",8))  // kind of improper, and requires sizeof(DATAHDR) >= 8
    {
      IsISHNE = true;
      
      // Find file size:
      long file_size;
      fseek(fp, 0, SEEK_END);
      file_size = ftell(fp);
      // rewind(fp);
      // fprintf( stderr, "filesize = %li.\n", file_size );  // debugging
      
      CloseFile(sizeof(DATAHDR));  // because readIshneHeader() is going to open it (and close it)
      ISHNEHeader m_ISHNEHeader = readIshneHeader(buffer);
      
      // If header reports total number of samples:
      //     Len = m_ISHNEHeader.Sample_Size_ECG / m_ISHNEHeader.nLeads;
      // If header reports number of samples for a single lead:
      //     Len = m_ISHNEHeader.Sample_Size_ECG;
      // I've found that ISHNE file headers can do either one, or just be
      // completely wrong.  So we'll ignore what the header says, and figure it
      // out on our own:
      Length = (file_size - m_ISHNEHeader.Offset_ECG_block) / m_ISHNEHeader.nLeads / 2;
      // Note that doing it this way and discarding the Sample Size reported by
      // the header mostly eliminates the ability to check for incorrect file
      // size.

      // fprintf( stderr, "Len = %i.\n", Len );  // debugging
      // fprintf( stderr, "Offset_ECG_block = %i.\n", m_ISHNEHeader.Offset_ECG_block );  // debugging
      // fprintf( stderr, "nLeads = %i.\n", m_ISHNEHeader.nLeads );  // debugging
      // fprintf( stderr, "reported nSamples = %i.\n", m_ISHNEHeader.Sample_Size_ECG );  // debugging
      
      SR = m_ISHNEHeader.Sampling_Rate;
      Bits = 16;
      UmV = 1000000 / m_ISHNEHeader.Resolution[0]; // TODO: set separately for each lead
      hh = m_ISHNEHeader.Start_Time[0];
      mm = m_ISHNEHeader.Start_Time[1];
      ss = m_ISHNEHeader.Start_Time[2];
      // TODO: Lead?
      
      return true;
    }
  else if ( !memcmp(pEcgHeader->hdr, "DATA", 4) ) {
    Length = pEcgHeader->size;
    SR = pEcgHeader->sr;
    Bits = pEcgHeader->bits;
    Lead = pEcgHeader->lead;
    UmV = pEcgHeader->umv;
    hh = pEcgHeader->hh;
    mm = pEcgHeader->mm;
    ss = pEcgHeader->ss;
    IsDat = true;
  }

  CloseFile(sizeof(DATAHDR));
  return true;
}

bool Signal::ReadDatFile()
{
        short tmp;

        // fp = CreateFileW(EcgFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        // if (fp == INVALID_HANDLE_VALUE)
        //         return false;

	// Convert name to char* for fopen():
	char buffer[PATH_MAX];
	wcstombs(buffer, EcgFileName, sizeof(buffer) );

	// fp = fopen( (const char*)EcgFileName, "r" );  // TODO: use open() instead?  and do char conversion.
	fp = fopen( buffer, "r" );  // TODO: use open() instead?
	if(fp == NULL) { return false; }

        // fpmap = CreateFileMapping(fp, 0, PAGE_READONLY, 0, 0, 0);
        // lpMap = MapViewOfFile(fpmap, FILE_MAP_READ, 0, 0, 0);
	int fp_int = fileno(fp);
	lpMap = mmap(NULL, 0, PROT_READ, MAP_SHARED, fp_int, 0);  // TODO: sizeof(PDATAHDR or DATAHDR) + Length*Bits/8

        if (lpMap == 0) {  // TODO: should be checking mmap() == MAP_FAILED on
			   //       all of these, and also make sure we munmap()
                CloseFile(0);
                return false;
        }

        pEcgHeader = (PDATAHDR)lpMap;
        EcgHeaders.push_back(*pEcgHeader);

        lpf = (float *)lpMap;
        lps = (short *)lpMap;
        lpu = (unsigned short *)lpMap;
        lpc = (char *)lpMap;

        Length = pEcgHeader->size;
        SR = pEcgHeader->sr;
        Bits = pEcgHeader->bits;
        Lead = pEcgHeader->lead;
        UmV = pEcgHeader->umv;

        lpf += sizeof(DATAHDR) / sizeof(float);
        lps += sizeof(DATAHDR) / sizeof(short);
        lpc += sizeof(DATAHDR);

        pData = new double[Length];
        EcgSignals.push_back(pData);

        for (int i = 0; i < Length; i++)
                switch (Bits) {
                case 12:                                             //212 format   12bit
                        if (i % 2 == 0) {
                                tmp = MAKEWORD(lpc[0], lpc[1] & 0x0f);
                                if (tmp > 0x7ff)
                                        tmp |= 0xf000;
                                pData[i] = (double)tmp / (double)UmV;
                        } else {
                                tmp = MAKEWORD(lpc[2], (lpc[1] & 0xf0) >> 4);
                                if (tmp > 0x7ff)
                                        tmp |= 0xf000;
                                pData[i] = (double)tmp / (double)UmV;
                                lpc += 3;
                        }
                        break;

                case 16:                                             //16format
                        pData[i] = (double)lps[i] / (double)UmV;
                        break;

                case 0:
                case 32:                                             //32bit float
                        pData[i] = (double)lpf[i];
                        break;
                }


        CloseFile(0);
        return true;
}

bool Signal::ReadTxtFile()
{
        vector<double> Buffer;
        double tmp;
        int res;

	// Convert name to char* for fopen():
	char buffer[PATH_MAX];
	wcstombs(buffer, EcgFileName, sizeof(buffer) );
	
        // if ((in = _wfopen(EcgFileName, L"rt")) == 0)
        // if ((in = fopen( (const char*)EcgFileName, "rt" )) == 0)  // no unicode.  TODO: char conversion.
        if ((in = fopen( buffer, "rt" )) == 0)  // no unicode.
	  { return false; }

        for (;;) {
                res = fscanf(in, "%lf", &tmp);
                if (res == EOF || res == 0)
                        break;
                else
                        Buffer.push_back(tmp);
        }

        fclose(in);
        Length = (int)Buffer.size();
        if (Length < 2)
                return false;

        pData = new double[Length];
        for (int i = 0; i < Length; i++)
                pData[i] = Buffer[i];
        EcgSignals.push_back(pData);

        DATAHDR hdr;
        memset(&hdr, 0, sizeof(DATAHDR));
        hdr.size = Length;
        hdr.umv = 1;
        hdr.bits = 32;
        EcgHeaders.push_back(hdr);

        return true;
}

bool Signal::ReadIshneFile() {
  // TODO: Redo/Verify all of this

  ISHNEHeader m_ISHNEHeader = readIshneHeader(fname);
  ISHNEData m_ISHNEData = readIshneECG(fname);

  // TODO: return false if one of those fails

  // Len = m_ISHNEHeader.Sample_Size_ECG;  // already prepped by GetInfo()
  if(Len < 2)
    return false;

  // For each lead...:
  for(int i=0; i<m_ISHNEHeader.nLeads; i++)
    {
      // Push the data for this lead onto vdata:
      data = new long double[Len];
      for(int j=0; j<Len; j++)
	{
	  data[j] = m_ISHNEData.data[i][j];
	  // do this instead?  looks like no :
	  // data[j] = (long double)m_ISHNEData.data[i][j] / (long double)UmV;
	}
      vdata.push_back(data);

      // Push the header for this lead onto hdrs.  (Note that hdr.lead is the
      // only thing that changes for each lead.):
      DATAHDR hdr;
      memset(&hdr,0,sizeof(DATAHDR));
      hdr.size  = Len;  // m_ISHNEHeader.Sample_Size_ECG
      hdr.sr    = SR;   // m_ISHNEHeader.Sampling_Rate
      hdr.umv   = UmV;  // e.g. 100 or 2000
      hdr.bits  = Bits; // 16
      hdr.hh    = hh;   // m_ISHNEHeader.Start_Time[0]
      hdr.mm    = mm;   // m_ISHNEHeader.Start_Time[1]
      hdr.ss    = ss;   // m_ISHNEHeader.Start_Time[2]
      hdr.lead  = i;    // arbitrary; doesn't seem to be used by anything
      hdr.bline = 0;    // ... I guess.
      hdrs.push_back(hdr);
    }

  return true;
}

bool Signal::ReadMitbihFile()
{
  //wprintf(L"reading mit file...\n");  // debugging

  wchar_t HeaFile[_MAX_PATH];
  wcscpy(HeaFile, EcgFileName);
  
  ChangeExtension(HeaFile, L".hea");

  // Convert name to char* for fopen():
  char buffer[PATH_MAX];
  wcstombs(buffer, HeaFile, sizeof(buffer) );

  // FILE* fh = _wfopen(HeaFile, L"rt");
  // FILE* fh = fopen( (const char*)HeaFile, "rt" );  // no unicode
  FILE* fh = fopen( buffer, "rt" );  // no unicode
  if (!fh) { return false; }
  
  if (ParseMitbihHeader(fh)) {
    fclose(fh);

    pEcgHeader = &EcgHeaders[0];
    int lNum = (int)EcgHeaders.size();
    int size = pEcgHeader->size;
    
    // fp = CreateFileW(EcgFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    // if (fp == INVALID_HANDLE_VALUE || (GetFileSize(fp, 0) != (lNum * pEcgHeader->size * pEcgHeader->bits) / 8))
    //         return false;
    wcstombs(buffer, EcgFileName, sizeof(buffer) );
    fp = fopen( buffer, "r" );  // TODO: use open() instead?
    // fp = fopen( (const char*)EcgFileName, "r" );  // TODO: use open() instead?
    if (fp == NULL) { return false; }
    
    // Find file size:
    long file_size;
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
    
    if (file_size != (lNum * pEcgHeader->size * pEcgHeader->bits) / 8)
      { return false; }
    
    // fpmap = CreateFileMapping(fp, 0, PAGE_READONLY, 0, 0, 0);
    // lpMap = MapViewOfFile(fpmap, FILE_MAP_READ, 0, 0, 0);
    int fp_int = fileno(fp);
    lpMap = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fp_int, 0);
    
    if (lpMap == 0) {
      CloseFile(0);
      return false;
    }
    
    lps = (short *)lpMap;
    lpc = (char *)lpMap;
    
    for (int i = 0; i < lNum; i++) {
      pData = new double[pEcgHeader->size];
      EcgSignals.push_back(pData);
    }
    
    //wprintf(L"starting loop.  s=%i, n=%i.\n", size, lNum);  // debugging

    short tmp;
    for (int s = 0; s < size; s++) {
      for (int n = 0; n < lNum; n++) {
	pData = EcgSignals[n];
	pEcgHeader = &EcgHeaders[n];
	
	switch (pEcgHeader->bits) {
	case 12:                                             //212 format   12bit
	  if ((s*lNum + n) % 2 == 0) {
	    tmp = MAKEWORD(lpc[0], lpc[1] & 0x0f);
	    if (tmp > 0x7ff)
	      tmp |= 0xf000;
	    tmp -= pEcgHeader->bline;
	    pData[s] = (double)tmp / (double)pEcgHeader->umv;
	  } else {
	    tmp = MAKEWORD(lpc[2], (lpc[1] & 0xf0) >> 4);
	    if (tmp > 0x7ff)
	      tmp |= 0xf000;
	    tmp -= pEcgHeader->bline;
	    pData[s] = (double)tmp / (double)pEcgHeader->umv;
	    lpc += 3;
	  }
	  break;
	  
	case 16:                                      //16format
	  pData[s] = (double)(*lps++ - pEcgHeader->bline) / (double)pEcgHeader->umv;
	  break;
	}
      }
    }
    
    CloseFile(0);
    return true;
  }
  else {
    fclose(fh);
    return false;
  }
}

double* Signal::GetData(int index)
{
        if (!EcgSignals.size())
                return 0;
        if (index > (int)EcgSignals.size() - 1)
                index = (int)EcgSignals.size() - 1;
        else if (index < 0)
                index = 0;

        pEcgHeader = &EcgHeaders[index];
        pData = EcgSignals[index];
        SR = pEcgHeader->sr;
        Lead = pEcgHeader->lead;
        UmV = pEcgHeader->umv;
        Bits = pEcgHeader->bits;
        Length = pEcgHeader->size;
        hh = pEcgHeader->hh;
        mm = pEcgHeader->mm;
        ss = pEcgHeader->ss;

        return pData;
}

int Signal::ParseMitbihHeader(FILE* fh)
{
        char leads[18][6] =  {"I", "II", "III", "aVR", "aVL", "aVF", "v1", "v2",
                              "v3", "v4", "v5", "v6", "MLI", "MLII", "MLIII", "vX", "vY", "vZ"
                             };
        char str[10][256];

        if (ReadLine(fh, str[0]) <= 0)
                return false;

        int sNum, size;
        float sr;
        int res = sscanf(str[0], "%s %s %s %s %s", str[1], str[2], str[3], str[4], str[5]);
        if (res < 4)
                return 0;
        if (res == 5 && strlen(str[5]) == 8) {
                char* ptime = str[5];
                hh = atoi(ptime);
                mm = atoi(ptime + 3);
                ss = atoi(ptime + 6);
        }

        sNum = atoi(str[2]);
        sr = (float)atof(str[3]);
        size = atoi(str[4]);

        int eNum = 0;
        for (int i = 0; i < sNum; i++) {
                if (ReadLine(fh, str[0]) <= 0)
                        return 0;

                int umv, bits, bline;
                memset(str[9], 0, 256);

                res = sscanf(str[0], "%s %s %s %s %s %s %s %s %s", str[1], str[2], str[3], str[4], str[5], str[6], str[7], str[8], str[9]);
                if (res < 5) return 0;

                bits = atoi(str[2]);
                umv = atoi(str[3]);
                bline = atoi(str[5]);

                int offs = (int)strlen(str[1]), j = 0;
                for (j = 0; j < offs; j++) {
                        if (EcgFileName[(wcslen(EcgFileName)-offs)+j] != str[1][j])  //wctomb
                                break;
                }
                if (j == offs) {
                        eNum++;
                        DATAHDR hdr;
                        memset(&hdr, 0, sizeof(DATAHDR));
                        hdr.sr = sr;
                        hdr.bits = (bits == 212) ? 12 : bits;
                        hdr.umv = (umv == 0) ? 200 : umv;
                        hdr.bline = bline;
                        hdr.size = size;
                        hdr.hh = hh;
                        hdr.mm = mm;
                        hdr.ss = ss;
                        for (int l = 0; l < 18; l++) {
                                if (!strcasecmp(leads[l], str[9])) {
                                        hdr.lead = l + 1;
                                        break;
                                }
                        }
                        EcgHeaders.push_back(hdr);
                }
        }
        return eNum;
}

bool Signal::SaveFile(const char* name, const double* buffer, PDATAHDR hdr)
{
        wchar_t ustr[_MAX_PATH] = L"";
        for (int i = 0; i < (int)strlen(name); i++)
                mbtowc(ustr + i, name + i, MB_CUR_MAX);
        return SaveFile(ustr, buffer, hdr);
}

bool Signal::SaveFile(const wchar_t* name, const double* buffer, PDATAHDR hdr)
{
        int filesize;
        int tmp;

        switch (hdr->bits) {
        case 12:
                if (hdr->size % 2 != 0)
                        filesize = int((double)(hdr->size + 1) * 1.5);
                else
                        filesize = int((double)(hdr->size) * 1.5);
                break;
        case 16:
                filesize = hdr->size * 2;
                break;
        case 0:
        case 32:
                filesize = hdr->size * 4;
                break;
        }

	// Convert name to char* for fopen():
	char fn_buffer[PATH_MAX];
	wcstombs(fn_buffer, name, sizeof(fn_buffer) );

        // fp = CreateFileW(name, GENERIC_WRITE | GENERIC_READ, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        // if (fp == INVALID_HANDLE_VALUE)
        //         return false;
        // fp = fopen( (const char*)name, "w+" );  // TODO: use open() instead?  and do char conversion.
        fp = fopen( fn_buffer, "w+" );  // TODO: use open() instead?
	if(fp == NULL) { return false; }

        // fpmap = CreateFileMapping(fp, 0, PAGE_READWRITE, 0, filesize + sizeof(DATAHDR), 0);
        // lpMap = MapViewOfFile(fpmap, FILE_MAP_WRITE, 0, 0, filesize + sizeof(DATAHDR));
	int fp_int = fileno(fp);
	lpMap = mmap(NULL, filesize + sizeof(DATAHDR), (PROT_READ | PROT_WRITE), MAP_SHARED, fp_int, 0);

	if (lpMap == 0) {
                CloseFile(filesize + sizeof(DATAHDR));
                return false;
        }

        lpf = (float *)lpMap;
        lps = (short *)lpMap;
        lpu = (unsigned short *)lpMap;
        lpc = (char *)lpMap;

        memset(lpMap, 0, filesize + sizeof(DATAHDR));
        memcpy(lpMap, hdr, sizeof(DATAHDR));

        lpf += sizeof(DATAHDR) / sizeof(float);
        lps += sizeof(DATAHDR) / sizeof(short);
        lpc += sizeof(DATAHDR);


        for (unsigned int i = 0; i < hdr->size; i++) {
                switch (hdr->bits) {
                case 12:                                               //212 format   12bit
                        tmp = int(buffer[i] * (double)hdr->umv);
                        if (tmp > 2047) tmp = 2047;
                        if (tmp < -2048) tmp = -2048;

                        if (i % 2 == 0) {
                                lpc[0] = LOBYTE((short)tmp);
                                lpc[1] = 0;
                                lpc[1] = HIBYTE((short)tmp) & 0x0f;
                        } else {
                                lpc[2] = LOBYTE((short)tmp);
                                lpc[1] |= HIBYTE((short)tmp) << 4;
                                lpc += 3;
                        }
                        break;

                case 16:                                               //16format
                        tmp = int(buffer[i] * (double)hdr->umv);
                        if (tmp > 32767) tmp = 32767;
                        if (tmp < -32768) tmp = -32768;
                        *lps++ = (short)tmp;
                        break;

                case 0:
                case 32:                                               //32bit float
                        *lpf++ = (float)buffer[i];
                        break;
                }
        }

        CloseFile(filesize + sizeof(DATAHDR));
        return true;
}

bool Signal::ToTxt(const wchar_t* name, const double* buffer, int size)
{
  // Convert name to char* for fopen():
  char fn_buffer[PATH_MAX];
  wcstombs(fn_buffer, name, sizeof(fn_buffer) );
  
  // in = _wfopen(name, L"wt");
  // in = fopen( (const char*)name, "wt" );  // no unicode.  TODO: char conversion.
  in = fopen( fn_buffer, "wt" );  // no unicode.
  if (in) {
    for (int i = 0; i < size; i++)
      fwprintf(in, L"%lf\n", buffer[i]);
    
    fclose(in);
    return true;
  }
  else
    { return false; }
}

void Signal::ChangeExtension(wchar_t* path, const wchar_t* ext) const
{
        for (int i = (int)wcslen(path) - 1; i > 0; i--) {
                if (path[i] == '.') {
                        path[i] = 0;
                        wcscat(path, ext);
                        return;
                }
        }

        wcscat(path, ext);
}

int Signal::ReadLine(FILE* fh, char* buffer) const
{
        int res = 0;
        char* pbuffer = buffer;

        while ((short)res != EOF) {
                res = fgetc(fh);
                if (res == 0xD || res == 0xA) {
                        if (pbuffer == buffer) continue;

                        *pbuffer = 0;
                        return 1;
                }
                if ((short)res != EOF) {
                        *pbuffer++ = (char)res;
                }
        }

        return (short)res;
}

void Signal::mSecToTime(int msec, int& h, int& m, int& s, int& ms) const
{
        ms = msec % 1000;
        msec /= 1000;

        if (msec < 60) {
                h = 0;
                m = 0;
                s = msec;                 //sec to final
        } else {
                double tmp;
                tmp = (double)(msec % 60) / 60;
                tmp *= 60;
                s = int(tmp);
                msec /= 60;

                if (msec < 60) {
                        h = 0;
                        m = msec;
                } else {
                        h = msec / 60;
                        tmp = (double)(msec % 60) / 60;
                        tmp *= 60;
                        m = int(tmp);
                }
        }
}

void Signal::MinMax(const double* buffer, int size, double& min, double& max) const
{
        max = buffer[0];
        min = buffer[0];
        for (int i = 1; i < size; i++) {
                if (buffer[i] > max)max = buffer[i];
                if (buffer[i] < min)min = buffer[i];
        }
}

void Signal::CloseFile(int sizeof_lpmap)
{
  // We pass in sizeof_lpmap because as far as I can tell there's no good way to
  // look it up here.  maybe we should just make a shared 'lpMap_size" variable?

  if (lpMap != 0) {
    // UnmapViewOfFile(lpMap);
    munmap(lpMap, sizeof_lpmap);
  }
  // if (fpmap != 0) {
  //   CloseHandle(fpmap);
  // }
  // if (fp != 0 && fp != INVALID_HANDLE_VALUE)
  if (fp != NULL) {
    fclose(fp);
  }
}

double Signal::Mean(const double* buffer, int size) const
{
        double mean = 0;

        for (int i = 0; i < size; i++)
                mean += buffer[i];

        return mean / (double)size;
}

double Signal::Std(const double* buffer, int size) const
{
        double mean, disp = 0;

        mean = Mean(buffer, size);

        for (int i = 0; i < size; i++)
                disp += (buffer[i] - mean) * (buffer[i] - mean);

        return (sqrt(disp / (double)(size - 1)));
}

void  Signal::nMinMax(double* buffer, int size, double a, double b) const
{
        double min, max;
        MinMax(buffer, size, min, max);

        for (int i = 0; i < size; i++) {
                if (max - min)
                        buffer[i] = (buffer[i] - min) * ((b - a) / (max - min)) + a;
                else
                        buffer[i] = a;
        }
}

void Signal::nMean(double* buffer, int size) const
{
        double mean = Mean(buffer, size);

        for (int i = 0; i < size; i++)
                buffer[i] = buffer[i] - mean;
}

void Signal::nZscore(double* buffer, int size) const
{
        double mean = Mean(buffer, size);
        double disp = Std(buffer, size);

        if (disp == 0.0) disp = 1.0;
        for (int i = 0; i < size; i++)
                buffer[i] = (buffer[i] - mean) / disp;
}

void Signal::nSoftmax(double* buffer, int size) const
{
        double mean = Mean(buffer, size);
        double disp = Std(buffer, size);

        if (disp == 0.0) disp = 1.0;
        for (int i = 0; i < size; i++)
                buffer[i] = 1.0 / (1 + exp(-((buffer[i] - mean) / disp)));
}

void Signal::nEnergy(double* buffer, int size, int L) const
{
        double enrg = 0.0;
        for (int i = 0; i < size; i++)
                enrg += pow(fabs(buffer[i]), (double)L);

        enrg = pow(enrg, 1.0 / (double)L);
        if (enrg == 0.0) enrg = 1.0;

        for (int i = 0; i < size; i++)
                buffer[i] /= enrg;
}

double Signal::MINIMAX(const double* buffer, int size) const
{
        return Std(buffer, size)*(0.3936 + 0.1829*log((double)size));
}

double Signal::FIXTHRES(const double* buffer, int size) const
{
        return Std(buffer, size)*sqrt(2.0*log((double)size));
}

double Signal::SURE(const double* buffer, int size) const
{
        return Std(buffer, size)*sqrt(2.0*log((double)size*log((double)size)));
}

void Signal::Denoise(double* buffer, int size, int window, int type, bool soft) const
{
        double TH;

        for (int i = 0; i < size / window; i++) {
                switch (type) {
                case 0:
                        TH = MINIMAX(buffer, window);
                        break;
                case 1:
                        TH = FIXTHRES(buffer, window);
                        break;
                case 2:
                        TH = SURE(buffer, window);
                        break;
                }
                if (soft)
                        SoftTH(buffer, window, TH);
                else
                        HardTH(buffer, window, TH);

                buffer += window;
        }

        if (size % window > 5) { //skip len=1
                switch (type) {
                case 0:
                        TH = MINIMAX(buffer, size % window);
                        break;
                case 1:
                        TH = FIXTHRES(buffer, size % window);
                        break;
                case 2:
                        TH = SURE(buffer, size % window);
                        break;
                }
                if (soft)
                        SoftTH(buffer, size % window, TH);
                else
                        HardTH(buffer, size % window, TH);
        }
}

void  Signal::HardTH(double* buffer, int size, double TH, double l) const
{
        for (int i = 0; i < size; i++)
                if (fabs(buffer[i]) <= TH)
                        buffer[i] *= l;
}

void  Signal::SoftTH(double* buffer, int size, double TH, double l) const
{
        for (int i = 0; i < size; i++) {
                if (fabs(buffer[i]) <= TH) {
                        buffer[i] *= l;
                } else {
                        if (buffer[i] > 0) {
                                buffer[i] -= TH * (1 - l);
                        } else {
                                buffer[i] += TH * (1 - l);
                        }
                }
        }
}


void Signal::AutoCov(double* buffer, int size) const
{
        double* rk, mu;
        int t;
        rk = new double[size];

        mu = Mean(buffer, size);

        for (int k = 0; k < size; k++) {
                rk[k] = 0;

                t = 0;
                while (t + k != size) {
                        rk[k] += (buffer[t] - mu) * (buffer[t+k] - mu);
                        t++;
                }

                rk[k] /= (double)t;                       // rk[k] /= t ?  autocovariance
        }

        for (int i = 0; i < size; i++)
                buffer[i] = rk[i];

        delete[] rk;
}

void Signal::AutoCov1(double* buffer, int size) const
{
        double* rk, mu;
        rk = new double[size];

        mu = Mean(buffer, size);

        for (int k = 0; k < size; k++) {
                rk[k] = 0;

                for (int t = 0; t < size; t++) {
                        if (t + k >= size)
                                rk[k] += (buffer[t] - mu) * (buffer[2*size-(t+k+2)] - mu);
                        else
                                rk[k] += (buffer[t] - mu) * (buffer[t+k] - mu);
                }

                rk[k] /= (double)size;
        }

        for (int i = 0; i < size; i++)
                buffer[i] = rk[i];

        delete[] rk;
}

void Signal::AutoCor(double* buffer, int size) const
{
        double* rk, mu, std;
        int t;
        rk = new double[size];

        mu = Mean(buffer, size);
        std = Std(buffer, size);

        for (int k = 0; k < size; k++) {
                rk[k] = 0;

                t = 0;
                while (t + k != size) {
                        rk[k] += (buffer[t] - mu) * (buffer[t+k] - mu);
                        t++;
                }

                rk[k] /= (double)t * std * std;
        }

        for (int i = 0; i < size; i++)
                buffer[i] = rk[i];

        delete[] rk;
}

void Signal::AutoCor1(double* buffer, int size) const
{
        double* rk, mu, std;
        rk = new double[size];

        mu = Mean(buffer, size);
        std = Std(buffer, size);

        for (int k = 0; k < size; k++) {
                rk[k] = 0;

                for (int t = 0; t < size; t++) {
                        if (t + k >= size)
                                rk[k] += (buffer[t] - mu) * (buffer[2*size-(t+k+2)] - mu);
                        else
                                rk[k] += (buffer[t] - mu) * (buffer[t+k] - mu);
                }

                rk[k] /= (double)size * std * std;
        }

        for (int i = 0; i < size; i++)
                buffer[i] = rk[i];

        delete[] rk;
}

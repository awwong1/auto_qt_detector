/*

ecg.cpp - ECG annotation console app based lib.cpp ECG library.  
Copyright (C) 2007 YURIY V. CHESNOKOV

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
#include "lib\lib.h"


void help();

int _tmain(int argc, _TCHAR* argv[])
{
    //add your code here
    if( argc < 2 ) {
        help();
    }
    else
    {
        int leadNumber = 0;
        if( argc == 2+1 ) {
            leadNumber = _wtoi( argv[2] ) - 1;
            if( leadNumber < 0 ) leadNumber = 0;
        }

        class Signal signal;
        if( signal.ReadFile( argv[1] ) ) {

            int size = signal.GetLength();
            long double sr = signal.GetSR();
            int h,m,s,ms;
            int msec = int( ((long double)size/sr) * 1000.0 );
            signal.mSecToTime(msec,h,m,s,ms);

            wprintf( L"  leads: %d\n", signal.GetLeadsNum() );
            wprintf( L"     sr: %.2lf Hz\n", sr );
            wprintf( L"   bits: %d\n", signal.GetBits() );
            wprintf( L"    UmV: %d\n", signal.GetUmV() );
            wprintf( L" length: %02d:%02d:%02d.%03d\n\n", h,m,s,ms );

            signal.GetData( leadNumber );
            long double* data = signal.GetData();
            
            //annotation
            class Annotation ann;  //default annotation params

            //or add your custom ECG params to annotation class from lib.h
            // ANNHDR hdr;
            //  hdr.minbpm = 30;
            //  etc...
            // class Annotation ann( &hdr );


            wprintf( L" getting QRS complexes... " );
            int** qrsAnn = ann.GetQRS( data, size, sr, L"filters" );       //get QRS complexes
            //qrsAnn = ann->GetQRS(psig,size,SR,L"filters",qNOISE);    //get QRS complexes if signal is quite noisy

            if( qrsAnn ) {
                wprintf( L" %d beats.\n", ann.GetQRSnum() );
                ann.GetECT( qrsAnn, ann.GetQRSnum(), sr );      //label Ectopic beats

                wprintf( L" getting P, T waves... " );
                int annNum = 0;
                int** ANN = ann.GetPTU( data, size, sr, L"filters", qrsAnn, ann.GetQRSnum() );   //find P,T waves
                if(ANN) {
                    annNum = ann.GetANNnum();
                    wprintf( L" done.\n" );
                }
                else {
                    ANN = qrsAnn;
                    annNum = 2 * ann.GetQRSnum();
                    wprintf( L" failed.\n" );
                }

                wprintf( L"\n" );
                //printing out annotation
                for( int i = 0; i < annNum; i++ ) {
                    int smpl = ANN[i][0];
                    int type = ANN[i][1];

                    msec = int( ((long double)smpl/sr) * 1000.0 );
                    signal.mSecToTime(msec,h,m,s,ms);

                    wprintf( L"  %02d:%02d:%02d.%03d   %s\n", h,m,s,ms, anncodes[type] );
                }

                //saving RR seq
                vector<long double> rrs;
                vector<int> rrsPos;

                if( ann.GetRRseq( ANN, annNum, sr, &rrs, &rrsPos) ) {
                    FILE *fp = _wfopen( L"rrs.txt", L"wt" );
                    for( int i = 0; i < (int)rrs.size(); i++ ) 
                        fwprintf( fp, L"%lf\n", rrs[i] );                    
                    fclose( fp );
                }
                
            }
            else {
                wprintf( L"could not get QRS complexes. make sure you have got \"filters\" directory in the ecg application dir." );
                exit(1);
            }

        }
        else {
            wprintf( L"failed to read %s file", argv[1] );
            exit(1);
        }

    }

	return 0;
}

void help()
{
    wprintf( L"usage: ecg.exe physionetfile.dat leadnum\n\
       do not forget about \\filters dir to be present." );
}


#include <time.h>
#include <windows.h>
#include <stdio.h>
//#include <files.h>
#include <sys/timeb.h>


#define  DllImport __declspec( dllimport )
#define  DLLAPI __declspec( dllexport )

#define NUMTIMERS 100

typedef struct
{
  int iCount;
  unsigned int iTotalTime;
} TIMERSTRUCT;


static BOOL bFirstTime = TRUE;
static TIMERSTRUCT zTime[NUMTIMERS][NUMTIMERS];
static struct _timeb zLastTime;
static int iLastId = 0; 


int InitializeTimer(int iId);


BOOL APIENTRY DllMain (HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
    case DLL_PROCESS_DETACH:
		default: break;
	}

	return TRUE;
}


DLLAPI int STDCALL WINAPI Timer(int iCurrentId)
{
  struct _timeb zCurrentTime;

	if (iCurrentId < 0 || iCurrentId >= NUMTIMERS)
		return -1;

  // first time this function is called, initialize everything
  if (bFirstTime)
  {
		InitializeTimer(iCurrentId);
    bFirstTime = FALSE;
		return 0;
  } // if FirstTime 

	// get current time
  _ftime(&zCurrentTime);

	// Time difference is stored in milliseconds. 
  zTime[iLastId][iCurrentId].iTotalTime += ((zCurrentTime.time - zLastTime.time) * 1000 +
																							zCurrentTime.millitm - zLastTime.millitm);
  zTime[iLastId][iCurrentId].iCount++;

	zLastTime = zCurrentTime;
  iLastId = iCurrentId;
    
	return 0;
} // Timer


int InitializeTimer(int iId)
{
  int i, j;

	for (i = 0; i < NUMTIMERS; i++)
	{
		for (j = 0; j < NUMTIMERS; j++)
    {
			zTime[i][j].iCount = 0;
      zTime[i][j].iTotalTime = 0;
    }
  
    _ftime(&zLastTime);
    iLastId = iId;
	} // for i < NUMTIMER
	
  return 0;
}


DLLAPI void STDCALL WINAPI TimerResult(char *sFileName)
{
	int    i, j, iGTTime;
  double fAvgTime;
	char   sDate[13], sTime[13];
  FILE	 *fp;

	fp = fopen(sFileName, "a");
  if (fp == NULL) 
		return;
	
	_strdate(sDate);
	_strtime(sTime);

	fprintf(fp, "\n\nCurrent Date & Time: %s %s. Following Are the Timer Results\n", sDate, sTime);
  iGTTime = 0;
	for (i = 0; i < NUMTIMERS; i++)
	{
		for(j = 0; j < NUMTIMERS; j++)
    {
      if (zTime[i][j].iCount == 0)
        continue;
 
      iGTTime += zTime[i][j].iTotalTime;
      fAvgTime = (double) (zTime[i][j].iTotalTime) / (double) zTime[i][j].iCount;
      fprintf(fp, "From %d to %d, total time %d ms, number of runs %d, Time/run %.2f ms\n",
						       i, j, zTime[i][j].iTotalTime, zTime[i][j].iCount, fAvgTime);
 
    } // for i
	} // for j
  
	fprintf(fp, "Grand Total Time %d ms\n", iGTTime);
	fclose(fp);
 
}

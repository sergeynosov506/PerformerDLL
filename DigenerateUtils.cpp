#include "DigenerateUtils.h"
#include "DigenerateOps.h"
#include "DigenerateSelectors.h"
#include <stdlib.h>

// Forward declarations for internal cleanup helpers
void FreeDivintUnloadState();
void FreeSelectPortfolioRangeState();

// Forward declarations for internal iteration helpers
int GetSelectPortfolioRangeID();
HRESULT MoveNextSelectPortfolioRange();
void CloseSelectPortfolioRange();

DLLAPI ERRSTRUCT InitializeDivIntGenerateIO(char *sMode, char *sType)
{
    ERRSTRUCT zErr; 
    InitializeErrStruct(&zErr);
    // nanodbc doesn't require explicit preparation of commands in the same way ATL OLE DB does.
    // Connections are managed globally/thread-locally.
    // We just return success here.
    return zErr;
}

void CloseDivIntGenerateIO(void)
{
    // Close any open cursors/statements if necessary
    CloseSelectPortfolioRange();
    FreeDivintUnloadState();
}

DLLAPI void FreeDivIntGenerateIO(void)
{
    CloseDivIntGenerateIO();
    FreeSelectPortfolioRangeState();
    // Other states are cleared in CloseDivIntGenerateIO or don't persist
}

// Helper for GetPortfolioRange
typedef struct
{
  int		*pzPortID;
  int       iSize; /* # of pzInfo elements created using malloc */
  int       iCount;/* # of actual transactions in pzInfo->zTrans */
} PIDTABLE;

ERRSTRUCT AddIdToPortIdTable(PIDTABLE *pzPIdTable, int iID)
{
  ERRSTRUCT zErr;
  InitializeErrStruct(&zErr);

  if (pzPIdTable->iCount == pzPIdTable->iSize)
  {
    pzPIdTable->iSize += 50;
    pzPIdTable->pzPortID = (int *) realloc(pzPIdTable->pzPortID, sizeof(int) * pzPIdTable->iSize);
    if (!pzPIdTable->pzPortID)
      return(PrintError("Insufficient Memory To Create Table", 0, 0, "", 997, 0, 0, "OLEDBIO AddIdToPortIdTable", FALSE));
  }

  pzPIdTable->pzPortID[pzPIdTable->iCount] = iID;
  pzPIdTable->iCount++;

  return zErr;
}

void GetAllPortfolios(PIDTABLE *pzPIdTable, ERRSTRUCT *pzErr)
{
    // Use the internal helpers from DigenerateSelectors.cpp
    // Note: These need to be exposed or declared. I added forward decls above.
    // Ideally they should be in a header, but for now I'm assuming linkage works or I need to add them to DigenerateSelectors.h?
    // Wait, DigenerateSelectors.cpp implements them but they are not DllExport.
    // I need to make sure they are accessible. 
    // I will add prototypes to DigenerateSelectors.h or just extern them here.
    // I added forward declarations above. Linker should find them if they are not static.
    // In DigenerateSelectors.cpp I defined them without static, so they are global symbols.
    
    CloseSelectPortfolioRange(); // Reset cursor
    
    HRESULT hr;
    while ((hr = MoveNextSelectPortfolioRange()) == S_OK)
    {
        *pzErr = AddIdToPortIdTable(pzPIdTable, GetSelectPortfolioRangeID());
        if (pzErr->iSqlError != 0) break;
    }
    
    CloseSelectPortfolioRange();
}

static int c_iIndex = 0;
PIDTABLE PIdTable;

DLLAPI void GetPortfolioRange(int *piStartPortId, int *piEndPortId, int iStepBy, 
								 int *piNoMoreRec, ERRSTRUCT *pzErr)
{
	int i, cnt = 0;

	if (!c_iIndex) 
	{
		PIdTable.iCount = 0;
		PIdTable.iSize = 0;
		PIdTable.pzPortID = NULL;
		GetAllPortfolios(&PIdTable, pzErr);
	}

	for (i = c_iIndex; PIdTable.iCount-1; i++)
	{  
		cnt++;
		*piStartPortId = *piEndPortId;
		if (cnt == iStepBy) 
		{
          *piEndPortId = PIdTable.pzPortID[i];
          c_iIndex = i+1;
          break;
		}
		if (i == PIdTable.iCount - 1) 
		{
			*piEndPortId = PIdTable.pzPortID[i];
			*piNoMoreRec = -1;
			free(PIdTable.pzPortID);
			PIdTable.iCount = 0;
			PIdTable.iSize = 0;
			PIdTable.pzPortID = NULL;
			c_iIndex = 0;
			break;
		}
	}
}

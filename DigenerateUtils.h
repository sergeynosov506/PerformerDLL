#pragma once
#include "OLEDBIOCommon.h"
#include "commonheader.h"

DLLAPI ERRSTRUCT InitializeDivIntGenerateIO(char *sMode, char *sType);
DLLAPI void CloseDivIntGenerateIO(void);
DLLAPI void FreeDivIntGenerateIO(void);
DLLAPI void GetPortfolioRange(int *piStartPortId, int *piEndPortId, int iStepBy, int *piNoMoreRec, ERRSTRUCT *pzErr);

#include "RollIO.h"

void CloseRollIO(void)
{
	// No-op for nanodbc
}

DLLAPI void STDCALL FreeRollIO(void)
{
	// No-op for nanodbc
}

DLLAPI int STDCALL UnprepareRollQueries(char *sTableOrQueryName, int iAction)
{
	// No-op for nanodbc
	return 0;
}

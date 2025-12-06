#pragma once

// Include the new modular headers
#include "MergeQueries.h"
#include "MergeOps.h"
#include "MergeSelectors.h"
#include "MergeUtils.h"

// Maintain existing exports and definitions for backward compatibility
// The original file content was:
/*
#pragma once
#include "OLEDBIOCommon.h"

DLLAPI void STDCALL CloseCompositeMergeIO(void);
DLLAPI void STDCALL FreeCompositeMergeIO(void);
// ... other exports ...
*/

// Since we moved the declarations to MergeUtils.h, we just include it.
// If there were other classes defined in CompositeMergeIO.h, they are now in MergeOps.h or MergeSelectors.h.

// This file now acts as a facade/aggregate header.

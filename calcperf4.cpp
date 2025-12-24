/**
 *
 * SUB-SYSTEM: pmr calcperf
 *
 * FILENAME: calcperf4.ec
 *
 * DESCRIPTION:
 *
 * PUBLIC FUNCTION(S):
 *
 * NOTES:
 *
 * USAGE:
 *
 * AUTHOR:
 *
 * 2024-03-04 J#PER13058 Fix issue w/duplicate asset on prior change - mk.
 * 2024-02-12 J#PER12987 Undid change - mk.
 * 2024-01-02 J#PER12987 Added logic to load historical industry levels for
 * continuing history assets - mk. 2021-05-06 J# PER 11571 Undid change - mk.
 * 2021-04-29 J# PER 11571 Removed Fees Out from NCF - mk.
 * 2021-03-11 J# PER 11415 Restored changes based on new FeesOut logic - mk.
 * 2021-03-03 J# PER-11415 Rolled back changes - mk
 * 2021-02-26 J# PER-11415 Adjustments for feesout based on perf.dlland
 * reporting - mk. 2020-11-11 J# PER-11247 Changed logic on NCF returns -mk.
 * 2020-10-14 J# PER-11169 Controlled calculation of NCF returns -mk.
 * 2020-04-17 J# PER-10655 Added CN transactions -mk.
 * 2017-12-07 VI# 60906 Fixed FindPerfkeyByID if StartIndex = iCount, just
 * return not found - sergeyn 2011-09-21 VI# 46694 More problems fixed -mk
 * 2010-06-16 VI# 42903 Added TodayFeesOut into DAILYINFO - sergeyn
 **/

#include "calcperf.h"
#include <string>

/**
** This function finds the passed asset(by secno and whenissue) in the given
** asset table. Sometimes an additional position(short/long)check is also
** required. If it finds the asset, the index at which it was found is
** returned else -1 is returned.
**/
DllExport int FindAssetInTable(ASSETTABLE2 zATable, char *sSecNo, char *sWi,
                               BOOL bCheckLongShort, short iLongShort) {
  int i, iIndex;

  iIndex = -1;
  i = 0;

  while (iIndex == -1 && i < zATable.iNumAsset) {
    if (strcmp(zATable.pzAsset[i].sSecNo, sSecNo) == 0 &&
        strcmp(zATable.pzAsset[i].sWhenIssue, sWi) == 0) {
      /*
      ** if bCheckLongShort is TRUE, do a bitwise logical and of the entry in
      ** table and the passed value of LongShort.
      */
      if (bCheckLongShort) {
        if ((zATable.pzAsset[i].iLongShort & iLongShort) != 0)
          iIndex = i;
      } else
        iIndex = i;
    } /* secno & wi match */

    i++;
  }

  return iIndex;
} /* findassetintable */

/**
** This function finds the passed holdings(by bracct and transno) in the given
** holdings table. If it finds the holdings, the index at which it was found is
** returned else -1 is returned.
**/
int FindHoldingInTable(HOLDINGTABLE zHTable, int iID, long lTransNo) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zHTable.iNumHolding) {
    if (zHTable.pzHold[i].iID == iID && zHTable.pzHold[i].lTransNo == lTransNo)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findholdingintable */

/**
** This function finds the passed sectype in the global sectype table. If it
** finds it, the index at which it was found is returned else -1 is returned.
**/
int FindSecTypeInTable(PARTSTYPETABLE zPSTTable, int iSecType) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zPSTTable.iNumSType) {
    if (zPSTTable.zSType[i].iSecType == iSecType)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findsectypeintable */

/**
** This function finds the passed trantype in the global trantype table. If it
** finds it, the index at which it was found is returned else -1 is returned.
**/
int FindTranTypeInTable(char *sTranType, char *sDrCr) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zTTypeTable.iNumTType) {
    if (strcmp(zTTypeTable.zTType[i].sTranType, sTranType) == 0 &&
        strcmp(zTTypeTable.zTType[i].sDrCr, sDrCr) == 0)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findtrantypeintable */

/**
** This function finds the passed currid in the global currency table. If it
** finds it, the index at which it was found is returned else -1 is returned.
**/
int FindCurrencyInTable(char *sCurrId) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zCurrTable.iNumCurrency) {
    if (strcmp(zCurrTable.zCurrency[i].sCurrId, sCurrId) == 0)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findcurrencyintable */

/**
** This function finds the passed country code in the global country table. If
* it
** finds it, the index at which it was found is returned else -1 is returned.
**/
int FindCountryInTable(char *sCountryCode) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zCountryTable.iNumCountry) {
    if (strcmp(zCountryTable.zCountry[i].sCode, sCountryCode) == 0)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findcountryintable */

/**
** This function finds the passed script header in the global script header
** table. If it finds it, the index at which it was found is returned else
** -1 is returned.
**/
int FindScrHdrByHdrNo(PSCRHDRDETTABLE zSHDTable, long lSHdrNo) {
  int i, iIndex;
  iIndex = -1;
  i = zSHDTable.iNumSHdrDet - 1;
  if (zSHDTable.iNumSHdrDet == 0) {
    return -1;
  }
  int l = 0;
  int h = zSHDTable.iNumSHdrDet - 1;
  if (zSHDTable.pzSHdrDet[l].zHeader.lScrhdrNo == lSHdrNo)
    iIndex = l;
  else if (zSHDTable.pzSHdrDet[h].zHeader.lScrhdrNo == lSHdrNo)
    iIndex = h;
  else {
    while (l <= h) {
      i = (l + h) / 2;
      if (zSHDTable.pzSHdrDet[i].zHeader.lScrhdrNo == lSHdrNo) {
        iIndex = i;
        break;
      } else if (zSHDTable.pzSHdrDet[i].zHeader.lScrhdrNo > lSHdrNo)
        h = i - 1;
      else
        l = i + 1;
    }
  }
  i = 0;
  if (zSHDTable.pzSHdrDet[zSHDTable.iNumSHdrDet - 1].zHeader.lScrhdrNo <
      lSHdrNo)
    return -1;
  else
    while (iIndex == -1 && i < zSHDTable.iNumSHdrDet) {
      auto header = zSHDTable.pzSHdrDet[i].zHeader;
      if (header.lScrhdrNo == lSHdrNo)
        iIndex = i;

      i++;
    }

  return iIndex;
} /* findscrhdrbyhdrno */

/**
** This function finds a script header in the global script header table by
** matching passed hashkey value and template no. If it finds it, the index at
** which it was found is returned else  -1 is returned.
**/
DllExport int FindScrHdrByHashkeyAndTmpNo(PSCRHDRDETTABLE zSHdrDetTable,
                                          long lHashkey, long lTmphdrNo,
                                          char *sSHAKey,
                                          PSCRHDRDET zSHdrDetail) {
  int i, iIndex;

  // lpfnTimer(10);
  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zSHdrDetTable.iNumSHdrDet) {
    if (lHashkey < 0) {
      if (strcmp(zSHdrDetTable.pzSHdrDet[i].zHeader.sHdrKey, sSHAKey) == 0 &&
          zSHdrDetTable.pzSHdrDet[i].zHeader.lTmphdrNo == lTmphdrNo) {
        if (AreTheseScriptsSame(zSHdrDetTable.pzSHdrDet[i], zSHdrDetail))
          iIndex = i;
      }
    } else {
      if (zSHdrDetTable.pzSHdrDet[i].zHeader.lHashKey == lHashkey &&
          zSHdrDetTable.pzSHdrDet[i].zHeader.lTmphdrNo == lTmphdrNo &&
          strcmp(zSHdrDetTable.pzSHdrDet[i].zHeader.sHdrKey, "") == 0) {
        if (AreTheseScriptsSame(zSHdrDetTable.pzSHdrDet[i], zSHdrDetail))
          iIndex = i;
      }
    }

    i++;
  }
  // lpfnTimer(11);

  return iIndex;
} /* findscrhdrbyhashkeyandtmpno */

/*int FindScrHdrBySegmentTypeAndTmpNo(int iSegmentTypeID, long lTmphdrNo)
{
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zSHdrDetTable.iNumSHdrDet)
  {
    if (zSHdrDetTable.pzSHdrDet[i].zHeader.iSegmentTypeID == iSegmentTypeID &&
        zSHdrDetTable.pzSHdrDet[i].zHeader.lTmphdrNo == lTmphdrNo)
      iIndex = i;

    i++;
  }

  return iIndex;
} / * FindScrHdrBySegmentTypeAndTmpNo */

/**
** This function finds the passed template header in the global template header
** table. If it finds it, the index at which it was found is returned else
** -1 is returned.
**/
int FindTemplateHeaderInTable(PTMPHDRDETTABLE zTHdrDetTable, long lTHdrNo) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zTHdrDetTable.iNumTHdrDet) {
    if (zTHdrDetTable.pzTHdrDet[i].zHeader.lTmphdrNo == lTHdrNo)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findtemplateheaderintable */

/**
** Function to find perfkey by matching perfkeyno in the
** given PKeyTable.
**/
int FindPerfkeyByPerfkeyNo(PKEYTABLE zPKTable, long lPKNo) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zPKTable.iCount) {
    if (zPKTable.pzPKey[i].zPK.lPerfkeyNo == lPKNo)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findperfkeyByPerfkeyNo */

/**
** Function to find perfkey by matching id in the given PKeyTable.
**/
int FindPerfkeyByID(PKEYTABLE zPKTable, int iID, int iStartIndex,
                    BOOL bSkipNewAndDeleted) {
  int i, iIndex;

  iIndex = -1;
  if (iStartIndex > 0 && iStartIndex < zPKTable.iCount)
    i = iStartIndex;
  else if (iStartIndex == zPKTable.iCount)
    i = zPKTable.iCount;
  else
    i = 0;
  while (iIndex == -1 && i < zPKTable.iCount) {
    if (bSkipNewAndDeleted &&
        (zPKTable.pzPKey[i].bDeletedFromDB || zPKTable.pzPKey[i].bNewKey ||
         IsKeyDeleted(zPKTable.pzPKey[i].zPK.lDeleteDate))) {
      i++;
      continue;
    }

    if (zPKTable.pzPKey[i].zPK.iID == iID)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* findperfkeyByID */

/**
** Function to find perfkey by matching ruleno, script hdr no and curr proc flag
** in the give PKeyTable. PerfRule has three flags, aggregate base flag,
** currency base flag and currency local flag, which one of these needs to be
** matched is indicated by the fourth argument, "CurrFlagNo"(1 - check currency
** base flag, 2 - check currency local flag, else check aggregate base flag).
**/
int FindPerfkeyByRule(PKEYTABLE zPKTable, long lRuleNo, long lScrhdrNo,
                      long lCurrFlagNo) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zPKTable.iCount) {
    /* If the key in the memory table is deleted, ignore it */
    if (IsKeyDeleted(zPKTable.pzPKey[i].zPK.lDeleteDate) == TRUE) {
      i++;
      continue;
    }

    if (zPKTable.pzPKey[i].zPK.lRuleNo == lRuleNo &&
        zPKTable.pzPKey[i].zPK.lScrhdrNo == lScrhdrNo) {
      switch (lCurrFlagNo) {
        /* Currency Local Flag should be 'L' */
      case 1:
        if (strcmp(zPKTable.pzPKey[i].zPK.sCurrProc, "L") == 0)
          iIndex = i;
        break;

        /* Currency Base Flag should be 'B' */
      case 2:
        if (strcmp(zPKTable.pzPKey[i].zPK.sCurrProc, "B") == 0)
          iIndex = i;
        break;

        /* Aggregate Base Flag should be 'A' */
      default:
        if (strcmp(zPKTable.pzPKey[i].zPK.sCurrProc, "A") == 0)
          iIndex = i;
      } /* switch */
    } /* if ruleno and scrhdrno match */

    i++;
  } /* while */

  return iIndex;
} /* findperfkeyByrule */

/**
** Function to find given perfkey in the ValidDatePRuel Table
**/
int FindPerfkeyInValidDPRTable(VALIDDPTABLE zVDPTable, long lPerfkeyNo) {
  int i, j;

  i = 0;
  j = -1;
  while (j == -1 && i < zVDPTable.iNumVDPR) {
    if (zVDPTable.pzVDPR[i].lPerfkeyNo == lPerfkeyNo)
      j = i;

    i++;
  }

  return j;
} /* FindPerfkeyInValidDPRTable */

/**
** Function to find the give rule number in the rule table
**/
int FindRule(PERFRULETABLE zRTable, long lRuleNo) {
  int i, j;

  j = -1;
  i = 0;
  while (j == -1 && i < zRTable.iCount) {
    if (zRTable.pzPRule[i].lRuleNo == lRuleNo)
      j = i;

    i++;
  }

  return j;
} /* FindRule */

/**
** Function to find the given segmap record in the global segmap table
** /
int FindSegmap(int iSegmentID)
{
  int i, j;

  j = -1;
  i = 0;
  while (j == -1 && i < zSegmapTable.iNumSegmap)
  {
    if (zSegmapTable.pzSegmap[i].iSegmentID == iSegmentID)
      j = i;

    i++;
  }

  return j;
} / * FindSegmap */

/**
** Function to find the given Segments ID in the global segments table
** /
int FindSegments(int iID)
{
  int i, j;

  j = -1;
  i = 0;
  while (j == -1 && i < zSegmentsTable.iNumSegments)
  {
    if (zSegmentsTable.pzSegments[i].iId == iID)
      j = i;

    i++;
  }

  return j;
} / * FindSegments */

/**
** This function adds a perfkey to the perfkey table. It doesn't check whether
** the perfkey already exist in the table or not, it just adds it.
**/
ERRSTRUCT AddPerfkeyToTable(PKEYTABLE *pzPKeyTable, PKEYSTRUCT zPKey) {
  ERRSTRUCT zErr;
  int i;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  /* If table is full to its limit, allocate more space */
  if (pzPKeyTable->iCapacity == pzPKeyTable->iCount) {
    pzPKeyTable->iCapacity += EXTRAPKEY;
    pzPKeyTable->pzPKey = (PKEYSTRUCT *)realloc(
        pzPKeyTable->pzPKey, pzPKeyTable->iCapacity * sizeof(PKEYSTRUCT));
    if (pzPKeyTable->pzPKey == NULL)
      return (lpfnPrintError("Insufficient Memory For PKeyTable", 0, 0, "", 997,
                             0, 0, "CALCPERF ADDPKEY", FALSE));

    // sprintf_s(smsg, "Memory Address For Pkey Table is; %Fp",
    // pzPKeyTable->pzPKey); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  i = pzPKeyTable->iCount;
  pzPKeyTable->pzPKey[i].iDInfoCapacity = 0;
  pzPKeyTable->pzPKey[i].iWDInfoCapacity = 0;
  InitializePKeyStruct(&pzPKeyTable->pzPKey[i]);

  pzPKeyTable->pzPKey[i] = zPKey;
  /* Copy all the fields and then null the pointers */
  pzPKeyTable->pzPKey[i].iDInfoCapacity = pzPKeyTable->pzPKey[i].iDInfoCount =
      0;
  pzPKeyTable->pzPKey[i].iWDInfoCapacity = pzPKeyTable->pzPKey[i].iWDInfoCount =
      0;
  pzPKeyTable->pzPKey[i].pzDInfo = NULL;
  pzPKeyTable->pzPKey[i].pzTInfo = NULL;
  pzPKeyTable->pzPKey[i].pzWDInfo = NULL;

  pzPKeyTable->iCount++;

  return zErr;
} /* AddPerfKeyToTable */

/**
** This function adds a WtdDailyInfo to the given perfkey.
**/
ERRSTRUCT AddWtdDailyInfoToPerfkey(PKEYSTRUCT *pzPKey, WTDDAILYINFO zWDInfo) {
  ERRSTRUCT zErr;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  /* If required, allocate more space */
  if (pzPKey->iWDInfoCapacity == pzPKey->iWDInfoCount) {
    pzPKey->iWDInfoCapacity += EXTRAWTDDINFO;
    pzPKey->pzWDInfo = (WTDDAILYINFO *)realloc(
        pzPKey->pzWDInfo, pzPKey->iWDInfoCapacity * sizeof(WTDDAILYINFO));
    if (pzPKey->pzWDInfo == NULL)
      return (lpfnPrintError("Insufficient Memory For PKey", 0, 0, "", 997, 0,
                             0, "CALCPERF ADDWTDDINFO", FALSE));

    // sprintf_s(smsg, "Memory Address For PKey is; %Fp", pzPKey->pzWDInfo);
    // lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG", TRUE);
  }

  pzPKey->pzWDInfo[pzPKey->iWDInfoCount] = zWDInfo;
  pzPKey->iWDInfoCount++;

  return zErr;
} /* AddWtdDailyInfoToPerfkey */

/**
** This function adds an asset in the asset table, if it doesn't already exist
** there. The third argument is set to the index at which it found/added the
** passed asset.
**/
DllExport ERRSTRUCT AddAssetToTable(ASSETTABLE2 *pzAssetTable,
                                    PARTASSET2 zPAsset, LEVELINFO zLevels,
                                    PARTSTYPETABLE zPSTTable, int *piArrayIndex,
                                    long lLastPerfDate, long lCurrentPerfDate) {
  ERRSTRUCT zErr;
  int i;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  /* First try to find Asset with LongShort Bit Set */
  *piArrayIndex =
      FindAssetInTable(*pzAssetTable, zPAsset.sSecNo, zPAsset.sWhenIssue, TRUE,
                       zPAsset.iLongShort);
  if (*piArrayIndex >= 0) /* asset already exist in the table */
    // even if asset already exist, one of its industry level may be different
    // starting on a specif date, so add it in the array
    return (AddDailyInfoToAsset(&pzAssetTable->pzAsset[*piArrayIndex], zLevels,
                                lLastPerfDate, lCurrentPerfDate));

  /*
  ** If asset not found with LongShort bit set, try to find it without checking
  * LongShort. If found, modify its
  ** LongShort field(do a bitwise OR with the passed value)
  */
  *piArrayIndex = FindAssetInTable(*pzAssetTable, zPAsset.sSecNo,
                                   zPAsset.sWhenIssue, FALSE, 0);
  if (*piArrayIndex >= 0) /* asset already exist in the table */
  {
    pzAssetTable->pzAsset[*piArrayIndex].iLongShort =
        pzAssetTable->pzAsset[*piArrayIndex].iLongShort | zPAsset.iLongShort;
    return (AddDailyInfoToAsset(&pzAssetTable->pzAsset[*piArrayIndex], zLevels,
                                lLastPerfDate, lCurrentPerfDate));
  }

  /* If table is full to its limit, allocate more space */
  if (pzAssetTable->iAssetCreated == pzAssetTable->iNumAsset) {
    pzAssetTable->iAssetCreated += EXTRAASSET;
    pzAssetTable->pzAsset =
        (PARTASSET2 *)realloc(pzAssetTable->pzAsset,
                              pzAssetTable->iAssetCreated * sizeof(PARTASSET2));
    if (pzAssetTable->pzAsset == NULL)
      return (lpfnPrintError("Insufficient Memory For AssetTable", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDASSET", FALSE));

    // initialize the extra spaces being created
    for (i = pzAssetTable->iNumAsset; i < pzAssetTable->iAssetCreated; i++) {
      pzAssetTable->pzAsset[i].iDailyCount = 0;
      InitializePartialAsset(&pzAssetTable->pzAsset[i]);
    }
  } // if more space needs to be created

  *piArrayIndex = pzAssetTable->iNumAsset++;
  zPAsset.iSTypeIndex = FindSecTypeInTable(zPSTTable, zPAsset.iSecType);
  if (zPAsset.iSTypeIndex < 0)
    return (lpfnPrintError("Invalid Sectype For Asset", 0, 0, "", 997, 0, 0,
                           "CALCPERF ADDASSET2", FALSE));

  // copy everything except dailyinfo
  memcpy(&pzAssetTable->pzAsset[*piArrayIndex], &zPAsset,
         sizeof(zPAsset) - sizeof(zPAsset.pzDAInfo) -
             sizeof(zPAsset.iDailyCount));
  pzAssetTable->pzAsset[*piArrayIndex].iDailyCount = 0;
  pzAssetTable->pzAsset[*piArrayIndex].pzDAInfo = NULL;

  // add daily info
  return (AddDailyInfoToAsset(&pzAssetTable->pzAsset[*piArrayIndex], zLevels,
                              lLastPerfDate, lCurrentPerfDate));
} /* AddAssetToTable */

/**
** This function adds a script header to the global script header detail table,
** if it doesn't already exist there. The third argument is set to the index at
** which it found/added the passed script header.
**/
DllExport ERRSTRUCT AddScriptHeaderToTable(PSCRHDR zPSHeader,
                                           PSCRHDRDETTABLE *pzSHDTable,
                                           int *piArrayIndex) {
  ERRSTRUCT zErr;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  *piArrayIndex = FindScrHdrByHdrNo(*pzSHDTable, zPSHeader.lScrhdrNo);
  if (*piArrayIndex >= 0) /* script header already exist in the table */
    return zErr;

  /* If table is full to its limit, allocate more space */
  if (pzSHDTable->iSHdrDetCreated == pzSHDTable->iNumSHdrDet) {
    pzSHDTable->iSHdrDetCreated += EXTRASHDRDET;
    pzSHDTable->pzSHdrDet =
        (PSCRHDRDET *)realloc(pzSHDTable->pzSHdrDet,
                              pzSHDTable->iSHdrDetCreated * sizeof(PSCRHDRDET));
    if (pzSHDTable->pzSHdrDet == NULL)
      return (lpfnPrintError("Insufficient Memory For ScrHdrDetTable", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDSCRHEADER", FALSE));
    // sprintf_s(smsg, "Memory Address For ScrHdrDet Table is; %Fp",
    // pzSHDTable->pzSHdrDet); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  *piArrayIndex = pzSHDTable->iNumSHdrDet++;
  pzSHDTable->pzSHdrDet[*piArrayIndex].zHeader = zPSHeader;
  pzSHDTable->pzSHdrDet[*piArrayIndex].iDetailCreated = 0;
  pzSHDTable->pzSHdrDet[*piArrayIndex].iNumDetail = 0;
  pzSHDTable->pzSHdrDet[*piArrayIndex].pzDetail = NULL;
  return zErr;
} /* AddScrHeaderToTable */

/**
** This function adds perf script detail record to a script header-detail
** structure. When the number of details created for the header is zero(the
** function is called with the first detail record for the header), memory for
** elements equal to sequnce number of the passed Detail is allocated. After
** that there should be no need to allocate more memory for the key(although
** this function will allocate more memory if need be). The reason for all this
** is that whenevr the records are added in the scriptdetail table, SeqNo is
** alway incremented in sequence, starting at 1 for each header and the cursor
** which retrieves them does that in DESCending order of SeqNo, so the first
** fetched detail record, for each header, should tell us how many detail
** records exist for the header.  The passed detail record is not added to the
** header at the next available space in the array, rather it is added at the
** ith element in the array, where i = SeqNo of the passed detail record.
**/
DllExport ERRSTRUCT AddDetailToScrHdrDet(PSCRDET zSDetail,
                                         PSCRHDRDET *pzScrHdrDet) {
  ERRSTRUCT zErr;
  BOOL bAllocateMemory;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  /*
  ** If the records in detail table(in the database) are entered in right order
  ** (in sequence, starting at 1, for every header) and cursor declaration has
  ** not been changed(ORDER BY scrhdr_no, seq_no DESC) then the first time this
  ** function is called for a header, enough memory will be allocated for all
  ** the details record for that header. But this function should not blow up if
  ** somebody changed the cursor or detail records with wrong seq_no are entered
  */
  if (pzScrHdrDet->iNumDetail == 0) {
    bAllocateMemory = TRUE;
    pzScrHdrDet->iDetailCreated = zSDetail.lSeqNo;
  } else if (pzScrHdrDet->iDetailCreated == pzScrHdrDet->iNumDetail) {
    bAllocateMemory = TRUE;
    if (zSDetail.lSeqNo > pzScrHdrDet->iDetailCreated)
      pzScrHdrDet->iDetailCreated = zSDetail.lSeqNo;
    else
      pzScrHdrDet->iDetailCreated++;
  } /* array is full */
  else
    bAllocateMemory = FALSE;

  if (zSDetail.lSeqNo > pzScrHdrDet->iDetailCreated)
    return (lpfnPrintError("Invalid SeqNo in PerfScrDet Table", 0, 0, "", 504,
                           0, 0, "CALCPERF ADDSCRDET2", FALSE));

  if (bAllocateMemory) {
    pzScrHdrDet->pzDetail = (PSCRDET *)realloc(
        pzScrHdrDet->pzDetail, pzScrHdrDet->iDetailCreated * sizeof(PSCRDET));
    if (pzScrHdrDet->pzDetail == NULL)
      return (lpfnPrintError("Insufficient Memory For PerfScrDet", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDSCRDET3", FALSE));

    // sprintf_s(smsg, "Memory Address For PerfScrDet is; %Fp",
    // pzScrHdrDet->pzDetail); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  pzScrHdrDet->pzDetail[zSDetail.lSeqNo - 1] = zSDetail;
  pzScrHdrDet->iNumDetail++;

  return zErr;
} /* adddetailtoscrhdrdet */

/**
** This function adds a template header to the global template header-detail
** table, if it doesn't already exist there. The third argument is set to the
** index at which it found/added the passed template header.
**/
DllExport ERRSTRUCT AddTemplateHeaderToTable(PTMPHDRDETTABLE *pzTHdrDetTable,
                                             PTMPHDR zPTHeader,
                                             int *piArrayIndex) {
  ERRSTRUCT zErr;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  *piArrayIndex =
      FindTemplateHeaderInTable(*pzTHdrDetTable, zPTHeader.lTmphdrNo);
  if (*piArrayIndex >= 0) /* template header already exist in the table */
    return zErr;

  /* If table is full to its limit, allocate more space */
  if (pzTHdrDetTable->iTHdrDetCreated == pzTHdrDetTable->iNumTHdrDet) {
    pzTHdrDetTable->iTHdrDetCreated += EXTRATHDRDET;
    pzTHdrDetTable->pzTHdrDet = (PTMPHDRDET *)realloc(
        pzTHdrDetTable->pzTHdrDet,
        pzTHdrDetTable->iTHdrDetCreated * sizeof(PTMPHDRDET));
    if (pzTHdrDetTable->pzTHdrDet == NULL)
      return (lpfnPrintError("Insufficient Memory For TmpHdrDetTable", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDTMPHEADER", FALSE));

    // sprintf_s(smsg, "Memory Address For TmpHdrdet Table is; %Fp",
    // pzTHdrDetTable->pzTHdrDet); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0,
    // "DEBUG", TRUE);
  }

  *piArrayIndex = pzTHdrDetTable->iNumTHdrDet++;
  pzTHdrDetTable->pzTHdrDet[*piArrayIndex].zHeader = zPTHeader;
  pzTHdrDetTable->pzTHdrDet[*piArrayIndex].iCapacity = 0;
  pzTHdrDetTable->pzTHdrDet[*piArrayIndex].iCount = 0;
  pzTHdrDetTable->pzTHdrDet[*piArrayIndex].pzDetail = NULL;

  return zErr;
} /* AddTmpHeaderToTable */

/**
** This function adds perf template detail record to the given template header
** in the global templateheaderdet table. When the number of details created for
** the header is zero(the function is called with the first detail record for
** the header), memory for elements equal to sequnce number of the passed Detail
** is allocated. After that there should be no need to allocate more memory for
** the key(although this function will allocate more memory if need be). The
** reason for all this is that whenever the records are added in the
** templatedetail table, SeqNo is alway incremented in sequence, starting at 1
** for each header and the cursor which retrieves them does that in DESCending
** order of SeqNo, so the first fetched detail record, for each header, should
** tell us how many detail records exist for the header. The passed detail
** record is not added to the header at the next avialable space in the array,
** rather it is added at the ith element in the array, where i = SeqNo of the
** passed detail record.
**/
DllExport ERRSTRUCT AddTemplateDetailToTable(PTMPHDRDETTABLE *pzTHdrDetTable,
                                             PTMPDET zTDetail,
                                             int iHeaderIndex) {
  ERRSTRUCT zErr;
  BOOL bAllocateMemory;
  char sErrMsg[80];
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  if (iHeaderIndex < 0 || iHeaderIndex > pzTHdrDetTable->iNumTHdrDet)
    return (lpfnPrintError("Invalid Header Index", 0, 0, "", 999, 0, 0,
                           "CALCPERF ADDTMPDET1", FALSE));

  /*
  ** If the records in detail table(in the database) are entered in right order
  ** (in sequence, starting at 1, for every header) and cursor declaration has
  ** not been changed(ORDER BY scrhdr_no, seq_no DESC) then the first time this
  ** function is called for a header, enough memory will be allocated for all
  ** the details record for that header. But this function should not blow up if
  ** somebody changed the cursor or detail records with wrong seq_no are entered
  */
  if (pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCount == 0) {
    bAllocateMemory = TRUE;
    pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCapacity = zTDetail.lSeqNo;
  } else if (pzTHdrDetTable->iTHdrDetCreated == pzTHdrDetTable->iNumTHdrDet) {
    bAllocateMemory = TRUE;
    if (zTDetail.lSeqNo > pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCapacity)
      pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCapacity = zTDetail.lSeqNo;
    else
      pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCapacity++;
  } /* array is full */
  else
    bAllocateMemory = FALSE;

  if (zTDetail.lSeqNo < 1 ||
      zTDetail.lSeqNo > pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCapacity) {
    sprintf_s(sErrMsg, "Invalid SeqNo - %d In Template - %d", zTDetail.lSeqNo,
              zTDetail.lTmphdrNo);
    return (lpfnPrintError(sErrMsg, 0, 0, "", 504, 0, 0, "CALCPERF ADDTMPDET2",
                           FALSE));
  }

  if (bAllocateMemory) {
    pzTHdrDetTable->pzTHdrDet[iHeaderIndex].pzDetail = (PTMPDET *)realloc(
        pzTHdrDetTable->pzTHdrDet[iHeaderIndex].pzDetail,
        pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCapacity * sizeof(PTMPDET));
    if (pzTHdrDetTable->pzTHdrDet[iHeaderIndex].pzDetail == NULL)
      return (lpfnPrintError("Insufficient Memory For PerfTmpDet", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDTMPDET3", FALSE));

    // sprintf_s(smsg, "Memory Address For Ror PerfTmpdet is; %Fp",
    // pzTHdrDetTable->pzTHdrDet[iHeaderIndex].pzDetail); lpfnPrintError(smsg,
    // 0, 0, "", 0, 0, 0, "DEBUG", TRUE);
  }

  pzTHdrDetTable->pzTHdrDet[iHeaderIndex].pzDetail[zTDetail.lSeqNo - 1] =
      zTDetail;
  pzTHdrDetTable->pzTHdrDet[iHeaderIndex].iCount++;

  return zErr;
} /* addtemplatedetailtotable */

/**
** This function adds a segmap to the global segmap table.
** /
ERRSTRUCT AddSegmapToTable(SEGMAP zSegmap)
{
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  // If table is full to its limit, allocate more space
  if (zSegmapTable.iSegmapCreated == zSegmapTable.iNumSegmap)
  {
    zSegmapTable.iSegmapCreated += EXTRASEGMAP;
    zSegmapTable.pzSegmap = (SEGMAP *)realloc(zSegmapTable.pzSegmap,
zSegmapTable.iSegmapCreated * sizeof(SEGMAP)); if (zSegmapTable.pzSegmap ==
NULL) return(lpfnPrintError("Insufficient Memory For SegmapTable", 0, 0, "",
997, 0, 0, "CALCPERF ADDSEGMAP", FALSE));
  }

  zSegmapTable.pzSegmap[zSegmapTable.iNumSegmap] = zSegmap;
  zSegmapTable.iNumSegmap++;

  return zErr;
} / * AddSegmapToTable */

/**
** This function adds a segments to the global segments table.
** /
ERRSTRUCT AddSegmentsToTable(SEGMENTS zSegments)
{
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  // If table is full to its limit, allocate more space
  if (zSegmentsTable.iSegmentsCreated == zSegmentsTable.iNumSegments)
  {
    zSegmentsTable.iSegmentsCreated += EXTRASEGMENTS;
    zSegmentsTable.pzSegments = (SEGMENTS *)realloc(zSegmentsTable.pzSegments,
zSegmentsTable.iSegmentsCreated * sizeof(SEGMENTS)); if
(zSegmentsTable.pzSegments == NULL) return(lpfnPrintError("Insufficient Memory
For SegmentsTable", 0, 0, "", 997, 0, 0, "CALCPERF ADDSEGMENTS", FALSE));
  }

  zSegmentsTable.pzSegments[zSegmentsTable.iNumSegments] = zSegments;
  zSegmentsTable.iNumSegments++;

  return zErr;
} / * AddSegmentsToTable */

/**
** This function adds a holding/holdcash record in the holding table. This
** function does not check whether the passed holding record already exist in
** the table or not.
**/
ERRSTRUCT AddHoldingToTable(HOLDINGTABLE *pzHoldTable, PARTHOLDING zPHold) {
  ERRSTRUCT zErr;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  /* If table is full to its limit, allocate more space */
  if (pzHoldTable->iHoldingCreated == pzHoldTable->iNumHolding) {
    pzHoldTable->iHoldingCreated += EXTRAHOLD;
    pzHoldTable->pzHold = (PARTHOLDING *)realloc(pzHoldTable->pzHold,
                                                 pzHoldTable->iHoldingCreated *
                                                     sizeof(PARTHOLDING));
    if (pzHoldTable->pzHold == NULL)
      return (lpfnPrintError("Insufficient Memory For HoldTable", 0, 0, "", 997,
                             0, 0, "CALCPERF ADDHOLD", FALSE));

    // sprintf_s(smsg, "Memory Address For Hold Table is; %Fp",
    // pzHoldTable->pzHold); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  pzHoldTable->pzHold[pzHoldTable->iNumHolding] = zPHold;
  pzHoldTable->iNumHolding++;

  return zErr;
} /* AddHoldingtotable */

/**
** This function adds trans record in the trans table. This function does not
** check whether the passed trans record already exist in the table or not.
**/
ERRSTRUCT AddTransToTable(TRANSTABLE *pzTransTable, PARTTRANS zPTrans) {
  // char smsg[100];
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  /* If table is full to its limit, allocate more space */
  if (pzTransTable->iTransCreated == pzTransTable->iNumTrans) {
    pzTransTable->iTransCreated += EXTRATRANS;
    pzTransTable->pzTrans = (PARTTRANS *)realloc(
        pzTransTable->pzTrans, pzTransTable->iTransCreated * sizeof(PARTTRANS));
    if (pzTransTable->pzTrans == NULL)
      return (lpfnPrintError("Insufficient Memory For Trans Table", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDTRANS", FALSE));

    // sprintf_s(smsg, "Memory Address For Trans Table is; %Fp",
    // pzTransTable->pzTrans); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  pzTransTable->pzTrans[pzTransTable->iNumTrans] = zPTrans;
  pzTransTable->iNumTrans++;

  return zErr;
} /* addtranstotable */

/**
** Function to add a perfrule to perfrule table.
**/
DllExport ERRSTRUCT AddPerfruleToTable(PERFRULETABLE *pzPRTable,
                                       PERFRULE zPRule,
                                       PTMPHDRDETTABLE zTHdrDetTable) {
  ERRSTRUCT zErr;
  char sErrMsg[80];
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  if (pzPRTable->iCapacity == pzPRTable->iCount) {
    pzPRTable->iCapacity += EXTRAPRULE;
    pzPRTable->pzPRule = (PERFRULE *)realloc(
        pzPRTable->pzPRule, pzPRTable->iCapacity * sizeof(PERFRULE));
    if (pzPRTable->pzPRule == NULL)
      return (lpfnPrintError("Insufficient Memory For Perfrule Table", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDPERFRULE1", FALSE));

    // sprintf_s(smsg, "Memory Address For Perfrule Table is; %Fp",
    // pzPRTable->pzPRule); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);

    pzPRTable->piTHDIndex = (int *)realloc(pzPRTable->piTHDIndex,
                                           pzPRTable->iCapacity * sizeof(int));
    if (pzPRTable->piTHDIndex == NULL)
      return (lpfnPrintError("Insufficient Memory For Perfrule Table", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDPERFRULE2", FALSE));

    // sprintf_s(smsg, "Memory Address For Perfrule (thdindex) Table is; %Fp",
    // pzPRTable->piTHDIndex); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  pzPRTable->piTHDIndex[pzPRTable->iCount] =
      FindTemplateHeaderInTable(zTHdrDetTable, zPRule.lTmphdrNo);
  if (pzPRTable->piTHDIndex[pzPRTable->iCount] < 0) {
    sprintf_s(sErrMsg, "Template - %d For Perfrule - %d Does Not Exist",
              zPRule.lTmphdrNo, zPRule.lRuleNo);
    return (lpfnPrintError(sErrMsg, zPRule.iPortfolioID, 0, "", 514, 0, 0,
                           "CALCPERF ADDPERFRULE3", FALSE));
  }

  pzPRTable->pzPRule[pzPRTable->iCount] = zPRule;
  pzPRTable->iCount++;

  return zErr;
} /* addperfruletotable */

/**
** This function adds the given rule number to the parent rule table. If the
** given rule already exists in the table, it is not added. If the table is
** full, more space is allocated to it before adding the rule.
**/
ERRSTRUCT AddParentRuleIfNew(PARENTRULETABLE *pzPRTable, long lRuleNo) {
  ERRSTRUCT zErr;
  int i, j;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  i = -1;
  j = 0;
  while (i == -1 && j < pzPRTable->iCount) {
    if (pzPRTable->plPRule[j] == lRuleNo)
      i = j;

    j++;
  }

  /* If rule already exists in the table, return without doing anything */
  if (i != -1)
    return zErr;

  /* If table is full, allocate more space */
  if (pzPRTable->iCapacity == pzPRTable->iCount) {
    pzPRTable->iCapacity += EXTRAPRULE;
    pzPRTable->plPRule = (long *)realloc(pzPRTable->plPRule,
                                         pzPRTable->iCapacity * sizeof(long));
    if (pzPRTable->plPRule == NULL)
      return (lpfnPrintError("Insufficient Memory For Parent Perfrule Table", 0,
                             0, "", 997, 0, 0, "CALCPERF ADDPARENTPERFRULE",
                             FALSE));

    // sprintf_s(smsg, "Memory Address For Parent perfrule Table is; %Fp",
    // pzPRTable->plPRule); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  pzPRTable->plPRule[pzPRTable->iCount] = lRuleNo;
  pzPRTable->iCount++;

  return zErr;
} /* AddParentRuleIfNew */

/**
** This function adds the given validdateprule to validdptable. If the table is
** full, more space is allocated to it before adding the new record.
**/
ERRSTRUCT AddValidDatePRuleToTable(VALIDDPTABLE *pzDPTable,
                                   VALIDDATEPRULE zVDPRecord) {
  ERRSTRUCT zErr;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  /* If table is full, allocate more space */
  if (pzDPTable->iVDPRCreated == pzDPTable->iNumVDPR) {
    pzDPTable->iVDPRCreated += EXTRAVALIDDPR;
    pzDPTable->pzVDPR = (VALIDDATEPRULE *)realloc(
        pzDPTable->pzVDPR, pzDPTable->iVDPRCreated * sizeof(VALIDDATEPRULE));
    if (pzDPTable->pzVDPR == NULL)
      return (lpfnPrintError("Insufficient Memory For Parent Perfrule Table", 0,
                             0, "", 997, 0, 0, "CALCPERF ADDPARENTPERFRULE",
                             FALSE));
    // sprintf_s(smsg, "Memory Address For ParentPerfrule 2 Table is; %Fp",
    // pzDPTable->pzVDPR); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  pzDPTable->pzVDPR[pzDPTable->iNumVDPR] = zVDPRecord;
  pzDPTable->iNumVDPR++;

  return zErr;
} /* AddParentRuleIfNew */

/**
** This function adds the given string to ResultList. If the List is
** full, more space is allocated to it before adding the new record.
**/
ERRSTRUCT AddAnItemToResultList(RESULTLIST *pzNewList, char *sStr) {
  ERRSTRUCT zErr;
  //	char smsg[100];

  lpprInitializeErrStruct(&zErr);

  if (strlen(sStr) > sizeof(STRING2))
    return (lpfnPrintError("String Too Big For The Structure", 0, 0, "", 999, 0,
                           0, "CALCPERF ADDITEMTORESULTLIST1", FALSE));

  // If list is full, allocate more space
  if (pzNewList->iCapacity == pzNewList->iCount) {
    pzNewList->iCapacity += EXTRARESULTITEM;
    pzNewList->sItem = (STRING2 *)realloc(
        pzNewList->sItem, pzNewList->iCapacity * sizeof(STRING2));
    if (pzNewList->sItem == NULL)
      return (lpfnPrintError("Insufficient Memory For Result Set", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDITEMTORESULTLIST2",
                             FALSE));
    // sprintf_s(smsg, "Memory Address For Ror Table is; %Fp",
    // pzNewList->sItem); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  strcpy_s(pzNewList->sItem[pzNewList->iCount].sValue, sStr);
  pzNewList->iCount++;

  return zErr;
} // AddAnItemToReturnList

/**
** This function adds an accdiv record to the accdiv table. This function does
* not
** check whether the passed accdiv record already exist in the table or not.
**/
ERRSTRUCT AddAccdivToTable(ACCDIVTABLE *pzAccdivTable, PARTACCDIV zPAccdiv) {
  ERRSTRUCT zErr;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  // If table is full to its limit, allocate more space
  if (pzAccdivTable->iAccdivCreated == pzAccdivTable->iNumAccdiv) {
    pzAccdivTable->iAccdivCreated += EXTRAACCDIV;
    pzAccdivTable->pzAccdiv = (PARTACCDIV *)realloc(
        pzAccdivTable->pzAccdiv,
        pzAccdivTable->iAccdivCreated * sizeof(PARTACCDIV));
    if (pzAccdivTable->pzAccdiv == NULL)
      return (lpfnPrintError("Insufficient Memory For Accdiv Table", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDACCDIV", FALSE));
    // sprintf_s(smsg, "Memory Address For Accdiv Table is; %Fp",
    // pzAccdivTable->pzAccdiv); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0,
    // "DEBUG", TRUE);
  }

  pzAccdivTable->pzAccdiv[pzAccdivTable->iNumAccdiv] = zPAccdiv;
  pzAccdivTable->iNumAccdiv++;

  return zErr;
} // AddAccdivtotable

/**
** This function adds an porttax record to the porttax table. This function does
* not
** check whether the passed porttax record already exist in the table or not.
**/
ERRSTRUCT AddPorttaxToTable(PORTTAXTABLE *pzPTaxTable, PORTTAX zPTax) {
  ERRSTRUCT zErr;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  // If table is full to its limit, allocate more space
  if (pzPTaxTable->iCapacity == pzPTaxTable->iCount) {
    pzPTaxTable->iCapacity += EXTRAPORTTAX;
    pzPTaxTable->pzPTax = (PORTTAX *)realloc(
        pzPTaxTable->pzPTax, pzPTaxTable->iCapacity * sizeof(PORTTAX));
    if (pzPTaxTable->pzPTax == NULL)
      return (lpfnPrintError("Insufficient Memory For PorttaxTable", 0, 0, "",
                             997, 0, 0, "CALCPERF ADDPORTTAX", FALSE));
    // sprintf_s(smsg, "Memory Address For Porttax Table is; %Fp",
    // pzPTaxTable->pzPTax); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0, "DEBUG",
    // TRUE);
  }

  pzPTaxTable->pzPTax[pzPTaxTable->iCount] = zPTax;
  pzPTaxTable->iCount++;

  return zErr;
} // AddPorttaxtotable

/*
** This function adds a perfassetmerge record to the perfassetmerge table. This
* function does not
** check whether the passed perfassetmerge record already exist in the table or
* not.
*/
ERRSTRUCT AddPerfAssetMergeToTable(PERFASSETMERGETABLE *pzAMTable,
                                   PERFASSETMERGE zPAMerge) {
  ERRSTRUCT zErr;

  lpprInitializeErrStruct(&zErr);

  // if table is full, allocate more space
  if (pzAMTable->iCapacity == pzAMTable->iCount) {
    pzAMTable->iCapacity += EXTRAPERFASSETMERGE;
    pzAMTable->pzMergedAsset = (PERFASSETMERGE *)realloc(
        pzAMTable->pzMergedAsset,
        pzAMTable->iCapacity * sizeof(PERFASSETMERGE));
    if (pzAMTable->pzMergedAsset == NULL)
      return (lpfnPrintError("Insufficient Memory For PerfAssetMergeTable", 0,
                             0, "", 997, 0, 0, "CALCPERF ADDPERFASSETMERGE",
                             FALSE));
  }

  pzAMTable->pzMergedAsset[pzAMTable->iCount] = zPAMerge;
  pzAMTable->iCount++;

  return zErr;
} // AddPerfAssetMergeToTable

/*
** This function returns a boolean vlaue for the passed string, if the passed
* string is "T",
** it returns TRUE in all other cases, it return FALSE
*/
BOOL StrToBool(char *sStr) {
  if (strcmp(sStr, "T") == 0)
    return TRUE;
  else
    return FALSE;
} // StrToBool

/*
** This functionr returns a string corresponding to the passed boolean vlaue.
*/
char *BoolToStr(BOOL bBool) {
  if (bBool)
    return "T";
  else
    return "F";
} // BoolToStr

/**
** This function is used to allocate memory for pzDInfo portion of all
** the keys(or one particular key) in the PKey Table.
**/
ERRSTRUCT CreateDailyInfo(PKEYTABLE *pzPKTable, int iKeyIndex,
                          long lLastPerfDate, long lCurrentPerfDate,
                          int iReturnsToCalculate) {
  ERRSTRUCT zErr;
  BOOL bDInfoInitRequired;
  int i, j, iDInfoCount, iStart, iEnd;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  if (iKeyIndex < 0 || iKeyIndex >= pzPKTable->iCount) {
    iKeyIndex = 0;
    iStart = 0;
    iEnd = pzPKTable->iCount;
  } else {
    iStart = iKeyIndex;
    iEnd = iKeyIndex + 1;
  }
  iDInfoCount = pzPKTable->pzPKey[iKeyIndex].iDInfoCount;

  /*
  ** If a valid key index is given then we have to fill pzDInfo array for that
  ** key only, else we have to fill the pzDInfo array for all the keys in the
  ** table. If we are working with all the keys then check the first key.
  ** If its pzDInfo array is already filled with those dates, then no need to do
  ** anything, else if it is filled with some other date range, free memory used
  ** by pzDInfo in all the perfkeys  and then fill them up with new date range.
  */
  if (iDInfoCount > 0) {
    if (pzPKTable->pzPKey[iKeyIndex].pzDInfo[0].lDate == lLastPerfDate &&
        pzPKTable->pzPKey[iKeyIndex].pzDInfo[iDInfoCount - 1].lDate ==
            lCurrentPerfDate)
      bDInfoInitRequired = FALSE;
    else {
      bDInfoInitRequired = TRUE;
      for (i = iStart; i < iEnd; i++)
        free(pzPKTable->pzPKey[i].pzDInfo);
    }
  } /* numdinfo > 0 */
  else
    bDInfoInitRequired = TRUE;

  if (bDInfoInitRequired) {
    for (i = iStart; i < iEnd; i++) {
      /* Allocate memory for pzDInfo */
      pzPKTable->pzPKey[i].iDInfoCapacity =
          lCurrentPerfDate - lLastPerfDate + 1;
      pzPKTable->pzPKey[i].pzDInfo = (DAILYINFO *)malloc(
          sizeof(DAILYINFO) * pzPKTable->pzPKey[i].iDInfoCapacity);
      if (pzPKTable->pzPKey[i].pzDInfo == NULL)
        return (lpfnPrintError("Insufficient Memory",
                               pzPKTable->pzPKey[i].zPK.iID, 0, "", 997, 0, 0,
                               "CALCPERF CREATEDINFO1", FALSE));
      // sprintf_s(smsg, "Memory Address For DailyInfo is; %Fp",
      // pzPKTable->pzPKey[i].pzDInfo); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0,
      // "DEBUG", TRUE);

      pzPKTable->pzPKey[i].iDInfoCount = pzPKTable->pzPKey[i].iDInfoCapacity;
      for (j = 0; j < pzPKTable->pzPKey[i].iDInfoCount; j++) {
        InitializeDailyInfo(&pzPKTable->pzPKey[i].pzDInfo[j]);
        pzPKTable->pzPKey[i].pzDInfo[j].lDate = lLastPerfDate + j;
      }

      /* Allocate memory for pzTInfo, if required */
      if (TaxInfoRequired(iReturnsToCalculate)) {
        pzPKTable->pzPKey[i].pzTInfo = (DAILYTAXINFO *)malloc(
            sizeof(DAILYTAXINFO) * (lCurrentPerfDate - lLastPerfDate + 1));
        if (pzPKTable->pzPKey[i].pzTInfo == NULL)
          return (lpfnPrintError("Insufficient Memory",
                                 pzPKTable->pzPKey[i].zPK.iID, 0, "", 997, 0, 0,
                                 "CALCPERF CREATEDINFO2", FALSE));
        // sprintf_s(smsg, "Memory Address For DailyTaxInfo is; %Fp",
        // pzPKTable->pzPKey[i].pzTInfo); lpfnPrintError(smsg, 0, 0, "", 0, 0,
        // 0, "DEBUG", TRUE);

        for (j = 0; j < pzPKTable->pzPKey[i].iDInfoCount; j++)
          InitializeDailyTaxinfo(&pzPKTable->pzPKey[i].pzTInfo[j]);
      } /* If any tax work is required */
    } /* for i < pzPKTable->iCount */
  } /* if difoinitrequired */

  return zErr;
} /* CreateDailyInfo */

/**
** Function to get date index in the DInfo element of the passed perfkey.
**/
int GetDateIndex(PKEYSTRUCT zPKey, long lDate) {
  int i, iIndex;

  iIndex = -1;
  i = 0;
  while (iIndex == -1 && i < zPKey.iDInfoCount) {
    if (zPKey.pzDInfo[i].lDate == lDate)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* getdateindex */

/**
** Function to create a unique hash key for a perf script detail,
   using either internal or CAPICOM algorithm
**/
DllExport long CreateHashkeyForScript(PSCRHDRDET *pzSHdrDet,
                                      BOOL bUseInternal) {
  long lHashkey;
  int i;
  char sStr[300];
  std::string sStrToHash;

  lHashkey = 0; /* start with a zero value */

  if (!bUseInternal) {
    strcpy_s(pzSHdrDet->zHeader.sHdrKey, "");
    sStrToHash.clear();
  }

  /*
  ** For all the details line, construct a string simply by concatenating all
  ** the essential field values, then call makehash to return a hash value for
  ** that line. Final hash value is our unique hash key.
  */
  for (i = 0; i < pzSHdrDet->iNumDetail; i++) {
    sprintf_s(
        sStr, "%s%s%s%s%s%s%s%s%s%s%s%s%ld%ld",
        pzSHdrDet->pzDetail[i].sSelectType,
        pzSHdrDet->pzDetail[i].sComparisonRule,
        pzSHdrDet->pzDetail[i].sBeginPoint, pzSHdrDet->pzDetail[i].sEndPoint,
        pzSHdrDet->pzDetail[i].sAndOrLogic,
        pzSHdrDet->pzDetail[i].sIncludeExclude,
        pzSHdrDet->pzDetail[i].sMaskRest, pzSHdrDet->pzDetail[i].sMatchRest,
        pzSHdrDet->pzDetail[i].sMaskWild, pzSHdrDet->pzDetail[i].sMatchWild,
        pzSHdrDet->pzDetail[i].sMaskExpand, pzSHdrDet->pzDetail[i].sMatchExpand,
        pzSHdrDet->pzDetail[i].lStartDate, pzSHdrDet->pzDetail[i].lEndDate);

    if (bUseInternal)
      lHashkey = MakeHash(lHashkey, sStr, pzSHdrDet->pzDetail[i].lSeqNo);
    else
      sStrToHash += sStr;
  }

  if (!bUseInternal) {
    if (sStrToHash != "") {
      HRESULT hr = pHashData->Hash(sStrToHash.c_str());
      if (!FAILED(hr))
        strcpy_s(pzSHdrDet->zHeader.sHdrKey, pHashData->Value);
    }
  }

  return lHashkey;
} /* createhashkeyforscript */

/**
** PURPOSE: Returns a unique hash number for a series of strings and LineNumbers
**
** INPUT PARAMETERS:
**     long lHashOld    ... The previous hash number or zero if starting
**     char *sString    ... The string for current line number to hash
**     int  iLineNo     ... The current line number (start with 0)
**
** OUTPUT RETURN:
**     The function returns a more-or-less unique hash number
**
** NOTES: The risk of coincidence of hash numbers should be 0 for small
**        (less than 20 lines of less than 100 characters)
**        But may approach one in a billion for larger tables
**
**        The technique used relies on a series of prime numbers
**        Multiplying each character (after subtracting the space char)
**/
long MakeHash(long lOldHash, char *sString, int iLineNo) {
  char *pcPtr, *pcLastPtr;
  long lCharMult, lTempHash, lThisHash, lTotalHash;
  int i, j, iLast, iThisChar, iCharTabRows, iLineTabRows;
  double dLineMult, dTotalHash;

  /* Find string length ignoring trailing spaces */
  for (pcPtr = sString, pcLastPtr = sString - 1; *pcPtr != '\0'; ++pcPtr) {
    if (*pcPtr != ' ')
      pcLastPtr = pcPtr;
  }
  iLast = pcLastPtr - sString + 1;

  /* Determine the hash value for the characters making up this line */
  iCharTabRows = sizeof(alPrimesForChars) / sizeof(long int);

  for (lThisHash = 1, pcPtr = sString, i = 0; i < iLast; i++, pcPtr++) {
    j = i % iCharTabRows;
    lCharMult = alPrimesForChars[j];
    iThisChar = *pcPtr - ' ' + 1;
    lTempHash = lCharMult * iThisChar;
    lThisHash += lTempHash;
  }

  /*
  ** Multiply it by a prime based on line number. We use a double to prevent
  ** overflow. However, the results are hashed back to about 1Gig
  */
  iLineTabRows = sizeof(alPrimesForLines) / sizeof(long int);
  dLineMult = alPrimesForLines[iLineNo % iLineTabRows];
  dTotalHash = dLineMult * lThisHash;
  dTotalHash += lOldHash;
  dTotalHash = fmod(dTotalHash, BIGPRIMENUMBER);
  lTotalHash = (long)dTotalHash;

  return lTotalHash;
} /* makehash */

/**
** A(Mkt Val + Accr Int + Accr Div) Gross of fees and Net of fees RORs.
** If bCalcOnlyBase = TRUE then only Base Ror is calculated else base, income
** and principal rors are calculated.
**/
// DllExport void CalculateGFAndNFRor(RORSTRUCT *pzRS, BOOL bCalcOnlyBase)
DllExport void CalculateGFAndNFRor(RORSTRUCT *pzRS) {
  double fNetFlowAdj, fWtdFlowAdj, fBegAccrAdj, fEndAccrAdj;

  pzRS->fNetFlow += pzRS->fFeesOut;
  pzRS->fWtFlow += pzRS->fWtFeesOut;

  /*
  **  Base ROR =  (mkt val - beg mkt val - net flow) / (beg mkt val + wtd flow )
  **  Income ROR = (End Accr - Beg Accr - Income) / Beg Mkt Val
  ** In both the cases mkt val also includes accruals.
  */

  /* Calculate time weighted ror, if needed */
  if (pzRS->iReturnstoCalculate & TWRorBit) {
    // SB 5/27/15 Simplified the structure
    pzRS->zAllRor.fBaseRorIdx[GTWRor - 1] =
        GFAndNFBaseROR(pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
                       pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
                       pzRS->fNetFlow, pzRS->fWtFlow, pzRS->fGFTWFFactor,
                       pzRS->bInceptionRor, pzRS->bTerminationRor);

    /* Calculate MV + AI + AD Net of fee time weighted ror, if needed - for
     * segments calculate it only if specifically asked */
    /*				SB 8/8/08 - After not saving net of fee returns
       was implemented, an issue with net of fee cbyc composites not working as
       expected was found, so for now, revert the changes regarding not saving
       net of fee returns on segments back to its original state (i.e. continue
       saving net of fee returns even on segments) until issue with composite
       merge is fixed. if (pzRS->bTotalPortfolio || pzRS->bCalcNetForSegments)*/
    pzRS->zAllRor.fBaseRorIdx[NTWRor - 1] = GFAndNFBaseROR(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
        pzRS->fNetFlow + pzRS->fFees, pzRS->fWtFlow + pzRS->fWtFees,
        pzRS->fNFTWFFactor, pzRS->bInceptionRor, pzRS->bTerminationRor);

    // 	net of consulting fee, if required
    // if (pzRS->iReturnstoCalculate & TWNCFRorBit)
    pzRS->zAllRor.fBaseRorIdx[CNWRor - 1] = GFAndNFBaseROR(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
        pzRS->fNetFlow + pzRS->fFees - pzRS->fCNFees,
        pzRS->fWtFlow + pzRS->fWtFees - pzRS->fWtCNFees, pzRS->fCNTWFFactor,
        pzRS->bInceptionRor, pzRS->bTerminationRor);

  } // Time weighted

  /* Calculate Income Ror, if needed */
  if (pzRS->iReturnstoCalculate & IncomeRorBit)
    pzRS->zAllRor.fBaseRorIdx[IncomeRor - 1] = GFAndNFIncomeROR(
        pzRS->fEndAI + pzRS->fEndAD, pzRS->fBeginAI + pzRS->fBeginAD,
        pzRS->fIncome, pzRS->fBeginMV, pzRS->fIncFFactor);

  /*
  ** For Principal Gross Of Fees Base ROR, there are two cases
  **  1) if total portfolio then
  **        new mkt val = mkt val - income
  **  2) if not total portfolio then
  **        new net fl = net fl + income
  **        new wtd fl = wtd fl + wtd inc
  ** For Principal Net Of Fees Base ROR, in addition to above calculations, fees
  ** and weigthed fees are added to new net fl and new wtd fl respectively.
  ** Income ROR is zero in this case
  */
  if (pzRS->iReturnstoCalculate & PcplRorBit) {
    if (pzRS->bTotalPortfolio) {
      // Principal Gross of fees ROR
      pzRS->zAllRor.fBaseRorIdx[GPcplRor - 1] =
          GFAndNFBaseROR(pzRS->fEndMV - pzRS->fIncome, pzRS->fBeginMV,
                         pzRS->fNetFlow, pzRS->fWtFlow, pzRS->fGFPcplFFactor,
                         pzRS->bInceptionRor, pzRS->bTerminationRor);
      /*
      ** Net Principal Ror. New net fl and new wtd flow are net flow + fees and
      ** wtd flow + wt fees which is what we want for a total portfolio
      */
      pzRS->zAllRor.fBaseRorIdx[NPcplRor - 1] = GFAndNFBaseROR(
          pzRS->fEndMV - pzRS->fIncome, pzRS->fBeginMV,
          pzRS->fNetFlow + pzRS->fFees, pzRS->fWtFlow + pzRS->fWtFees,
          pzRS->fNFPcplFFactor, pzRS->bInceptionRor, pzRS->bTerminationRor);
    } /* totalportfolio */
    else {
      // Principal Gross of fees ROR
      pzRS->zAllRor.fBaseRorIdx[GPcplRor - 1] = GFAndNFBaseROR(
          pzRS->fEndMV, pzRS->fBeginMV, pzRS->fNetFlow + pzRS->fIncome,
          pzRS->fWtFlow + pzRS->fWtIncome, pzRS->fGFPcplFFactor,
          pzRS->bInceptionRor, pzRS->bTerminationRor);
      // Net Principal ROR, if needed
      /*		SB 8/8/08 - After not saving net of fee returns was
      implemented, an issue with net of fee cbyc composites not working as
      expected was found, so for now, revert the changes regarding not saving
      net of fee returns on segments back to its original state (i.e. continue
      saving net of fee returns even on segments) until issue with composite
      merge is fixed. if (pzRS->bCalcNetForSegments)*/
      pzRS->zAllRor.fBaseRorIdx[NPcplRor - 1] = GFAndNFBaseROR(
          pzRS->fEndMV, pzRS->fBeginMV,
          pzRS->fNetFlow + pzRS->fIncome + pzRS->fFees,
          pzRS->fWtFlow + pzRS->fWtIncome + pzRS->fWtFlow, pzRS->fNFPcplFFactor,
          pzRS->bInceptionRor, pzRS->bTerminationRor);
    } /* not totalportfolio */
  } // if income return calc is required

  // after tax returns, if required
  if (pzRS->iReturnstoCalculate & AfterTaxRorBit) {
    // calculate after tax adjustments
    TaxAdjustments(*pzRS, "A", &fNetFlowAdj, &fWtdFlowAdj, &fBegAccrAdj,
                   &fEndAccrAdj);

    // Gross after tax
    pzRS->zAllRor.fBaseRorIdx[GTWAfterTaxRor - 1] = GFAndNFBaseROR(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
        pzRS->fNetFlow + fNetFlowAdj, pzRS->fWtFlow + fWtdFlowAdj,
        pzRS->fGFATFFactor, pzRS->bInceptionRor, pzRS->bTerminationRor);

    // Net after tax, if total or specifically asked
    /*		SB 8/8/08 - After not saving net of fee returns was implemented,
an issue with net of fee cbyc composites not working as expected was found, so
for now, revert the changes regarding not saving net of fee returns on segments
back to its original state (i.e. continue saving net of fee returns even on
segments) until issue with composite merge is fixed. if (pzRS->bTotalPortfolio
|| pzRS->bCalcNetForSegments)*/
    pzRS->zAllRor.fBaseRorIdx[NTWAfterTaxRor - 1] = GFAndNFBaseROR(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
        pzRS->fNetFlow + pzRS->fFees + fNetFlowAdj,
        pzRS->fWtFlow + pzRS->fWtFees + fWtdFlowAdj, pzRS->fNFATFFactor,
        pzRS->bInceptionRor, pzRS->bTerminationRor);
  } // After tax returns

  // Tax equivalent returns, if needed
  if (pzRS->iReturnstoCalculate & TaxEquivRorBit) {
    // calculate tax equivalent adjustments
    TaxAdjustments(*pzRS, "E", &fNetFlowAdj, &fWtdFlowAdj, &fBegAccrAdj,
                   &fEndAccrAdj);

    // Flow = flow + inc adj
    // Accruals = Accruals + Accrual Adj
    pzRS->zAllRor.fBaseRorIdx[GTWTaxEquivRor - 1] = GFAndNFBaseROR(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
        pzRS->fNetFlow + fNetFlowAdj, pzRS->fWtFlow + fWtdFlowAdj,
        pzRS->fGFTEFFactor, pzRS->bInceptionRor, pzRS->bTerminationRor);

    /*		SB 8/8/08 - After not saving net of fee returns was implemented,
an issue with net of fee cbyc composites not working as expected was found, so
for now, revert the changes regarding not saving net of fee returns on segments
back to its original state (i.e. continue saving net of fee returns even on
segments) until issue with composite merge is fixed. if (pzRS->bTotalPortfolio
|| pzRS->bCalcNetForSegments)*/
    pzRS->zAllRor.fBaseRorIdx[NTWTaxEquivRor - 1] = GFAndNFBaseROR(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
        pzRS->fNetFlow + pzRS->fFees + fNetFlowAdj,
        pzRS->fWtFlow + pzRS->fWtFees + fWtdFlowAdj, pzRS->fNFTEFFactor,
        pzRS->bInceptionRor, pzRS->bTerminationRor);
  } /* Tax equivalent */

} // CalcgfAndNfRors

// This function calculates adjustments to flows and accruals for tax
// equivalent/after tax return
void TaxAdjustments(RORSTRUCT zRS, char *sTaxType, double *pfNetFlowAdj,
                    double *pfWtdFlowAdj, double *pfBegAccrAdj,
                    double *pfEndAccrAdj) {
  double fTaxRate, fFIncAdj, fWtFIncAdj, fFWthldAdj, fWtFWthldAdj, fFRclmAdj,
      fWtFRclmAdj;
  double fFEndAIAdj, fFEndADAdj, fFBeginAIAdj, fFBeginADAdj, fFAIRclmAdj,
      fFADRclmAdj;
  double fFLTGLAdj, fFSTGLAdj, fFCurrGLAdj, fWtFLTGLAdj, fWtFSTGLAdj,
      fWtFCurrGLAdj;
  double fFAmortAdj, fWtFAmortAdj;

  fFIncAdj = fWtFIncAdj = fFWthldAdj = fWtFWthldAdj = fFRclmAdj = fWtFRclmAdj =
      0;
  fFEndAIAdj = fFEndADAdj = fFBeginAIAdj = fFBeginADAdj = fFAIRclmAdj =
      fFADRclmAdj = 0;
  fFLTGLAdj = fFSTGLAdj = fFCurrGLAdj = fWtFLTGLAdj = fWtFSTGLAdj =
      fWtFCurrGLAdj = 0;
  fFAmortAdj = fWtFAmortAdj = 0;

  *pfNetFlowAdj = *pfWtdFlowAdj = *pfBegAccrAdj = *pfEndAccrAdj = 0;

  if (strcmp(sTaxType, "A") == 0) {
    // get the effective tax rate for federal tax calculations
    fTaxRate = EffectiveTaxRate(zRS.zPTax.fFedIncomeRate);

    // Federal withholding income adjustment
    fFWthldAdj = AfterTaxAdjustment(zRS.zTInfo.fFedinctaxWthld, fTaxRate);
    fWtFWthldAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedinctaxWthld, fTaxRate);

    // Federal reclaim adjustment
    fFRclmAdj = AfterTaxAdjustment(zRS.zTInfo.fFedtaxRclm, fTaxRate);
    fWtFRclmAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedtaxRclm, fTaxRate);

    // Federal taxable income adjustment
    fFIncAdj = AfterTaxAdjustment(zRS.zTInfo.fFedataxInc, fTaxRate);
    fWtFIncAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedataxInc, fTaxRate);

    // Federal Begining Accr Inc adjustment
    fFBeginAIAdj = AfterTaxAdjustment(zRS.zTInfo.fFedBegataxAccrInc, fTaxRate);

    // Federal Ending Accr Inc adjustment
    fFEndAIAdj = AfterTaxAdjustment(zRS.zTInfo.fFedEndataxAccrInc, fTaxRate);

    // Federal Begining Accr Div adjustment
    fFBeginADAdj = AfterTaxAdjustment(zRS.zTInfo.fFedBegataxAccrDiv, fTaxRate);

    // Federal Ending Accr Div adjustment
    fFEndADAdj = AfterTaxAdjustment(zRS.zTInfo.fFedEndataxAccrDiv, fTaxRate);

    // Federal Accr Inc Reclaim adjustment
    fFAIRclmAdj = AfterTaxAdjustment(zRS.zTInfo.fFedataxIncRclm, fTaxRate);

    // Federal Accr Div Reclaim adjustment
    fFADRclmAdj = AfterTaxAdjustment(zRS.zTInfo.fFedataxDivRclm, fTaxRate);

    // Federal withholding income adjustment
    fFWthldAdj = AfterTaxAdjustment(zRS.zTInfo.fFedinctaxWthld, fTaxRate);
    fWtFWthldAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedinctaxWthld, fTaxRate);

    // Federal amortization/accretion adjustment
    fFAmortAdj = AfterTaxAdjustment(zRS.zTInfo.fFedataxAmort, fTaxRate);
    fWtFAmortAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedataxAmort, fTaxRate);

    if (zSysSet.bGLTaxAdj) {
      // get the effective tax rate for federal LT GL tax calculations
      fTaxRate = EffectiveTaxRate(zRS.zPTax.fFedLtGLRate);
      // Federal long term gain/loss adjustment
      fFLTGLAdj = AfterTaxAdjustment(zRS.zTInfo.fFedataxLtrgl, fTaxRate);
      fWtFLTGLAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedataxLtrgl, fTaxRate);

      // get the effective tax rate for federal ST GL tax calculations
      fTaxRate = EffectiveTaxRate(zRS.zPTax.fFedStGLRate);
      // Federal short term gain/loss adjustment
      fFSTGLAdj = AfterTaxAdjustment(zRS.zTInfo.fFedataxStrgl, fTaxRate);
      fWtFSTGLAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedataxStrgl, fTaxRate);

      // the tax rate for federal Currency GL tax calculations = ST GL tax rate
      // (from above) Federal currency gain/loss adjustment
      fFCurrGLAdj = AfterTaxAdjustment(zRS.zTInfo.fFedataxCrrgl, fTaxRate);
      fWtFCurrGLAdj = AfterTaxAdjustment(zRS.zTInfo.fWtdFedataxCrrgl, fTaxRate);
    }

    // Flow = flow + inc adj + withld adj + recl adj + Gain Loss Adj +
    // Amort/Accret Adj Accruals = Accruals + Accrual Adj
    *pfNetFlowAdj = fFIncAdj + fFWthldAdj + fFRclmAdj + fFLTGLAdj + fFSTGLAdj +
                    fFCurrGLAdj + fFAmortAdj;

    // 2006-05-16 vay
    // According to GIPS US After Tax guidance document (page 13), taxes should
    // not be included in the portfolio flow adjustment in the denominator
    // (taxes are an expense but formulae assume that cash is not withdrawn from
    // the account in order to pay the taxes)
    /*
    pfWtdFlowAdj = fWtFIncAdj + fWtFWthldAdj + fWtFRclmAdj +
                                                            fWtFLTGLAdj +
    fWtFSTGLAdj + fWtFCurrGLAdj + fWtFAmortAdj;
    */
    *pfWtdFlowAdj = 0;

    *pfBegAccrAdj = fFBeginAIAdj + fFBeginADAdj;
    *pfEndAccrAdj = fFEndAIAdj + fFEndADAdj + fFAIRclmAdj + fFADRclmAdj;
  } // After tax returns
  else if (strcmp(sTaxType, "E") == 0) {
    fTaxRate = EffectiveTaxRate(zRS.zPTax.fFedIncomeRate);

    // Federal income adjustments
    fFIncAdj = TaxEquivalentAdjustment(zRS.zTInfo.fFedetaxInc, fTaxRate);
    fWtFIncAdj = TaxEquivalentAdjustment(zRS.zTInfo.fWtdFedetaxInc, fTaxRate);

    // Federal Ending Accr Inc adjustment
    fFEndAIAdj =
        TaxEquivalentAdjustment(zRS.zTInfo.fFedEndetaxAccrInc, fTaxRate);

    // Federal Ending Accr Div adjustment
    fFEndADAdj =
        TaxEquivalentAdjustment(zRS.zTInfo.fFedEndetaxAccrDiv, fTaxRate);

    // Federal Begining Accr Inc adjustment
    fFBeginAIAdj =
        TaxEquivalentAdjustment(zRS.zTInfo.fFedBegetaxAccrInc, fTaxRate);

    // Federal Begining Accr Div adjustment
    fFBeginADAdj =
        TaxEquivalentAdjustment(zRS.zTInfo.fFedBegetaxAccrDiv, fTaxRate);

    // Federal Accr Inc Reclaim adjustment
    fFAIRclmAdj = TaxEquivalentAdjustment(zRS.zTInfo.fFedetaxIncRclm, fTaxRate);

    // Federal Accr Div Reclaim adjustment
    fFADRclmAdj = TaxEquivalentAdjustment(zRS.zTInfo.fFedetaxDivRclm, fTaxRate);

    // Federal amortization/accretion adjustment
    fFAmortAdj = TaxEquivalentAdjustment(zRS.zTInfo.fFedetaxAmort, fTaxRate);
    fWtFAmortAdj =
        TaxEquivalentAdjustment(zRS.zTInfo.fWtdFedetaxAmort, fTaxRate);

    if (zSysSet.bGLTaxAdj) {
      // get the effective tax rate for federal LT GL tax calculations
      fTaxRate = EffectiveTaxRate(zRS.zPTax.fFedLtGLRate);
      // Federal long term gain/loss adjustment
      fFLTGLAdj = TaxEquivalentAdjustment(zRS.zTInfo.fFedetaxLtrgl, fTaxRate);
      fWtFLTGLAdj =
          TaxEquivalentAdjustment(zRS.zTInfo.fWtdFedetaxLtrgl, fTaxRate);

      // get the effective tax rate for federal ST GL tax calculations
      fTaxRate = EffectiveTaxRate(zRS.zPTax.fFedStGLRate);
      // Federal short term gain/loss adjustment
      fFSTGLAdj = TaxEquivalentAdjustment(zRS.zTInfo.fFedetaxStrgl, fTaxRate);
      fWtFSTGLAdj =
          TaxEquivalentAdjustment(zRS.zTInfo.fWtdFedetaxStrgl, fTaxRate);

      // the tax rate for federal Currency GL tax calculations = ST GL tax rate
      // (from above) Federal currency gain/loss adjustment
      fFCurrGLAdj = TaxEquivalentAdjustment(zRS.zTInfo.fFedetaxCrrgl, fTaxRate);
      fWtFCurrGLAdj =
          TaxEquivalentAdjustment(zRS.zTInfo.fWtdFedetaxCrrgl, fTaxRate);
    }

    // Flow = flow + inc adj + Gain / Loss Adj + Amort/Accret adj
    // Accruals = Accruals + Accrual Adj
    *pfNetFlowAdj = fFIncAdj + fFLTGLAdj + fFSTGLAdj + fFCurrGLAdj + fFAmortAdj;

    // 2006-05-16 vay
    // According to GIPS US After Tax guidance document (page 13), taxes should
    // not be included in the portfolio flow adjustment in the denominator
    // (taxes are an expense but formulae assume that cash is not withdrawn from
    // the account in order to pay the taxes) *pfWtdFlowAdj = fWtFIncAdj +
    // fWtFLTGLAdj + fWtFSTGLAdj + fWtFCurrGLAdj + fWtFAmortAdj;
    *pfWtdFlowAdj = 0;

    *pfBegAccrAdj = fFBeginAIAdj + fFBeginADAdj;
    *pfEndAccrAdj = fFEndAIAdj + fFEndADAdj + fFAIRclmAdj + fFADRclmAdj;
  } // Tax Equivalent return
} // TaxAdjustments

/**
** Function to calculate ROR using the formula:
**   ROR = (end mkt val - beg mkt val - net flow)/abs(beg mkt val + wtd flow)
** The above formula works only if it is not an inception day's ror. In case
** of inception day's return the formula is:
**   ROR = (end mkt val - net flow)/net flow
**/
double GFAndNFBaseROR(double fEMV, double fBMV, double fNFlow, double fWFlow,
                      double fFudgeFactor, BOOL bInceptionRor,
                      BOOL bTerminationRor) {
  double fROR;

  if (bTerminationRor)
    fWFlow = 0.0;

  if (bInceptionRor && IsValueZero(fBMV, 2)) {
    if (!IsValueZero(fNFlow, 3) & !IsValueZero(fEMV, 3))
      fROR = (fEMV - fNFlow) / fNFlow;
    else // special case which can happen for Total segment
      fROR = 0.0;
  } /* Inception day ror */
  else {
    if (IsValueZero(fBMV + fWFlow + (fFudgeFactor / 2), 3))
      fROR = NAVALUE;
    else {
      fROR = (fEMV - fBMV - fNFlow - fFudgeFactor) /
             (fBMV + fWFlow + (fFudgeFactor / 2));

      if (fBMV + fWFlow < 0.0)
        fROR *= -1.0;
    }
  } /* Not an inception ror */

  if (fROR != NAVALUE)
    fROR *= 100.0;

  return fROR;
} /* gfandnfbaseRor */

/**
** Function to calculate Income ROR using the formula:
**  Income ROR = (end accr - beg accr + income)/beg mkt val
**/
double GFAndNFIncomeROR(double fEAccr, double fBAccr, double fInc,
                        double fBegMV, double fFudgeFactor) {
  double fROR;

  if (IsValueZero(fBegMV + (fFudgeFactor / 2), 3))
    fROR = NAVALUE;
  else
    fROR = ((fEAccr - fBAccr + fInc - fFudgeFactor) /
            (fBegMV + (fFudgeFactor / 2))) *
           100.0;

  return fROR;
} /* GfAndNfIncomeRor */

/**
** Funtion to calculate M(Mkt Val), I(Mkt Val + Accr Int), D(Mkt Val + Accr Div)
** A(Mkt Val + Accr Int + Accr Div) and N(Mkt Val - Income) Dollar Weighted RORs
** The output array always have all the above types of Rors in the above given
** order(MIDAN).
** If there is an error in calculating one of the Rors(say "M"), the function
** makes that ROR as NAVALUE and continues until it has calculated all the RORs.
** /
ERRSTRUCT CalculateDWRor(DWRORSTRUCT *pzDWRor)
{
  ERRSTRUCT zErr;
  double    fOriginalBegMV, fNewEndMV;
  int       j;

  lpprInitializeErrStruct(&zErr);

  // Save the Original MV
  fOriginalBegMV = pzDWRor->pfNetFlow[0];

  // Dollar Weighted Market Value ROR
  zErr = DWBaseRor(pzDWRor->fEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
                   pzDWRor->pfWeight, &pzDWRor->fMRor);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    pzDWRor->fMRor = NAVALUE;

  / *
  ** For Market Value + Accrued Interest Dollar Weigthed ROR
  **     new mkt val = mkt val + accr int
  **     new beg mkt val = beg mkt val + beg accr int
  * /
  fNewEndMV = pzDWRor->fEndMV + pzDWRor->fEndAI;
  pzDWRor->pfNetFlow[0] = fOriginalBegMV + pzDWRor->fBeginAI;
  zErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
                   pzDWRor->pfWeight, &pzDWRor->fIRor);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    pzDWRor->fMRor = NAVALUE;

  / *
  ** For MV + AD Dollar Weigthed ROR
  **     new mkt val = mkt val + accr div
  **     new beg mkt val = beg mkt val + beg accr div
  * /
  fNewEndMV = pzDWRor->fEndMV + pzDWRor->fEndAD;
  pzDWRor->pfNetFlow[0] = fOriginalBegMV + pzDWRor->fBeginAD;
  zErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
                   pzDWRor->pfWeight, &pzDWRor->fDRor);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    pzDWRor->fMRor = NAVALUE;

  / *
  ** For MV + AI + AD Dollar Weighted ROR
  **     new mkt val = mkt val + accr int + accr div
  **     new beg mkt val = beg mkt val + beg accr int + beg accr div
  * /
  fNewEndMV = pzDWRor->fEndMV + pzDWRor->fEndAI + pzDWRor->fEndAD;
  pzDWRor->pfNetFlow[0] = fOriginalBegMV + pzDWRor->fBeginAI +pzDWRor->fBeginAD;
  zErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
                   pzDWRor->pfWeight, &pzDWRor->fARor);
  if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
    pzDWRor->fMRor = NAVALUE;

  / *
  ** For Principal Dollar Weighted ROR, there are two cases
  **  1) if total portfolio then
  **        new mkt val = mkt val - income
  **  2) if not total portfolio then
  **        new net fl = net fl + income
  * /
  if (pzDWRor->bTotalPortfolio)
  {
    fNewEndMV = pzDWRor->fEndMV;
    for (j = 1; j < pzDWRor->iNumFlows; j++)
      fNewEndMV -= pzDWRor->pfIncome[j];

    zErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
                     pzDWRor->pfWeight, &pzDWRor->fNRor);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      pzDWRor->fMRor = NAVALUE;

  } // totalportfolio
  else
  {
    for (j = 1; j < pzDWRor->iNumFlows; j++)
      pzDWRor->pfNetFlow[j] -= pzDWRor->pfIncome[j];

    zErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
                     pzDWRor->pfWeight, &pzDWRor->fNRor);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      pzDWRor->fMRor = NAVALUE;

  } // not totalportfolio

  lpprInitializeErrStruct(&zErr);
  return zErr;
} // CalcDWRors */

/**
** Funtion to calculate I(Mkt Val + Accr Int) and N(Mkt Val - Income) Dollar
* Weighted RORs
** If there is an error in calculating base Ror, the function makes that ROR as
* NAVALUE and
** continues to calculate Income ROR.
**/
DllExport void CalculateDWRor(DWRORSTRUCT *pzDWRor) {
  double fOriginalBegMV, fNewEndMV;
  // int       j;
  int iErr;

  // Save the Original MV
  fOriginalBegMV = pzDWRor->pfNetFlow[0];

  /*
  ** For Market Value + Accrued Interest/Div Dollar Weigthed ROR
  **     new mkt val = mkt val + accr int + accr_div
  **     new beg mkt val = beg mkt val + beg accr int + beg accr div
  */
  fNewEndMV = pzDWRor->fEndMV + pzDWRor->fEndAI + pzDWRor->fEndAD;
  pzDWRor->pfNetFlow[0] =
      fOriginalBegMV + pzDWRor->fBeginAI + pzDWRor->fBeginAD;
  iErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
                   pzDWRor->pfWeight, &pzDWRor->fBaseRor);
  if (iErr != 0)
    pzDWRor->fBaseRor = NAVALUE;

  // 7/13/05 - this is not really used anywere, commented out - vay & SB
  /*
  ** For Income Dollar Weighted ROR, there are two cases
  **  1) if total portfolio then
  **        new mkt val = mkt val - income
  **  2) if not total portfolio then
  **        new net fl = net fl + income
  */
  /*
        if (pzDWRor->bTotalPortfolio)
  {
    fNewEndMV = pzDWRor->fEndMV;
    for (j = 1; j < pzDWRor->iNumFlows; j++)
      fNewEndMV -= pzDWRor->pfIncome[j];

    iErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
  pzDWRor->pfWeight, &pzDWRor->fIncomeRor); if (iErr != 0) pzDWRor->fIncomeRor =
  NAVALUE;

  } //* totalportfolio
  else
  {
    for (j = 1; j < pzDWRor->iNumFlows; j++)
      pzDWRor->pfNetFlow[j] -= pzDWRor->pfIncome[j];

    iErr = DWBaseRor(fNewEndMV, pzDWRor->pfNetFlow, pzDWRor->iNumFlows,
  pzDWRor->pfWeight, &pzDWRor->fIncomeRor); if (iErr != 0 ) pzDWRor->fIncomeRor
  = NAVALUE;

  } // not totalportfolio
        */

} /* CalcDWRors */

double EstimateMV(double fFlow[], int iNumFlows, double fWeight[],
                  double fROR) {
  int j;
  double fEstEndMV = 0;

  for (j = 0; j < iNumFlows; j++)
    fEstEndMV += fFlow[j] * pow(1.0 + fROR, fWeight[j]);

  return fEstEndMV;
}

double AdaptiveDelta(double fROR) {
  double fDelta;

  if (fabs(fROR) > 0.000001)
    fDelta = pow(10, floor(log10(fabs(fROR))));
  else
    fDelta = 0.000001;

  return fDelta;
}

// common error checking/initialization for DWROR
int DWRORCheckAndSetup(double fEndMV, double fFlow[], int iNumFlows,
                       double fWeight[], double *pfROR, double *pfNetF,
                       double *pfWtdF, double *pfMktValDiff) {
  int i;

  /* Do some basic error testing and estimate the first value of ROR */
  if (iNumFlows <= 0) {
    if (lpfnPrintError)
      lpfnPrintError("Invalid Number Of Cash Flows", 0, 0, "", 989, 0, 0,
                     "CALCPERF DWBASEROR1", FALSE);

    return 989;
  }

  /*
  ** First calculate sum of net and weighted flows. Ignore array element 0,
  ** as this is the begining market value. If any of the weight is more than
  ** or equal to one or less than zero, it is an error. If the first weight
  ** is not 1(for begining market value), that's an error too.
  */
  *pfNetF = *pfWtdF = 0;
  if (fWeight[0] != 1.0) {
    if (lpfnPrintError)
      lpfnPrintError("Invalid Weight", 0, 0, "", 988, 0, 0,
                     "CALCPERF DWBASEROR2", FALSE);

    return 988;
  }

  for (i = 1; i < iNumFlows; i++) {
    if (fWeight[i] < 0.0 || fWeight[i] >= 1.0) {
      if (lpfnPrintError)
        lpfnPrintError("Invalid Weight", 0, 0, "", 987, 0, 0,
                       "CALCPERF DWBASEROR3", FALSE);

      return 987;
    }

    *pfNetF += fFlow[i];
    *pfWtdF += fFlow[i] * fWeight[i];
  }

  /*
  ** ROR = (End MV - Beg MV - Net Flow) / (Beg MV + Wtd Flow).
  ** If the begining MV is 0 and there is no flow, ROR can't be calculated. If
  ** the denominator is very small(does not have to be exact zero), divison
  ** will cause floating point error, in that case, start with ror = 0.
  */
  if (IsValueZero(fFlow[0], 2) && IsValueZero(*pfNetF, 2)) {
    return 986;
  } else if (IsValueZero(fFlow[0] + *pfWtdF, 2))
    *pfROR = 0;
  else {
    *pfROR = (fEndMV - fFlow[0] - *pfNetF) / (fFlow[0] + *pfWtdF);
    if (*pfROR < -0.999999) // SB 4/4/07 - if return is less than 99.99%,
                            // start(force) it with(to) 99.99%, otherwise
      *pfROR = -0.999999;   // when estimating market value, power function will
                            // cause floating point error.
  }

  /* if End MV is 1000 or more, then a dollar diff is sufficient,  */
  /* else it should be a proprtion of MV with least being a penny */
  /* and most being a dollar
   */
  if (fabs((double)fEndMV) >= (double)1000)
    *pfMktValDiff = 1.0;
  else if (fabs(fEndMV * 0.0001) > 0.01)
    *pfMktValDiff = fabs(fEndMV * 0.0001);
  else
    *pfMktValDiff = 0.01;

  return 0;
}

// this is the version of DWROR function using bisection approach
// we've decided to use it only if original version does not find
// the answer within max number of allowed iterations

int DWBaseRorBisect(double fEndMV, double fFlow[], int iNumFlows,
                    double fWeight[], double *pfROR) {
  double fMktValDiff; // Market value and returns
  int i;
  double fEstEndMV, fNetF, fWtdF;
  double fR0, fR1, fDelta;
  BOOL bR0found, bR1found;

  i = DWRORCheckAndSetup(fEndMV, fFlow, iNumFlows, fWeight, pfROR, &fNetF,
                         &fWtdF, &fMktValDiff);
  if (i)
    return i;

  /* Estimate End MV for current value of *pfROR */
  fEstEndMV = EstimateMV(fFlow, iNumFlows, fWeight, *pfROR);

  fR0 = fR1 = *pfROR;
  bR0found = (fEstEndMV < fEndMV);
  bR1found = (fEstEndMV > fEndMV);

  i = 0;
  while (!bR0found && (i <= MAXITERATIONS)) {
    fDelta = AdaptiveDelta(fR0);
    fR0 -= fDelta;
    if (fR0 < -0.999999)
      fR0 = -0.999999;

    fEstEndMV = EstimateMV(fFlow, iNumFlows, fWeight, fR0);
    bR0found = (fEstEndMV < fEndMV);
    i++;
  }
  if (!bR0found)
    return 985;

  i = 0;
  while (!bR1found && (i <= MAXITERATIONS)) {
    fDelta = AdaptiveDelta(fR1);
    fR1 += fDelta;
    if (fR1 > 999999)
      fR1 = 999999;

    fEstEndMV = EstimateMV(fFlow, iNumFlows, fWeight, fR1);
    bR1found = (fEstEndMV > fEndMV);
    i++;
  }
  if (!bR1found)
    return 984;

  for (i = 0; i < MAXITERATIONS; i++) {
    *pfROR = (fR0 + fR1) * 0.5;
    if (fabs(fR1 - fR0) <= 0.000001)
      return 0;

    /* Estimate End MV for current value of *pfROR */
    fEstEndMV = EstimateMV(fFlow, iNumFlows, fWeight, *pfROR);

    /*
    ** If estimated value = actual value(upto 2 decimal places), Current value
    ** of ROR is DWROR
    */
    if (fabs(fEstEndMV - fEndMV) < fMktValDiff)
      return 0;
    else if (fEstEndMV > fEndMV) {
      fR1 = *pfROR;
    } /* if estimated value is greater than the actual value */
    else {
      fR0 = *pfROR;
    } /* if estimated value is less than the actual value */
  } /* for i < NUMITERATIONS */

  return 986;
} /* DWBaseRorBisect */

/**
** Function to calculate dollar weighted ror. The process is to iteratively
** solve for R which satisfies the following equation:
**   EndMV = Summation [ F(i) * (1 + R)^W(i) ]
**    End MV = ending market value
**    F(i)   = ith Flow ( F(0) is the begining market value)
**    W(i)   = ith weight
** To start with, function estimates the value of R by solving it for time
** weighted return[R = (EndMV - BegMV - Net Flow)/(Beg MV + Wtd Flow) ].
** For the ** subsequent iterations, R is estimated as
** R = R(previous) + (Actual EMV - Est MV) / Beg MV.
** If at any time estimated end mv = actual mv, function stops with the return.
** If the absolute difference between R(when est end MV is more than actual)
** and R(when est end mv is less than actual) is less than .001, the function
** returns with the estimated return. If the function cannot estimate returns in
** MAXITERATIONS, then it returns with a business error = 998 and
** then the calling program should ignore the results.
**/
int DWBaseRor(double fEndMV, double fFlow[], int iNumFlows, double fWeight[],
              double *pfROR) {
  double fLTActualMV, fGTActualMV; /*Less than and Greater Than Actual*/
  double fLTActualROR, fGTActualROR, fMktValDiff; /* Market value and returns */
  BOOL bFirstLT, bFirstGT, bBigIncrement;
  int i, iClosestIteration;
  double fEstEndMV, fNetF, fWtdF, fClosestMV, fClosestROR;

  i = DWRORCheckAndSetup(fEndMV, fFlow, iNumFlows, fWeight, pfROR, &fNetF,
                         &fWtdF, &fMktValDiff);

  if (i)
    return i;

  bFirstLT = bFirstGT = bBigIncrement = TRUE;
  fLTActualROR = fGTActualROR = fClosestMV = fClosestROR = 0;
  fLTActualMV = fGTActualMV = 0;
  iClosestIteration = 0;

  for (i = 0; i < MAXITERATIONS; i++) {

    if (*pfROR < -0.999999) {
      DWBaseRorBisect(fEndMV, fFlow, iNumFlows, fWeight, pfROR);
      if (*pfROR < -0.999999)
        return 998;
    }

    /* Estimate End MV for current value of *pfROR */
    fEstEndMV = EstimateMV(fFlow, iNumFlows, fWeight, *pfROR);

    /*
    ** If estimated value = actual value(upto a dollar in most case and between
    * a dollar and
    ** a penny in rest decimal places), Current value of ROR is DWROR
    */
    if (fabs(fEstEndMV - fEndMV) < fMktValDiff)
      return 0;
    else if (fEstEndMV > fEndMV) {
      /*Find out the least estimated end mv which is greater than actual endmv*/
      if (bFirstGT == TRUE || fEstEndMV < fGTActualMV) {
        fGTActualMV = fEstEndMV;
        fGTActualROR = *pfROR;
        bFirstGT = FALSE;
      }
    } /* if estimated value is greater than the actual value */
    else {
      /* Find out the biggest estimated end mv which is less than actual endmv*/
      if (bFirstLT == TRUE || fEstEndMV > fLTActualMV) {
        fLTActualMV = fEstEndMV;
        fLTActualROR = *pfROR;
        bFirstLT = FALSE;
      }
    } /* if estimated value is less than the actual value */

    /*
    ** If an ending MV estimate higher than actual end mkt value and another
    ** lower than actual has been found and the absolute difference in estimated
    ** ROR at these two points is less than .0001 then this is the closest
    ** estimate of ROR, we are going to get.
    */
    if (bFirstLT == FALSE && bFirstGT == FALSE &&
        RoundDouble(fabs(fLTActualROR - fGTActualROR), 6) <= 0.000001) {
      // Select the return for which market value is closest to actual market
      // value
      if (fabs(fGTActualMV - fEndMV) < fabs(fLTActualMV - fEndMV))
        *pfROR = fGTActualROR;
      else
        *pfROR = fLTActualROR;
      return 0;
    }

    // If a better estimation is found, reset the variables
    if ((fabs(fEstEndMV - fEndMV) < fabs(fClosestMV - fEndMV)) || i == 0) {
      fClosestMV = fEstEndMV;
      iClosestIteration = i;
      fClosestROR = *pfROR;
    }

    /* Estimate new ror
    SB 4/28/04 - Changed the way new ROR is estimated, now denominator is
    MV + wtdFlow instead of just MV - this gives more accurate result and hence
    yields final result in less number of iterations thn before
    */
    if (fabs(fFlow[0] + fWtdF) >= 0.1 && bBigIncrement)
      *pfROR += (fEndMV - fEstEndMV) / (fFlow[0] + fWtdF);
    else {
      if (fabs(fEndMV + fWtdF) >= 0.1 && bBigIncrement)
        *pfROR += (fEndMV - fEstEndMV) / (fEndMV + fWtdF);
      else if (fabs(fEstEndMV + fWtdF) >= 0.1 && bBigIncrement)
        *pfROR += (fEndMV - fEstEndMV) / (fEstEndMV + fWtdF);
      else if (fEndMV > fEstEndMV)
        *pfROR += 0.0001;
      else
        *pfROR -= 0.0001;
    } /* Begin MV = 0 */

    // If in last five times a better estimate is not found (or the estimation
    // has gone terribly wrong), it means we are going in wrong direction and
    // instead of changing ror by big increment, change it by 0.0001 only SB
    // 12/9/03 - make sure iCloseIteration has been set at least once, if not
    // then program is most likely going in the right direction and then don't
    // change increment
    if ((iClosestIteration > 0 && i - iClosestIteration > 5) ||
        *pfROR < -0.999999 || *pfROR > 999999) {
      *pfROR = fClosestROR;       // go back to best estimation we had so far
      bFirstLT = bFirstGT = TRUE; // reset as if starting from the begining
      bBigIncrement = FALSE;      // increment/decrement ROR by 0.0001
      iClosestIteration = i;
    }
  } /* for i < NUMITERATIONS */

  return DWBaseRorBisect(fEndMV, fFlow, iNumFlows, fWeight, pfROR);
} /* DWBaseRor */

/**
** EndIndex = (1 + ror) * begin index
**/
double GetEndIndex(double fBegIndex, double fRor) {
  double fEndIndex;

  if (fRor <= NAVALUE)
    fEndIndex = NAVALUE;
  else
    fEndIndex = (1.0 + fRor / 100.0) * fBegIndex;

  if ((fEndIndex <= -999.99) || (fEndIndex >= 99999.99))
    fEndIndex = NAVALUE;

  return fEndIndex;
} /* getendindex */

/**
** Ror = ((end_index/beg_index) - 1) * 100
**/
double GetRorFromTwoIndexes(double fBegIndex, double fEndIndex) {
  double fRor;

  fRor = NAVALUE;

  if (fBegIndex != NAVALUE && fEndIndex != NAVALUE &&
      !IsValueZero(fBegIndex, 3))
    fRor = ((fEndIndex / fBegIndex) - 1) * 100.0;

  return fRor;
} /* GetRorFromTwoIndexes */

void LinkReturnsForTheMonth(ALLRORS zPreviousRor, ALLRORS zCurrentRor,
                            ALLRORS *pzNewRor) {
  ALLRORS zUV;
  int i;

  InitializeAllRors(
      &zUV,
      TRUE); // initialize the structure with 100.0 as the initial Unit Value

  // Start a Unit Value stream at 100, link previous ror to it then link current
  // ror to it and then get the return from the result and 100 - that's the
  // linked return for the period
  for (i = 0; i < NUMRORTYPE_ALL; i++) {
    // SB 5/27/15 Simplified the structure
    zUV.fBaseRorIdx[i] =
        GetEndIndex(zUV.fBaseRorIdx[i], zPreviousRor.fBaseRorIdx[i]);
    zUV.fBaseRorIdx[i] =
        GetEndIndex(zUV.fBaseRorIdx[i], zCurrentRor.fBaseRorIdx[i]);
    pzNewRor->fBaseRorIdx[i] = GetRorFromTwoIndexes(100, zUV.fBaseRorIdx[i]);
  }
} // LinkRetunrnsForTheMonth

/*F*
** Function to calculate different fudge factors
*F*/
DllExport void WINAPI CalculateFudgeFactor(RORSTRUCT *pzRS) {
  double fNetFlowAdj, fWtdFlowAdj, fBegAccrAdj, fEndAccrAdj;

  // 	time weighted fudge factor, if required
  if (pzRS->iReturnstoCalculate & TWRorBit) {
    // SB - 5/27/15 Simplified the structure
    pzRS->fGFTWFFactor = TimeWeightedFudgeFactor(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
        pzRS->fNetFlow + pzRS->fFeesOut, pzRS->fWtFlow + pzRS->fWtFeesOut,
        pzRS->zAllRor.fBaseRorIdx[GTWRor - 1], pzRS->bInceptionRor);

    // Net of (consulting) fee time weighted fudge factor
    if (pzRS->bTotalPortfolio || pzRS->bCalcNetForSegments) {
      pzRS->fNFTWFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
          pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
          pzRS->fNetFlow + pzRS->fFees, pzRS->fWtFlow + pzRS->fWtFees,
          pzRS->zAllRor.fBaseRorIdx[NTWRor - 1], pzRS->bInceptionRor);

      pzRS->fCNTWFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
          pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
          pzRS->fNetFlow + pzRS->fFees - pzRS->fCNFees,
          pzRS->fWtFlow + pzRS->fWtFees - pzRS->fWtCNFees,
          pzRS->zAllRor.fBaseRorIdx[CNWRor - 1], pzRS->bInceptionRor);
    }
  }

  // principal fudge factor, if required
  if (pzRS->iReturnstoCalculate & PcplRorBit) {
    if (pzRS->bTotalPortfolio)
      pzRS->fGFPcplFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV - pzRS->fIncome, pzRS->fBeginMV, pzRS->fNetFlow,
          pzRS->fWtFlow, pzRS->zAllRor.fBaseRorIdx[GPcplRor - 1],
          pzRS->bInceptionRor);
    else
      pzRS->fGFPcplFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV, pzRS->fBeginMV, pzRS->fNetFlow + pzRS->fIncome,
          pzRS->fWtFlow + pzRS->fWtIncome,
          pzRS->zAllRor.fBaseRorIdx[GPcplRor - 1], pzRS->bInceptionRor);

    // Net of fee principal fudge factor, if required
    if (pzRS->bTotalPortfolio)
      pzRS->fNFPcplFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV - pzRS->fIncome, pzRS->fBeginMV,
          pzRS->fNetFlow + pzRS->fFees, pzRS->fWtFlow + pzRS->fWtFees,
          pzRS->zAllRor.fBaseRorIdx[NPcplRor - 1], pzRS->bInceptionRor);
    else if (pzRS->bCalcNetForSegments)
      pzRS->fNFPcplFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV, pzRS->fBeginMV,
          pzRS->fNetFlow + pzRS->fIncome + pzRS->fFees,
          pzRS->fWtFlow + pzRS->fWtIncome + pzRS->fWtFees,
          pzRS->zAllRor.fBaseRorIdx[NPcplRor - 1], pzRS->bInceptionRor);
  }

  // Income fudge factor, if required
  if (pzRS->iReturnstoCalculate & IncomeRorBit)
    pzRS->fIncFFactor = TimeWeightedIncomeFudgeFactor(
        pzRS->fEndAI + pzRS->fEndAD, pzRS->fBeginAI + pzRS->fBeginAD,
        pzRS->fIncome, pzRS->fBeginMV,
        pzRS->zAllRor.fBaseRorIdx[IncomeRor - 1]);

  // After tax fudge factor, if required
  if (pzRS->iReturnstoCalculate & AfterTaxRorBit) {
    TaxAdjustments(*pzRS, "A", &fNetFlowAdj, &fWtdFlowAdj, &fBegAccrAdj,
                   &fEndAccrAdj);

    // Gross of fee after tax fudge factor
    pzRS->fGFATFFactor = TimeWeightedFudgeFactor(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
        pzRS->fNetFlow + fNetFlowAdj, pzRS->fWtFlow + fWtdFlowAdj,
        pzRS->zAllRor.fBaseRorIdx[GTWAfterTaxRor - 1], pzRS->bInceptionRor);
    // Net of fee after tax fudge factor
    if (pzRS->bTotalPortfolio || pzRS->bCalcNetForSegments)
      pzRS->fNFATFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
          pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
          pzRS->fNetFlow + pzRS->fFees + fNetFlowAdj,
          pzRS->fWtFlow + pzRS->fWtFees + fWtdFlowAdj,
          pzRS->zAllRor.fBaseRorIdx[NTWAfterTaxRor - 1], pzRS->bInceptionRor);
  } // after tax

  // If Tax equivalent calculations are required
  if (pzRS->iReturnstoCalculate & TaxEquivRorBit) {
    TaxAdjustments(*pzRS, "E", &fNetFlowAdj, &fWtdFlowAdj, &fBegAccrAdj,
                   &fEndAccrAdj);

    // Gross of fee after tax fudge factor
    pzRS->fGFTEFFactor = TimeWeightedFudgeFactor(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
        pzRS->fNetFlow + fNetFlowAdj, pzRS->fWtFlow + fWtdFlowAdj,
        pzRS->zAllRor.fBaseRorIdx[GTWTaxEquivRor - 1], pzRS->bInceptionRor);
    // Net of fee after tax fudge factor
    if (pzRS->bTotalPortfolio || pzRS->bCalcNetForSegments)
      pzRS->fNFTEFFactor = TimeWeightedFudgeFactor(
          pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
          pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
          pzRS->fNetFlow + pzRS->fFees + fNetFlowAdj,
          pzRS->fWtFlow + pzRS->fWtFees + fWtdFlowAdj,
          pzRS->zAllRor.fBaseRorIdx[NTWTaxEquivRor - 1], pzRS->bInceptionRor);
  } // tax equivalent

} // CalculateFudgeFactor*/

/*F*
** Function to calculate different weighted flows
*F*/
DllExport void CalculateWeightedFlow(RORSTRUCT *pzRS) {
  double fNetFlowAdj, fWtdFlowAdj, fBegAccrAdj, fEndAccrAdj;

  // Sb 5/27/15 Simplified the structure
  // Wtd Flow for time weighted ror, if required
  if (pzRS->iReturnstoCalculate & TWRorBit) {
    pzRS->fGFTWWtdFlow = TWWeightedFlow(
        pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
        pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
        pzRS->fNetFlow + pzRS->fFeesOut, pzRS->fGFTWFFactor,
        pzRS->zAllRor.fBaseRorIdx[GTWRor - 1], pzRS->bInceptionRor);

    // Weighted Flow for Net of fees Total return.
    if (pzRS->bTotalPortfolio || pzRS->bCalcNetForSegments) {
      pzRS->fNFTWWtdFlow =
          TWWeightedFlow(pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
                         pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
                         pzRS->fNetFlow + pzRS->fFees, pzRS->fNFTWFFactor,
                         pzRS->zAllRor.fBaseRorIdx[NTWRor - 1],
                         pzRS->bInceptionRor) -
          pzRS->fWtFees;

      pzRS->fCNTWWtdFlow =
          TWWeightedFlow(
              pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD,
              pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD,
              pzRS->fNetFlow + pzRS->fFees - pzRS->fCNFees, pzRS->fCNTWFFactor,
              pzRS->zAllRor.fBaseRorIdx[CNWRor - 1], pzRS->bInceptionRor) -
          pzRS->fWtFees + pzRS->fWtCNFees;
    }
  }

  // Weighted flow for Principal return, if required
  if (pzRS->iReturnstoCalculate & PcplRorBit) {
    if (pzRS->bTotalPortfolio)
      pzRS->fGFPcplWtdFlow = TWWeightedFlow(
          pzRS->fEndMV - pzRS->fIncome, pzRS->fBeginMV, pzRS->fNetFlow,
          pzRS->fGFPcplFFactor, pzRS->zAllRor.fBaseRorIdx[GPcplRor - 1],
          pzRS->bInceptionRor);
    else
      pzRS->fGFPcplWtdFlow =
          TWWeightedFlow(pzRS->fEndMV, pzRS->fBeginMV,
                         pzRS->fNetFlow + pzRS->fIncome, pzRS->fGFPcplFFactor,
                         pzRS->zAllRor.fBaseRorIdx[GPcplRor - 1],
                         pzRS->bInceptionRor) -
          pzRS->fWtIncome;

    // Weighted flow for Net of fee Principal return
    if (pzRS->bTotalPortfolio)
      pzRS->fNFPcplWtdFlow =
          TWWeightedFlow(pzRS->fEndMV - pzRS->fIncome, pzRS->fBeginMV,
                         pzRS->fNetFlow + pzRS->fFees, pzRS->fNFPcplFFactor,
                         pzRS->zAllRor.fBaseRorIdx[NPcplRor - 1],
                         pzRS->bInceptionRor) -
          pzRS->fWtFees;
    else if (pzRS->bCalcNetForSegments)
      pzRS->fNFPcplWtdFlow =
          TWWeightedFlow(pzRS->fEndMV, pzRS->fBeginMV,
                         pzRS->fNetFlow + pzRS->fIncome + pzRS->fFees,
                         pzRS->fNFPcplFFactor,
                         pzRS->zAllRor.fBaseRorIdx[NPcplRor - 1],
                         pzRS->bInceptionRor) -
          pzRS->fWtIncome - pzRS->fWtFees;
  }

  pzRS->fIncWtdFlow = 0;

  // If After tax calculations are required
  if (pzRS->iReturnstoCalculate & AfterTaxRorBit) {
    TaxAdjustments(*pzRS, "A", &fNetFlowAdj, &fWtdFlowAdj, &fBegAccrAdj,
                   &fEndAccrAdj);

    // Wtd Flow for gross of fee after tax ror
    pzRS->fGFATWtdFlow =
        TWWeightedFlow(pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
                       pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD +
                           fBegAccrAdj,
                       pzRS->fNetFlow + fNetFlowAdj, pzRS->fGFATFFactor,
                       pzRS->zAllRor.fBaseRorIdx[GTWAfterTaxRor - 1],
                       pzRS->bInceptionRor) -
        fWtdFlowAdj;

    // Wtd Flow for net of fee after tax ror
    if (pzRS->bTotalPortfolio || pzRS->bCalcNetForSegments)
      pzRS->fNFATWtdFlow =
          TWWeightedFlow(
              pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
              pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
              pzRS->fNetFlow + pzRS->fFees + fNetFlowAdj, pzRS->fNFATFFactor,
              pzRS->zAllRor.fBaseRorIdx[NTWAfterTaxRor - 1],
              pzRS->bInceptionRor) -
          (fWtdFlowAdj + pzRS->fWtFees);
  } // After tax

  // If Tax equivalent calculations are required
  if (pzRS->iReturnstoCalculate & TaxEquivRorBit) {
    TaxAdjustments(*pzRS, "E", &fNetFlowAdj, &fWtdFlowAdj, &fBegAccrAdj,
                   &fEndAccrAdj);

    // Wtd Flow for gross of fee after tax ror
    pzRS->fGFTEWtdFlow =
        TWWeightedFlow(pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
                       pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD +
                           fBegAccrAdj,
                       pzRS->fNetFlow + fNetFlowAdj, pzRS->fGFTEFFactor,
                       pzRS->zAllRor.fBaseRorIdx[GTWTaxEquivRor - 1],
                       pzRS->bInceptionRor) -
        fWtdFlowAdj;

    // Wtd Flow for net of fee after tax ror
    if (pzRS->bTotalPortfolio || pzRS->bCalcNetForSegments)
      pzRS->fNFTEWtdFlow =
          TWWeightedFlow(
              pzRS->fEndMV + pzRS->fEndAI + pzRS->fEndAD + fEndAccrAdj,
              pzRS->fBeginMV + pzRS->fBeginAI + pzRS->fBeginAD + fBegAccrAdj,
              pzRS->fNetFlow + pzRS->fFees + fNetFlowAdj, pzRS->fNFTEFFactor,
              pzRS->zAllRor.fBaseRorIdx[NTWTaxEquivRor - 1],
              pzRS->bInceptionRor) -
          (fWtdFlowAdj + pzRS->fWtFees);
  } // Tax equivalent

  // Calculated values stored in Monthsum
  /* not in use yet - 12/7/04 - vay
  pzRS->fFudgedWtdFees = pzRS->fNFTWWtdFlow - pzRS->fGFTWWtdFlow;
pzRS->fFudgedWtdIncome = pzRS->fIncWtdFlow - pzRS->fGFTWWtdFlow;
  */

} // CalculateWeightedFlow

/*F*
** Function to calculate (time weighted) fudge factor using following formula:
**   Fudge = ( EndMV - (1 + Ror/100) * BeginMV - Flow - Wtd Flow * Ror/100) / (1
* + ror / 200)
** This formula is obtained by using the Time Weighted Base Ror equation and
* solving it
** for fudge factor.
*F*/
double TimeWeightedFudgeFactor(double fEndMV, double fBegMV, double fNFlow,
                               double fWtFlow, double fRor,
                               BOOL bInceptionRor) {
  // For the inception ror, fudge factor is not used
  if (bInceptionRor)
    return 0;
  else if (IsValueZero(1 + fRor / 200, 3)) // Denominator is zero
    return 0;
  else {
    if (fBegMV + fWtFlow < 0)
      return ((fEndMV - ((1 - fRor / 100) * fBegMV) - fNFlow +
               (fWtFlow * fRor / 100)) /
              (1 - fRor / 200));
    else
      return ((fEndMV - ((1 + fRor / 100) * fBegMV) - fNFlow -
               (fWtFlow * fRor / 100)) /
              (1 + fRor / 200));
  }
} // TimeWeightedFudgeFactor

/*F*
** Function to calculate (time weighted) income fudge factor using following
* formula:
**   Fudge = ( EndAccr - (1 + Ror/100) * BeginAccrl - Begin MV * Ror/100 +
* Income) / (1 + ror / 200)
** This formula is obtained by using the Time Weighted Incopme Ror equation and
* solving it
** for fudge factor.
*F*/
double TimeWeightedIncomeFudgeFactor(double fEndAccr, double fBegAccr,
                                     double fIncome, double fBegMV,
                                     double fRor) {
  if (IsValueZero(1 + fRor / 200, 3))
    return 0;
  else
    return ((fEndAccr - ((1 + fRor / 100) * fBegAccr) - (fBegMV * fRor / 100) +
             fIncome) /
            (1 + fRor / 200));
} // TimeWeightedIncomeFudgeFactor

/*F*
** Function to calculate (time weighted) weighted flow using following formula:
**   Wt Flow = ((end mv - beg mv - net flow - fudge) / ror) * 100 - beg mv -
* fudge/2
** This formula is obtained by using the Time Weighted Base Ror equation and
* solving it
** for weighted flow.
*F*/
double TWWeightedFlow(double fEndMV, double fBegMV, double fNFlow,
                      double fFudge, double fRor, BOOL bInceptionRor) {
  if (IsValueZero(fRor, 4) || fEndMV < NA_MV || fBegMV < NA_MV ||
      fNFlow < NA_MV || fFudge < NA_MV)
    return 0;
  else {
    if (fBegMV < 0)
      return (((-fEndMV + fBegMV + fNFlow + fFudge) / fRor) * 100 + fBegMV +
              fFudge / 2);
    else
      return (((fEndMV - fBegMV - fNFlow - fFudge) / fRor) * 100 - fBegMV -
              fFudge / 2);
  }
} // TWWeightedFlow

/**
** This function calculates the new index value using passed Rors and
** PreviousIndex values for the same ror type. Since tax related return
** calculations are done seperately, when this function is called, new index
** need not be calculated for all fields, WhichIndex tells us which fields need
** to be calculated, its value can be one of the folloing :
**   1  - Taxable Federal Calculations
**   2  - Taxable State Calculations
**   3  - Taxable Total Calculations
**   4  - Tax Equivalent Federal Calculations
**   5  - Tax Equivalent State Calculations
**   6  - Tax Equivalent Total Calculations
** else - Principal, income and base
**/
/*void CalculateNewRorIndex(ALLRORS zBeginIndex, ALLRORS zRor, ALLRORS
*pzNewIndex, int iWhichIndex)
{
  int i;

  if (iWhichIndex == 1) // Federal taxable
  {
    for (i = 0; i < NUMRORTYPE_BASIC; i++)
    {
      pzNewIndex->zIndex[i].iRorType = zBeginIndex.zIndex[i].iRorType;
      pzNewIndex->zIndex[i].fFedtaxRorIdx =
GetEndIndex(zBeginIndex.zIndex[i].fFedtaxRorIdx, zRor.zIndex[i].fFedtaxRorIdx);
    }
  } // whichindex = 1
  else if (iWhichIndex == 2) // State taxable
  {
    for (i = 0; i < NUMRORTYPE_BASIC; i++)
    {
      pzNewIndex->zIndex[i].iRorType = zBeginIndex.zIndex[i].iRorType;
      pzNewIndex->zIndex[i].fStatetaxRorIdx =
GetEndIndex(zBeginIndex.zIndex[i].fStatetaxRorIdx,
                                                                                                                                                                                                                                        zRor.zIndex[i].fStatetaxRorIdx);
    } // for i < numrortype
  } // WhichIndex = 2
  else if (iWhichIndex == 3) // Total taxable
  {
    for (i = 0; i < NUMRORTYPE_BASIC; i++)
    {
      pzNewIndex->zIndex[i].iRorType = zBeginIndex.zIndex[i].iRorType;
      pzNewIndex->zIndex[i].fAtaxRorIdx =
GetEndIndex(zBeginIndex.zIndex[i].fAtaxRorIdx, zRor.zIndex[i].fAtaxRorIdx); } //
For i < numrortype } // whichIndex = 3 else if (iWhichIndex == 4) // Federal tax
equivalent (tax free)
  {
    for (i = 0; i < NUMRORTYPE_BASIC; i++)
    {
      pzNewIndex->zIndex[i].iRorType = zBeginIndex.zIndex[i].iRorType;
      pzNewIndex->zIndex[i].fFedfreeRorIdx =
GetEndIndex(zBeginIndex.zIndex[i].fFedfreeRorIdx,
                                                                                                                                                                                                                                 zRor.zIndex[i].fFedfreeRorIdx);
    }
  } // WhichIndex = 4
  else if (iWhichIndex == 5) // State Tax equivalent (tax free)
  {
    for (i = 0; i < NUMRORTYPE_BASIC; i++)
    {
      pzNewIndex->zIndex[i].iRorType = zBeginIndex.zIndex[i].iRorType;
      pzNewIndex->zIndex[i].fStatefreeRorIdx =
GetEndIndex(zBeginIndex.zIndex[i].fStatefreeRorIdx,
                                                                                                                                                                                                                                         zRor.zIndex[i].fStatefreeRorIdx);
    } // For i < numrortype
  } // whichIndex = 5
  else if (iWhichIndex == 6)
  {
    for (i = 0; i < NUMRORTYPE_BASIC; i++)
    {
      pzNewIndex->zIndex[i].iRorType = zBeginIndex.zIndex[i].iRorType;
      pzNewIndex->zIndex[i].fEtaxRorIdx =
GetEndIndex(zBeginIndex.zIndex[i].fEtaxRorIdx, zRor.zIndex[i].fEtaxRorIdx); } //
For i < numrortype } // WhichIndex = 6 else
  {
    for (i = 0; i < NUMRORTYPE_BASIC; i++)
    {
      pzNewIndex->zIndex[i].iRorType = zBeginIndex.zIndex[i].iRorType;
      pzNewIndex->zIndex[i].fPrincipalIdx =
GetEndIndex(zBeginIndex.zIndex[i].fPrincipalIdx, zRor.zIndex[i].fPrincipalIdx);
      pzNewIndex->zIndex[i].fIncomeIdx =
GetEndIndex(zBeginIndex.zIndex[i].fIncomeIdx, zRor.zIndex[i].fIncomeIdx);
      pzNewIndex->zIndex[i].fBaseRorIdx =
GetEndIndex(zBeginIndex.zIndex[i].fBaseRorIdx, zRor.zIndex[i].fBaseRorIdx); } //
For i < numrortype } // whichIndex is not between 1 - 6 pzNewIndex->iNumRorType
= zBeginIndex.iNumRorType;

} // CalculateNewRorIndex
*/

/**
** Function to test whether a key is deleted or not
**/
BOOL IsKeyDeleted(long lDeleteDate) {
  if (lDeleteDate == 0)
    return FALSE;

  return TRUE;
} /* IsKeyDeleted */

/**
** Function to get tax rate by using the formula :
**   Tax Rate = 1 - (portfolio's tax rate / 100)
**/
double EffectiveTaxRate(double fPortTaxRate) {
  double fTaxRate;

  fTaxRate = 1 - (fPortTaxRate / 100);

  return fTaxRate;
} /*  EffectiveTaxRate */

/**
** Function to calculate tax equivalent adjustments(income, withholding,
* reclaim, etc.)
** using the formula:
**  XXX Adjustment = (Total XXX / tax rate) - Total XXX
**  where XXX - Income, withholding, reclaim, accruals, etc.
** NOTE : THIS FUNCTION IS NOT DOING ANY CHECKING TO MAKE SURE THERE IS NO
**        DIVIDE BY ZERO, BECAUSE THE CALLING FUNCTION IS ALREADY DOING THAT
**        CHECKING.
**/
double TaxEquivalentAdjustment(double fTotValue, double fTaxRate) {
  double fAdjustment;

  fAdjustment = (fTotValue / fTaxRate) - fTotValue;

  return fAdjustment;
} /* taxadjustment */

/**
** Function to calculate after tax adjustments(income, withholding, reclaim,
* etc.)
** using the formula:
**  XXX Adjustment = (Total XXX * tax rate) - Total XXX
**  where XXX - Income, withholding, reclaim, accruals, etc.
** NOTE : THIS FUNCTION IS NOT DOING ANY CHECKING TO MAKE SURE THERE IS NO
**        DIVIDE BY ZERO, BECAUSE THE CALLING FUNCTION IS ALREADY DOING THAT
**        CHECKING.
**/
double AfterTaxAdjustment(double fTotValue, double fTaxRate) {
  double fAdjustment;

  fAdjustment = (fTotValue * fTaxRate) - fTotValue;

  return fAdjustment;
} /* aftertaxadjustment */

/**
** Function to find Monthly record by matching segment id
**/
int FindMonthlyDataByID(MONTHTABLE zMTable, int iID) {
  int i, iIndex;

  iIndex = -1;
  i = 0;

  while (iIndex == -1 && i < zMTable.iCount) {
    if (zMTable.pzMonthlyData[i].zMonthsum.iID == iID)
      iIndex = i;

    i++;
  }

  return iIndex;
} /* FindMonthlyDataByID */

/**
** This function adds Monthly data record to the Monthly table.
**/
ERRSTRUCT AddMonthlyDataToTable(MONTHTABLE *pzMTable, SUMMDATA zMonthsum,
                                RORSTRUCT zMonthlyRor) {
  ERRSTRUCT zErr;
  int i;
  // char smsg[100];

  lpprInitializeErrStruct(&zErr);

  /* If table is full to its limit, allocate more space */
  if (pzMTable->iCapacity == pzMTable->iCount) {
    pzMTable->iCapacity += EXTRAPKEY;
    pzMTable->pzMonthlyData = (MONTHSTRUCT *)realloc(
        pzMTable->pzMonthlyData, pzMTable->iCapacity * sizeof(MONTHSTRUCT));
    if (pzMTable->pzMonthlyData == NULL)
      return (lpfnPrintError("Insufficient Memory For MTable", 0, 0, "", 997, 0,
                             0, "CALCPERF ADDMONTHLY", FALSE));
    // sprintf_s(smsg, "Memory Address For MTable is; %Fp",
    // pzMTable->pzMonthlyData); lpfnPrintError(smsg, 0, 0, "", 0, 0, 0,
    // "DEBUG", TRUE);
  }

  i = pzMTable->iCount;

  pzMTable->pzMonthlyData[i].zMonthlyRor = zMonthlyRor;
  pzMTable->pzMonthlyData[i].zMonthsum = zMonthsum;
  pzMTable->pzMonthlyData[i].iNumSubPeriods = 0;

  pzMTable->iCount++;

  return zErr;
} /* AddMonthlyDataToTable */

/*
** This function gets called to add daily info (add this time only daily info is
* industry levels) to the asset. In
** addition to adding the date which is being asked to add, if the date is later
* than lLastPerfDate then the previous
** day is also automatically added to the daily info array. This is done becuase
* when an industry level changes on
** a day, previous day's market value is used to generate notional flow. So, to
* keep the previous day's market value
** in the array, we need to add previous day to the array.
*/
ERRSTRUCT AddDailyInfoToAsset(PARTASSET2 *pzPAsset, LEVELINFO zDInfo,
                              long lLastPerfDate, long lCurrentPerfDate) {
  ERRSTRUCT zErr;
  int iIndex;

  lpprInitializeErrStruct(&zErr);

  // find the date from which industry level1 should be effective and add that
  // day and set industry level1 for that date
  if (zDInfo.iIndustLevel1 > 0 && zDInfo.lEffDate1 <= lCurrentPerfDate) {
    zErr = FindOrAddDailyAssetInfo(pzPAsset, zDInfo.lEffDate1, lCurrentPerfDate,
                                   &iIndex);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    if (iIndex >= 0 && iIndex < pzPAsset->iDailyCount)
      pzPAsset->pzDAInfo[iIndex].iIndustLevel1 = zDInfo.iIndustLevel1;

    if (zDInfo.lEffDate1 > lLastPerfDate) {
      zErr = FindOrAddDailyAssetInfo(pzPAsset, zDInfo.lEffDate1 - 1,
                                     lCurrentPerfDate, &iIndex);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    } // add previous day as well if it's after last perf date
  }

  // find the date from which industry level2 should be effective and set
  // industry level2 for that date
  if (zDInfo.iIndustLevel2 > 0 && zDInfo.lEffDate2 <= lCurrentPerfDate) {
    zErr = FindOrAddDailyAssetInfo(pzPAsset, zDInfo.lEffDate2, lCurrentPerfDate,
                                   &iIndex);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    if (iIndex >= 0 && iIndex < pzPAsset->iDailyCount)
      pzPAsset->pzDAInfo[iIndex].iIndustLevel2 = zDInfo.iIndustLevel2;

    if (zDInfo.lEffDate2 > lLastPerfDate) {
      zErr = FindOrAddDailyAssetInfo(pzPAsset, zDInfo.lEffDate2 - 1,
                                     lCurrentPerfDate, &iIndex);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    } // add previous day as well if it's after last perf date
  }

  // find the date from which industry level3 should be effective and set
  // industry level1 for that date
  if (zDInfo.iIndustLevel3 > 0 && zDInfo.lEffDate3 <= lCurrentPerfDate) {
    zErr = FindOrAddDailyAssetInfo(pzPAsset, zDInfo.lEffDate3, lCurrentPerfDate,
                                   &iIndex);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    if (iIndex >= 0 && iIndex < pzPAsset->iDailyCount)
      pzPAsset->pzDAInfo[iIndex].iIndustLevel3 = zDInfo.iIndustLevel3;

    if (zDInfo.lEffDate3 > lLastPerfDate) {
      zErr = FindOrAddDailyAssetInfo(pzPAsset, zDInfo.lEffDate3 - 1,
                                     lCurrentPerfDate, &iIndex);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    } // add previous day as well if it's after last perf date
  }

  return zErr;
} // AddDailyInfoToAsset

/*
** When a date is added to daily info array in the asset, it's quite possible
* that one or more of the industry
** levels are 0 (not set). The rule for industry levels is that its previous
* value is valid until the day a new
** valid date is found. So this functtion goes through all the dates for all the
* assets and if an industry level
** is not set, it sets it up to its previous value.
*/
void FillLevelsForThePeriod(ASSETTABLE2 *pzATable) {
  int i, j, iLevel1, iLevel2, iLevel3;

  for (i = 0; i < pzATable->iNumAsset; i++) {
    if (pzATable->pzAsset[i].iDailyCount <= 0)
      continue;

    iLevel1 = pzATable->pzAsset[i].pzDAInfo[0].iIndustLevel1;
    iLevel2 = pzATable->pzAsset[i].pzDAInfo[0].iIndustLevel2;
    iLevel3 = pzATable->pzAsset[i].pzDAInfo[0].iIndustLevel3;

    for (j = 1; j < pzATable->pzAsset[i].iDailyCount; j++) {
      // if current day's industry level1 is non-zero and different than
      // previous (day's) industry level1, set AnyValDifferent to TRUE and use
      // today's industry level1 to populate future day's zero value for
      // indiustry level1.
      if (pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel1 == 0)
        pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel1 = iLevel1;
      else
        iLevel1 = pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel1;

      // do same for level2
      if (pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel2 == 0)
        pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel2 = iLevel2;
      else
        iLevel2 = pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel2;

      // do same for level3
      if (pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel3 == 0)
        pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel3 = iLevel3;
      else
        iLevel3 = pzATable->pzAsset[i].pzDAInfo[j].iIndustLevel3;
    } // loop through all days for an asset
  } // i < pzATable->iNumAsset
  return;
} // FillLevelsForThePeriod

/*
** This function compares two script details and if every field in two scripts
* match then the scripts are
** considered same. This function does not compare template header and hash key
* fields in the headers. This
** is an additional check being added in the system becuase checking template
* header and hash key has been
** giving some false positives causing performance on some segments going to
* another segment. Check to
** compare every single field for all the scripts is not very effiecient, that's
* why this check is not performed
** on every single script, it's performed only on those scripts which have
* already been found to be same
** becuase their template header and hash key already matches.
*/
BOOL AreTheseScriptsSame(PSCRHDRDET zSHD1, PSCRHDRDET zSHD2) {
  BOOL bResult;
  int i;

  // if even the number of rows in two scripts are not same these scripts can't
  // be same
  if (zSHD1.iNumDetail != zSHD2.iNumDetail)
    return FALSE;

  // start with assuming these two scripts are same
  bResult = TRUE;
  i = 0;
  while (i < zSHD1.iNumDetail && bResult) {
    if (zSHD1.pzDetail[i].lSeqNo != zSHD2.pzDetail[i].lSeqNo ||
        strcmp(zSHD1.pzDetail[i].sSelectType, zSHD2.pzDetail[i].sSelectType) !=
            0 ||
        strcmp(zSHD1.pzDetail[i].sComparisonRule,
               zSHD2.pzDetail[i].sComparisonRule) != 0 ||
        strcmp(zSHD1.pzDetail[i].sBeginPoint, zSHD2.pzDetail[i].sBeginPoint) !=
            0 ||
        strcmp(zSHD1.pzDetail[i].sEndPoint, zSHD2.pzDetail[i].sEndPoint) != 0 ||
        strcmp(zSHD1.pzDetail[i].sAndOrLogic, zSHD2.pzDetail[i].sAndOrLogic) !=
            0 ||
        strcmp(zSHD1.pzDetail[i].sIncludeExclude,
               zSHD2.pzDetail[i].sIncludeExclude) != 0 ||
        strcmp(zSHD1.pzDetail[i].sMaskRest, zSHD2.pzDetail[i].sMaskRest) != 0 ||
        strcmp(zSHD1.pzDetail[i].sMatchRest, zSHD2.pzDetail[i].sMatchRest) !=
            0 ||
        strcmp(zSHD1.pzDetail[i].sMaskWild, zSHD2.pzDetail[i].sMaskWild) != 0 ||
        strcmp(zSHD1.pzDetail[i].sMatchWild, zSHD2.pzDetail[i].sMatchWild) !=
            0 ||
        strcmp(zSHD1.pzDetail[i].sMaskExpand, zSHD2.pzDetail[i].sMaskExpand) !=
            0 ||
        strcmp(zSHD1.pzDetail[i].sMatchExpand,
               zSHD2.pzDetail[i].sMatchExpand) != 0 ||
        zSHD1.pzDetail[i].lStartDate != zSHD2.pzDetail[i].lStartDate ||
        zSHD1.pzDetail[i].lEndDate != zSHD2.pzDetail[i].lEndDate)
      bResult = FALSE;

    i++;
  }

  return bResult;
}

/*
** Function to select perfasset merge records and add them to memory table
*/
ERRSTRUCT FillPerfAssetMergeTable(PERFASSETMERGETABLE *pzPAMTable,
                                  ASSETTABLE2 *pzATable, int iVendorID,
                                  long lLastPerfDate, long lCurrentPerfDate) {
  ERRSTRUCT zErr;
  PERFASSETMERGE zPAM;
  PARTASSET2 zTempAsset;
  LEVELINFO zLevels;
  int iLongShort;

  lpprInitializeErrStruct(&zErr);

  InitializePerfAssetMergeTable(pzPAMTable);

  iLongShort = SRESULT_LONG | ARESULT_LONG;

  while (TRUE) {
    InitializePerfAssetMerge(&zPAM);
    // select any asset that needs to be merged
    lpprSelectPerfAssetMerge(&zPAM, &zErr);
    if (zErr.iSqlError == SQLNOTFOUND) {
      zErr.iSqlError = 0;
      break;
    } else if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
      return zErr;

    // If the end date is blank(12/30/1899), set it to day after current
    // performance day
    if (zPAM.lEndDate == 0)
      zPAM.lEndDate = lCurrentPerfDate + 1;

    // If begin or end date is invalid OR the end date is on or prior to begin
    // date, skip the record
    if (zPAM.lBeginDate >= zPAM.lEndDate || zPAM.lBeginDate < 0 ||
        zPAM.lEndDate < 0)
      continue;

    // If the "merge from" asset  doesn't exist in assets table (not needed for
    // this account), then no need to add it to perfassetmerge table
    zPAM.iFromSecNoIndex = FindAssetInTable(*pzATable, zPAM.sMergeFromSecNo,
                                            "N", FALSE, iLongShort);
    if (zPAM.iFromSecNoIndex < 0)
      continue;

    // Find the "merge to" asset in the asset table, if it doesn't exist, add it
    // to the table
    zPAM.iToSecNoIndex =
        FindAssetInTable(*pzATable, zPAM.sMergeToSecNo, "N", FALSE, iLongShort);
    if (zPAM.iToSecNoIndex < 0) {
      zErr = SelectAssetAllLevels(pzATable, zPAM.sMergeToSecNo, "N", iVendorID,
                                  &zTempAsset, &zLevels, iLongShort, 0, 0,
                                  &zPAM.iToSecNoIndex, lLastPerfDate,
                                  lCurrentPerfDate);
      if (zErr.iSqlError != 0 || zErr.iBusinessError != 0)
        break;
    } // If merged to security doesn't exist in assets table

    // Add perfassetmerge record to the table
    zErr = AddPerfAssetMergeToTable(pzPAMTable, zPAM);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    // Add a daily info record on the merge dates (start date and if required
    // end date + 1) to asset being merged from
    zErr = AddDailyInfoIfNeededOnPertfAssetMergeDates(
        pzATable, zPAM, TRUE, lLastPerfDate, lCurrentPerfDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    // Add a daily info record on the merge dates (start date and if required
    // end date) to asset being merged to
    zErr = AddDailyInfoIfNeededOnPertfAssetMergeDates(
        pzATable, zPAM, FALSE, lLastPerfDate, lCurrentPerfDate);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;
  } // while TRUE

  return zErr;
} // FillperfAssetMergeTable

/*
** Function to add daily asset info records for the merge date (and previous
* date) to the asset being merged
** from OR asset being merged to in the asset table
*/
ERRSTRUCT AddDailyInfoIfNeededOnPertfAssetMergeDates(ASSETTABLE2 *pzATable,
                                                     PERFASSETMERGE zPAM,
                                                     BOOL bAddToFromMergedAsset,
                                                     long lLastPerfDate,
                                                     long lCurrentPerfDate) {
  ERRSTRUCT zErr;
  int iAssetIndex, iDInfoIndex;

  lpprInitializeErrStruct(&zErr);

  if (bAddToFromMergedAsset)
    iAssetIndex = zPAM.iFromSecNoIndex;
  else
    iAssetIndex = zPAM.iToSecNoIndex;

  if (iAssetIndex >= 0 && iAssetIndex <= pzATable->iNumAsset) {
    zErr = FindOrAddDailyAssetInfo(&pzATable->pzAsset[iAssetIndex],
                                   zPAM.lBeginDate, lCurrentPerfDate,
                                   &iDInfoIndex);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    // If the begin date is between the dates for which we are calculating
    // performance then add the previous day as well. This is done becuase we
    // will need to create a notional flow for the assets being merged from and
    // merged into on the merge date. Notional flow will equal the market value
    // of the security on the day before merge start date.
    if (zPAM.lBeginDate > lLastPerfDate &&
        zPAM.lBeginDate <= lCurrentPerfDate) {
      zErr = FindOrAddDailyAssetInfo(&pzATable->pzAsset[iAssetIndex],
                                     zPAM.lBeginDate - 1, lCurrentPerfDate,
                                     &iDInfoIndex);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    }

    // Add the merge end date. If the date falls between the days performance is
    // being calculated then need to create notional flow on this day
    zErr =
        FindOrAddDailyAssetInfo(&pzATable->pzAsset[iAssetIndex], zPAM.lEndDate,
                                lCurrentPerfDate, &iDInfoIndex);
    if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
      return zErr;

    // if the end date falls between the time period for which we are
    // calculating performance, add the day after end date. This is done so that
    // in perfkeyasset table we have the first day on which merge rule cease to
    // exist and things go back to original security rules.
    if (zPAM.lEndDate >= lLastPerfDate && zPAM.lEndDate < lCurrentPerfDate) {
      zErr = FindOrAddDailyAssetInfo(&pzATable->pzAsset[iAssetIndex],
                                     zPAM.lEndDate + 1, lCurrentPerfDate,
                                     &iDInfoIndex);
      if (zErr.iBusinessError != 0 || zErr.iSqlError != 0)
        return zErr;
    }
  }

  return zErr;
} // AssDailyInfoIfNeededOnPerfAssetMergeDates

/*
** Function to find whether for a given day, is the given asset included in the
* merge and if it is or not
*/
int CurrentOrMergedAssetIndexForToday(PERFASSETMERGETABLE zPAMTable,
                                      int iAssetIndex, long lDate) {
  int i;

  for (i = 0; i < zPAMTable.iCount; i++) {
    if (zPAMTable.pzMergedAsset[i].iFromSecNoIndex == iAssetIndex) {
      // found the record, now look whether for the given date, asset merge rule
      // is in place or not
      if (zPAMTable.pzMergedAsset[i].iToSecNoIndex >= 0 &&
          lDate >= zPAMTable.pzMergedAsset[i].lBeginDate &&
          lDate <= zPAMTable.pzMergedAsset[i].lEndDate)
        return zPAMTable.pzMergedAsset[i].iToSecNoIndex;
      else
        return iAssetIndex;
    } // if found the assetindex
  } // for i loop

  // if above loop didn't find asset index in the PAMTable, return the same
  // index back
  return iAssetIndex;
} // CurrentorMergedAssetIndexForToday

/**
*
* SUB-SYSTEM: Data Entry Objects
*
* FILENAME: DataEntryObjects.cpp
*
* DESCRIPTION: Subset of Data Entry Objects implemented in Delphi's DataEntryObjectsUnit.pas
*				to provide required funcionality for Performance.Dll (AddReturnToBlob)
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:
*
* AUTHOR: Valeriy Yegorov
*
*
**/
#include "DataEntryObjects.h"

// implementation of C_ReturnSet methods
C_ReturnSet::C_ReturnSet()
{
	memset(this, 0, sizeof(*this));
}

// this is a functionally identical replica of DelphiCInterface.DLL's AddReturnToBlob
int  AddReturnToBlob(int iSegmentID, int iReturnType, long lReturnDate,
                     double fROR, BOOL bStart, BOOL bFinish)
{
	C_ReturnSet *cReturnSet = NULL;

  /*
  ** If starting a new segment returns the previous segment must have been finished and
  ** at the time of finish RorList gets freeed and niled. If RorList is nil then create
  ** a new one and if it is not then make it nil, print a warning and continue (this most
  ** likely happens when there is some error in the previous portfolio in adding returns to the blob).
 	if (bStart) {
  
		// starting a new one without finishing the last one, print a warning and continue
		if (cRorList) {
      PrintError('Start Of the Process And Return List Is Not Nil. Making It Nil', 0, SegmentID, 'G', 999, 0, 0, 'DELPHICINT ADDRETURNTOBLOB1', TRUE);
      RorList.Clear;
      RorList.Free;
      RorList := nil;
    end;

    RorList := T_ListOfReturns.Create;
    RorList.ItemKey.ID := SegmentID;
    RorList.ItemKey.ItemType := itSelfEntered; //itPortfolio;
    RorList.ItemKey.DatabaseID := c_SelfEnteredDBID;
  end
  else if (RorList = nil) then
  begin
    PrintError('Not The Start Of the Process And Return List Is Nil', 0, SegmentID, 'G', 999, 0, 0, 'DELPHICINT ADDRETURNTOBLOB2', FALSE);
    Result := -2;
    exit;
  end;

*/
	return 0;
}

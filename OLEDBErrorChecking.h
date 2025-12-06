/**
*
* SUB-SYSTEM: Database Input/Output for TranProc
*
* FILENAME: OLEDBErrorChecking.h
*
* DESCRIPTION:	Class COLEDBErrorChecking is defined to retrieve error information
*				while accessing data in SQL Server database through  OLE DB
*
* PUBLIC FUNCTION(S):
*
* NOTES:
*
* USAGE:	As a part of OLEDB.DLL. Instance of COLEDBErrorChecking class should be used
*			to obtain error description if OLE DB operation fails.
*
* AUTHOR:	This code is borrowed from the book
*			"OLE DB and ODBC Developer's Guide" by Chuck Wood, (C) 1999 IDG Books Worldwide, Inc.
*			Partially revised by Valeriy Yegorov. (C) 2001 Effron Enterprises, Inc.
*
*
**/

// History.
// 08/15/2001 Fixed AV while accessing spErrInfo. Initial version.

#ifndef OLEDB_Error_Routine_Included
#define OLEDB_Error_Routine_Included
#include <oledberr.h>
//#include <atldbcli.h>
#include <msoledbsql.h>

class COLEDBErrorChecking {
public:
	//Inline functions
	BOOL DBErrorsAreSupported(CComPtr<IUnknown> m_spUnk)
	{
		CComPtr<ISupportErrorInfo> spSupportErrorInfo;
		if (SUCCEEDED(
			m_spUnk->QueryInterface(IID_ISupportErrorInfo,
			(void **)&spSupportErrorInfo))) {
			if (SUCCEEDED(
				spSupportErrorInfo->
				InterfaceSupportsErrorInfo(IID_ICommandPrepare))){
				return TRUE;
			}
		}
		return FALSE;
	}
	void GetDBErrors(CComPtr<IUnknown> m_spUnk, char *msg)
	{
		CDBErrorInfo errInfo;
		IErrorInfo *pErrorInfo = NULL;
		BSTR pErrorDescription = NULL;
		ULONG ulRecords = 0;
		HRESULT hr =
			errInfo.GetErrorRecords(m_spUnk,
			IID_ICommandPrepare, &ulRecords);
		if (FAILED(hr) || hr == S_FALSE || ulRecords == 0) {
			//The error info object could not be retrieved
			strcat_s(msg, 1024, "\n\nCould not retrieve an error info object.");
			strcat_s(msg, 1024, "\nTherefore, additional error information is not available.");
		}
		else {
			// Error info object was retrieved successfully
			LCID lcid = GetUserDefaultLCID();
			for (ULONG loop = 0; loop < ulRecords; loop++) {
				// Get the error information from the source
				hr = errInfo.GetErrorInfo(loop, lcid, &pErrorInfo);
				if (FAILED(hr)) {
					continue;
				}
				//Get the error description
				pErrorInfo->GetDescription(&pErrorDescription);
				//Convert error description to single-width character
				sprintf_s(msg, 1024, "%s\n\n%S", msg, pErrorDescription);
				//Get SQLState and Error Code
				GetSQLCodes(msg, &errInfo, loop);
				//Clean up
				SysFreeString(pErrorDescription);
				pErrorInfo->Release();
			}
		}
	}
	void DisplayDBErrors(CComPtr<IUnknown> m_spUnk, char *msg)
	{
		//Allow 1 k for the error message
		char message[1024];
		if (msg)
			strcpy_s(message, msg);
		else
			strcpy_s(message, "");
		if (m_spUnk)
			if (DBErrorsAreSupported(m_spUnk))
			{
				GetDBErrors(m_spUnk, message);
#ifdef DEBUG
				::MessageBox(NULL, message, "DB Error", MB_OK);
#endif
			}
			else
				DisplaySingleError();
		else
			DisplaySingleError();
	}
	void DisplayAllErrors(CComPtr<IUnknown> m_spUnk,
		char *msg = NULL,
		HRESULT hresult = S_OK,
		char *strFunction = NULL)
	{
		//Allow 1 k for the error message
		char message[1024];
		if (msg)
			strcpy_s(message, msg);
		else
			strcpy_s(message, "");
		if (strFunction) {	//Check for function equal to null
			//Function was passed, so see what function it was
			strcat_s(message, " in the ");
			strcat_s(message, strFunction);
			strcat_s(message, " function ");
		}
		if (FAILED(hresult)) {
			GetHRRESULTMessage(hresult, message);
		}
		DisplayDBErrors(m_spUnk, message);
	}
	//Static functions
	static void DisplaySingleError(HRESULT hresult = S_OK, char *strFunction = NULL)
	{
		//Allow 1 k for the error message
		char msg[1024];
		strcpy_s(msg, "");
		if (strFunction) {	//Check for function equal to null
			//Function was passed, so see what function it was
			strcat_s(msg, "Error occurred in the ");
			strcat_s(msg, strFunction);
			strcat_s(msg, " function \n");
		}
		if (FAILED(hresult)) {
			GetHRRESULTMessage(hresult, msg);
		}
		GetSingleError(msg);
#ifdef DEBUG
		::MessageBox(NULL, msg, "Error Message", MB_OK);
#endif
	}
	static void GetSingleError(char *msg)
	{
		IErrorInfo *pErrorInfo = NULL;
		BSTR pErrorDescription = NULL;
		if (SUCCEEDED(GetErrorInfo(0, &pErrorInfo)) && (pErrorInfo)) {
			//Get the error description
			if (SUCCEEDED(pErrorInfo->GetDescription(&pErrorDescription)) && (pErrorDescription)) {
				//Convert error description to single-width character
				sprintf_s(msg, 1024, "%s\n\n%S", msg, pErrorDescription);
				//Clean up
				SysFreeString(pErrorDescription);
			}
			else {
				strcat_s(msg, 1024, "Could not find the error description");
			}
			pErrorInfo->Release();
		}
		else {
			strcat_s(msg, 1024, "Could not retrieve error information");
		}
	}
	static void GetSQLCodes(char *msg, CDBErrorInfo *errInfo, ULONG errorNum = 0)
	{
		//COM Error Interface to retrieve error codes
		CComPtr<ISQLErrorInfo> spSQLErrorInfo;
		char SQLState[100];	//Buffer for error message
		if (SUCCEEDED(errInfo->GetCustomErrorObject(errorNum,
			IID_ISQLErrorInfo,
			(IUnknown **)&spSQLErrorInfo)) && spSQLErrorInfo) {
			BSTR bstrSQLState = NULL;	//SQLState that's returned
			LONG errorCode;		//SQL Code that's returned
			//Retrieve the error code and SQLState
			if (SUCCEEDED(spSQLErrorInfo->GetSQLInfo(&bstrSQLState, &errorCode))) {
				//Form an error message
				sprintf_s(SQLState,
					"\n\nSQLState is %S\nError code is %ld",
					bstrSQLState, errorCode);
				//Concatenate the error message to the existing message
				strcat_s(msg, 1024, SQLState);
				SysFreeString(bstrSQLState);	//Clean up
			}
			else {
				strcat_s(msg, 1024,
					"\n\nCould not get SQL info.");
			}
		}
		else {    //Something went wrong
			strcat_s(msg, 1024,
				"\n\nCould not get error or SQLState code.");
		}
	}
	static void DisplayHRRESULTMessage(HRESULT hr, char *strFunction = NULL)
	{
		if (FAILED(hr)) {
			//Allow 1 k for the error message
			char message[1024];
			strcpy_s(message, "");
			if (strFunction) {	//Check for function equal to null
				//Function was passed, so see what function it was
				strcat_s(message, "An Error occurred in the ");
				strcat_s(message, strFunction);
				strcat_s(message, " function \n\n");
			}
#ifdef DEBUG
			GetHRRESULTMessage(hr, message);
			::MessageBox(NULL, message, "Database HR Error", MB_OK);
#endif
		}
	}
	static void GetHRRESULTMessage(HRESULT hr, char *msg)
	{
		sprintf_s(msg, 1024, "%s\nHRESULT was 0x%X:", msg, hr);
		switch (hr) {
		case DB_E_ABORTLIMITREACHED:
			strcat_s(msg, 1024, "\nYour execution was aborted because a resource limit has been reached. No results are returned when this error occurs.");
			break;
		case DB_E_ALREADYINITIALIZED:
			strcat_s(msg, 1024, "\nYou tried to initialize a data source that has already been initialized.");
			break;
		case DB_E_BADACCESSORFLAGS:
			strcat_s(msg, 1024, "\nInvalid accessor flags");
			break;
		case DB_E_BADACCESSORHANDLE:
			strcat_s(msg, 1024, "\nInvalid accessor handle");
			break;
		case DB_E_BADACCESSORTYPE:
			strcat_s(msg, 1024, "\nThe specified accessor was not a parameter accessor");
			break;
		case DB_E_BADBINDINFO:
			strcat_s(msg, 1024, "\nInvalid binding information");
			break;
		case DB_E_BADBOOKMARK:
			strcat_s(msg, 1024, "\nInvalid bookmark");
			break;
		case DB_E_BADCHAPTER:
			strcat_s(msg, 1024, "\nInvalid chapter");
			break;
		case DB_E_BADCOLUMNID:
			strcat_s(msg, 1024, "\nInvalid column ID");
			break;
		case DB_E_BADCOMPAREOP:
			strcat_s(msg, 1024, "\nThe comparison operator was invalid");
			break;
		case DB_E_BADCONVERTFLAG:
			strcat_s(msg, 1024, "\nInvalid conversion flag");
			break;
		case DB_E_BADCOPY:
			strcat_s(msg, 1024, "\nErrors were detected during a copy");
			break;
		case DB_E_BADDYNAMICERRORID:
			strcat_s(msg, 1024, "\nThe supplied DynamicErrorID was invalid");
			break;
		case DB_E_BADHRESULT:
			strcat_s(msg, 1024, "\nThe supplied HRESULT was invalid");
			break;
			//		case DB_E_BADID :
			//			DB_E_BADID is deprecated. 
			//			Use DB_E_BADTABLEID instead.
			//			break;
		case DB_E_BADLOCKMODE:
			strcat_s(msg, 1024, "\nInvalid lock mode");
			break;
		case DB_E_BADLOOKUPID:
			strcat_s(msg, 1024, "\nInvalid LookupID");
			break;
		case DB_E_BADORDINAL:
			strcat_s(msg, 1024, "\nThe specified column number does not exist.");
			break;
		case DB_E_BADPARAMETERNAME:
			strcat_s(msg, 1024, "\nThe given parameter name is not recognized.");
			break;
		case DB_E_BADPRECISION:
			strcat_s(msg, 1024, "\nA specified precision is invalid");
			break;
		case DB_E_BADPROPERTYVALUE:
			strcat_s(msg, 1024, "\nThe value of a property is invalid");
			break;
		case DB_E_BADRATIO:
			strcat_s(msg, 1024, "\nInvalid ratio");
			break;
		case DB_E_BADRECORDNUM:
			strcat_s(msg, 1024, "\nThe specified record number is invalid");
			break;
		case DB_E_BADROWHANDLE:
			strcat_s(msg, 1024, "\nInvalid row handle.  This error often occurs when you are at BOF or EOF of a rowset and you try to update your data set.");
			break;
		case DB_E_BADSCALE:
			strcat_s(msg, 1024, "\nA specified scale was invalid");
			break;
		case DB_E_BADSOURCEHANDLE:
			strcat_s(msg, 1024, "\nInvalid source handle");
			break;
		case DB_E_BADSTARTPOSITION:
			strcat_s(msg, 1024, "\nThe rows offset specified would position you before the beginning or past the end of the rowset.");
			break;
		case DB_E_BADSTATUSVALUE:
			strcat_s(msg, 1024, "\nThe specified status flag was neither DBCOLUMNSTATUS_OK nor DBCOLUMNSTATUS_ISNULL");
			break;
		case DB_E_BADSTORAGEFLAG:
			strcat_s(msg, 1024, "\nOne of the specified storage flags was not supported");
			break;
		case DB_E_BADSTORAGEFLAGS:
			strcat_s(msg, 1024, "\nInvalid storage flags");
			break;
		case DB_E_BADTABLEID:
			strcat_s(msg, 1024, "\nInvalid table ID");
			break;
		case DB_E_BADTYPE:
			strcat_s(msg, 1024, "\nA specified type was invalid");
			break;
		case DB_E_BADTYPENAME:
			strcat_s(msg, 1024, "\nThe given type name was unrecognized");
			break;
		case DB_E_BADVALUES:
			strcat_s(msg, 1024, "\nInvalid value");
			break;
		case DB_E_BOOKMARKSKIPPED:
			strcat_s(msg, 1024, "\nAlthough the bookmark was validly formed, no row could be found to match it");
			break;
		case DB_E_BYREFACCESSORNOTSUPPORTED:
			strcat_s(msg, 1024, "\nBy reference accessors are not supported by this provider");
			break;
		case DB_E_CANCELED:
			strcat_s(msg, 1024, "\nThe change was canceled during notification; no columns are changed");
			break;
		case DB_E_CANNOTRESTART:
			strcat_s(msg, 1024, "\nThe rowset was built over a live data feed and cannot be restarted.");
			break;
		case DB_E_CANTCANCEL:
			strcat_s(msg, 1024, "\nThe executing command cannot be canceled.");
			break;
		case DB_E_CANTCONVERTVALUE:
			strcat_s(msg, 1024, "\nA literal value in the command could not be converted to the correct type due to a reason other than data overflow.");
			break;
		case DB_E_CANTFETCHBACKWARDS:
			strcat_s(msg, 1024, "\nThe rowset does not support backward scrolling.");
			break;
		case DB_E_CANTFILTER:
			strcat_s(msg, 1024, "\nThe requested filter could not be opened.");
			break;
		case DB_E_CANTORDER:
			strcat_s(msg, 1024, "\nThe requested order could not be opened.");
			break;
		case DB_E_CANTSCROLLBACKWARDS:
			strcat_s(msg, 1024, "\nThe rowset cannot scroll backwards.");
			break;
		case DB_E_CANTTRANSLATE:
			strcat_s(msg, 1024, "\nCannot represent the current tree as text.");
			break;
		case DB_E_CHAPTERNOTRELEASED:
			strcat_s(msg, 1024, "\nThe rowset was single-chaptered and the chapter was not released when a new chapter formation is attempted.");
			break;
		case DB_E_CONCURRENCYVIOLATION:
			strcat_s(msg, 1024, "\nThe rowset was using optimistic concurrency and the value of a column has been changed since it was last read.");
			break;
		case DB_E_DATAOVERFLOW:
			strcat_s(msg, 1024, "\nA literal value in the command overflowed the range of the type of the associated column");
			break;
		case DB_E_DELETEDROW:
			strcat_s(msg, 1024, "\nThe row that is referred to has been deleted.");
			break;
		case DB_E_DIALECTNOTSUPPORTED:
			strcat_s(msg, 1024, "\nThe provider does not support the specified dialect");
			break;
		case DB_E_DUPLICATECOLUMNID:
			strcat_s(msg, 1024, "\nA column ID was occurred more than once in the specification");
			break;
		case DB_E_DUPLICATEDATASOURCE:
			strcat_s(msg, 1024, "\nA new data source is trying to be formed when a data source with that specified name already exists.");
			break;
		case DB_E_DUPLICATEINDEXID:
			strcat_s(msg, 1024, "\nThe specified index already exists.");
			break;
		case DB_E_DUPLICATETABLEID:
			strcat_s(msg, 1024, "\nThe specified table already exists.");
			break;
		case DB_E_ERRORSINCOMMAND:
			strcat_s(msg, 1024, "\nThe command contained one or more errors.");
			break;
		case DB_E_ERRORSOCCURRED:
			strcat_s(msg, 1024, "\nErrors occurred.  This message is thrown when an error occurs that is not captured by one of the other error messages.");
			break;
		case DB_E_INDEXINUSE:
			strcat_s(msg, 1024, "\nThe specified index was in use.");
			break;
		case DB_E_INTEGRITYVIOLATION:
			strcat_s(msg, 1024, "\nA specified value violated the referential integrity constraints for a column or table.");
			break;
		case DB_E_INVALID:
			strcat_s(msg, 1024, "\nThe rowset is invalide.");
			break;
		case DB_E_MAXPENDCHANGESEXCEEDED:
			strcat_s(msg, 1024, "\nThe number of rows with pending changes has exceeded the set limit.");
			break;
		case DB_E_MULTIPLESTATEMENTS:
			strcat_s(msg, 1024, "\nThe provider does not support multi-statement commands.");
			break;
		case DB_E_MULTIPLESTORAGE:
			strcat_s(msg, 1024, "\nMultiple storage objects can not be open simultaneously.");
			break;
		case DB_E_NEWLYINSERTED:
			strcat_s(msg, 1024, "\nThe provider is unable to determine identity for newly inserted rows.");
			break;
		case DB_E_NOAGGREGATION:
			strcat_s(msg, 1024, "\nA non-NULL controlling IUnknown was specified and the object being created does not support aggregation.");
			break;
		case DB_E_NOCOMMAND:
			strcat_s(msg, 1024, "\nNo command has been set for the command object.");
			break;
		case DB_E_NOINDEX:
			strcat_s(msg, 1024, "\nThe specified index does not exist.");
			break;
		case DB_E_NOLOCALE:
			strcat_s(msg, 1024, "\nThe specified locale ID was not supported.");
			break;
		case DB_E_NOQUERY:
			strcat_s(msg, 1024, "\nInformation was requested for a query, and the query was not set.");
			break;
		case DB_E_NOTABLE:
			strcat_s(msg, 1024, "\nThe specified table does not exist.");
			break;
		case DB_E_NOTAREFERENCECOLUMN:
			strcat_s(msg, 1024, "\nSpecified column does not contain bookmarks or chapters.");
			break;
		case DB_E_NOTFOUND:
			strcat_s(msg, 1024, "\nNo key matching the described characteristics could be found within the current range.");
			break;
		case DB_E_NOTPREPARED:
			strcat_s(msg, 1024, "\nThe command was not prepared.");
			break;
		case DB_E_NOTREENTRANT:
			strcat_s(msg, 1024, "\nProvider called a method from IRowsetNotify in the consumer and the method has not yet returned.");
			break;
		case DB_E_NOTSUPPORTED:
			strcat_s(msg, 1024, "\nThe provider does not support this method.");
			break;
		case DB_E_NULLACCESSORNOTSUPPORTED:
			strcat_s(msg, 1024, "\nNull accessors are not supported by this provider.");
			break;
		case DB_E_OBJECTOPEN:
			strcat_s(msg, 1024, "\nAn object was open.");
			break;
		case DB_E_PARAMNOTOPTIONAL:
			strcat_s(msg, 1024, "\nNo value given for one or more required parameters.");
			break;
		case DB_E_PARAMUNAVAILABLE:
			strcat_s(msg, 1024, "\nThe provider cannot derive parameter info and SetParameterInfo has not been called.");
			break;
		case DB_E_PENDINGCHANGES:
			strcat_s(msg, 1024, "\nThere are pending changes on a row with a reference count of zero.");
			break;
		case DB_E_PENDINGINSERT:
			strcat_s(msg, 1024, "\nUnable to get visible data for a newly-inserted row that has not yet been updated.");
			break;
		case DB_E_READONLYACCESSOR:
			strcat_s(msg, 1024, "\nUnable to write with a read-only accessor.");
			break;
		case DB_E_ROWLIMITEXCEEDED:
			strcat_s(msg, 1024, "\nCreating another row would have exceeded the total number of active rows supported by the rowset.");
			break;
		case DB_E_ROWSETINCOMMAND:
			strcat_s(msg, 1024, "\nCannot clone a command object whose command tree contains a rowset or rowsets.");
			break;
		case DB_E_ROWSNOTRELEASED:
			strcat_s(msg, 1024, "\nAll HROWs must be released before new ones can be obtained.");
			break;
		case DB_E_SCHEMAVIOLATION:
			strcat_s(msg, 1024, "\nGiven values violate the database schema.");
			break;
		case DB_E_TABLEINUSE:
			strcat_s(msg, 1024, "\nThe specified table was in use.");
			break;
		case DB_E_UNSUPPORTEDCONVERSION:
			strcat_s(msg, 1024, "\nRequested conversion is not supported.");
			break;
		case DB_E_WRITEONLYACCESSOR:
			strcat_s(msg, 1024, "\nThe given accessor was write-only.");
			break;
		case DB_S_ASYNCHRONOUS:
			strcat_s(msg, 1024, "\nThe operation is being processed asynchronously.");
			break;
		case DB_S_BADROWHANDLE:
			strcat_s(msg, 1024, "\nInvalid row handle. This error often occurs when you are at BOF or EOF of a rowset and you try to update your data set.");
			break;
		case DB_S_BOOKMARKSKIPPED:
			strcat_s(msg, 1024, "\nSkipped bookmark for deleted or non-member row.");
			break;
		case DB_S_BUFFERFULL:
			strcat_s(msg, 1024, "\nVariable data buffer full.  Increase system memory, commit open transactions, free more system memory, or declare larger buffers in you database setup.");
			break;
		case DB_S_CANTRELEASE:
			strcat_s(msg, 1024, "\nServer cannot release or \
												downgrade a lock until the end of the transaction.");
			break;
		case DB_S_COLUMNSCHANGED:
			strcat_s(msg, 1024, "\nIn order to reposition to the start of the rowset, the provider had to reexecute the query; either the order of the columns changed or columns were added to or removed from the rowset.");
			break;
		case DB_S_COLUMNTYPEMISMATCH:
			strcat_s(msg, 1024, "\nOne or more column types are incompatible. Conversion errors will occur during copying.");
			break;
		case DB_S_COMMANDREEXECUTED:
			strcat_s(msg, 1024, "\nThe provider re-executed the command.");
			break;
		case DB_S_DELETEDROW:
			strcat_s(msg, 1024, "\nA given HROW referred to a hard-deleted row.");
			break;
		case DB_S_DIALECTIGNORED:
			strcat_s(msg, 1024, "\nInput dialect was ignored and text was returned in different dialect.");
			break;
		case DB_S_ENDOFROWSET:
			strcat_s(msg, 1024, "\nReached start or end of rowset or chapter.");
			break;
		case DB_S_ERRORSOCCURRED:
			strcat_s(msg, 1024, "\nErrors occurred. DB_S_ERRORSOCCURRED flag was set.");
			break;
		case DB_S_ERRORSRETURNED:
			strcat_s(msg, 1024, "\nThe method had some errors; errors have been returned in the error array.");
			break;
		case DB_S_LOCKUPGRADED:
			strcat_s(msg, 1024, "\nA lock was upgraded from the value specified.");
			break;
		case DB_S_MULTIPLECHANGES:
			strcat_s(msg, 1024, "\nUpdating this row caused more than one row to be updated in the data source.");
			break;
		case DB_S_NONEXTROWSET:
			strcat_s(msg, 1024, "\nThere are no more rowsets.");
			break;
		case DB_S_NORESULT:
			strcat_s(msg, 1024, "\nThere are no more results.");
			break;
		case DB_S_PARAMUNAVAILABLE:
			strcat_s(msg, 1024, "\nA specified parameter was invalid.");
			break;
		case DB_S_PROPERTIESCHANGED:
			strcat_s(msg, 1024, "\nOne or more properties were changed as allowed by provider.");
			break;
		case DB_S_ROWLIMITEXCEEDED:
			strcat_s(msg, 1024, "\nFetching requested number of rows would have exceeded total number of active rows supported by the rowset.");
			break;
		case DB_S_STOPLIMITREACHED:
			strcat_s(msg, 1024, "\nExecution stopped because a resource limit has been reached. Results obtained so far have been returned but execution cannot be resumed.");
			break;
		case DB_S_TYPEINFOOVERRIDDEN:
			strcat_s(msg, 1024, "\nCaller has overridden parameter type information.");
			break;
		case DB_S_UNWANTEDOPERATION:
			strcat_s(msg, 1024, "\nConsumer is uninterested in receiving further notification calls for this reason");
			break;
		case DB_S_UNWANTEDPHASE:
			strcat_s(msg, 1024, "\nConsumer is uninterested in receiving further notification calls for this phase");
			break;
		case DB_S_UNWANTEDREASON:
			strcat_s(msg, 1024, "\nConsumer is uninterested in receiving further notification calls for this reason.");
			break;
		case DB_SEC_E_AUTH_FAILED:
			strcat_s(msg, 1024, "\nAuthentication failed");
			break;
		case DB_SEC_E_PERMISSIONDENIED:
			strcat_s(msg, 1024, "\nPermission denied");
			break;
		case MD_E_BADCOORDINATE:
			strcat_s(msg, 1024, "\nBad coordinate for the OLAP dataset.");
			break;
		case MD_E_BADTUPLE:
			strcat_s(msg, 1024, "\nBad tuple for the OLAP dataset");
			break;
		case MD_E_INVALIDAXIS:
			strcat_s(msg, 1024, "\nThe given axis was not valid for this OLAP dataset.");
			break;
		case MD_E_INVALIDCELLRANGE:
			strcat_s(msg, 1024, "\nOne or more of the given cell ordinals was invalid for this OLAP dataset.");
			break;
#if(OLEDBVER >= 0x0250)	
			//Errors if OLE DB version is greater than 2.5
		case DB_E_BADREGIONHANDLE:
			strcat_s(msg, 1024, "\nInvalid region handle");
			break;
		case DB_E_CANNOTFREE:
			strcat_s(msg, 1024, "\nOwnership of this tree has been given to the provider.  You cannot free the tree.");
			break;
		case DB_E_COSTLIMIT:
			strcat_s(msg, 1024, "\nUnable to find a query plan within the given cost limit");
			break;
		case DB_E_GOALREJECTED:
			strcat_s(msg, 1024, "\nNo nonzero weights specified for any goals supported, so goal was rejected; current goal was not changed.");
			break;
		case DB_E_INVALIDTRANSITION:
			strcat_s(msg, 1024, "\nA transition from ALL* to MOVE* or EXTEND* was specified.");
			break;
		case DB_E_LIMITREJECTED:
			strcat_s(msg, 1024, "\nSome cost limits were rejected.");
			break;
		case DB_E_NONCONTIGUOUSRANGE:
			strcat_s(msg, 1024, "\nThe specified set of rows was not contiguous to or overlapping the rows in the specified watch region.");
			break;
			//		case DB_S_ERRORSINTREE :
			//			strcat_s(msg,1024, "\nErrors found in validating tree.");
			//			break;
		case DB_S_GOALCHANGED:
			strcat_s(msg, 1024, "\nSpecified weight was not supported or exceeded the supported limit and was set to 0 or the supported limit.");
			break;
		case DB_S_TOOMANYCHANGES:
			strcat_s(msg, 1024, "\nThe provider was unable to keep track of all the changes. You must refetch the data associated with the watch region using another method.");
			break;
#endif	//OLEDBVER >= 0x0250
			// BLOB ISequentialStream errors
		case STG_E_INVALIDFUNCTION:
			strcat_s(msg, 1024, "\nYou tried an invalid function.");
			break;
		case S_FALSE:
			strcat_s(msg, 1024, "\nS_FALSE was returned.  The ISequentialStream data could not be read from the stream object.");
			break;
		case E_PENDING:
			strcat_s(msg, 1024, "\nAsynchronous Storage only: Part or all of the data to be written is currently unavailable. For more information, see IFillLockBytes and Asynchronous Storage in MSDN.");
			break;
		case STG_E_MEDIUMFULL:
			strcat_s(msg, 1024, "\nThe write operation was not completed because there is no space left on the storage device.");
			break;
		case STG_E_ACCESSDENIED:
			strcat_s(msg, 1024, "\nThe caller does not have sufficient permissions for writing this stream object.");
			break;
		case STG_E_CANTSAVE:
			strcat_s(msg, 1024, "\nData cannot be written for reasons other than lack of access or space.");
			break;
		case STG_E_INVALIDPOINTER:
			strcat_s(msg, 1024, "\nOne of the pointer values is invalid.");
			break;
		case STG_E_REVERTED:
			strcat_s(msg, 1024, "\nThe object has been invalidated by a revert operation above it in the transaction tree.");
			break;
		case STG_E_WRITEFAULT:
			strcat_s(msg, 1024, "\nThe write operation was not completed due to a disk error.");
			break;
			// Unknown error
		default:
			strcat_s(msg, 1024, "\nHRESULT returned an unknown error.");
			break;
		}
	}
};
#endif // !defined(OLEDB_Error_Routine_Included)

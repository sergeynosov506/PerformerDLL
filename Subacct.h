/*H*
* 
* 
* FILENAME: subacct.h
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
*
*H*/

#ifndef NT
	#define NT 1
#endif

#define NUMSUBACCT				 15	// maximum number of records in subaccount table

typedef struct
{
	char sAcctType[1+NT];
  char sDescription[30+NT];
  char sXrefAcctType[1+NT];
  char sSellAcctType[1+NT];
} SUBACCT;


typedef struct 
{
  char sAcctType[1+NT];
	char sXrefAcctType[1+NT];
} PARTSUBACCT;

typedef struct 
{
	int         iNumSAcct;
	PARTSUBACCT zSAcct[NUMSUBACCT];
} SUBACCTTABLE;  

typedef struct 
{
	int         iNumSAcct;
	SUBACCT zSAcct[NUMSUBACCT];
} SUBACCTTABLE1;  
/*H*
* 
* FILENAME: hedgexref.h
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
*H*/
#ifndef HEDGEXREF_H
#define HEDGEXREF_H

#ifndef NT
  #define NT 1
#endif

typedef struct
{
	int		 iID;
  char	 sSecNo[12+NT];
  char	 sWi[1+NT];
  char	 sSecXtend[2+NT];
  char	 sAcctType[1+NT];
  long	 lTransNo;
	int		 iSecID;
  long	 lAsofDate;
  char	 sSecNo2[12+NT];
  char	 sWi2[1+NT];
  char	 sSecXtend2[2+NT];
  char	 sAcctType2[1+NT];
  long	 lTransNo2;
	int		 iSecID2;
  double fHedgeUnits;
  double fHedgeValBase;
  double fHedgeValNative;
  double fHedgeValSystem;
  double fHedgeUnits2;
  char	 sHedgeType[2+NT];
  char	 sValuationSrce[2+NT];
} HEDGEXREF;

typedef struct
{
	int			iID;
	char		sSecNo[12+NT];
	char		sWi[1+NT];
	char		sSecXtend[2+NT];
	char		sAcctType[1+NT];
	long		lTransNo;
	double	fHedgeValNative;
	double	fHedgeValSystem;
} PARTHXREF;

typedef struct
{
	int				iPhxrefCreated;
	int				iNumPhxref;
	PARTHXREF	*pzPhxref;
} PHXREFTABLE;


typedef void	(CALLBACK* LPPRHEDGEXREF)(HEDGEXREF, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRSELECTHEDGEXREF)(HEDGEXREF *, int, char *, char *, char *, char *, long, ERRSTRUCT *);
typedef void	(CALLBACK* LPPRALLHXREFFORANACCOUNT)(int, HEDGEXREF *, ERRSTRUCT *);

#endif // !hedgexref_H
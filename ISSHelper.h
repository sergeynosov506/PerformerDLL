/**
* 
* SUB-SYSTEM: Database Input/Output for Performance   
* 
* FILENAME: PerformanceIO.h
* 
* DESCRIPTION:	Defines class to help read/write  
*				blob fields 
*
* 
* PUBLIC FUNCTIONS(S): 
* 
* NOTES:  
*        
* USAGE:	Part of OLEDB.DLL (Performance IO) project. 
*
* AUTHOR:	(C) Microsoft MSDN, partially revised by
*			Valeriy Yegorov. (C) 2001 Effron Enterprises, Inc. 
*
*
**/

// ISSHelper.h: interface for the CISSHelper class.
//#include <atldbcli.h>
#include <msoledbsql.h>

class CISSHelper : public ISequentialStream  
{
public:

	// Constructor/destructor.
	CISSHelper();
	virtual ~CISSHelper();

	// Helper function to clean up memory.
	virtual void Clear();

	// ISequentialStream interface implementation.
	STDMETHODIMP_(ULONG)	AddRef(void);
	STDMETHODIMP_(ULONG)	Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);
    STDMETHODIMP Read( 
            /* [out] */ void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbRead);
    STDMETHODIMP Write( 
            /* [in] */ const void __RPC_FAR *pv,
            /* [in] */ ULONG cb,
            /* [out] */ ULONG __RPC_FAR *pcbWritten);

public:

	void*       m_pBuffer;		// Buffer
	ULONG       m_ulLength;     // Total buffer size.
	ULONG       m_ulStatus;     // Column status.

private:

	ULONG		m_cRef;			// Reference count (not used).
	ULONG       m_iReadPos;     // Current index position for reading from the buffer.
	ULONG       m_iWritePos;    // Current index position for writing to the buffer.

};

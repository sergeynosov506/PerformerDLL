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

// ISSHelper.cpp: implementation of the CISSHelper class.
//
//////////////////////////////////////////////////////////////////////

#include "ISSHelper.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Class CISSHelper
// 
// Implementation of ISequentialStream interface
///////////////////////////////////////////////////////////////////////////////

CISSHelper::CISSHelper()
{
	m_cRef		= 0;
	m_pBuffer	= NULL;
	m_ulLength	= 0;
	m_ulStatus  = DBSTATUS_S_OK;
	m_iReadPos	= 0;
	m_iWritePos	= 0;
}

CISSHelper::~CISSHelper()
{
	Clear();
}

void CISSHelper::Clear() 
{
	CoTaskMemFree( m_pBuffer );
	m_cRef		= 0;
	m_pBuffer	= NULL;
	m_ulLength	= 0;
	m_ulStatus  = DBSTATUS_S_OK;
	m_iReadPos	= 0;
	m_iWritePos	= 0;
}

ULONG CISSHelper::AddRef(void)
{
	return ++m_cRef;
}

ULONG CISSHelper::Release(void)
{
	return --m_cRef;
}

HRESULT CISSHelper::QueryInterface( REFIID riid, void** ppv )
{
	*ppv = NULL;
	if ( riid == IID_IUnknown )			 *ppv = this;
	if ( riid == IID_ISequentialStream ) *ppv = this;
	if ( *ppv )
	{
		( (IUnknown*) *ppv )->AddRef();
		return S_OK;
	}
	return E_NOINTERFACE;
}

HRESULT CISSHelper::Read( void *pv,	ULONG cb, ULONG* pcbRead )
{
	// Check parameters.
	if ( pcbRead ) *pcbRead = 0;
	if ( !pv ) return STG_E_INVALIDPOINTER;
	if ( 0 == cb ) return S_OK; 

	// Calculate bytes left and bytes to read.
	ULONG cBytesLeft = m_ulLength - m_iReadPos;
	ULONG cBytesRead = cb > cBytesLeft ? cBytesLeft : cb;

	// If no more bytes to retrive return S_FALSE.
	if ( 0 == cBytesLeft ) return S_FALSE;

	// Copy to users buffer the number of bytes requested or remaining
	memcpy( pv, (void*)((BYTE*)m_pBuffer + m_iReadPos), cBytesRead );
	m_iReadPos += cBytesRead;

	// Return bytes read to caller.
	if ( pcbRead ) *pcbRead = cBytesRead;
	if ( cb != cBytesRead ) return S_FALSE; 

	return S_OK;
}
        
HRESULT CISSHelper::Write( const void *pv, ULONG cb, ULONG* pcbWritten )
{
	// Check parameters.
	if ( !pv ) return STG_E_INVALIDPOINTER;
	if ( pcbWritten ) *pcbWritten = 0;
	if ( 0 == cb ) return S_OK;

	// Enlarge the current buffer.
	m_ulLength += cb;

	// Grow internal buffer to new size.
	m_pBuffer = CoTaskMemRealloc( m_pBuffer, m_ulLength );

	// Check for out of memory situation.
	if ( NULL == m_pBuffer ) 
	{
		Clear();
		return E_OUTOFMEMORY;
	}

	// Copy callers memory to internal bufffer and update write position.
	memcpy( (void*)((BYTE*)m_pBuffer + m_iWritePos), pv, cb );
	m_iWritePos += cb;

	// Return bytes written to caller.
	if ( pcbWritten ) *pcbWritten = cb;

	return S_OK;
}

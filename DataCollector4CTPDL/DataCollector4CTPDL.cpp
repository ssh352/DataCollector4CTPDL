#include "targetver.h"
#include <exception>
#include <algorithm>
#include <functional>
#include "Configuration.h"
#include "UnitTest/UnitTest.h"
#include "DataCollector4CTPDL.h"


QuoCollector::QuoCollector()
 : m_pCbDataHandle( NULL )
{
}

QuoCollector& QuoCollector::GetCollector()
{
	static	QuoCollector	obj;

	return obj;
}

int QuoCollector::Initialize( I_DataHandle* pIDataHandle )
{
	int		nErrorCode = 0;

	m_pCbDataHandle = pIDataHandle;
	if( NULL == m_pCbDataHandle )
	{
		::printf( "QuoCollector::Initialize() : invalid arguments (NULL)\n" );
		return -1;
	}

	if( 0 != (nErrorCode = Configuration::GetConfig().Initialize()) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuoCollector::Initialize() : failed 2 initialize configuration obj, errorcode=%d", nErrorCode );
		return -2;
	}

	if( 0 != (nErrorCode = SimpleTask::Activate()) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "QuoCollector::Initialize() : failed 2 initialize task thread, errorcode=%d", nErrorCode );
		return -3;
	}

	return 0;
}

void QuoCollector::Release()
{
	SimpleTask::StopThread();
	SimpleTask::Join( 1000*3 );
//	m_pCbDataHandle = NULL;
}

I_DataHandle* QuoCollector::operator->()
{
	if( NULL == m_pCbDataHandle )
	{
		::printf( "QuoCollector::operator->() : invalid data callback interface ptr.(NULL)\n" );
	}

	return m_pCbDataHandle;
}

int QuoCollector::GetSubscribeCodeList( char (&pszCodeList)[1024*5][20], unsigned int nListSize )
{
	return m_oImageData.GetSubscribeCodeList( pszCodeList, nListSize );
}

int QuoCollector::Execute()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuoCollector::Execute() : enter into thread func ......" );
/*
	int			nErrorCode = 0;

	while( true == IsAlive() )
	{
		try
		{
			SimpleTask::Sleep( 1000 );
		}
		catch( std::exception& err )
		{
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuoCollector::Execute() : exception : %s", err.what() );
		}
		catch( ... )
		{
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuoCollector::Execute() : unknow exception" );
		}
	}
*/
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "QuoCollector::Execute() : exit thread func ......" );

	return 0;
}

void QuoCollector::Halt()
{
	m_oQuotationData.Destroy();
}

int QuoCollector::RecoverQuotation()
{
	unsigned int	nSec = 0;
	int				nErrorCode = 0;

	if( (nErrorCode=m_oImageData.FreshCache()) <= 0 )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "QuoCollector::RecoverQuotation() : failed 2 refresh image data, errorcode=%d", nErrorCode );
		return -1;
	}

	if( 0 != (nErrorCode=m_oQuotationData.Activate()) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "QuoCollector::RecoverQuotation() : failed 2 subscript quotation, errorcode=%d", nErrorCode );
		return -2;
	}

	for( nSec = 0; nSec < 60 && ET_SS_WORKING != m_oQuotationData.GetWorkStatus(); nSec++ )
	{
		SimpleTask::Sleep( 1000 * 1 );
	}

	if( m_oQuotationData.GetCodeCount() > 0 )
	{
		m_oQuotationData.GetWorkStatus() = ET_SS_WORKING;
	}

	if( ET_SS_WORKING == m_oQuotationData.GetWorkStatus() )
	{
		return 0;
	}
	else
	{
		m_oQuotationData.Destroy();
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "QuoCollector::RecoverQuotation() : overtime [> %d sec.], errorcode=%d", nSec, nErrorCode );
		return -3;
	}
}

enum E_SS_Status QuoCollector::GetCollectorStatus( char* pszStatusDesc, unsigned int& nStrLen )
{
	Configuration&		refCnf = Configuration::GetConfig();
	CTPWorkStatus&		refStatus = m_oQuotationData.GetWorkStatus();
	std::string&		sStatus = CTPWorkStatus::CastStatusStr( (enum E_SS_Status)refStatus );

	nStrLen = ::sprintf( pszStatusDesc, "市场编号=%u,快照路径=%s,连接状态=%s"
		, refCnf.GetMarketID(), refCnf.GetDumpFolder().c_str(), sStatus.c_str() );

	return (enum E_SS_Status)(m_oQuotationData.GetWorkStatus());
}


extern "C"
{
	__declspec(dllexport) int __stdcall	Initialize( I_DataHandle* pIDataHandle )
	{
		return QuoCollector::GetCollector().Initialize( pIDataHandle );
	}

	__declspec(dllexport) void __stdcall Release()
	{
		QuoCollector::GetCollector().Release();
	}

	__declspec(dllexport) int __stdcall	RecoverQuotation()
	{
		return QuoCollector::GetCollector().RecoverQuotation();
	}

	__declspec(dllexport) void __stdcall HaltQuotation()
	{
		QuoCollector::GetCollector().Halt();
	}

	__declspec(dllexport) int __stdcall	GetStatus( char* pszStatusDesc, unsigned int& nStrLen )
	{
		return QuoCollector::GetCollector().GetCollectorStatus( pszStatusDesc, nStrLen );
	}

	__declspec(dllexport) bool __stdcall IsProxy()
	{
		return false;
	}

	__declspec(dllexport) int __stdcall	GetMarketID()
	{
		return Configuration::GetConfig().GetMarketID();
	}

	__declspec(dllexport) void __stdcall	ExecuteUnitTest()
	{
		::printf( "\n\n---------------------- [Begin] -------------------------\n" );
		ExecuteTestCase();
		::printf( "----------------------  [End]  -------------------------\n\n\n" );
	}

}





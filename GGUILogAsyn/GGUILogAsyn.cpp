//-----------------------------------------------------------------------------
// (C) oil
// 2013-04-17
//
//
// 工程选项中用户可以设置使用ANSI字符集还是Unicode字符集。如果是Unicode字符集，
// 输出log过程中会把Unicode字符串转换为ANSI字符串。
//-----------------------------------------------------------------------------
#include <Windows.h>
#include <tchar.h>
#include <process.h>
#include "GGUILogAsyn.h"
#include "GGUIStringHelp.h"
//-----------------------------------------------------------------------------
namespace GGUI
{
	GGUILogAsyn* GGUILogAsyn::ms_pInstance = SoNULL;
	GGUILogAsyn::stSubThreadInfo GGUILogAsyn::m_SubThreadInfo;
	//-----------------------------------------------------------------------------
	GGUILogAsyn::GGUILogAsyn()
	:m_hSubThread(SoNULL)
	,m_uiSubThreadID(0)
	,m_theLogLevel(LogLevel_Debug)
	,m_bOutputDebugString(true)
	{
		ms_pInstance = this;
	}
	//-----------------------------------------------------------------------------
	GGUILogAsyn::~GGUILogAsyn()
	{
		ReleaseLog();
		ms_pInstance = SoNULL;
	}
	//-----------------------------------------------------------------------------
	GGUILogAsyn* GGUILogAsyn::GetInstance()
	{
		return ms_pInstance;
	}
	//-----------------------------------------------------------------------------
	bool GGUILogAsyn::InitLog(const stLogParam& theLogParam)
	{
		//检验传入的参数。
		if (theLogParam.pszLogFileName == NULL)
		{
			return false;
		}
		size_t theFileNameLength = 0; //这个值不包括结束符。
		HRESULT hResult = StringCbLength(theLogParam.pszLogFileName, MaxLength_LogFileName*sizeof(tchar), &theFileNameLength);
		if (FAILED(hResult))
		{
			//文件名的字符个数（包括结束符）超过了MaxLength_LogFileName。
			//::MessageBox(NULL, TEXT("theLogParam.pszLogFileName"), TEXT("GGUILogAsyn::InitLog"), MB_OK);
			return false;
		}
		//
		m_hSubThread = SoNULL;
		m_uiSubThreadID = 0;
		m_theLogLevel = theLogParam.theLogLevel;
		m_bOutputDebugString = theLogParam.bOutputDebugString;
		//
		m_SubThreadInfo.m_szFileName[0] = 0;
		const char* pFileName_Ansi = "";
		if (sizeof(tchar) == 1)
		{
			pFileName_Ansi = (const char*)theLogParam.pszLogFileName;
		}
		else
		{
			pFileName_Ansi = UnicodeToAnsi((const wchar_t*)theLogParam.pszLogFileName);
		}
		StringCbCopyA(m_SubThreadInfo.m_szFileName, sizeof(m_SubThreadInfo.m_szFileName), pFileName_Ansi);
		m_SubThreadInfo.m_fp = SoNULL;
		m_SubThreadInfo.m_pBufferForReceive = new char[theLogParam.nDataBufferSize];
		m_SubThreadInfo.m_pBufferForReceive[0] = 0;
		m_SubThreadInfo.m_nBufferForReceive_MaxSize = theLogParam.nDataBufferSize;
		m_SubThreadInfo.m_nBufferForReceive_CurrentSize = 0;
		m_SubThreadInfo.m_pBufferForWrite = new char[theLogParam.nDataBufferSize];
		m_SubThreadInfo.m_pBufferForWrite[0] = 0;
		m_SubThreadInfo.m_nBufferForWrite_MaxSize = theLogParam.nDataBufferSize;
		m_SubThreadInfo.m_nBufferForWrite_CurrentSize = 0;
		m_SubThreadInfo.m_nMaxFileSize = theLogParam.nMaxFileSize;
		m_SubThreadInfo.m_nCurrentFileSize = 0;
		m_SubThreadInfo.m_nFileCount = 0;
		m_SubThreadInfo.m_nSleepTime = theLogParam.nSleepTime;
		m_SubThreadInfo.m_theSubThreadState = SubThread_Running;
		InitializeCriticalSection(&m_SubThreadInfo.m_Lock);
		//创建线程。
		m_hSubThread = (HANDLE)_beginthreadex(NULL, 0, GGUILogAsyn::ThreadProcess,
			this, CREATE_SUSPENDED, &m_uiSubThreadID);
		if (m_hSubThread == NULL)
		{
			//::MessageBox(NULL, TEXT("_beginthreadex fail"), TEXT("GGUILogAsyn::InitLog"), MB_OK);
			return false;
		}
		ResumeThread(m_hSubThread);
		//
		return true;
	}
	//-----------------------------------------------------------------------------
	void GGUILogAsyn::ReleaseLog()
	{
		//结束子线程。
		if (m_SubThreadInfo.m_theSubThreadState == SubThread_Running)
		{
			m_SubThreadInfo.m_theSubThreadState = SubThread_Finishing;
			WaitForSingleObject(m_hSubThread, INFINITE);
		}
		if (m_hSubThread)
		{
			CloseHandle(m_hSubThread);
		}
		//
		m_SubThreadInfo.m_szFileName[0] = 0;
		m_SubThreadInfo.m_fp = SoNULL;
		if (m_SubThreadInfo.m_pBufferForReceive)
		{
			delete [] m_SubThreadInfo.m_pBufferForReceive;
			m_SubThreadInfo.m_pBufferForReceive = SoNULL;
		}
		m_SubThreadInfo.m_nBufferForReceive_MaxSize = 0;
		m_SubThreadInfo.m_nBufferForReceive_CurrentSize = 0;
		if (m_SubThreadInfo.m_pBufferForWrite)
		{
			delete [] m_SubThreadInfo.m_pBufferForWrite;
			m_SubThreadInfo.m_pBufferForWrite = SoNULL;
		}
		m_SubThreadInfo.m_nBufferForWrite_MaxSize = 0;
		m_SubThreadInfo.m_nBufferForWrite_CurrentSize = 0;
		m_SubThreadInfo.m_nMaxFileSize = 0;
		m_SubThreadInfo.m_nCurrentFileSize = 0;
		m_SubThreadInfo.m_nFileCount = 0;
		m_SubThreadInfo.m_nSleepTime = 0;
		m_SubThreadInfo.m_theSubThreadState = SubThread_Running;
		DeleteCriticalSection(&m_SubThreadInfo.m_Lock);
		//
		m_hSubThread = SoNULL;
		m_uiSubThreadID = 0;
		m_theLogLevel = LogLevel_Debug;
		m_bOutputDebugString = true;
	}
	//-----------------------------------------------------------------------------
	void GGUILogAsyn::OutputDebug(const tchar* pFormat, ...)
	{
		if (m_theLogLevel < LogLevel_Debug)
		{
			return;
		}
		if (m_SubThreadInfo.m_theSubThreadState != SubThread_Running)
		{
			return;
		}
		//主线程对公共数据锁定的时间比分线程锁定的时间要长。
		//这样做是对的，要保证主线程流程的运行，分线程可以等待，主线程不能等待。
		EnterCriticalSection(&m_SubThreadInfo.m_Lock);
		AddLogHead("DEBUG");
		//
		va_list marker;
		va_start(marker, pFormat);
		AddLogBody(pFormat, marker);
		va_end(marker);
		LeaveCriticalSection(&m_SubThreadInfo.m_Lock);
	}
	//-----------------------------------------------------------------------------
	void GGUILogAsyn::OutputWaring(const tchar* pFormat, ...)
	{
		if (m_theLogLevel < LogLevel_Waring)
		{
			return;
		}
		if (m_SubThreadInfo.m_theSubThreadState != SubThread_Running)
		{
			return;
		}
		EnterCriticalSection(&m_SubThreadInfo.m_Lock);
		AddLogHead("WARING");
		//
		va_list marker;
		va_start(marker, pFormat);
		AddLogBody(pFormat, marker);
		va_end(marker);
		LeaveCriticalSection(&m_SubThreadInfo.m_Lock);
	}
	//-----------------------------------------------------------------------------
	void GGUILogAsyn::OutputError(const tchar* pFormat, ...)
	{
		if (m_theLogLevel < LogLevel_Error)
		{
			return;
		}
		if (m_SubThreadInfo.m_theSubThreadState != SubThread_Running)
		{
			return;
		}
		EnterCriticalSection(&m_SubThreadInfo.m_Lock);
		AddLogHead("ERROR");
		//
		va_list marker;
		va_start(marker, pFormat);
		AddLogBody(pFormat, marker);
		va_end(marker);
		LeaveCriticalSection(&m_SubThreadInfo.m_Lock);
	}
	//-----------------------------------------------------------------------------
	void GGUILogAsyn::AddLogHead(char* pType)
	{
		char szBuff[1024] = {0};
		SYSTEMTIME stTime;
		GetSystemTime(&stTime);
		StringCbPrintfA(szBuff, sizeof(szBuff),
			"%02u:%02u:%02u:%03u [%s] ",
			stTime.wHour+8, stTime.wMinute, stTime.wSecond, stTime.wMilliseconds, pType);
		size_t theBuffLength = 0;
		StringCbLengthA(szBuff, sizeof(szBuff), &theBuffLength);
		if (m_SubThreadInfo.m_nBufferForReceive_CurrentSize + (int)theBuffLength < m_SubThreadInfo.m_nBufferForReceive_MaxSize)
		{
			StringCbCopyA(m_SubThreadInfo.m_pBufferForReceive+m_SubThreadInfo.m_nBufferForReceive_CurrentSize,
				m_SubThreadInfo.m_nBufferForReceive_MaxSize-m_SubThreadInfo.m_nBufferForReceive_CurrentSize, szBuff);
			m_SubThreadInfo.m_nBufferForReceive_CurrentSize += (int)theBuffLength;
		}
		else
		{
			//m_pBufferForReceive容器空间不足了！
			//此处的信息会被丢掉。
			//::MessageBox(SoNULL, TEXT("m_pBufferForReceive is not enough"), TEXT("GGUILogAsyn::AddLogHead"), MB_OK);
		}
		//
		if (m_bOutputDebugString)
		{
			OutputDebugStringA(szBuff);
		}
	}
	//-----------------------------------------------------------------------------
	void GGUILogAsyn::AddLogBody(const tchar* pFormat, const va_list& kVaList)
	{
		tchar szBuff[2048] = {0};
		HRESULT	hr = StringCbVPrintf(szBuff, sizeof(szBuff), pFormat, kVaList);
		if (SUCCEEDED(hr))
		{
			size_t theBuffLength = 0;
			StringCbLength(szBuff, sizeof(szBuff), &theBuffLength);
			char* pDestString = SoNULL;
			//添加换行符，并执行必要的字符转换。
			if (sizeof(tchar) == 1)
			{
				szBuff[theBuffLength] = TEXT('\n');
				++theBuffLength;
				szBuff[theBuffLength] = 0;
				pDestString = (char*)szBuff;
			}
			else
			{
				size_t theCharCount = theBuffLength / 2;
				szBuff[theCharCount] = TEXT('\n');
				szBuff[theCharCount+1] = TEXT('\0');
				theBuffLength += 2;
				//字符转换。
				int nNewStringLength = 0;
				pDestString = UnicodeToAnsi((const wchar_t*)szBuff, &nNewStringLength);
				theBuffLength = nNewStringLength;
			}
			if (m_SubThreadInfo.m_nBufferForReceive_CurrentSize + (int)theBuffLength < m_SubThreadInfo.m_nBufferForReceive_MaxSize)
			{
				StringCbCopyA(m_SubThreadInfo.m_pBufferForReceive+m_SubThreadInfo.m_nBufferForReceive_CurrentSize,
					m_SubThreadInfo.m_nBufferForReceive_MaxSize-m_SubThreadInfo.m_nBufferForReceive_CurrentSize, pDestString);
				m_SubThreadInfo.m_nBufferForReceive_CurrentSize += (int)theBuffLength;
			}
			else
			{
				//m_pBufferForReceive容器空间不足了！
				//此处的信息会被丢掉。
				//::MessageBox(SoNULL, TEXT("m_pBufferForReceive is not enough"), TEXT("GGUILogAsyn::AddLogBody"), MB_OK);
			}
			//
			if (m_bOutputDebugString)
			{
				OutputDebugString(szBuff);
			}
		}
		else
		{
			if (STRSAFE_E_INVALID_PARAMETER == hr)
			{
				//::MessageBox(SoNULL, TEXT("STRSAFE_E_INVALID_PARAMETER"), TEXT("GGUILogAsyn::AddLogBody"), MB_OK);
			}
			else if (STRSAFE_E_INSUFFICIENT_BUFFER == hr)
			{
				//::MessageBox(SoNULL, TEXT("STRSAFE_E_INSUFFICIENT_BUFFER"), TEXT("GGUILogAsyn::AddLogBody"), MB_OK);
			}
		}
	}
	//-----------------------------------------------------------------------------
	unsigned int WINAPI GGUILogAsyn::ThreadProcess(void* pThis)
	{
		if (!SubThread_OpenFile())
		{
			//::MessageBox(SoNULL, TEXT("SubThread_OpenFile fail"), TEXT("GGUILogAsyn::ThreadProcess"), MB_OK);
			m_SubThreadInfo.m_theSubThreadState = SubThread_Finished;
			return 0;
		}
		//
		while (m_SubThreadInfo.m_theSubThreadState != SubThread_Finished)
		{
			//判断本线程是否要结束运行。
			//注意，这个判断要放到While循环的最开始处。
			if (m_SubThreadInfo.m_theSubThreadState == SubThread_Finishing)
			{
				m_SubThreadInfo.m_theSubThreadState = SubThread_Finished;
			}
			//从主线程取到数据。
			EnterCriticalSection(&m_SubThreadInfo.m_Lock);
			if (m_SubThreadInfo.m_nBufferForReceive_CurrentSize > 0)
			{
				StringCbCopyA(m_SubThreadInfo.m_pBufferForWrite,
					m_SubThreadInfo.m_nBufferForWrite_MaxSize, m_SubThreadInfo.m_pBufferForReceive);
				m_SubThreadInfo.m_nBufferForWrite_CurrentSize = m_SubThreadInfo.m_nBufferForReceive_CurrentSize;
				m_SubThreadInfo.m_nBufferForReceive_CurrentSize = 0;
			}
			LeaveCriticalSection(&m_SubThreadInfo.m_Lock);
			//把数据写入到磁盘。
			if (m_SubThreadInfo.m_nBufferForWrite_CurrentSize > 0)
			{
				fwrite(m_SubThreadInfo.m_pBufferForWrite, 
					m_SubThreadInfo.m_nBufferForWrite_CurrentSize, 1, m_SubThreadInfo.m_fp);
				fflush(m_SubThreadInfo.m_fp);
				m_SubThreadInfo.m_nCurrentFileSize += m_SubThreadInfo.m_nBufferForWrite_CurrentSize;
				m_SubThreadInfo.m_nBufferForWrite_CurrentSize = 0;
			}
			//判断是否需要做文件分割。
			if (m_SubThreadInfo.m_nCurrentFileSize > m_SubThreadInfo.m_nMaxFileSize)
			{
				//文件分割。结束本文件，新建另一个文件。
				SubThread_CloseFile();
				++m_SubThreadInfo.m_nFileCount;
				m_SubThreadInfo.m_nCurrentFileSize = 0;
				if (!SubThread_OpenFile())
				{
					//::MessageBox(SoNULL, TEXT("SubThread_OpenFile fail 2"), TEXT("GGUILogAsyn::ThreadProcess"), MB_OK);
					m_SubThreadInfo.m_theSubThreadState = SubThread_Finished;
					return 0;
				}
			}
			//本线程小睡一下。
			if (m_SubThreadInfo.m_nSleepTime >= 0)
			{
				Sleep(m_SubThreadInfo.m_nSleepTime);
			}
		}
		//结束运行。
		SubThread_CloseFile();
		//
		return 1;
	}
	//-----------------------------------------------------------------------------
	bool GGUILogAsyn::SubThread_OpenFile()
	{
		//生成磁盘文件名。
		//第一次执行时，生成的文件名就是用户指定的原文件名。
		//当需要做文件分割时，生成的文件名会在原文件名的后面加后缀。
		char szDestFileName[MaxLength_LogFileName_Ex] = {0};
		if (m_SubThreadInfo.m_nFileCount == 0)
		{
			StringCbCopyA(szDestFileName, sizeof(szDestFileName), m_SubThreadInfo.m_szFileName);
		}
		else
		{
			//需要做文件分割，在原文件名后面加后缀。
			size_t theBuffLength = 0;
			StringCbLengthA(m_SubThreadInfo.m_szFileName, sizeof(m_SubThreadInfo.m_szFileName), &theBuffLength);
			int nTestPos = (int)theBuffLength;
			int nDotPos = 0;
			int nSlashPos = 0;
			while (nTestPos > 0)
			{
				if (m_SubThreadInfo.m_szFileName[nTestPos] == '.')
				{
					nDotPos = nTestPos;
					break;
				}
				else if (m_SubThreadInfo.m_szFileName[nTestPos] == '\\')
				{
					nSlashPos = nTestPos;
					break;
				}
				else if (m_SubThreadInfo.m_szFileName[nTestPos] == '/')
				{
					nSlashPos = nTestPos;
					break;
				}
				--nTestPos;
			}
			if (nSlashPos < nDotPos)
			{
				//从文件名中找到了扩展名。
				m_SubThreadInfo.m_szFileName[nDotPos] = 0;
				const char* pEXE = m_SubThreadInfo.m_szFileName + nDotPos + 1;
				StringCbPrintfA(szDestFileName, sizeof(szDestFileName), "%s_%d.%s", m_SubThreadInfo.m_szFileName, m_SubThreadInfo.m_nFileCount, pEXE);
				m_SubThreadInfo.m_szFileName[nDotPos] = '.';
			}
			else
			{
				//文件名中没有扩展名。
				StringCbPrintfA(szDestFileName, sizeof(szDestFileName), "%s_%d", m_SubThreadInfo.m_szFileName, m_SubThreadInfo.m_nFileCount);
			}
		}
		//尝试打开或创建这个文件。
		if (fopen_s(&m_SubThreadInfo.m_fp, szDestFileName, "at+") != 0)
		{
			//打开文件失败。
			m_SubThreadInfo.m_fp = SoNULL;
			//::MessageBox(SoNULL, TEXT("fopen_s fail"), TEXT("GGUILogAsyn::SubThread_OpenFile"), MB_OK);
			return false;
		}
		//得到这个文件的大小。
		fseek(m_SubThreadInfo.m_fp, 0, SEEK_END);
		m_SubThreadInfo.m_nCurrentFileSize = (int)ftell(m_SubThreadInfo.m_fp);
		//
		char szBuff[1024] = {0};
		SYSTEMTIME stTime;
		GetSystemTime(&stTime);
		StringCbPrintfA(szBuff, sizeof(szBuff), 
			"========%04u:%02u:%02u %02u:%02u:%02u begin========\n",
			stTime.wYear, stTime.wMonth, stTime.wDay, stTime.wHour+8, stTime.wMinute, stTime.wSecond);
		size_t theBuffLength = 0;
		StringCbLengthA(szBuff, sizeof(szBuff), &theBuffLength);
		fwrite(szBuff, theBuffLength, 1, m_SubThreadInfo.m_fp);
		fflush(m_SubThreadInfo.m_fp);
		//
		return true;
	}
	//-----------------------------------------------------------------------------
	void GGUILogAsyn::SubThread_CloseFile()
	{
		if (m_SubThreadInfo.m_fp)
		{
			char szBuff[1024] = {0};
			SYSTEMTIME stTime;
			GetSystemTime(&stTime);
			StringCbPrintfA(szBuff, sizeof(szBuff), 
				"========%04u:%02u:%02u %02u:%02u:%02u end========\n",
				stTime.wYear, stTime.wMonth, stTime.wDay, stTime.wHour+8, stTime.wMinute, stTime.wSecond);
			size_t theBuffLength = 0;
			StringCbLengthA(szBuff, sizeof(szBuff), &theBuffLength);
			fwrite(szBuff, theBuffLength, 1, m_SubThreadInfo.m_fp);
			fflush(m_SubThreadInfo.m_fp);
			fclose(m_SubThreadInfo.m_fp);
			m_SubThreadInfo.m_fp = SoNULL;
		}
	}
}
//-----------------------------------------------------------------------------

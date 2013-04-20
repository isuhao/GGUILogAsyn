//--------------------------------------------------------------------
//  (C) oil
//  2012-04-29
//--------------------------------------------------------------------
#include <Windows.h>
#include "GGUIStringHelp.h"
//--------------------------------------------------------------------
namespace GGUI
{
	//�ַ����������洢�ַ���ת����Ľ����
	//ע�⣬�������Ĵ�С�ǹ̶��ģ������ú���ʱҪС�ģ�ת������ַ���������̫�ࡣ
	const int MaxBufferSize = 2048;
	char theCharBuffer[MaxBufferSize];
	//--------------------------------------------------------------------
	int UnicodeCountFromAnsi(const char* pString)
	{
		return ::MultiByteToWideChar(CP_ACP, 0, pString, -1, NULL, 0);
	}
	//--------------------------------------------------------------------
	int AnsiCountFromUnicode(const wchar_t* pString)
	{
		return ::WideCharToMultiByte(CP_ACP, 0, pString, -1, NULL, 0, NULL, NULL);
	}
	//--------------------------------------------------------------------
	int UnicodeCountFromUtf8(const char* pString)
	{
		return ::MultiByteToWideChar(CP_UTF8, 0, pString, -1, NULL, 0);
	}
	//--------------------------------------------------------------------
	int Utf8CountFromUnicode(const wchar_t* pString)
	{
		return ::WideCharToMultiByte(CP_UTF8, 0, pString, -1, NULL, 0, NULL, NULL);
	}
	//--------------------------------------------------------------------
	wchar_t* AnsiToUnicode(const char* pSrcString) 
	{
		wchar_t* pResult = (wchar_t*)theCharBuffer;
		pResult[0] = 0;
		//
		if (pSrcString == NULL)
		{
			return pResult;
		}
		int nWCharCount = UnicodeCountFromAnsi(pSrcString);
		if (nWCharCount == 0)
		{
			//���ִ���
			return pResult;
		}
		if (nWCharCount*2 >= MaxBufferSize)
		{
			//�ַ�����̫�࣬������������С��
			return pResult;
		}
		::MultiByteToWideChar(CP_ACP, 0, pSrcString, -1, pResult, nWCharCount);
		return pResult;
	}
	//--------------------------------------------------------------------
	char* UnicodeToAnsi(const wchar_t* pSrcString, int* pNewStringLength/*=NULL*/)
	{
		char* pResult = theCharBuffer;
		pResult[0] = 0;
		//
		if (pSrcString == NULL)
		{
			return pResult;
		}
		int nByteCount = AnsiCountFromUnicode(pSrcString);
		if (nByteCount == 0)
		{
			//���ִ���
			return pResult;
		}
		if (nByteCount >= MaxBufferSize)
		{
			//�ַ�����̫�࣬������������С��
			return pResult;
		}
		::WideCharToMultiByte(CP_ACP, 0, pSrcString, -1, pResult, nByteCount, NULL, NULL);
		if (pNewStringLength)
		{
			*pNewStringLength = nByteCount - 1;
		}
		return pResult;
	}
	//--------------------------------------------------------------------
	wchar_t* Utf8ToUnicode(const char* pSrcString)
	{
		wchar_t* pResult = (wchar_t*)theCharBuffer;
		pResult[0] = 0;
		//
		if (pSrcString == NULL)
		{
			return pResult;
		}
		int nWCharCount = UnicodeCountFromUtf8(pSrcString);
		if (nWCharCount == 0)
		{
			//���ִ���
			return pResult;
		}
		if (nWCharCount*2 >= MaxBufferSize)
		{
			//�ַ�����̫�࣬������������С��
			return pResult;
		}
		::MultiByteToWideChar(CP_UTF8, 0, pSrcString, -1, pResult, nWCharCount);
		return pResult;
	}
	//--------------------------------------------------------------------
	char* UnicodeToUtf8(const wchar_t* pSrcString)
	{
		char* pResult = theCharBuffer;
		pResult[0] = 0;
		//
		if (pSrcString == NULL)
		{
			return pResult;
		}
		int nByteCount = Utf8CountFromUnicode(pSrcString);
		if (nByteCount == 0)
		{
			//���ִ���
			return pResult;
		}
		if (nByteCount >= MaxBufferSize)
		{
			//�ַ�����̫�࣬������������С��
			return pResult;
		}
		::WideCharToMultiByte(CP_UTF8, 0, pSrcString, -1, pResult, nByteCount, NULL, NULL);
		return pResult;
	}
	//--------------------------------------------------------------------
	char* AnsiToUtf8(const char* pSrcString)
	{
		//���Ȱ�Ansi�ַ���ת����Unicode��ʽ��
		wchar_t* pUnicodeString = AnsiToUnicode(pSrcString);
		if (pUnicodeString[0] == 0)
		{
			return theCharBuffer;
		}
		//
		wchar_t tempBuffer[MaxBufferSize];
		StringCbCopyW(tempBuffer, sizeof(tempBuffer), pUnicodeString);
		//�ٰ�Unicode�ַ���ת����Utf8��ʽ��
		return UnicodeToUtf8(tempBuffer);
	}
	//--------------------------------------------------------------------
	char* Utf8ToAnsi(const char* pSrcString)
	{
		//���Ȱ�Utf8�ַ���ת����Unicode��ʽ��
		wchar_t* pUnicodeString = Utf8ToUnicode(pSrcString);
		if (pUnicodeString[0] == 0)
		{
			return theCharBuffer;
		}
		//
		wchar_t tempBuffer[MaxBufferSize];
		StringCbCopyW(tempBuffer, sizeof(tempBuffer), pUnicodeString);
		//�ٰ�Unicode�ַ���ת����Ansi��ʽ��
		return UnicodeToAnsi(tempBuffer);
	}
} //namespace GGUI
//--------------------------------------------------------------------

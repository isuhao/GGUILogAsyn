//-----------------------------------------------------------------------------
// (C) oil
// 2013-04-17
//-----------------------------------------------------------------------------
#ifndef _GGUILogAsyn_h_
#define _GGUILogAsyn_h_
//-----------------------------------------------------------------------------
#include <stdio.h>
//-----------------------------------------------------------------------------
namespace GGUI
{
	//-----------------------------------------------------------------------------
#if defined(_UNICODE) || defined(UNICODE)
	typedef wchar_t tchar;
#else
	typedef char tchar;
#endif
	#define SoNULL 0
	//-----------------------------------------------------------------------------
	//Log磁盘文件名的最大长度。
	#define MaxLength_LogFileName 30
	//下面这个值仅限Log系统内部使用。为文件分割时添加后缀预留出空间来。
	#define MaxLength_LogFileName_Ex 35
	//
	enum eLogLevel
	{
		LogLevel_Error,
		LogLevel_Waring,
		LogLevel_Debug,
	};
	//
	struct stLogParam
	{
		//Log磁盘文件名。是相对于EXE的相对路径。
		//Log系统会保存一份这个字符串的拷贝。
		//如果磁盘文件不存在则创建，如果存在则在末尾写入log。
		//如果文件名中包含路径，例如"Output/Log.txt"，则用户要保证"Output"文件夹存在，
		//否则打开文件会失败。
		//字符串长度不能超过MaxLength_LogFileName。
		const tchar* pszLogFileName;
		//主线程有一个数据容器，用于接收Log数据。
		//这个值表示这个数据容器的大小。单位字节。
		int nDataBufferSize;
		//当文件大小超过多大时，就做文件分割。单位字节。
		int nMaxFileSize;
		//分线程主循环中小睡多长时间。单位毫秒。
		//值小于0，表示不小睡。
		int nSleepTime;
		eLogLevel theLogLevel;
		//是否要输出到VS的Output窗口。
		bool bOutputDebugString;

		stLogParam()
		{
			pszLogFileName = TEXT("Log.txt");
			nDataBufferSize = 2048;
			nMaxFileSize = 1000000000;
			nSleepTime = 0;
			theLogLevel = LogLevel_Debug;
			bOutputDebugString = true;
		}
	};
	//-----------------------------------------------------------------------------
	class GGUILogAsyn
	{
	public:
		GGUILogAsyn();
		~GGUILogAsyn();
		static GGUILogAsyn* GetInstance();

		//初始化函数。
		bool InitLog(const stLogParam& theLogParam);
		//释放资源，析构前调用。析构函数中不要调用虚函数。
		void ReleaseLog();
		//
		void OutputDebug(const tchar* pFormat, ...);
		void OutputWaring(const tchar* pFormat, ...);
		void OutputError(const tchar* pFormat, ...);

	protected:
		void AddLogHead(char* pType);
		void AddLogBody(const tchar* pFormat, const va_list& kVaList);

	protected:
		//<<<<<<<<<<<<<<<<<<< 子线程使用的函数 <<<<<<<<<<<<<<<<<<<<
		static unsigned int WINAPI ThreadProcess(void* pThis);
		//打开磁盘上指定的log文件。
		//如果文件存在，则在文件尾部继续写入log信息；
		//如果文件不存在，则新建这个文件。
		//如果文件名中包含路径，例如"Output/Log.txt"，则用户要保证"Output"文件夹存在，
		//否则打开文件会失败。
		static bool SubThread_OpenFile();
		//关闭磁盘文件。
		static void SubThread_CloseFile();
		//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

	protected:
		static GGUILogAsyn* ms_pInstance;
	protected:
		//子线程运转的状态。
		enum eSubThreadState
		{
			SubThread_Running, //子线程运转中。
			SubThread_Finishing, //主线程要求结束子线程的运行；子线程再执行一次While循环（为了取出并写入最后的log数据），就结束运行。
			SubThread_Finished, //子线程已经结束。
		};
		struct stSubThreadInfo
		{
			char m_szFileName[MaxLength_LogFileName];
			FILE* m_fp;
			//数据容器，用于接收主线程的log数据。
			char* m_pBufferForReceive;
			int m_nBufferForReceive_MaxSize;
			//数据容器中存储了多少数据。单位字节。
			int m_nBufferForReceive_CurrentSize;
			//数据容器，用于存储等待写入磁盘的数据。
			char* m_pBufferForWrite;
			int m_nBufferForWrite_MaxSize;
			//数据容器中已经存储了多少数据。单位字节。
			int m_nBufferForWrite_CurrentSize;
			//当文件大小超过多大时，就做文件分割。单位字节。
			int m_nMaxFileSize;
			//记录Log文件的大小，当超过设定的值时就另写一个文件（文件分割）。
			int m_nCurrentFileSize;
			//记录已经进行了几次文件分割。
			int m_nFileCount;
			//分线程主循环中小睡多长时间。单位毫秒。
			//值小于0，表示不小睡。
			int m_nSleepTime;
			eSubThreadState m_theSubThreadState;
			CRITICAL_SECTION m_Lock;
		};
	protected:
		HANDLE m_hSubThread;
		unsigned int m_uiSubThreadID;
		eLogLevel m_theLogLevel;
		//是否要输出到VS的Output窗口。
		bool m_bOutputDebugString;
		//
		static stSubThreadInfo m_SubThreadInfo;

	};
	//-----------------------------------------------------------------------------
	#define LOG_DEBUG if(GGUILogAsyn::GetInstance()) GGUILogAsyn::GetInstance()->OutputDebug
	#define LOG_WARING if(GGUILogAsyn::GetInstance()) GGUILogAsyn::GetInstance()->OutputWaring
	#define LOG_ERROR if(GGUILogAsyn::GetInstance()) GGUILogAsyn::GetInstance()->OutputError
	//-----------------------------------------------------------------------------
} //namespace GGUI
//-----------------------------------------------------------------------------
#endif //_GGUILogAsyn_h_
//-----------------------------------------------------------------------------

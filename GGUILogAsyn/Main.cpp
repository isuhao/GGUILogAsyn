//-----------------------------------------------------------------------------
#include <Windows.h>
#include "GGUILogAsyn.h"
//-----------------------------------------------------------------------------
using namespace GGUI;
//-----------------------------------------------------------------------------
void main()
{
	new GGUILogAsyn;
	stLogParam theParam;
	theParam.pszLogFileName = TEXT("Log\\Log.txt");
	theParam.nMaxFileSize = 1000;
	GGUILogAsyn::GetInstance()->InitLog(theParam);

	int nCount = 0;
	int nMaxCount = 1000;
	while (nCount < nMaxCount)
	{
		LOG_DEBUG(TEXT("Test 惊天动地的好log啊 [%d]"), nCount);
		++nCount;
		//如果没有下面这句Sleep，主线程的数据容器会被塞满！
		//因为分线程有一句Sleep(0)，导致分线程没有机会从主线程取出数据。
		//分线程要打开磁盘文件，中途还有分割文件，远比主线程执行的复杂，
		//有可能会出现主线程数据容器被塞满的情况。
		Sleep(1);
	}

	GGUILogAsyn::GetInstance()->ReleaseLog();
	delete GGUILogAsyn::GetInstance();
}
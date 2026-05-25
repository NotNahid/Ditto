#include "stdafx.h"
#include "TypingLogger.h"
#include "CP_Main.h"
#include "Misc.h"
#include "Clip.h"

CTypingLogger* CTypingLogger::m_pInstance = NULL;

CTypingLogger::CTypingLogger()
{
	m_pInstance = this;
	m_hhkLowLevelKybd = NULL;
	m_bPaused = FALSE;
	m_hLastWnd = NULL;
	m_lastEventTime = GetTickCount();
}

CTypingLogger::~CTypingLogger()
{
	Stop();
}

BOOL CTypingLogger::Start()
{
	if (m_hhkLowLevelKybd) return TRUE;
	m_hhkLowLevelKybd = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
	return m_hhkLowLevelKybd != NULL;
}

void CTypingLogger::Stop()
{
	if (m_hhkLowLevelKybd)
	{
		UnhookWindowsHookEx(m_hhkLowLevelKybd);
		m_hhkLowLevelKybd = NULL;
	}
	FlushBuffer();
}

void CTypingLogger::Pause(BOOL bPause)
{
	m_bPaused = bPause;
	if (m_bPaused) FlushBuffer();
}

LRESULT CALLBACK CTypingLogger::LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && m_pInstance != NULL && !m_pInstance->m_bPaused)
	{
		KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
		if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)
		{
			m_pInstance->HandleKey(pKey->vkCode, TRUE);
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

void CTypingLogger::HandleKey(DWORD vkCode, BOOL bKeyDown)
{
	HWND hActive = GetForegroundWindow();
	if (hActive != m_hLastWnd)
	{
		FlushBuffer();
		m_hLastWnd = hActive;
		TCHAR title[256];
		GetWindowText(hActive, title, 256);
		m_csLastWndTitle = title;
	}

	if (IsSensitiveWindow(hActive) || IsPasswordField(hActive))
	{
		return;
	}

	// Simple key to char conversion
	TCHAR buf[16];
	BYTE state[256];
	GetKeyboardState(state);
	if (ToUnicode(vkCode, MapVirtualKey(vkCode, MAPVK_VK_TO_VSC), state, buf, 16, 0) > 0)
	{
		// Only log printable characters
		if (buf[0] >= 32 || buf[0] == '\t')
		{
			m_csBuffer += buf[0];
		}
	}
	else if (vkCode == VK_RETURN)
	{
		m_csBuffer += _T("\r\n");
		FlushBuffer();
	}
	else if (vkCode == VK_BACK)
	{
		if (m_csBuffer.GetLength() > 0)
			m_csBuffer.Truncate(m_csBuffer.GetLength() - 1);
	}

	if (m_csBuffer.GetLength() > 500 || (GetTickCount() - m_lastEventTime > 5000 && m_csBuffer.GetLength() > 0))
	{
		FlushBuffer();
	}
	m_lastEventTime = GetTickCount();
}

void CTypingLogger::FlushBuffer()
{
	if (m_csBuffer.IsEmpty()) return;

	CClip* pClip = new CClip;
	pClip->m_Time = CTime::GetCurrentTime();
	pClip->m_Desc = m_csLastWndTitle + _T(": ") + m_csBuffer;
	pClip->m_lType = 1; // Typing
	
	// Add format CF_UNICODETEXT
	int len = (m_csBuffer.GetLength() + 1) * sizeof(TCHAR);
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, len);
	if (hGlobal)
	{
		LPVOID pData = GlobalLock(hGlobal);
		memcpy(pData, (LPCTSTR)m_csBuffer, len);
		GlobalUnlock(hGlobal);
		pClip->m_Formats.AddNew(CF_UNICODETEXT, hGlobal);
	}

	pClip->AddToDB();
	delete pClip;

	m_csBuffer.Empty();
}

BOOL CTypingLogger::IsSensitiveWindow(HWND hWnd)
{
	TCHAR title[256];
	GetWindowText(hWnd, title, 256);
	CString csTitle = title;
	csTitle.MakeLower();
	if (csTitle.Find(_T("bank")) >= 0 || csTitle.Find(_T("paypal")) >= 0 || csTitle.Find(_T("credit card")) >= 0)
		return TRUE;
	return FALSE;
}

BOOL CTypingLogger::IsPasswordField(HWND hWnd)
{
	GUITHREADINFO gui;
	gui.cbSize = sizeof(GUITHREADINFO);
	if (GetGUIThreadInfo(GetWindowThreadProcessId(hWnd, NULL), &gui))
	{
		if (gui.hwndFocus)
		{
			TCHAR className[256];
			GetClassName(gui.hwndFocus, className, 256);
			CString csClassName = className;
			if (csClassName.Find(_T("Edit")) >= 0 || csClassName.Find(_T("Password")) >= 0)
			{
				LONG_PTR style = GetWindowLongPtr(gui.hwndFocus, GWL_STYLE);
				if (style & ES_PASSWORD)
					return TRUE;
			}
		}
	}
	return FALSE;
}

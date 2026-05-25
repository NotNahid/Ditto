#pragma once

class CTypingLogger
{
public:
	CTypingLogger();
	~CTypingLogger();

	BOOL Start();
	void Stop();
	void Pause(BOOL bPause);
	BOOL IsPaused() const { return m_bPaused; }

	static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);

private:
	void HandleKey(DWORD vkCode, BOOL bKeyDown);
	void FlushBuffer();
	BOOL IsSensitiveWindow(HWND hWnd);
	BOOL IsPasswordField(HWND hWnd);

	HHOOK m_hhkLowLevelKybd;
	CString m_csBuffer;
	CString m_csLastWndTitle;
	HWND m_hLastWnd;
	DWORD m_lastEventTime;
	BOOL m_bPaused;

	static CTypingLogger* m_pInstance;
};

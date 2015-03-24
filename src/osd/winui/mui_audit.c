/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

 /***************************************************************************

  mui_audit.c

  Audit dialog

***************************************************************************/

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <richedit.h>

// standard C headers
#include <stdio.h>
#include <tchar.h>

// MAME/MAMEUI headers
#include "winui.h"
#include "winutf8.h"
#include "strconv.h"
#include "audit.h"
#include "resource.h"
#include "mui_opts.h"
#include "mui_util.h"
#include "properties.h"

#define SAMPLES_NOT_USED    3
#define MAX_AUDITBOX_TEXT	0x7FFFFFFE

/***************************************************************************
    function prototypes
 ***************************************************************************/

static DWORD WINAPI AuditThreadProc(LPVOID hDlg);
static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
static void ProcessNextRom(void);
static void ProcessNextSample(void);
static void CLIB_DECL DetailsPrintf(const char *fmt, ...) ATTR_PRINTF(1,2);
static const char * StatusString(int iStatus);

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HWND hAudit;
static int rom_index;
static int roms_correct;
static int roms_incorrect;
static int sample_index;
static int samples_correct;
static int samples_incorrect;

static int audit_color = 0;
static int audit_samples = 0;
static BOOL bCancel = FALSE;
static HICON audit_icon = NULL;
static HICON hIcon = NULL;
static HBRUSH hBrush = NULL;
static HDC hDC = NULL;
static TCHAR* type;
static CHARFORMAT font;
static HANDLE hThread;
static DWORD dwThreadID;

/***************************************************************************
    External functions
 ***************************************************************************/

void AuditDialog(HWND hParent)
{
	rom_index         = 0;
	roms_correct      = 0;
	roms_incorrect    = 0;
	sample_index      = 0;
	samples_correct   = 0;
	samples_incorrect = 0;

	DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_AUDIT), hParent, AuditWindowProc);
}

void InitGameAudit(int gameIndex)
{
	rom_index = gameIndex;
}

BOOL IsAuditResultKnown(int audit_result)
{
	return TRUE;
}

BOOL IsAuditResultYes(int audit_result)
{
	return audit_result == media_auditor::CORRECT || audit_result == media_auditor::BEST_AVAILABLE;
}

BOOL IsAuditResultNo(int audit_result)
{
	return audit_result == media_auditor::NOTFOUND || audit_result == media_auditor::INCORRECT;
}

/***************************************************************************
    Internal functions
 ***************************************************************************/
// Verifies the ROM set while calling SetRomAuditResults
int MameUIVerifyRomSet(int game)
{
	driver_enumerator enumerator(MameUIGlobal(), driver_list::driver(game));
	enumerator.next();
	media_auditor auditor(enumerator);
	media_auditor::summary summary = auditor.audit_media(AUDIT_VALIDATE_FAST);

	// output the summary of the audit
	astring summary_string;
	auditor.summarize(driver_list::driver(game).name, &summary_string);
	DetailsPrintf("%s", summary_string.cstr());

	SetRomAuditResults(game, summary);
	
	return summary;
}

// Verifies the Sample set while calling SetSampleAuditResults
int MameUIVerifySampleSet(int game)
{
	driver_enumerator enumerator(MameUIGlobal(), driver_list::driver(game));
	enumerator.next();
	media_auditor auditor(enumerator);
	media_auditor::summary summary = auditor.audit_samples();

	// output the summary of the audit
	astring summary_string;
	auditor.summarize(driver_list::driver(game).name, &summary_string);
	DetailsPrintf("%s", summary_string.cstr());

	return summary;
}

static DWORD WINAPI AuditThreadProc(LPVOID hDlg)
{
	char buffer[200];

	while (!bCancel)
	{
		if (rom_index != -1)
		{
			sprintf(buffer, "Checking game... - %s", driver_list::driver(rom_index).name);
			win_set_window_text_utf8((HWND)hDlg, buffer);
			ProcessNextRom();
		}
		else
		{
			if (sample_index != -1)
			{
				sprintf(buffer, "Checking game... - %s", driver_list::driver(sample_index).name);
				win_set_window_text_utf8((HWND)hDlg, buffer);
				ProcessNextSample();
			}
			else
			{
				win_set_window_text_utf8((HWND)hDlg, "Audit process completed");
				ExitThread(1);
			}
		}
	}
	return 0;
}

static INT_PTR CALLBACK AuditWindowProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	DWORD dwExitCode;
	bCancel = FALSE;
	
	switch (Msg)
	{
	case WM_INITDIALOG:
		CenterWindow(hDlg);
        hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAMEUI_ICON));
        SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
		hBrush = GetSysColorBrush(COLOR_WINDOW);
		
		memset (&font, 0, sizeof(font));
		font.cbSize				= sizeof(CHARFORMAT);
		font.dwMask				= CFM_COLOR | CFM_FACE | CFM_SIZE;
		font.yHeight			= 160;
		font.crTextColor		= RGB(63, 72, 204);
		font.bPitchAndFamily	= 34;
	
		type = tstring_from_utf8("Tahoma");
		_tcscpy(font.szFaceName, type);
		osd_free(type);
		
		hAudit = hDlg;
		
		SendDlgItemMessage(hAudit, IDC_AUDIT_DETAILS, EM_SETBKGNDCOLOR, 0, RGB(239, 239, 239));
		SendDlgItemMessage(hAudit, IDC_AUDIT_DETAILS, EM_SETCHARFORMAT, 0, (LPARAM)&font);
		SendDlgItemMessage(hAudit, IDC_AUDIT_DETAILS, EM_SETLIMITTEXT, MAX_AUDITBOX_TEXT, 0);

		SendDlgItemMessage(hAudit, IDC_ROMS_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, driver_list::total()));
		SendDlgItemMessage(hAudit, IDC_SAMPLES_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, driver_list::total()));
		
		rom_index = 0;
		hThread = CreateThread(NULL, 0, AuditThreadProc, hDlg, 0, &dwThreadID);
		SetThreadPriority(hThread, THREAD_PRIORITY_IDLE);
		
		return 1;
	
	case WM_CTLCOLORDLG:
		return (LRESULT) hBrush;	
		
	case WM_CTLCOLORSTATIC:
	case WM_CTLCOLORBTN:
		hDC=(HDC)wParam;
		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_ROMS_CORRECT))
			SetTextColor(hDC, RGB(34, 177, 76));
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_ROMS_INCORRECT))
			SetTextColor(hDC, RGB(237, 28, 36));
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_ROMS_TOTAL))
			SetTextColor(hDC, RGB(63, 72, 204));
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_SAMPLES_CORRECT))
			SetTextColor(hDC, RGB(34, 177, 76));
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_SAMPLES_INCORRECT))
			SetTextColor(hDC, RGB(237, 28, 36));
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_SAMPLES_TOTAL))
			SetTextColor(hDC, RGB(63, 72, 204));
			
		return (LRESULT) hBrush;
		
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDCANCEL:
			if (hThread)
			{
				bCancel = TRUE;
				if (GetExitCodeThread(hThread, &dwExitCode) && (dwExitCode == STILL_ACTIVE))
				{
					PostMessage(hDlg, WM_COMMAND, wParam, lParam);
					return 1;
				}
				CloseHandle(hThread);
			}
			DeleteObject(hBrush);
			DestroyIcon(hIcon);
			EndDialog(hAudit,0);
			break;
		}
		
		return 1;
	}
	
	return 0;
}

/* Callback for the Audit property sheet */
INT_PTR CALLBACK GameAuditDialogProc(HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
		{
		const game_driver *game = &driver_list::driver(rom_index);
		machine_config config(*game, MameUIGlobal());
		int iStatus;
		LPCSTR lpStatus;
		char buffer[4096];
		char details[4096];
		UINT32 crctext;

		buffer[0] = '\0';
		details[0] = '\0';

		ModifyPropertySheetForTreeSheet(hDlg);

		FlushFileCaches();
		hAudit = hDlg;
		hBrush = GetSysColorBrush(COLOR_WINDOW);
		win_set_window_text_utf8(GetDlgItem(hAudit, IDC_PROP_TITLE), GameInfoTitle(OPTIONS_GAME, rom_index));

		memset (&font, 0, sizeof(font));
		font.cbSize				= sizeof(CHARFORMAT);
		font.dwMask				= CFM_COLOR | CFM_FACE | CFM_SIZE;
		font.yHeight			= 160;
		font.crTextColor		= RGB(63, 72, 204);
		font.bPitchAndFamily	= 34;
	
		type = tstring_from_utf8("Tahoma");
		_tcscpy(font.szFaceName, type);
		osd_free(type);

		SendDlgItemMessage(hAudit, IDC_ROM_DETAILS, EM_SETBKGNDCOLOR, 0, RGB(239, 239, 239));
		SendDlgItemMessage(hAudit, IDC_ROM_DETAILS, EM_SETCHARFORMAT, 0, (LPARAM)&font);

		font.crTextColor		= RGB(237, 28, 36);
		
		SendDlgItemMessage(hAudit, IDC_AUDIT_DETAILS_PROP, EM_SETBKGNDCOLOR, 0, RGB(239, 239, 239));
		SendDlgItemMessage(hAudit, IDC_AUDIT_DETAILS_PROP, EM_SETCHARFORMAT, 0, (LPARAM)&font);
		
		iStatus = MameUIVerifyRomSet(rom_index);
		
		switch (iStatus)
		{
		case media_auditor::CORRECT:
		case media_auditor::BEST_AVAILABLE:
			audit_icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_AUDIT_PASS));
			break;

		case media_auditor::NOTFOUND:
		case media_auditor::INCORRECT:
			audit_icon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_AUDIT_FAIL));
			break;
		}
		
		SendDlgItemMessage(hAudit, IDC_AUDIT_ICON, STM_SETICON, (WPARAM) audit_icon, 0);

		lpStatus = StatusString(iStatus);
		win_set_window_text_utf8(GetDlgItem(hAudit, IDC_PROP_ROMS), lpStatus);

		if (DriverUsesSamples(rom_index))
		{
			iStatus = MameUIVerifySampleSet(rom_index);
			lpStatus = StatusString(iStatus);
		}
		else
		{
			lpStatus = "None required";
			audit_samples = 2;
		}
		
		win_set_window_text_utf8(GetDlgItem(hAudit, IDC_PROP_SAMPLES), lpStatus);

		sprintf(buffer, "NAME                \tSIZE        CRC\n"
			"-------------------------------------------------------------------\n");
		strcat(details, buffer);
		device_iterator deviter(config.root_device());
		for (device_t *device = deviter.first(); device != NULL; device = deviter.next())
			for (const rom_entry *region = rom_first_region(*device); region != NULL; region = rom_next_region(region))
				for (const rom_entry *rom = rom_first_file(region); rom != NULL; rom = rom_next_file(rom))
				{
					UINT32 crc;
					if (hash_collection(ROM_GETHASHDATA(rom)).crc(crc))
						crctext = crc;
					else
						crctext = 0;
					sprintf(buffer, "%-16s \t%07d  %08x\n", ROM_GETNAME(rom), ROM_GETLENGTH(rom), crctext);
					strcat(details, buffer);
				}
		win_set_window_text_utf8(GetDlgItem(hAudit, IDC_ROM_DETAILS), ConvertToWindowsNewlines(details));

		ShowWindow(hAudit, SW_SHOW);
		
		return 1;
		}
		
	case WM_CTLCOLORDLG:
		return (LRESULT) hBrush;
		
	case WM_CTLCOLORSTATIC:
		{	
		hDC=(HDC)wParam;
		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_PROP_ROMS))
		{
			if (audit_color == 0)
				SetTextColor(hDC, RGB(34, 177, 76));
			else if (audit_color == 1)
				SetTextColor(hDC, RGB(237, 28, 36));
			else
				SetTextColor(hDC, RGB(63, 72, 204));
		}
		if ((HWND)lParam == GetDlgItem(hAudit, IDC_PROP_SAMPLES))
		{
			if (audit_samples == 0)
				SetTextColor(hDC, RGB(34, 177, 76));
			else if (audit_samples == 1)
				SetTextColor(hDC, RGB(237, 28, 36));
			else
				SetTextColor(hDC, RGB(63, 72, 204));
		}
		
		return (LRESULT) hBrush;
		}
	}
	DeleteObject(hBrush);
	DestroyIcon(audit_icon);
	return 0;
}

static void ProcessNextRom()
{
	int retval;
	TCHAR buffer[200];

	retval = MameUIVerifyRomSet(rom_index);
	
	switch (retval)
	{
	case media_auditor::BEST_AVAILABLE: 	/* correct, incorrect or separate count? */
	case media_auditor::CORRECT:
		roms_correct++;
		_stprintf(buffer, TEXT("%i"), roms_correct);
		SendDlgItemMessage(hAudit, IDC_ROMS_CORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		_stprintf(buffer, TEXT("%i"), roms_correct + roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		break;

	case media_auditor::NOTFOUND:
		break;

	case media_auditor::INCORRECT:
		roms_incorrect++;
		_stprintf(buffer, TEXT("%i"), roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_INCORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		_stprintf(buffer, TEXT("%i"), roms_correct + roms_incorrect);
		SendDlgItemMessage(hAudit, IDC_ROMS_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
		break;
	}

	rom_index++;
	SendDlgItemMessage(hAudit, IDC_ROMS_PROGRESS, PBM_SETPOS, rom_index, 0);

	if (rom_index == driver_list::total())
	{
		sample_index = 0;
		rom_index = -1;
	}
}

static void ProcessNextSample()
{
	int  retval;
	TCHAR buffer[200];

	retval = MameUIVerifySampleSet(sample_index);

	switch (retval)
	{
	case media_auditor::CORRECT:
		if (DriverUsesSamples(sample_index))
		{
			samples_correct++;
			_stprintf(buffer, TEXT("%i"), samples_correct);
			SendDlgItemMessage(hAudit, IDC_SAMPLES_CORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
			_stprintf(buffer, TEXT("%i"), samples_correct + samples_incorrect);
			SendDlgItemMessage(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);
			break;
		}

	case media_auditor::NOTFOUND:
		break;

	case media_auditor::INCORRECT:
		samples_incorrect++;
		_stprintf(buffer, TEXT("%i"), samples_incorrect);
		SendDlgItemMessage(hAudit, IDC_SAMPLES_INCORRECT, WM_SETTEXT, 0, (LPARAM)buffer);
		_stprintf(buffer, TEXT("%i"), samples_correct + samples_incorrect);
		SendDlgItemMessage(hAudit, IDC_SAMPLES_TOTAL, WM_SETTEXT, 0, (LPARAM)buffer);

		break;
	}

	sample_index++;
	SendDlgItemMessage(hAudit, IDC_SAMPLES_PROGRESS, PBM_SETPOS, sample_index, 0);

	if (sample_index == driver_list::total())
	{
		DetailsPrintf("Audit complete.\n");
		SendDlgItemMessage(hAudit, IDCANCEL, WM_SETTEXT, 0, (LPARAM)TEXT("Close"));
		sample_index = -1;
	}
}

static void CLIB_DECL DetailsPrintf(const char *fmt, ...)
{
	HWND	hEdit;
	va_list marker;
	char	buffer[8000];
	TCHAR*  t_s;
	int		textLength;

	//RS 20030613 Different Ids for Property Page and Dialog
	// so see which one's currently instantiated
	hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS);
	
	if (hEdit ==  NULL)
		hEdit = GetDlgItem(hAudit, IDC_AUDIT_DETAILS_PROP);

	if (hEdit == NULL)
		return;

	va_start(marker, fmt);

	vsprintf(buffer, fmt, marker);

	va_end(marker);

	t_s = tstring_from_utf8(ConvertToWindowsNewlines(buffer));
	
	if( !t_s || _tcscmp(TEXT(""), t_s) == 0)
		return;

	textLength = Edit_GetTextLength(hEdit);
	Edit_SetSel(hEdit, textLength, textLength);
	SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)(LPCTSTR)win_tstring_strdup(t_s) );
	SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
	osd_free(t_s);
}

static const char * StatusString(int iStatus)
{
	static const char *ptr;

	ptr = "None required";
	audit_color = 2;

	switch (iStatus)
	{
	case media_auditor::CORRECT:
		ptr = "Passed";
		audit_color = 0;
		audit_samples = 0;
		break;

	case media_auditor::BEST_AVAILABLE:
		ptr = "Best available";
		audit_color = 0;
		audit_samples = 0;
		break;

	case media_auditor::NOTFOUND:
		ptr = "Not found";
		audit_color = 1;
		audit_samples = 1;
		break;

	case media_auditor::INCORRECT:
		ptr = "Failed";
		audit_color = 1;
		audit_samples = 1;
		break;
	}

	return ptr;
}

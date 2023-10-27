/****************************************************************************
 *                                                                          *
 * File    : main.c                                                         *
 *                                                                          *
 * Purpose : TOTP Experiments by Mihai Gaitos                               *
 *                                                                          *
 ****************************************************************************/

/* 
 * Either define WIN32_LEAN_AND_MEAN, or one or more of NOCRYPT,
 * NOSERVICE, NOMCX and NOIME, to decrease compile time (if you
 * don't need these defines -- see windows.h).
 */

/* #define WIN32_LEAN_AND_MEAN */
/* #define NOCRYPT */
#define __STDC_WANT_LIB_EXT2__ 1
#define NOSERVICE
#define NOMCX
#define NOIME

#define USE_OWN_SHA1

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "sha1_otp.h"
#include "b32.h"
#include "main.h"

#define NELEMS(a)  (sizeof(a) / sizeof((a)[0]))
#define BUF_SIZE 4096

/** Prototypes **************************************************************/

static INT_PTR CALLBACK MainDlgProc(HWND, UINT, WPARAM, LPARAM);

/** Global variables ********************************************************/

static HANDLE ghInstance;


/****************************************************************************
 *                                                                          *
 * Function: WinMain                                                        *
 *                                                                          *
 * Purpose : Initialize the application.  Register a window class,          *
 *           create and display the main window and enter the               *
 *           message loop.                                                  *
 *                                                                          *
 * History : Date      Reason                                               *
 *           00/00/00  Created                                              *
 *                                                                          *
 ****************************************************************************/

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    INITCOMMONCONTROLSEX icc;
    WNDCLASSEX wcx;

    ghInstance = hInstance;

    /* Initialize common controls. Also needed for MANIFEST's */
    /*
     * TODO: set the ICC_???_CLASSES that you need.
     */
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icc);

    /* Get system dialog information */
    wcx.cbSize = sizeof(wcx);
    if (!GetClassInfoEx(NULL, MAKEINTRESOURCE(32770), &wcx))
        return 0;

    /* Add our own stuff */
    wcx.hInstance = hInstance;
    wcx.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDR_ICO_MAIN));
    wcx.lpszClassName = _T("totpClass");
    if (!RegisterClassEx(&wcx))
        return 0;

    /* The user interface is a modal dialog box */
    return DialogBox(hInstance, MAKEINTRESOURCE(DLG_MAIN), NULL, (DLGPROC)MainDlgProc);
}

/**********************************
 * application specific functions *
 **********************************/

/* fill digits combo with items having the no. of digits for OTP as both value and label */
void initdigitscb(HWND hcb)
{
	int i;
	int r;
	char cbuf[16];
	for(i=4;i<10;i++)
	{
		snprintf(cbuf,16,"%d",i);
		r=SendMessage(hcb,CB_ADDSTRING,0,(LPARAM) cbuf);
		SendMessage(hcb,CB_SETITEMDATA,r,i);
	}
	/* default 6 digits (index 2) */
	SendMessage(hcb,CB_SETCURSEL,2,0);
}

/* fill "step" combo */
void initstepcb(HWND hcb)
{
	int r;
	r=SendMessage(hcb,CB_ADDSTRING,0,(LPARAM) "30 sec");
	SendMessage(hcb,CB_SETITEMDATA,r,30);

	r=SendMessage(hcb,CB_ADDSTRING,0,(LPARAM) "1 min");
	SendMessage(hcb,CB_SETITEMDATA,r,60);

	r=SendMessage(hcb,CB_ADDSTRING,0,(LPARAM) "2 min");
	SendMessage(hcb,CB_SETITEMDATA,r,120);

	SendMessage(hcb,CB_SETCURSEL,0,0);
}

/* get "combo value" for chosen item */
uint32_t getcrtcbval(HWND hcb)
{
	return SendMessage(hcb,CB_GETITEMDATA,SendMessage(hcb,CB_GETCURSEL,0,0),0);
}

/* genereate TOTP for values in dialog box */
void w_totp(HWND hDlg)
{
	uint64_t e,t;
	uint32_t mdd,step;

	char *b32k;
	uint8_t *bink;
	ssize_t tl,kl;

	char obuf[16];	/* never more than 9 digits anyway */
	char tbuf[20];	/* time/count buf */

	tl=1+SendDlgItemMessage(hDlg,IDE_KH,WM_GETTEXTLENGTH,0,0);
	if(tl<=1) return;
	b32k=malloc(tl+1);
	if(!b32k) return;
	GetDlgItemText(hDlg,IDE_KH,b32k,tl);
	b32k[tl]=0;

	b32_deca(&bink,b32k,strlen(b32k),&kl);

	if(IsDlgButtonChecked(hDlg,IDC_CE))
		e=GetDlgItemInt(hDlg,IDE_EPOCH,NULL,FALSE);
	else
		e=time(NULL);

	step=getcrtcbval(GetDlgItem(hDlg,IDCB_STEP));
	mdd=getcrtcbval(GetDlgItem(hDlg,IDCB_DIGITS));
	t=e/step;

	qw2otp(obuf,t,mdd,"sha1",bink,kl);	/* only sha1 supported without openssl! */

	SetDlgItemInt(hDlg,IDE_EPOCH,e,FALSE);

	snprintf(tbuf,20,"%16llx",t);
	SetDlgItemText(hDlg,IDE_T,tbuf);
	SetDlgItemText(hDlg,IDE_RESULT,obuf);

	free(b32k);
	free(bink);
}

/* convert ASCII text to HEX digits */

void w_a2h(HWND ed, HWND es)
{
	char dbuf[128];

	char *buf,*bh;
	int lmax,l;

	lmax=1+SendMessage(es,WM_GETTEXTLENGTH,0,0);
	if(lmax<0) return;
	buf=malloc(lmax+1);
	if(!buf) return;
	l=GetWindowText(es,buf,lmax);
	snprintf(dbuf,128,"l=%d, lmax=%d",l,lmax);
	buf[lmax]=0;	/* just in case */
	
	bh=malloc(2*l+1);
	if(!bh)
	{
		free(buf);
		return;
	}

	
	OutputDebugString(buf);
	OutputDebugString(dbuf);

/*	b2h(bh,buf,l);
	bh[2*l]=0;

	OutputDebugString(bh); */

	SetWindowText(ed,bh);

	free(buf);
	free(bh);
}



/****************************************************************************
 *                                                                          *
 * Function: MainDlgProc                                                    *
 *                                                                          *
 * Purpose : Process messages for the Main dialog.                          *
 *                                                                          *
 * History : Date      Reason                                               *
 *           00/00/00  Created                                              *
 *                                                                          *
 ****************************************************************************/


static INT_PTR CALLBACK MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
			/* code to initialize the dialog. */
			initdigitscb(GetDlgItem(hwndDlg,IDCB_DIGITS));
			initstepcb(GetDlgItem(hwndDlg,IDCB_STEP));
            
            return TRUE;

        case WM_SIZE:
            /*
             * TODO: Add code to process resizing, when needed.
             */
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case ID_GENERATE:
                    w_totp(hwndDlg);
                    return TRUE;
				case IDE_KA:
					if(GET_WM_COMMAND_CMD(wParam,lParam)==EN_CHANGE)
					{
						w_a2h(GetDlgItem(hwndDlg,IDE_KH),GetDlgItem(hwndDlg,IDE_KA));
						return TRUE;
					}
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, 0);
            return TRUE;

        /*
         * TODO: Add more messages, when needed.
         */
    }

    return FALSE;
}

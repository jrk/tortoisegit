// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "TortoiseProc.h"
#include "ExportDlg.h"

#include "Messagebox.h"
#include "PathUtils.h"
#include "BrowseFolder.h"
#include "AppUtils.h"


IMPLEMENT_DYNAMIC(CExportDlg, CResizableStandAloneDialog)
CExportDlg::CExportDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CExportDlg::IDD, pParent)
	, CChooseVersion(this)
	, m_Revision(_T("HEAD"))
	, m_strExportDirectory(_T(""))
	, m_sExportDirOrig(_T(""))
	, m_bNoExternals(FALSE)
	, m_pLogDlg(NULL)
{
}

CExportDlg::~CExportDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CExportDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_CHECKOUTDIRECTORY, m_strExportDirectory);
	DDX_Control(pDX, IDC_CHECKOUTDIRECTORY, m_cCheckoutEdit);

	CHOOSE_VERSION_DDX;

}


BEGIN_MESSAGE_MAP(CExportDlg, CResizableStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_BN_CLICKED(IDC_CHECKOUTDIRECTORY_BROWSE, OnBnClickedCheckoutdirectoryBrowse)
	ON_EN_CHANGE(IDC_CHECKOUTDIRECTORY, OnEnChangeCheckoutdirectory)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)

	CHOOSE_VERSION_EVENT
END_MESSAGE_MAP()

BOOL CExportDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	m_sExportDirOrig = m_strExportDirectory;
	m_bAutoCreateTargetName = !PathIsDirectoryEmpty(m_sExportDirOrig);

	AddAnchor(IDC_REPOGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EXPORT_CHECKOUTDIR, TOP_LEFT);
	AddAnchor(IDC_CHECKOUTDIRECTORY, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	CHOOSE_VERSION_ADDANCHOR;
	Init();
	if(this->m_Revision.IsEmpty())
	{
		SetDefaultChoose(IDC_RADIO_HEAD);
	}
	else
	{
		SetDefaultChoose(IDC_RADIO_VERSION);
		this->GetDlgItem(IDC_COMBOBOXEX_VERSION)->SetWindowTextW(m_Revision);
	}

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_CHECKOUTDIRECTORY, IDS_CHECKOUT_TT_DIR);

	SHAutoComplete(GetDlgItem(IDC_CHECKOUTDIRECTORY)->m_hWnd, SHACF_FILESYSTEM);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("ExportDlg"));
	return TRUE;
}

void CExportDlg::OnOK()
{
	if (!UpdateData(TRUE))
		return; // don't dismiss dialog (error message already shown by MFC framework)

	// check it the export path is a valid windows path
	UpdateRevsionName();

	if (m_VersionName.IsEmpty())
	{
		ShowBalloon(IDC_REVISION_NUM, IDS_ERR_INVALIDREV);
		return;
	}

	m_bAutoCreateTargetName = false;

//	m_URLCombo.SaveHistory();
//	m_URL = m_URLCombo.GetString();


	UpdateData(FALSE);
	CResizableStandAloneDialog::OnOK();
}

void CExportDlg::OnBnClickedBrowse()
{
	m_tooltips.Pop();	// hide the tooltips

}

void CExportDlg::OnBnClickedCheckoutdirectoryBrowse()
{
	m_tooltips.Pop();	// hide the tooltips
	//
	// Create a folder browser dialog. If the user selects OK, we should update
	// the local data members with values from the controls, copy the checkout
	// directory from the browse folder, then restore the local values into the
	// dialog controls.
	//
	this->UpdateRevsionName();
	CFileDialog dlg(FALSE,_T("Zip"),this->m_VersionName,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						_T("*.Zip"));
	
	if(dlg.DoModal()==IDOK)
	{
		UpdateData(TRUE);
		m_strExportDirectory = dlg.GetPathName();
		UpdateData(FALSE);
	}
}

BOOL CExportDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CExportDlg::OnEnChangeCheckoutdirectory()
{
	UpdateData(TRUE);
	DialogEnableWindow(IDOK, !m_strExportDirectory.IsEmpty());
}

void CExportDlg::OnBnClickedHelp()
{
	OnHelp();
}

void CExportDlg::OnBnClickedShowlog()
{
	m_tooltips.Pop();	// hide the tooltips

}

LPARAM CExportDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	SetDlgItemText(IDC_REVISION_NUM, temp);
	CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
	return 0;
}

void CExportDlg::OnEnChangeRevisionNum()
{
	UpdateData();
	if (m_sRevision.IsEmpty())
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
}

void CExportDlg::OnCbnSelchangeEolcombo()
{
}

void CExportDlg::SetRevision(const CString& rev)
{
	if (rev==_T("HEAD"))
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_HEAD);
	else
	{
		CheckRadioButton(IDC_REVISION_HEAD, IDC_REVISION_N, IDC_REVISION_N);
		CString sRev;
		sRev.Format(_T("%s"), rev);
		SetDlgItemText(IDC_REVISION_NUM, sRev);
	}
}

void CExportDlg::OnCbnEditchangeUrlcombo()
{
	if (!m_bAutoCreateTargetName)
		return;
	if (m_sExportDirOrig.IsEmpty())
		return;
	// find out what to use as the checkout directory name
	UpdateData();
	m_URLCombo.GetWindowText(m_URL);
	if (m_URL.IsEmpty())
		return;
	CString name = CAppUtils::GetProjectNameFromURL(m_URL);
	m_strExportDirectory = m_sExportDirOrig+_T('\\')+name;
	UpdateData(FALSE);
}

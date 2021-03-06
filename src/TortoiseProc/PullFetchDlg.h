#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
// CPullFetchDlg dialog

class CPullFetchDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CPullFetchDlg)

public:
	CPullFetchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPullFetchDlg();

// Dialog Data
	enum { IDD = IDD_PULLFETCH };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	CHistoryCombo	m_Remote;
	CHistoryCombo	m_Other;
	CHistoryCombo	m_RemoteBranch;
	virtual BOOL OnInitDialog();

	
	DECLARE_MESSAGE_MAP()
public:
	BOOL m_IsPull;
	afx_msg void OnBnClickedRd();
	afx_msg void OnBnClickedOk();
	CString m_RemoteURL;
	CString m_RemoteBranchName;
};

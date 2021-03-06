#pragma once

#include "StandAloneDlg.h"
#include "HistoryCombo.h"
#include "ChooseVersion.h"
// CCreateBranchTagDlg dialog

class CCreateBranchTagDlg : public CResizableStandAloneDialog,public CChooseVersion
{
	DECLARE_DYNAMIC(CCreateBranchTagDlg)

public:
	CCreateBranchTagDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCreateBranchTagDlg();

// Dialog Data
	enum { IDD = IDD_NEW_BRANCH_TAG };

	BOOL m_bForce;
	BOOL m_bTrack;
	BOOL m_bIsTag;
	BOOL m_bSwitch;

	CString m_Base;
	CString m_BranchTagName;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	
	CHOOSE_EVENT_RADIO();
	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedRadio();
	afx_msg void OnBnClickedOk();
	afx_msg void OnCbnSelchangeComboboxexBranch();
};

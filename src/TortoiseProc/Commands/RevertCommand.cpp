// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "RevertCommand.h"

#include "RevertDlg.h"
//#include "SVNProgressDlg.h"
#include "MessageBox.h"

bool RevertCommand::Execute()
{
	CRevertDlg dlg;
	dlg.m_pathList = pathList;
	if (dlg.DoModal() == IDOK)
	{

		if (dlg.m_selectedPathList.GetCount() == 0)
			return FALSE;
		CString cmd;
		CString out;
		for(int i=0;i< dlg.m_selectedPathList.GetCount() ;i++)
		{
			cmd.Format(_T("git.exe checkout -f -- \"%s\""),dlg.m_selectedPathList[i].GetGitPathString());
			if(g_Git.Run(cmd,&out,CP_OEMCP))
			{
				CMessageBox::Show(NULL,out,_T("TortoiseGit"),MB_OK);
			}
		}
#if 0
		CSVNProgressDlg progDlg;
		theApp.m_pMainWnd = &progDlg;
		progDlg.SetCommand(CSVNProgressDlg::SVNProgress_Revert);
		if (parser.HasVal(_T("closeonend")))
			progDlg.SetAutoClose(parser.GetLongVal(_T("closeonend")));
		progDlg.SetOptions(dlg.m_bRecursive ? ProgOptRecursive : ProgOptNonRecursive);
		progDlg.SetPathList(dlg.m_pathList);
		progDlg.SetItemCount(dlg.m_selectedPathList.GetCount());
		progDlg.SetSelectedList(dlg.m_selectedPathList);
		progDlg.DoModal();
#endif
		return true;
	}
	return false;
}

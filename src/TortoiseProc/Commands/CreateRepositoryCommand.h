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
#pragma once
#include "Command.h"

#include "MessageBox.h"
#include "CommonResource.h"
#include "git.h"
/**
 * \ingroup TortoiseProc
 * Creates a repository
 */
class CreateRepositoryCommand : public Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute()
	{
		CGit git;
		git.m_CurrentDir=this->orgCmdLinePath.GetWinPath();
		CString output;

		if (git.Run(_T("git.exe init-db"),&output,CP_UTF8))
		{
			CMessageBox::Show(hwndExplorer, IDS_PROC_REPOCREATEERR, IDS_APPNAME, MB_ICONERROR);
			return false;
		}
		else
		{
			CMessageBox::Show(hwndExplorer, output, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
		}
		return true;
	}
};



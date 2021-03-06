// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "registry.h"

/**
 * \ingroup TortoiseProc
 * Handles the UI colors used in TortoiseSVN
 */
class CColors
{
public:
	CColors(void);
	~CColors(void);
	
	enum Colors
	{
		Cmd,
		Conflict,
		Modified,
		Merged,
		Deleted,
		Added,
		LastCommit,
		DeletedNode,
		AddedNode,
		ReplacedNode,
		RenamedNode,
		LastCommitNode,
		PropertyChanged,
		CurrentBranch,
		LocalBranch,
		RemoteBranch,
		Tag,
		BranchLine1,
		BranchLine2,
		BranchLine3,
		BranchLine4,
		BranchLine5,
		BranchLine6,
		BranchLine7,
		BranchLine8,
		COLOR_END=-1
	};
	
	COLORREF GetColor(Colors col, bool bDefault = false);
	void SetColor(Colors col, COLORREF cr);

	struct COLOR_DATA
	{
		Colors		Color;
		TCHAR *		RegKey;
		COLORREF	Default;
	};

private:

	static COLOR_DATA m_ColorArray[];

	/*
	CRegDWORD m_regCmd;
	CRegDWORD m_regConflict;
	CRegDWORD m_regModified;
	CRegDWORD m_regMerged;
	CRegDWORD m_regDeleted;
	CRegDWORD m_regAdded;
	CRegDWORD m_regLastCommit;
	CRegDWORD m_regDeletedNode;
	CRegDWORD m_regAddedNode;
	CRegDWORD m_regReplacedNode;
	CRegDWORD m_regRenamedNode;
	CRegDWORD m_regLastCommitNode;
	CRegDWORD m_regPropertyChanged;
	CRegDWORD m_regCurrentBranch;
	CRegDWORD m_regLocalBranch;
	CRegDWORD m_regRemoteBranch;
	CRegDWORD m_regTag;
	CRegDWORD m_regBranchLine1;
	CRegDWORD m_regBranchLine2;
	CRegDWORD m_regBranchLine3;
	CRegDWORD m_regBranchLine4;
	CRegDWORD m_regBranchLine5;
	CRegDWORD m_regBranchLine6;
	CRegDWORD m_regBranchLine7;
	CRegDWORD m_regBranchLine8;
*/
};

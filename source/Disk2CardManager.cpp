/*
AppleWin : An Apple //e emulator for Windows

Copyright (C) 1994-1996, Michael O'Brien
Copyright (C) 1999-2001, Oliver Schmidt
Copyright (C) 2002-2005, Tom Charlesworth
Copyright (C) 2006-2019, Tom Charlesworth, Michael Pohoreski, Nick Westgate

AppleWin is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

AppleWin is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with AppleWin; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Description: Disk2 Card Manager
 *
 * Author: Various
 *
 */

#include "StdAfx.h"

#include "Applewin.h"
#include "CardManager.h"
#include "Disk.h"
#include "Disk2CardManager.h"

bool Disk2CardManager::IsConditionForFullSpeed(void)
{
	for (UINT i = 0; i < NUM_SLOTS; i++)
	{
		if (g_CardMgr.QuerySlot(i) == CT_Disk2)
		{
			if (dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->IsConditionForFullSpeed())
				return true;	// if any card is true then the condition for full-speed is true
		}
	}

	return false;
}

void Disk2CardManager::UpdateDriveState(UINT cycles)
{
	for (UINT i = 0; i < NUM_SLOTS; i++)
	{
		if (g_CardMgr.QuerySlot(i) == CT_Disk2)
		{
			dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->UpdateDriveState(cycles);
		}
	}
}

void Disk2CardManager::Reset(const bool powerCycle /*=false*/)
{
	for (UINT i = 0; i < NUM_SLOTS; i++)
	{
		if (g_CardMgr.QuerySlot(i) == CT_Disk2)
		{
			dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->Reset(powerCycle);
		}
	}
}

bool Disk2CardManager::GetEnhanceDisk(void)
{
	for (UINT i = 0; i < NUM_SLOTS; i++)
	{
		if (g_CardMgr.QuerySlot(i) == CT_Disk2)
		{
			// All Disk2 cards should have the same setting, so just return the state of the first card
			return dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->GetEnhanceDisk();
		}
	}
	return false;
}

void Disk2CardManager::SetEnhanceDisk(bool enhanceDisk)
{
	for (UINT i = 0; i < NUM_SLOTS; i++)
	{
		if (g_CardMgr.QuerySlot(i) == CT_Disk2)
		{
			dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->SetEnhanceDisk(enhanceDisk);
		}
	}
}

void Disk2CardManager::LoadLastDiskImage(void)
{
	for (UINT i = 0; i < NUM_SLOTS; i++)
	{
		if (i != SLOT6) continue;	// FIXME

		if (g_CardMgr.QuerySlot(i) == CT_Disk2)
		{
			dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->LoadLastDiskImage(DRIVE_1);
			dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->LoadLastDiskImage(DRIVE_2);
		}
	}
}

void Disk2CardManager::Destroy(void)
{
	for (UINT i = 0; i < NUM_SLOTS; i++)
	{
		if (g_CardMgr.QuerySlot(i) == CT_Disk2)
		{
			dynamic_cast<Disk2InterfaceCard*>(g_CardMgr.GetObj(i))->Destroy();
		}
	}
}

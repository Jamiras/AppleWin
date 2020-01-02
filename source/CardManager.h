#pragma once

#include "Card.h"
#include "Disk2CardManager.h"

class CardManager
{
public:
	CardManager(void) :
		m_pMouseCard(NULL),
		m_pSSC(NULL)
	{
		Insert(0, CT_Empty);
		Insert(1, CT_GenericPrinter);
		Insert(2, CT_SSC);
		Insert(3, CT_Uthernet);
		Insert(4, CT_Empty);
		Insert(5, CT_Empty);
		Insert(6, CT_Disk2);
		Insert(7, CT_Empty);
		InsertAux(CT_Extended80Col);	// For Apple //e and above
	}
	~CardManager(void)
	{
		for (UINT i=0; i<NUM_SLOTS; i++)
			Remove(i);
		RemoveAux();
	}

	void Insert(UINT slot, SS_CARDTYPE type);
	void Remove(UINT slot);
	SS_CARDTYPE QuerySlot(UINT slot) { return m_slot[slot]->QueryType(); }
	Card* GetObj(UINT slot) { SS_CARDTYPE t=QuerySlot(slot); _ASSERT(t==CT_SSC || t==CT_MouseInterface || t==CT_Disk2); return m_slot[slot]; }

	void InsertAux(SS_CARDTYPE type);
	void RemoveAux(void);
	SS_CARDTYPE QueryAux(void) { return m_aux->QueryType(); }
	Card* GetObjAux(void) { _ASSERT(0); return m_aux; }	// ASSERT because this is a DummyCard

	//

	Disk2CardManager& GetDisk2CardMgr(void) { return m_disk2CardMgr; }
	class CMouseInterface* GetMouseCard(void) { return m_pMouseCard; }
	bool IsMouseCardInstalled(void) { return m_pMouseCard != NULL; }
	class CSuperSerialCard* GetSSC(void) { return m_pSSC; }
	bool IsSSCInstalled(void) { return m_pSSC != NULL; }

private:
	void RemoveInternal(UINT slot);

	Card* m_slot[NUM_SLOTS];
	Card* m_aux;
	Disk2CardManager m_disk2CardMgr;
	class CMouseInterface* m_pMouseCard;
	class CSuperSerialCard* m_pSSC;
};
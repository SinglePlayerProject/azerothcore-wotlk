#ifndef MARKETER_MANAGER_H
#define MARKETER_MANAGER_H

#include <string>
#include "Log.h"
#include "MarketerConfig.h"
#include "AuctionHouseMgr.h"
#include "Item.h";
#include "ObjectMgr.h"

class MarketerManager
{
	MarketerManager();
	MarketerManager(MarketerManager const&) = delete;
	MarketerManager& operator=(MarketerManager const&) = delete;
	~MarketerManager() = default;

public:
    void ResetMarketer();
	bool UpdateSeller(uint32 pmDiff);
	bool UpdateBuyer(uint32 pmDiff);
	static MarketerManager* instance();

private:    
    bool MarketEmpty(uint32 pmFaction);
    void SellItem(uint32 pmItemEntry, SQLTransaction pmSQLT);
    void SellItem(const ItemTemplate* pmIT, uint8 pmStackCount, uint8 pmSellCount, uint32 pmFaction, uint64 pmAuctioneerGUID, SQLTransaction pmSQLT);
    void UpdateFactionBuyer(uint32 pmFaction);
    void GenerateSellingItemIDVector();

public:
	std::unordered_set<uint32> vendorUnlimitItemSet;

	int32 buyerCheckDelay;
    int32 sellerCheckDelay;    

private:    
    std::vector<uint32> sellingItemIDVector;
    bool selling;
};

#define sMarketerManager MarketerManager::instance()

#endif

#include "MarketerManager.h"

MarketerManager::MarketerManager()
{
    buyerCheckDelay = 120000;
    sellerCheckDelay = 60000;

    vendorUnlimitItemSet.clear();
    QueryResult vendorItemQR = WorldDatabase.Query("SELECT distinct item FROM npc_vendor where maxcount = 0");
    if (vendorItemQR)
    {
        do
        {
            Field* fields = vendorItemQR->Fetch();
            uint32 eachItemEntry = fields[0].GetUInt32();
            vendorUnlimitItemSet.insert(eachItemEntry);
        } while (vendorItemQR->NextRow());
    }
}

MarketerManager* MarketerManager::instance()
{
    static MarketerManager instance;
    return &instance;
}

void MarketerManager::ResetMarketer()
{
    if (!sMarketerConfig->reset)
    {
        return;
    }
    sLog->outBasic("Ready to reset marketer seller");
    std::set<uint32> factionSet;
    //factionSet.insert(1);
    //factionSet.insert(6);
    //factionSet.insert(7);
    factionSet.insert(sMarketerConfig->allianceFaction);
    factionSet.insert(sMarketerConfig->neutralFaction);
    factionSet.insert(sMarketerConfig->hordeFaction);
    for (std::set<uint32>::iterator factionIT = factionSet.begin(); factionIT != factionSet.end(); factionIT++)
    {
        uint32 factionID = *factionIT;
        AuctionHouseEntry const* ahEntry = sAuctionMgr->GetAuctionHouseEntry(factionID);
        AuctionHouseObject* aho = sAuctionMgr->GetAuctionsMap(factionID);
        if (!aho)
        {
            sLog->outError("AuctionHouseObject is null");
            return;
        }
        std::set<uint32> auctionIDSet;
        auctionIDSet.clear();
        for (std::map<uint32, AuctionEntry*>::iterator aeIT = aho->GetAuctionsBegin(); aeIT != aho->GetAuctionsEnd(); aeIT++)
        {
            auctionIDSet.insert(aeIT->first);
        }
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        for (std::set<uint32>::iterator auctionIDIT = auctionIDSet.begin(); auctionIDIT != auctionIDSet.end(); auctionIDIT++)
        {
            AuctionEntry* eachAE = aho->GetAuction(*auctionIDIT);
            if (eachAE)
            {
                if (eachAE->owner == 0)
                {
                    uint32 itemEntry = eachAE->item_template;
                    sAuctionMgr->RemoveAItem(eachAE->item_guidlow);
                    aho->RemoveAuction(eachAE);
                    eachAE->DeleteFromDB(trans);
                    sLog->outBasic("Auction %d removed for auctionhouse %d", itemEntry, factionID);
                }
            }
        }
        CharacterDatabase.CommitTransaction(trans);
    }

    sLog->outBasic("Marketer seller reset");
}

bool MarketerManager::UpdateSeller(uint32 pmDiff)
{
    if (!sMarketerConfig->enable)
    {
        return false;
    }
    if (sellerCheckDelay > 0)
    {
        sellerCheckDelay -= pmDiff;
        return true;
    }

    if (selling)
    {
        int checkSellCount = 0;
        int maxSellCount = 100;
        SQLTransaction trans = CharacterDatabase.BeginTransaction();
        while (checkSellCount < maxSellCount)
        {
            if (sellingItemIDVector.size() > 0)
            {
                int itemEntry = sellingItemIDVector.back();
                SellItem(itemEntry, trans);
                sellingItemIDVector.pop_back();
                checkSellCount++;
            }
            else
            {
                selling = false;
                break;
            }
        }
        CharacterDatabase.CommitTransaction(trans);
        sLog->outBasic("Marketer keep selling");
        sellerCheckDelay = MINUTE * IN_MILLISECONDS;
    }
    else if (MarketEmpty(sMarketerConfig->allianceFaction) && MarketEmpty(sMarketerConfig->neutralFaction) && MarketEmpty(sMarketerConfig->hordeFaction))
    {
        GenerateSellingItemIDVector();
        sLog->outBasic("Marketer start selling");
        selling = true;
    }
    else
    {
        sellerCheckDelay = 2 * HOUR * IN_MILLISECONDS;
    }

    return true;
}

void MarketerManager::GenerateSellingItemIDVector()
{
    sellingItemIDVector.clear();
    ItemTemplateContainer const* its = sObjectMgr->GetItemTemplateStore();
    for (ItemTemplateContainer::const_iterator itr = its->begin(); itr != its->end(); ++itr)
    {
        if (itr->second.ItemLevel < 1)
        {
            continue;
        }
        if (itr->second.Quality < 1)
        {
            continue;
        }
        if (itr->second.Quality > 4)
        {
            continue;
        }
        if (itr->second.Bonding != ItemBondingType::NO_BIND&&itr->second.Bonding != ItemBondingType::BIND_WHEN_EQUIPED&&itr->second.Bonding != ItemBondingType::BIND_WHEN_USE)
        {
            continue;
        }
        if (itr->second.SellPrice == 0 && itr->second.BuyPrice == 0)
        {
            continue;
        }
        if (vendorUnlimitItemSet.find(itr->second.ItemId) != vendorUnlimitItemSet.end())
        {
            continue;
        }
        bool sellThis = false;
        switch (itr->second.Class)
        {
        case ItemClass::ITEM_CLASS_CONSUMABLE:
        {
            sellThis = true;
            break;
        }
        case ItemClass::ITEM_CLASS_CONTAINER:
        {
            if (itr->second.Quality >= 2)
            {
                sellThis = true;
            }
            break;
        }
        case ItemClass::ITEM_CLASS_WEAPON:
        {
            if (itr->second.Quality >= 2)
            {
                sellThis = true;
            }
            break;
        }
        case ItemClass::ITEM_CLASS_GEM:
        {
            sellThis = true;
            break;
        }
        case ItemClass::ITEM_CLASS_ARMOR:
        {
            if (itr->second.Quality >= 2)
            {
                sellThis = true;
            }
            break;
        }
        case ItemClass::ITEM_CLASS_REAGENT:
        {
            sellThis = true;
            break;
        }
        case ItemClass::ITEM_CLASS_PROJECTILE:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_TRADE_GOODS:
        {
            sellThis = true;
            break;
        }
        case ItemClass::ITEM_CLASS_GENERIC:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_RECIPE:
        {
            sellThis = true;
            break;
        }
        case ItemClass::ITEM_CLASS_MONEY:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_QUIVER:
        {
            if (itr->second.Quality >= 2)
            {
                sellThis = true;
            }
            break;
        }
        case ItemClass::ITEM_CLASS_QUEST:
        {
            sellThis = true;
            break;
        }
        case ItemClass::ITEM_CLASS_KEY:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_PERMANENT:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_MISC:
        {
            if (itr->second.Quality > 0)
            {
                sellThis = true;
            }
        }
        case ItemClass::ITEM_CLASS_GLYPH:
        {
            sellThis = true;
            break;
        }
        default:
        {
            break;
        }
        }
        if (sellThis)
        {
            sellingItemIDVector.push_back(itr->first);
        }
    }
}

bool MarketerManager::MarketEmpty(uint32 pmFaction)
{
    AuctionHouseEntry const* ahEntry = sAuctionMgr->GetAuctionHouseEntry(pmFaction);
    AuctionHouseObject* aho = sAuctionMgr->GetAuctionsMap(pmFaction);
    if (!aho)
    {
        sLog->outError("AuctionHouseObject is null");
        return false;
    }
    bool result = true;
    for (std::map<uint32, AuctionEntry*>::iterator aeIT = aho->GetAuctionsBegin(); aeIT != aho->GetAuctionsEnd(); aeIT++)
    {
        if (aeIT->second->owner == 0)
        {
            return false;            
        }
    }
    return true;
}

void MarketerManager::SellItem(uint32 pmItemEntry, SQLTransaction pmSQLT)
{
    const ItemTemplate* pIT = sObjectMgr->GetItemTemplate(pmItemEntry);
    if (!pIT)
    {
        return;
    }
    uint32 stackCount = 1;
    switch (pIT->Class)
    {
    case ItemClass::ITEM_CLASS_CONSUMABLE:
    {
        stackCount = pIT->Stackable;
        break;
    }
    case ItemClass::ITEM_CLASS_CONTAINER:
    {
        if (pIT->Quality >= 2)
        {
            stackCount = pIT->Stackable;
        }
        break;
    }
    case ItemClass::ITEM_CLASS_WEAPON:
    {
        if (pIT->Quality >= 2)
        {
            stackCount = 1;
        }
        break;
    }
    case ItemClass::ITEM_CLASS_GEM:
    {
        stackCount = 1;
        break;
    }
    case ItemClass::ITEM_CLASS_ARMOR:
    {
        if (pIT->Quality >= 2)
        {
            stackCount = 1;
        }
        break;
    }
    case ItemClass::ITEM_CLASS_REAGENT:
    {
        stackCount = pIT->Stackable;
        break;
    }
    case ItemClass::ITEM_CLASS_PROJECTILE:
    {
        break;
    }
    case ItemClass::ITEM_CLASS_TRADE_GOODS:
    {
        stackCount = pIT->Stackable;
        break;
    }
    case ItemClass::ITEM_CLASS_GENERIC:
    {
        break;
    }
    case ItemClass::ITEM_CLASS_RECIPE:
    {
        stackCount = 1;
        break;
    }
    case ItemClass::ITEM_CLASS_MONEY:
    {
        break;
    }
    case ItemClass::ITEM_CLASS_QUIVER:
    {
        if (pIT->Quality >= 2)
        {
            stackCount = 1;
        }
        break;
    }
    case ItemClass::ITEM_CLASS_QUEST:
    {
        stackCount = 1;
        break;
    }
    case ItemClass::ITEM_CLASS_KEY:
    {
        break;
    }
    case ItemClass::ITEM_CLASS_PERMANENT:
    {
        break;
    }
    case ItemClass::ITEM_CLASS_MISC:
    {
        if (pIT->Quality > 0)
        {
            stackCount = 1;
        }
    }
    default:
    {
        break;
    }
    }
    uint8 unitCount = 1;
    if (pIT->Stackable > 1)
    {
        unitCount = urand(1, 5);
    }
    SellItem(pIT, stackCount, unitCount, sMarketerConfig->allianceFaction, sMarketerConfig->allianceAuctioneerGUID, pmSQLT);
    SellItem(pIT, stackCount, unitCount, sMarketerConfig->neutralFaction, sMarketerConfig->neutralAuctioneerGUID, pmSQLT);
    SellItem(pIT, stackCount, unitCount, sMarketerConfig->hordeFaction, sMarketerConfig->hordeAuctioneerGUID, pmSQLT);
}

void MarketerManager::SellItem(const ItemTemplate* pmIT, uint8 pmStackCount, uint8 pmSellCount, uint32 pmFaction, uint64 pmAuctioneerGUID, SQLTransaction pmSQLT)
{
    AuctionHouseEntry const* ahe = sAuctionMgr->GetAuctionHouseEntry(pmFaction);
    AuctionHouseObject* aho = sAuctionMgr->GetAuctionsMap(pmFaction);
    if (!ahe || !aho)
    {
        return;
    }
    uint8 checkSellCount = 0;
    while (checkSellCount <= pmSellCount)
    {
        Item* item = Item::CreateItem(pmIT->ItemId, pmStackCount, NULL);
        if (item)
        {
            if (int32 randomPropertyId = Item::GenerateItemRandomPropertyId(item->GetEntry()))
            {
                item->SetItemRandomProperties(randomPropertyId);
            }
            uint32 finalPrice = 0;
            uint8 qualityMuliplier = 1;
            if (pmIT->Quality > 2)
            {
                qualityMuliplier = pmIT->Quality - 1;
            }
            finalPrice = pmIT->SellPrice * pmStackCount * urand(10, 20);            
            if (finalPrice == 0)
            {
                finalPrice = pmIT->BuyPrice * pmStackCount * urand(2, 4);
            }
            if (finalPrice == 0)
            {
                break;
            }
            finalPrice = finalPrice * qualityMuliplier;
            if (finalPrice > 100)
            {
                uint32 dep = sAuctionMgr->GetAuctionDeposit(ahe, 86400, item, item->GetCount());
                AuctionEntry* auctionEntry = new AuctionEntry;
                auctionEntry->Id = sObjectMgr->GenerateAuctionID();
                auctionEntry->auctioneer = pmAuctioneerGUID;
                auctionEntry->auctionHouseEntry = ahe;
                auctionEntry->item_guidlow = item->GetGUIDLow();
                auctionEntry->item_template = item->GetEntry();
                auctionEntry->owner = 0;
                auctionEntry->startbid = finalPrice / 2;
                auctionEntry->buyout = finalPrice;
                auctionEntry->bidder = 0;
                auctionEntry->bid = 0;
                auctionEntry->deposit = dep;
                auctionEntry->expire_time = (time_t)86400 + time(NULL);
                item->SaveToDB(pmSQLT);
                sAuctionMgr->AddAItem(item);
                aho->AddAuction(auctionEntry);
                auctionEntry->SaveToDB(pmSQLT);
                sLog->outBasic("Auction %s added for auctionhouse %d", pmIT->Name1.c_str(), pmFaction);
            }
        }
        checkSellCount++;
    }
}

bool MarketerManager::UpdateBuyer(uint32 pmDiff)
{
    if (!sMarketerConfig->enable)
    {
        return false;
    }
    if (buyerCheckDelay > 0)
    {
        buyerCheckDelay -= pmDiff;
        return true;
    }
    buyerCheckDelay = 2 * HOUR * IN_MILLISECONDS;
    sLog->outBasic("Ready to update marketer buyer");
    UpdateFactionBuyer(sMarketerConfig->allianceFaction);
    UpdateFactionBuyer(sMarketerConfig->neutralFaction);
    UpdateFactionBuyer(sMarketerConfig->hordeFaction);
    sLog->outBasic("Marketer buyer updated");
    return true;
}

void MarketerManager::UpdateFactionBuyer(uint32 pmFaction)
{
    AuctionHouseEntry const* ahe = sAuctionMgr->GetAuctionHouseEntry(pmFaction);
    AuctionHouseObject* aho = sAuctionMgr->GetAuctionsMap(pmFaction);
    if (!ahe || aho)
    {
        return;
    }
    std::set<uint32> toBuyAuctionIDSet;
    for (std::map<uint32, AuctionEntry*>::iterator aeIT = aho->GetAuctionsBegin(); aeIT != aho->GetAuctionsEnd(); aeIT++)
    {
        Item *checkItem = sAuctionMgr->GetAItem(aeIT->second->item_guidlow);
        if (!checkItem)
        {
            continue;
        }
        if (aeIT->second->owner == 0)
        {
            continue;
        }
        uint8 checkThis = urand(0, 2);
        if (checkThis != 0)
        {
            continue;
        }
        const ItemTemplate* destIT = sObjectMgr->GetItemTemplate(aeIT->second->item_template);
        if (destIT->SellPrice == 0 && destIT->BuyPrice == 0)
        {
            continue;
        }
        if (destIT->Quality < 1)
        {
            continue;
        }
        if (destIT->Quality > 4)
        {
            continue;
        }
        if (vendorUnlimitItemSet.find(aeIT->second->item_template) != vendorUnlimitItemSet.end())
        {
            continue;
        }
        uint8 buyThis = 1;

        if (!destIT)
        {
            continue;
        }
        switch (destIT->Class)
        {
        case ItemClass::ITEM_CLASS_CONSUMABLE:
        {
            buyThis = urand(0, 5);
            break;
        }
        case ItemClass::ITEM_CLASS_CONTAINER:
        {
            if (destIT->Quality > 2)
            {
                buyThis = 0;
            }
            else if (destIT->Quality == 2)
            {
                buyThis = urand(0, 5);
            }
            break;
        }
        case ItemClass::ITEM_CLASS_WEAPON:
        {
            if (destIT->Quality > 2)
            {
                buyThis = 0;
            }
            else if (destIT->Quality == 2)
            {
                buyThis = urand(0, 5);
            }
            break;
        }
        case ItemClass::ITEM_CLASS_GEM:
        {
            buyThis = 0;
            break;
        }
        case ItemClass::ITEM_CLASS_ARMOR:
        {
            if (destIT->Quality > 2)
            {
                buyThis = 0;
            }
            else if (destIT->Quality == 2)
            {
                buyThis = urand(0, 5);
            }
            break;
        }
        case ItemClass::ITEM_CLASS_REAGENT:
        {
            buyThis = urand(0, 5);
            break;
        }
        case ItemClass::ITEM_CLASS_PROJECTILE:
        {
            buyThis = urand(0, 10);
            break;
        }
        case ItemClass::ITEM_CLASS_TRADE_GOODS:
        {
            buyThis = 0;
            break;
        }
        case ItemClass::ITEM_CLASS_GENERIC:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_RECIPE:
        {
            buyThis = urand(0, 3);
            break;
        }
        case ItemClass::ITEM_CLASS_MONEY:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_QUIVER:
        {
            if (destIT->Quality > 2)
            {
                buyThis = 0;
            }
            else if (destIT->Quality == 2)
            {
                buyThis = urand(0, 5);
            }
            break;
        }
        case ItemClass::ITEM_CLASS_QUEST:
        {
            buyThis = urand(0, 5);
            break;
        }
        case ItemClass::ITEM_CLASS_KEY:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_PERMANENT:
        {
            break;
        }
        case ItemClass::ITEM_CLASS_GLYPH:
        {
            buyThis = 0;
            break;
        }
        default:
        {
            break;
        }
        }
        if (buyThis != 0)
        {
            continue;
        }
        uint32 basePrice = 0;
        uint32 baseMultiple = 0;
        uint8 qualityMuliplier = 1;
        if (destIT->Quality > 2)
        {
            qualityMuliplier = destIT->Quality - 1;
        }
        basePrice = destIT->SellPrice * checkItem->GetCount();
        baseMultiple = urand(10, 20);
        if (basePrice == 0)
        {
            basePrice = destIT->BuyPrice * checkItem->GetCount();
            baseMultiple = urand(2, 4);
        }
        if (basePrice == 0)
        {
            continue;
        }        
        uint32 finalPrice = basePrice * baseMultiple* qualityMuliplier;
        bool excuteBuying = false;
        if (aeIT->second->buyout > finalPrice)
        {
            uint32 gapMultiple = aeIT->second->buyout / basePrice;
            uint8 buyRand = urand(0, gapMultiple);
            if (buyRand == 0)
            {
                excuteBuying = true;
            }
        }
        else
        {
            excuteBuying = true;
        }
        if (excuteBuying)
        {
            toBuyAuctionIDSet.insert(aeIT->first);
        }
    }

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    for (std::set<uint32>::iterator toBuyIT = toBuyAuctionIDSet.begin(); toBuyIT != toBuyAuctionIDSet.end(); toBuyIT++)
    {
        AuctionEntry* destAE = aho->GetAuction(*toBuyIT);
        if (destAE)
        {
            destAE->bid = destAE->buyout;
            sAuctionMgr->SendAuctionSuccessfulMail(destAE, trans);
            sAuctionMgr->SendAuctionWonMail(destAE, trans);
            sAuctionMgr->RemoveAItem(destAE->item_guidlow);
            aho->RemoveAuction(destAE);
            destAE->DeleteFromDB(trans);
            delete destAE;
            destAE = nullptr;
            sLog->outBasic("Auction %d was bought by marketer buyer", *toBuyIT);
        }
    }
    CharacterDatabase.CommitTransaction(trans);
}

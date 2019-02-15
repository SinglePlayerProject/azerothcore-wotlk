#include "RobotManager.h"

RobotManager::RobotManager()
{
    Preparation();
}

RobotManager* RobotManager::instance()
{
    static RobotManager instance;
    return &instance;
}

void RobotManager::UpdateManager(uint32 pmDiff)
{
    if (sRobotConfig->enable == 0)
    {
        return;
    }
    if (activeRobotsCheckDelay > 0)
    {
        activeRobotsCheckDelay -= pmDiff;
    }
    if (checkDelay > 0)
    {
        checkDelay -= pmDiff;
        return;
    }
    checkDelay = 1000;

    switch (managerState)
    {
    case 0:
    {
        if (sRobotConfig->resetRobots == 0)
        {
            managerState = 3;
        }
        else
        {
            managerState = 1;
        }
        break;
    }
    case 1:
    {
        if (DeleteRobots())
        {
            managerState = 2;
        }
        else
        {
            managerState = 3;
        }
        break;
    }
    case 2:
    {
        if (RobotsDeleted())
        {
            sWorld->ShutdownServ(1, SHUTDOWN_MASK_RESTART, RESTART_EXIT_CODE);
            return;
        }
        break;
    }
    case 3:
    {
        if (!ProcessRobots())
        {
            managerState = 20;
        }
        checkDelay = 60000;
        sLog->outBasic("Next process check in 60 seconds");
        break;
    }
    default:
        break;
    }
}

void RobotManager::UpdateRobots()
{
    if (sRobotConfig->enable == 0)
    {
        return;
    }
    if (managerState != 3)
    {
        return;
    }
    if (updateIndex >= activeRobotSessionMap.size())
    {
        updateIndex = 0;
    }
    uint32 endIndex = updateIndex + 50;
    if (endIndex > activeRobotSessionMap.size())
    {
        endIndex = activeRobotSessionMap.size();
    }
    while (updateIndex < endIndex)
    {
        if (activeRobotSessionMap[updateIndex]->GetPlayer())
        {
            if (activeRobotSessionMap[updateIndex]->GetPlayer()->rai)
            {
                activeRobotSessionMap[updateIndex]->GetPlayer()->rai->Update();
            }
        }
        updateIndex++;
    }

    return;
}

bool RobotManager::DeleteRobots()
{
    QueryResult accountQR = LoginDatabase.PQuery("SELECT id, username FROM account where username like '%s%%'", sRobotConfig->robotAccountNamePrefix.c_str());

    if (accountQR)
    {
        do
        {
            Field* fields = accountQR->Fetch();
            uint32 id = fields[0].GetUInt32();
            std::string userName = fields[1].GetString();
            deleteRobotAccountSet.insert(id);
            AccountMgr::DeleteAccount(id);
            sLog->outBasic("Delete robot account %d - %s", id, userName.c_str());
        } while (accountQR->NextRow());
        return true;
    }

    return false;
}

bool RobotManager::RobotsDeleted()
{
    for (std::set<uint32>::iterator it = deleteRobotAccountSet.begin(); it != deleteRobotAccountSet.end(); it++)
    {
        QueryResult accountQR = LoginDatabase.PQuery("SELECT id FROM account where id = '%d'", (*it));
        if (accountQR)
        {
            sLog->outBasic("Account %d is under deleting", (*it));
            return false;
        }
        QueryResult characterQR = CharacterDatabase.PQuery("SELECT count(*) FROM characters where account = '%d'", (*it));
        if (characterQR)
        {
            Field* fields = characterQR->Fetch();
            uint32 count = fields[0].GetUInt32();
            if (count > 0)
            {
                sLog->outBasic("Characters for account %d are under deleting", (*it));
                return false;
            }
        }
    }

    sLog->outBasic("Robot accounts are deleted");
    return true;
}

bool RobotManager::CreateRobotAccount(std::string pmAccountName)
{
    QueryResult accountQR = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", pmAccountName.c_str());
    if (accountQR)
    {
        return false;
    }
    AccountOpResult aor = AccountMgr::CreateAccount(pmAccountName, "robot");
    sLog->outBasic("Create robot account %s", pmAccountName.c_str());

    return true;
}

bool RobotManager::CreateRobotCharacter(uint32 pmAccountID, uint8 pmCharacterClass, uint8 pmCharacterLevel)
{
    std::string currentName = "";
    bool nameValid = false;
    while (nameIndex < robotNameMap.size())
    {
        currentName = robotNameMap[nameIndex];
        QueryResult checkNameQR = CharacterDatabase.PQuery("SELECT count(*) FROM characters where name = '%s'", currentName.c_str());

        if (!checkNameQR)
        {
            sLog->outBasic("Name %s is available", currentName.c_str());
            nameValid = true;
        }
        else
        {
            Field* nameCountFields = checkNameQR->Fetch();
            uint32 nameCount = nameCountFields[0].GetUInt32();
            if (nameCount == 0)
            {
                nameValid = true;
            }
        }
        nameIndex++;
        if (nameValid)
        {
            break;
        }
    }
    if (!nameValid)
    {
        sLog->outError("No available names");
        return false;
    }

    uint8 race = 0, gender = 0, skin = 0, face = 0, hairStyle = 0, hairColor = 0, facialHair = 0;
    std::vector<uint8> targetRaces = availableRaces[pmCharacterClass];
    uint8 rdRaceIndex = urand(0, targetRaces.size() - 1);
    race = targetRaces[rdRaceIndex];
    uint8 destRace = race;
    while (true)
    {
        gender = urand(0, 1);
        face = urand(0, 5);
        hairStyle = urand(0, 5);
        hairColor = urand(0, 5);
        facialHair = urand(0, 5);
        WorldSession* eachSession = new WorldSession(pmAccountID, NULL, SEC_PLAYER, 2, 0, LOCALE_enUS, 0, false, false, 0);
        eachSession->isRobot = true;
        Player* newPlayer = new Player(eachSession);
        WorldPacket wp;
        CharacterCreateInfo* cci = new CharacterCreateInfo(currentName, destRace, pmCharacterClass, gender, skin, face, hairStyle, hairColor, facialHair, 0, wp);

        if (!newPlayer->Create(sObjectMgr->GenerateLowGuid(HighGuid::HIGHGUID_PLAYER), cci))
        {
            newPlayer->CleanupsBeforeDelete();
            delete eachSession;
            delete newPlayer;
            sLog->outError("Character create failed, %s %d %d", currentName.c_str(), race, pmCharacterClass);
            sLog->outString("Try again");
            continue;
        }
        newPlayer->GetMotionMaster()->Initialize();
        newPlayer->GiveLevel(pmCharacterLevel);
        newPlayer->setCinematic(2);
        newPlayer->SetAtLoginFlag(AT_LOGIN_NONE);
        newPlayer->SaveToDB(true, false);
        creatingCharacterGUIDSet.insert(newPlayer->GetGUIDLow());
        sWorld->AddSession(eachSession);
        sLog->outBasic("Create character %d - %s for account %d", newPlayer->GetGUIDLow(), currentName.c_str(), pmAccountID);
        break;
    }

    return true;
}

bool RobotManager::LoginRobot(uint32 pmAccountID, uint64 pmGUID)
{
    Player* currentPlayer = ObjectAccessor::FindPlayer(pmGUID);
    if (currentPlayer)
    {
        if (currentPlayer->IsInWorld())
        {
            sLog->outBasic("Robot %d %s is already in world", pmGUID, currentPlayer->GetName().c_str());
            return false;
        }
    }
    QueryResult characterQR = CharacterDatabase.PQuery("SELECT name, level FROM characters where guid = '%d'", pmGUID);
    if (!characterQR)
    {
        sLog->outError("Found zero robot characters for account %d while processing logging in", pmAccountID);
        return false;
    }
    Field* characterFields = characterQR->Fetch();
    std::string characterName = characterFields[0].GetString();
    uint8 characterLevel = characterFields[1].GetUInt8();
    WorldSession* loginSession = sWorld->FindSession(pmAccountID);
    if (!loginSession)
    {
        loginSession = new WorldSession(pmAccountID, NULL, SEC_PLAYER, 2, 0, LOCALE_enUS, 0, false, true, 0);
        loginSession->isRobot = true;
        sWorld->AddSession(loginSession);
    }
    loginSession->LoginPlayer(pmGUID);
    sLog->outBasic("Log in character %d %s (level %d)", pmGUID, characterName.c_str(), characterLevel);

    return true;
}

bool RobotManager::ProcessRobots()
{
    sLog->outBasic("Ready to process robots");
    std::unordered_map<uint32, WorldSession*> allSessionMap = sWorld->GetAllSessions();
    if (updateActiveRobotSessionMap)
    {
        updateActiveRobotSessionMap = false;
        activeRobotSessionMap.clear();
        for (std::unordered_map<uint32, WorldSession*>::iterator it = allSessionMap.begin(); it != allSessionMap.end(); it++)
        {
            if (it->second->GetPlayer())
            {
                if (it->second->isRobot)
                {
                    activeRobotSessionMap[activeRobotSessionMap.size()] = it->second;
                }
            }
        }
    }
    if (AdjustActiveRobots(allSessionMap))
    {
        updateActiveRobotSessionMap = true;
        sLog->outBasic("Active robots changed");
        sLog->outBasic("Next active robots check in 300 seconds");
        activeRobotsCheckDelay = 300000;
        return true;
    }
    for (std::unordered_map<uint32, WorldSession*>::iterator it = allSessionMap.begin(); it != allSessionMap.end(); it++)
    {
        if (it->second->isRobot)
        {
            Player* eachPlayer = it->second->GetPlayer();
            if (eachPlayer)
            {
                if (eachPlayer->IsInWorld())
                {
                    ProcessRobot(eachPlayer);
                }
            }
        }
    }
    sLog->outBasic("All robots processed.");
    return true;
}

bool RobotManager::ProcessRobot(Player* pmPlayer)
{
    if (!pmPlayer->rai)
    {
        pmPlayer->rai = new RobotAI(pmPlayer);
        std::ostringstream msgStream;
        msgStream << pmPlayer->GetName() << " activated";
        sWorld->SendServerMessage(ServerMessageType::SERVER_MSG_STRING, msgStream.str().c_str());
        pmPlayer->rai->Refresh();
        pmPlayer->rai->Prepare();
        pmPlayer->rai->RandomTeleport();
        return true;
    }
    pmPlayer->rai->Prepare();
    pmPlayer->SetPvP(true);
    Group* playerGroup = pmPlayer->GetGroup();
    if (playerGroup)
    {
        Player* master = pmPlayer->rai->masterPlayer;
        if (!master)
        {
            UninviteRobot(pmPlayer);
            pmPlayer->rai->Refresh();
            pmPlayer->rai->RandomTeleport();
        }
        else if (!master->IsInWorld())
        {
            UninviteRobot(pmPlayer);
            pmPlayer->rai->Refresh();
            pmPlayer->rai->RandomTeleport();
        }
        else if (!pmPlayer->IsInSameGroupWith(master))
        {
            UninviteRobot(pmPlayer);
            pmPlayer->rai->Refresh();
            pmPlayer->rai->RandomTeleport();
        }
    }
    else
    {
        if (!pmPlayer->IsAlive())
        {
            if (pmPlayer->rai->st_Solo_Normal->deathDuration > 60)
            {
                sLog->outBasic("Revive robot %s", pmPlayer->GetName().c_str());
                pmPlayer->rai->Refresh();
                pmPlayer->rai->RandomTeleport();
                pmPlayer->rai->st_Solo_Normal->deathDuration = 0;
            }
        }
    }

    return true;
}

bool RobotManager::AdjustActiveRobots(std::unordered_map<uint32, WorldSession*> pmActiveSessionMap)
{
    if (activeRobotsCheckDelay > 0)
    {
        return false;
    }
    std::set<uint32> activeRobotLevelSet;
    activeRobotLevelSet.clear();
    for (std::unordered_map<uint32, WorldSession*>::iterator it = pmActiveSessionMap.begin(); it != pmActiveSessionMap.end(); it++)
    {
        if (it->second->isRobot)
        {
            continue;
        }
        Player* eachPlayer = it->second->GetPlayer();
        if (eachPlayer)
        {
            if (eachPlayer->IsInWorld())
            {
                uint8 eachLevel = eachPlayer->getLevel();
                uint8 checkLevelRange = eachLevel;
                while (checkLevelRange <= eachLevel + 2)
                {
                    if (checkLevelRange >= 20)
                    {
                        if (activeRobotLevelSet.find(checkLevelRange) == activeRobotLevelSet.end())
                        {
                            activeRobotLevelSet.insert(checkLevelRange);
                        }
                    }
                    checkLevelRange++;
                }
            }
        }
    }
    // logout robots not in level range
    for (std::unordered_map<uint32, WorldSession*>::iterator it = pmActiveSessionMap.begin(); it != pmActiveSessionMap.end(); it++)
    {
        if (it->second->isRobot)
        {
            Player* eachPlayer = it->second->GetPlayer();
            if (eachPlayer)
            {
                if (eachPlayer->IsInWorld())
                {
                    uint8 eachLevel = eachPlayer->getLevel();
                    if (activeRobotLevelSet.find(eachLevel) == activeRobotLevelSet.end())
                    {
                        LogoutRobot(eachPlayer);
                    }
                }
            }
        }
    }
    for (std::set<uint32>::iterator levelIT = activeRobotLevelSet.begin(); levelIT != activeRobotLevelSet.end(); levelIT++)
    {
        for (int checkClass = Classes::CLASS_WARRIOR; checkClass <= Classes::CLASS_DRUID; checkClass++)
        {
            if (checkClass == 6 || checkClass == 10)
            {
                continue;
            }
            std::stringstream accountNameStream;
            accountNameStream << sRobotConfig->robotAccountNamePrefix << *levelIT << checkClass;
            std::string eachName = accountNameStream.str();
            QueryResult accountQR = LoginDatabase.PQuery("SELECT id FROM account where username = '%s'", eachName.c_str());
            if (!accountQR)
            {
                if (CreateRobotAccount(eachName))
                {
                    std::ostringstream msgStream;
                    msgStream << "Create account " << eachName;
                    sWorld->SendServerMessage(ServerMessageType::SERVER_MSG_STRING, msgStream.str().c_str());
                }
                continue;
            }
            Field* accountField = accountQR->Fetch();
            uint32 accountID = accountField[0].GetUInt32();
            QueryResult characterQR = CharacterDatabase.PQuery("SELECT guid FROM characters where account = %d", accountID);
            if (!characterQR)
            {
                if (CreateRobotCharacter(accountID, checkClass, *levelIT))
                {
                    std::ostringstream msgStream;
                    msgStream << "Create character for account " << accountID << " (class " << checkClass << ") (level " << *levelIT << ")";
                    sWorld->SendServerMessage(ServerMessageType::SERVER_MSG_STRING, msgStream.str().c_str());
                }
                continue;
            }
            Field* characterField = characterQR->Fetch();
            uint32 guid = characterField[0].GetUInt32();
            Player* currentPlayer = ObjectAccessor::FindPlayer(guid);
            if (currentPlayer)
            {
                if (currentPlayer->IsInWorld())
                {
                    continue;
                }
            }
            LoginRobot(accountID, guid);
        }
    }

    return true;
}

void RobotManager::Preparation()
{
    sLog->outBasic("Prepare robot manager");

    QueryResult robotNamesQR = WorldDatabase.Query("SELECT name FROM robot_names order by rand()");
    if (!robotNamesQR)
    {
        sLog->outError("Found zero robot names");
        sRobotConfig->enable = false;
        return;
    }
    do
    {
        Field* fields = robotNamesQR->Fetch();
        std::string eachName = fields[0].GetString();
        robotNameMap[robotNameMap.size()] = eachName;
    } while (robotNamesQR->NextRow());

    nameIndex = 0;

    managerState = 0;
    updateIndex = 0;
    checkDelay = 1000;
    activeRobotsCheckDelay = 300000;
    availableRaces[CLASS_WARRIOR].push_back(RACE_HUMAN);
    availableRaces[CLASS_WARRIOR].push_back(RACE_NIGHTELF);
    availableRaces[CLASS_WARRIOR].push_back(RACE_GNOME);
    availableRaces[CLASS_WARRIOR].push_back(RACE_DWARF);
    availableRaces[CLASS_WARRIOR].push_back(RACE_ORC);
    availableRaces[CLASS_WARRIOR].push_back(RACE_UNDEAD_PLAYER);
    availableRaces[CLASS_WARRIOR].push_back(RACE_TAUREN);
    availableRaces[CLASS_WARRIOR].push_back(RACE_TROLL);

    availableRaces[CLASS_PALADIN].push_back(RACE_HUMAN);
    availableRaces[CLASS_PALADIN].push_back(RACE_DWARF);

    availableRaces[CLASS_ROGUE].push_back(RACE_HUMAN);
    availableRaces[CLASS_ROGUE].push_back(RACE_DWARF);
    availableRaces[CLASS_ROGUE].push_back(RACE_NIGHTELF);
    availableRaces[CLASS_ROGUE].push_back(RACE_GNOME);
    availableRaces[CLASS_ROGUE].push_back(RACE_ORC);
    availableRaces[CLASS_ROGUE].push_back(RACE_TROLL);

    availableRaces[CLASS_PRIEST].push_back(RACE_HUMAN);
    availableRaces[CLASS_PRIEST].push_back(RACE_DWARF);
    availableRaces[CLASS_PRIEST].push_back(RACE_NIGHTELF);
    availableRaces[CLASS_PRIEST].push_back(RACE_TROLL);
    availableRaces[CLASS_PRIEST].push_back(RACE_UNDEAD_PLAYER);

    availableRaces[CLASS_MAGE].push_back(RACE_HUMAN);
    availableRaces[CLASS_MAGE].push_back(RACE_GNOME);
    availableRaces[CLASS_MAGE].push_back(RACE_UNDEAD_PLAYER);
    availableRaces[CLASS_MAGE].push_back(RACE_TROLL);

    availableRaces[CLASS_WARLOCK].push_back(RACE_HUMAN);
    availableRaces[CLASS_WARLOCK].push_back(RACE_GNOME);
    availableRaces[CLASS_WARLOCK].push_back(RACE_UNDEAD_PLAYER);
    availableRaces[CLASS_WARLOCK].push_back(RACE_ORC);

    availableRaces[CLASS_SHAMAN].push_back(RACE_ORC);
    availableRaces[CLASS_SHAMAN].push_back(RACE_TAUREN);
    availableRaces[CLASS_SHAMAN].push_back(RACE_TROLL);

    availableRaces[CLASS_HUNTER].push_back(RACE_DWARF);
    availableRaces[CLASS_HUNTER].push_back(RACE_NIGHTELF);
    availableRaces[CLASS_HUNTER].push_back(RACE_ORC);
    availableRaces[CLASS_HUNTER].push_back(RACE_TAUREN);
    availableRaces[CLASS_HUNTER].push_back(RACE_TROLL);

    availableRaces[CLASS_DRUID].push_back(RACE_NIGHTELF);
    availableRaces[CLASS_DRUID].push_back(RACE_TAUREN);

    robotAccountMap.clear();
    deleteRobotAccountSet.clear();
    creatingCharacterGUIDSet.clear();

    armorInventorySet.insert(InventoryType::INVTYPE_CHEST);
    armorInventorySet.insert(InventoryType::INVTYPE_FEET);
    armorInventorySet.insert(InventoryType::INVTYPE_HANDS);
    armorInventorySet.insert(InventoryType::INVTYPE_HEAD);
    armorInventorySet.insert(InventoryType::INVTYPE_LEGS);
    armorInventorySet.insert(InventoryType::INVTYPE_SHOULDERS);
    armorInventorySet.insert(InventoryType::INVTYPE_WAIST);
    armorInventorySet.insert(InventoryType::INVTYPE_WRISTS);

    miscInventoryMap[0] = InventoryType::INVTYPE_CLOAK;
    miscInventoryMap[1] = InventoryType::INVTYPE_FINGER;
    miscInventoryMap[2] = InventoryType::INVTYPE_FINGER;
    miscInventoryMap[3] = InventoryType::INVTYPE_NECK;

    classCharacterTypeNameMap.clear();
    classCharacterTypeNameMap[Classes::CLASS_WARRIOR][0] = "Arms";
    classCharacterTypeNameMap[Classes::CLASS_WARRIOR][1] = "Fury";
    classCharacterTypeNameMap[Classes::CLASS_WARRIOR][2] = "Protection";

    classCharacterTypeNameMap[Classes::CLASS_HUNTER][0] = "Beast Mastery";
    classCharacterTypeNameMap[Classes::CLASS_HUNTER][1] = "Marksmanship";
    classCharacterTypeNameMap[Classes::CLASS_HUNTER][2] = "Survival";

    classCharacterTypeNameMap[Classes::CLASS_SHAMAN][0] = "Elemental";
    classCharacterTypeNameMap[Classes::CLASS_SHAMAN][1] = "Enhancement";
    classCharacterTypeNameMap[Classes::CLASS_SHAMAN][2] = "Restoration";

    classCharacterTypeNameMap[Classes::CLASS_PALADIN][0] = "Holy";
    classCharacterTypeNameMap[Classes::CLASS_PALADIN][1] = "Protection";
    classCharacterTypeNameMap[Classes::CLASS_PALADIN][2] = "Retribution";

    classCharacterTypeNameMap[Classes::CLASS_WARLOCK][0] = "Affliction";
    classCharacterTypeNameMap[Classes::CLASS_WARLOCK][1] = "Demonology";
    classCharacterTypeNameMap[Classes::CLASS_WARLOCK][2] = "Destruction";

    classCharacterTypeNameMap[Classes::CLASS_PRIEST][0] = "Descipline";
    classCharacterTypeNameMap[Classes::CLASS_PRIEST][1] = "Holy";
    classCharacterTypeNameMap[Classes::CLASS_PRIEST][2] = "Shadow";

    classCharacterTypeNameMap[Classes::CLASS_ROGUE][0] = "Assassination";
    classCharacterTypeNameMap[Classes::CLASS_ROGUE][1] = "Combat";
    classCharacterTypeNameMap[Classes::CLASS_ROGUE][2] = "subtlety";

    classCharacterTypeNameMap[Classes::CLASS_MAGE][0] = "Arcane";
    classCharacterTypeNameMap[Classes::CLASS_MAGE][1] = "Fire";
    classCharacterTypeNameMap[Classes::CLASS_MAGE][2] = "Frost";

    classCharacterTypeNameMap[Classes::CLASS_DRUID][0] = "Balance";
    classCharacterTypeNameMap[Classes::CLASS_DRUID][1] = "Feral";
    classCharacterTypeNameMap[Classes::CLASS_DRUID][2] = "Restoration";

    meleeWeaponMap.clear();
    rangeWeaponMap.clear();
    armorMap.clear();
    miscMap.clear();
    uint8 levelRange = 0;
    ItemTemplateContainer const* its = sObjectMgr->GetItemTemplateStore();
    for (ItemTemplateContainer::const_iterator itr = its->begin(); itr != its->end(); ++itr)
    {
        if (itr->second.Quality < 2)
        {
            continue;
        }
        levelRange = itr->second.RequiredLevel / 10;
        if (itr->second.InventoryType == InventoryType::INVTYPE_CLOAK || itr->second.InventoryType == InventoryType::INVTYPE_FINGER || itr->second.InventoryType == InventoryType::INVTYPE_NECK)
        {
            miscMap[itr->second.InventoryType][levelRange][miscMap[itr->second.InventoryType][levelRange].size()] = itr->second.ItemId;
            continue;
        }
        if (itr->second.Class == ItemClass::ITEM_CLASS_WEAPON)
        {
            switch (itr->second.SubClass)
            {
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_STAFF:
            {
                meleeWeaponMap[0][levelRange][meleeWeaponMap[0][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_SWORD:
            {
                meleeWeaponMap[1][levelRange][meleeWeaponMap[1][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_SWORD2:
            {
                meleeWeaponMap[2][levelRange][meleeWeaponMap[2][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_DAGGER:
            {
                meleeWeaponMap[3][levelRange][meleeWeaponMap[3][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_AXE2:
            {
                meleeWeaponMap[5][levelRange][meleeWeaponMap[5][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_POLEARM:
            {
                meleeWeaponMap[6][levelRange][meleeWeaponMap[6][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_BOW:
            {
                rangeWeaponMap[0][levelRange][rangeWeaponMap[0][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_CROSSBOW:
            {
                rangeWeaponMap[0][levelRange][rangeWeaponMap[0][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_GUN:
            {
                rangeWeaponMap[0][levelRange][rangeWeaponMap[0][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_WAND:
            {
                rangeWeaponMap[1][levelRange][rangeWeaponMap[1][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassWeapon::ITEM_SUBCLASS_WEAPON_THROWN:
            {
                rangeWeaponMap[2][levelRange][rangeWeaponMap[2][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            default:
            {
                break;
            }
            }
        }
        else if (itr->second.Class == ItemClass::ITEM_CLASS_ARMOR)
        {
            switch (itr->second.SubClass)
            {
            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_CLOTH:
            {
                armorMap[0][itr->second.InventoryType][levelRange][armorMap[0][itr->second.InventoryType][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_LEATHER:
            {
                armorMap[1][itr->second.InventoryType][levelRange][armorMap[1][itr->second.InventoryType][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_MAIL:
            {
                armorMap[2][itr->second.InventoryType][levelRange][armorMap[2][itr->second.InventoryType][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_PLATE:
            {
                armorMap[3][itr->second.InventoryType][levelRange][armorMap[3][itr->second.InventoryType][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            case ItemSubclassArmor::ITEM_SUBCLASS_ARMOR_SHIELD:
            {
                meleeWeaponMap[4][levelRange][meleeWeaponMap[4][levelRange].size()] = itr->second.ItemId;
                continue;
            }
            default:
            {
                break;
            }
            }
        }
    }

    spellRewardClassQuestIDSet.clear();
    std::unordered_map<uint32, Quest*> allQuestMap = sObjectMgr->GetQuestTemplates();
    for (std::unordered_map<uint32, Quest*>::iterator it = allQuestMap.begin(); it != allQuestMap.end(); it++)
    {
        if (it->second->GetRequiredClasses() > 0)
        {
            if (it->second->GetRewSpellCast() > 0)
            {
                spellRewardClassQuestIDSet.insert(it->first);
            }
        }
    }

    teleportCacheMap.clear();
    QueryResult normalCreatureQR = WorldDatabase.Query("SELECT CT.maxlevel, C.map, C.position_x, C.position_y, C.position_z FROM creature_template CT join creature C on CT.entry = C.id where CT.rank = 0 and (C.map <> 0 or C.map <> 1 or C.map <> 530 or C.map <> 571)");
    if (normalCreatureQR)
    {
        do
        {
            Field* creatureField = normalCreatureQR->Fetch();
            uint8 level = creatureField[0].GetUInt8();
            level = level / 10;
            int mapID = creatureField[1].GetInt32();
            float x = creatureField[2].GetFloat();
            float y = creatureField[3].GetFloat();
            float z = creatureField[4].GetFloat();
            WorldLocation eachWL = WorldLocation(mapID, x, y, z, 0);
            teleportCacheMap[level][teleportCacheMap[level].size()] = eachWL;
        } while (normalCreatureQR->NextRow());
    }

    tamableBeastEntryMap.clear();
    CreatureTemplateContainer const* ctc = sObjectMgr->GetCreatureTemplates();
    for (CreatureTemplateContainer::const_iterator itr = ctc->begin(); itr != ctc->end(); ++itr)
    {
        if (itr->second.IsTameable(false))
        {
            tamableBeastEntryMap[tamableBeastEntryMap.size()] = itr->second.Entry;
        }
    }

    spellNameEntryMap.clear();
    for (uint32 i = 0; i < sSpellMgr->GetSpellInfoStoreSize(); ++i)
    {
        SpellInfo const* pSI = sSpellMgr->GetSpellInfo(i);
        if (!pSI)
        {
            continue;
        }
        spellNameEntryMap[pSI->SpellName[0]].insert(pSI->Id);
    }

    activeRobotSessionMap.clear();
    updateActiveRobotSessionMap = false;
}

void RobotManager::LogoutRobots()
{
    std::unordered_map<uint32, WorldSession*> allSessionMap = sWorld->GetAllSessions();
    for (std::unordered_map<uint32, WorldSession*>::iterator it = allSessionMap.begin(); it != allSessionMap.end(); it++)
    {
        if (it->second->isRobot)
        {
            Player* eachPlayer = it->second->GetPlayer();
            LogoutRobot(eachPlayer);
        }
    }
    managerState = 12;
}

bool RobotManager::LogoutRobot(Player* pmPlayer)
{
    if (pmPlayer)
    {
        if (pmPlayer->IsInWorld())
        {
            sLog->outBasic("Log out robot %s", pmPlayer->GetName().c_str());

            std::ostringstream msgStream;
            msgStream << pmPlayer->GetName() << " logged out";
            sWorld->SendServerMessage(ServerMessageType::SERVER_MSG_STRING, msgStream.str().c_str());

            pmPlayer->GetSession()->LogoutPlayer(true);
            return true;
        }
    }

    return false;
}

void RobotManager::UninviteRobot(Player* pmPlayer)
{
    if (pmPlayer->rai)
    {
        pmPlayer->rai->masterPlayer = NULL;
    }
    WorldPacket p;
    p << pmPlayer->GetName();
    pmPlayer->GetSession()->HandleGroupDisbandOpcode(p);
}

void RobotManager::HandlePlayerSay(Player* pmPlayer, std::string pmContent)
{
    std::vector<std::string> commandVector = sRobotManager->SplitString(pmContent, " ", true);
    std::string commandName = commandVector.at(0);
    if (commandName == "role")
    {
        std::ostringstream replyStream;
        if (commandVector.size() > 1)
        {
            std::string newRole = commandVector.at(1);
            if (newRole == "dps")
            {
                pmPlayer->groupRole = 0;
            }
            else if (newRole == "tank")
            {
                pmPlayer->groupRole = 1;
            }
            else if (newRole == "healer")
            {
                pmPlayer->groupRole = 2;
            }
        }

        replyStream << "Your group role is ";
        switch (pmPlayer->groupRole)
        {
        case 0:
        {
            replyStream << "dps";
            break;
        }
        case 1:
        {
            replyStream << "tank";
            break;
        }
        case 2:
        {
            replyStream << "healer";
            break;
        }
        default:
        {
            replyStream << "dps";
            break;
        }
        }

        sWorld->SendServerMessage(ServerMessageType::SERVER_MSG_STRING, replyStream.str().c_str(), pmPlayer);
    }
}

bool RobotManager::StringEndWith(const std::string &str, const std::string &tail)
{
    return str.compare(str.size() - tail.size(), tail.size(), tail) == 0;
}

bool RobotManager::StringStartWith(const std::string &str, const std::string &head)
{
    return str.compare(0, head.size(), head) == 0;
}

std::vector<std::string> RobotManager::SplitString(std::string srcStr, std::string delimStr, bool repeatedCharIgnored)
{
    std::vector<std::string> resultStringVector;
    std::replace_if(srcStr.begin(), srcStr.end(), [&](const char& c) {if (delimStr.find(c) != std::string::npos) { return true; } else { return false; }}/*pred*/, delimStr.at(0));
    size_t pos = srcStr.find(delimStr.at(0));
    std::string addedString = "";
    while (pos != std::string::npos) {
        addedString = srcStr.substr(0, pos);
        if (!addedString.empty() || !repeatedCharIgnored) {
            resultStringVector.push_back(addedString);
        }
        srcStr.erase(srcStr.begin(), srcStr.begin() + pos + 1);
        pos = srcStr.find(delimStr.at(0));
    }
    addedString = srcStr;
    if (!addedString.empty() || !repeatedCharIgnored) {
        resultStringVector.push_back(addedString);
    }
    return resultStringVector;
}

std::string RobotManager::TrimString(std::string srcStr)
{
    std::string result = srcStr;
    if (!result.empty())
    {
        result.erase(0, result.find_first_not_of(" "));
        result.erase(result.find_last_not_of(" ") + 1);
    }

    return result;
}

/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license: https://github.com/azerothcore/azerothcore-wotlk/blob/master/LICENSE-GPL2
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#ifndef MARKETER_CONFIG_H
#define MARKETER_CONFIG_H

#include <string>
#include <list>
#include <vector>
#include <ace/Singleton.h>
#include <ace/Configuration_Import_Export.h>
#include <ace/Thread_Mutex.h>
#include <AutoPtr.h>

class MarketerConfig
{
    friend class ACE_Singleton<MarketerConfig, ACE_Null_Mutex>;
    friend class ConfigLoader;

    MarketerConfig() { }
    ~MarketerConfig() { }

public:   
    bool Reload();

    std::string GetStringDefault(const char* name, const std::string& def, bool logUnused = true);
    bool GetBoolDefault(const char* name, bool def, bool logUnused = true);
    int GetIntDefault(const char* name, int def, bool logUnused = true);
    float GetFloatDefault(const char* name, float def, bool logUnused = true);

    std::list<std::string> GetKeysByString(std::string const& name);

    bool isDryRun() { return this->dryRun; }
    void setDryRun(bool mode) { this->dryRun = mode; }

    void LoadMarketerConfig();

    uint8 enable;
    uint8 reset;
    uint32 allianceFaction;
    uint32 neutralFaction;
    uint32 hordeFaction;
    uint64 allianceAuctioneerGUID;
    uint64 neutralAuctioneerGUID;
    uint64 hordeAuctioneerGUID;

private:
    /// Method used only for loading main configuration files (authserver.conf and worldserver.conf)
    bool LoadInitial(char const* file);

    /**
     * This method loads additional configuration files
     * It is recommended to use this method in WorldScript::OnConfigLoad hooks
     *
     * @return true if loading was successful
     */
    bool LoadMore(char const* file);

    bool dryRun = false;

    bool GetValueHelper(const char* name, ACE_TString &result);
    bool LoadData(char const* file);

    typedef ACE_Thread_Mutex LockType;
    typedef ACE_Guard<LockType> GuardType;

    std::vector<std::string> _confFiles;
    Trinity::AutoPtr<ACE_Configuration_Heap, ACE_Null_Mutex> _config;
    LockType _configLock;

    MarketerConfig(MarketerConfig const&);
    MarketerConfig& operator=(MarketerConfig const&);
};

#define sMarketerConfig ACE_Singleton<MarketerConfig, ACE_Null_Mutex>::instance()

#endif

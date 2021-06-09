/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2019 Sky UK
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
/*
 * File:   Netfilter.h
 *
 */
#ifndef NETFILTER_H
#define NETFILTER_H

#include <map>
#include <list>
#include <string>
#include <mutex>


// -----------------------------------------------------------------------------
/**
 *  @class Netfilter
 *  @brief Class that can read / write iptables rule sets
 *
 *  There is no programming API for iptables, so this class uses the
 *  iptables-save and iptables-restore cmdline tools for reading and writing
 *  the rules.
 *
 *  TODO: replace use of iptables-save and iptables-restore with libiptc
 *
 */
class Netfilter
{
public:
    Netfilter();
    ~Netfilter() = default;

public:
    enum class TableType { Invalid, Raw, Nat, Mangle, Filter, Security };
    typedef std::map<TableType, std::list<std::string>> RuleSet;

    RuleSet rules(const int ipVersion) const;
    bool setRules(const RuleSet &ruleSet, const int ipVersion);

    enum class Operation { Append, Insert, Delete, Unchanged };

    bool addRules(RuleSet &ruleSet, const int ipVersion, Operation operation);

    bool createNewChain(TableType table, const std::string &name,
                        const int ipVersion);

    bool applyRules(const int ipVersion);

private:
    bool forkExec(const std::string &file,
                  const std::list<std::string> &args,
                  int stdinFd, int stdoutFd, int stderrFd) const;

    bool writeString(int fd, const std::string &str) const;

    RuleSet getRuleSet(const int ipVersion) const;

    bool ruleInList(const std::string &rule,
                    const std::list<std::string> &rulesList) const;

    typedef struct RuleSets
    {
        RuleSet appendRuleSet;
        RuleSet insertRuleSet;
        RuleSet deleteRuleSet;
        RuleSet unchangedRuleSet;
    } RuleSets;

    RuleSets mIpv4RuleCache;
    RuleSets mIpv6RuleCache;

    void trimDuplicates(RuleSet &existing, RuleSet &newRuleSet,
                        Operation operation) const;
    bool checkDuplicates(RuleSets ruleCache, const int ipVersion) const;

    void dump(const RuleSet &ruleSet, const char *title = nullptr) const;

    typedef struct IptablesVersion
    {
        int major;
        int minor;
        int patch;
    } IptablesVersion;

    IptablesVersion getIptablesVersion() const;

private:
    mutable std::mutex mLock;
    IptablesVersion mIptablesVersion;
};

#endif // !defined(NETFILTER_H)

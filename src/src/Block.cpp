/*
 callblocker - blocking unwanted calls from your home phone
 Copyright (C) 2015-2015 Patrick Ammann <pammann@gmx.net>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "Block.h" // API

#include <json-c/json.h>
#include <boost/algorithm/string/predicate.hpp>

#include "Logger.h"
#include "Helper.h"


Block::Block(Settings* pSettings) {
  Logger::debug("Block::Block()...");
  m_pSettings = pSettings;

  m_pWhitelists = new FileLists(SYSCONFDIR "/" PACKAGE_NAME "/configs/whitelists");
  m_pBlacklists = new FileLists(SYSCONFDIR "/" PACKAGE_NAME "/configs/blacklists");
}

Block::~Block() {
  Logger::debug("Block::~Block()...");

  delete m_pWhitelists;
  m_pWhitelists = NULL;
  delete m_pBlacklists;
  m_pBlacklists = NULL;
}

void Block::run() {
  m_pWhitelists->run();
  m_pBlacklists->run();
}

bool Block::isAnonymousNumberBlocked(const struct SettingBase* pSettings, std::string* pMsg) {
  bool blockenabled = pSettings->blockAnonymousCID;
  bool block = false;
  
  // Incoming call number='anonymous' [blocked]
  std::ostringstream oss;
  oss << "Incoming call: number='anonymous'";
  if (blockenabled) {
    std::string script = "/usr/callblocker/scripts/anonymous.py";
    std::string parameters = "--number anonymous";
  
    std::string res;
    if (!Helper::executeCommand(script + " " + parameters + " 2>&1", &res)) {
      return blockenabled; // script failed, error already logged
    }
    
    struct json_object* root;
	root = json_tokener_parse(res.c_str());
	
    if (!Helper::getObject(root, "block", true, "script result", &block)) {
      return blockenabled;
    }
  }
	
  if (block)
  {
    oss << " blocked";
  }

  *pMsg = oss.str();
  return block;
}

bool Block::isNumberBlocked(const struct SettingBase* pSettings, const std::string& rNumber, std::string* pMsg) {
  Logger::debug("Block::isNumberBlocked(%s,number=%s)", pSettings->toString().c_str(), rNumber.c_str());

  std::string listName = "";
  std::string callerName = "";
  std::string score = "";
  bool onWhitelist = false;
  bool onBlacklist = false;
  bool block = false;

  switch (pSettings->blockMode) {
    default:
      Logger::warn("invalid block mode %d", pSettings->blockMode);
    case LOGGING_ONLY:
      if (isWhiteListed(pSettings, rNumber, &listName, &callerName)) {
        onWhitelist = true;
        break;
      }
      if (isBlacklisted(pSettings, rNumber, &listName, &callerName, &score)) {
        onBlacklist = true;
        break;
      }
      break;

    case WHITELISTS_ONLY:
      if (isWhiteListed(pSettings, rNumber, &listName, &callerName)) {
        onWhitelist = true;
        break;
      }
      block = true;
      break;

    case WHITELISTS_AND_BLACKLISTS:
      if (isWhiteListed(pSettings, rNumber, &listName, &callerName)) {
        onWhitelist = true;
        break;
      }
      if (isBlacklisted(pSettings, rNumber, &listName, &callerName, &score)) {
        onBlacklist = true;
        block = true;
        break;
      }
      break;

    case BLACKLISTS_ONLY:
      if (isBlacklisted(pSettings, rNumber, &listName, &callerName, &score)) {
        onBlacklist = true;
        block = true;
        break;
      }
      break;
  }

  if (!onWhitelist && !onBlacklist) {
    // online lookup caller name
    if (pSettings->onlineLookup.length() != 0) {
      struct json_object* root;
      if (checkOnline("onlinelookup_", pSettings->onlineLookup, rNumber, &root)) {
        (void)Helper::getObject(root, "name", false, "script result", &callerName);
      }
    }
  }

  // Incoming call number='x' name='y' [blocked] [whitelist='w'] [blacklist='b'] [score=s]
  std::ostringstream oss;
  oss << "Incoming call: number='" << rNumber << "'";
  if (callerName.length() != 0) {
    oss << " name='" << Helper::escapeSqString(callerName) << "'";
  }
  if (block) {
    oss << " blocked";
  }
  if (onWhitelist) {
    oss << " whitelist='" << listName << "'";
  }
  if (onBlacklist) {
    oss << " blacklist='" << listName << "'";
  }
  if (score.length() != 0) {
    oss << " score=" << score;
  }

  *pMsg = oss.str();
  return block;
}

bool Block::isWhiteListed(const struct SettingBase* pSettings, const std::string& rNumber,
                          std::string* pListName, std::string* pCallerName) {
  return m_pWhitelists->isListed(rNumber, pListName, pCallerName);
}

bool Block::isBlacklisted(const struct SettingBase* pSettings, const std::string& rNumber,
                          std::string* pListName, std::string* pCallerName, std::string* pScore) {
  if (m_pBlacklists->isListed(rNumber, pListName, pCallerName)) {
    return true;
  }

  // online check if spam
  if (pSettings->onlineCheck.length() != 0) {
    struct json_object* root;
    if (checkOnline("onlinecheck_", pSettings->onlineCheck, rNumber, &root)) {
      bool spam;
      if (!Helper::getObject(root, "spam", true, "script result", &spam)) {
        return false;
      }
      if (spam) {
        *pListName = pSettings->onlineCheck;
        (void)Helper::getObject(root, "name", false, "script result", pCallerName);
        int score;
        if (Helper::getObject(root, "score", false, "script result", &score)) {
          *pScore = std::to_string(score);
        }
        return true;
      }
    }
  }

  // no spam
  return false;
}

bool Block::checkOnline(std::string prefix, std::string scriptBaseName, const std::string& rNumber, struct json_object** root) {
  if (boost::starts_with(rNumber, "**")) {
    // it is an intern number, thus makes no sense to ask the world
    return false;
  }

  std::string script = "/usr/callblocker/scripts/" + prefix + scriptBaseName + ".py";
  std::string parameters = "--number " + rNumber;
  std::vector<struct SettingOnlineCredential> creds = m_pSettings->getOnlineCredentials();
  for(size_t i = 0; i < creds.size(); i++) {
    struct SettingOnlineCredential* cred = &creds[i];
    if (cred->name == scriptBaseName) {
      for (std::map<std::string,std::string>::iterator it = cred->data.begin(); it != cred->data.end(); ++it) {
        parameters += " --" + it->first + "=" + it->second;
      }
      break;
    }
  }

  std::string res;
  if (!Helper::executeCommand(script + " " + parameters + " 2>&1", &res)) {
    return false; // script failed, error already logged
  }

  *root = json_tokener_parse(res.c_str());
  return true; // script executed successful
}


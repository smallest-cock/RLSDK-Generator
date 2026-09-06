#include "pch.hpp"
#include "Engine.hpp"

/*
# ========================================================================================= #
# Engine
# ========================================================================================= #
*/

std::string GEngine::m_name    = "RLSDKGenerator";
std::string GEngine::m_version = "v1.1.5";
std::string GEngine::m_credits = "ItsBranK, TheFeckless, SSLow";
std::string GEngine::m_links   = "www.github.com/smallest-cock/RLSDK-Generator, discord.gg/d5ahhQmJbJ";

const std::string &GEngine::GetName() { return m_name; }
const std::string &GEngine::GetVersion() { return m_version; }
const std::string &GEngine::GetCredits() { return m_credits; }
const std::string &GEngine::GetLinks() { return m_links; }

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/

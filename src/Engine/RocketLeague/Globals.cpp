#include "Globals.hpp"
#include "GameDefines.hpp"
#include "../../Framework/Printer.hpp"
#include <format>

GlobalsManager g_Globals{};

// =========================================================================================

// If you add a value to the EGlobalVar enum, make sure to update these accordingly
const std::unordered_map<EGlobalVar, std::string> GlobalsManager::m_enumNames = {
    {EGlobalVar::GMalloc, "GMalloc"},
    {EGlobalVar::GNames, "GNames"},
    {EGlobalVar::GObjects, "GObjects"},
};

const std::unordered_map<EGlobalVar, std::string> GlobalsManager::m_enumTypes = {
    {EGlobalVar::GMalloc, "GMallocWrapper"},
    {EGlobalVar::GNames, "class TArray<class FNameEntry*>*"},
    {EGlobalVar::GObjects, "class TArray<class UObject*>*"},
};

// =========================================================================================

std::unordered_map<EGlobalVar, uintptr_t> GlobalsManager::m_scanResults;

// Only GNames and GObjects need to be valid for SDK generation to work
// ... but you can also add validation functions for other globals here if you want
std::unordered_map<EGlobalVar, std::function<bool(uintptr_t)>> GlobalsManager::m_validators = {
    {EGlobalVar::GNames,
        [](uintptr_t address) -> bool
        {
	        if (address && !FName::Names()->empty() && (FName::Names()->capacity() > FName::Names()->size()))
		        return true;
	        return false;
        }},
    {EGlobalVar::GObjects,
        [](uintptr_t address) -> bool
        {
	        if (address && !UObject::GObjObjects()->empty() && (UObject::GObjObjects()->capacity() > UObject::GObjObjects()->size()))
		        return true;
	        return false;
        }}};

bool GlobalsManager::initGlobals()
{
	if (!initPatterns())
		return false;
	if (!resolveAllAddresses())
		return false;
	setGlobals();
	return checkGlobals();
}

// Sets the globals needed for SDK generation, which is only GNames and GObjects for now
void GlobalsManager::setGlobals()
{
	using GNames_t   = class TArray<class FNameEntry*>*;
	using GObjects_t = class TArray<class UObject*>*;

	GNames   = reinterpret_cast<GNames_t>(getAddress(EGlobalVar::GNames));
	GObjects = reinterpret_cast<GObjects_t>(getAddress(EGlobalVar::GObjects));
	GMalloc  = reinterpret_cast<void*>(getAddress(EGlobalVar::GMalloc));
}

bool GlobalsManager::checkGlobals()
{
	for (const auto& [var, address] : m_addresses)
	{
		auto it = m_validators.find(var);
		if (it == m_validators.end())
			continue;

		if (!it->second(address))
			return false;
	}
	return true;
}

bool GlobalsManager::initPatterns()
{
	for (const auto& [id, sig] : m_patternStrings)
	{
		Memory::PatternData data;
		if (!Memory::parseSig(sig, data))
			return false;

		m_patterns.emplace(id, std::move(data));
	}
	return true;
}

bool GlobalsManager::resolveAllAddresses()
{
	// Step 1: get initial addresses, either from offsets or pattern scans
	uintptr_t gameBase = Memory::getBaseAddress();
	// std::unordered_map<EGlobalVar, uintptr_t> rawScanResults;

	// 1. apply offsets
	for (const auto& [id, offset] : m_offsets)
		m_scanResults[id] = gameBase + offset;

	// 2. scan patterns and update m_scanResults (patterns take precedence over offsets if there's overlap)
	for (const auto& [id, pattern] : m_patterns)
	{
		if (pattern.length > 0)
			m_scanResults[id] = Memory::findPattern(pattern.arrayOfBytes, pattern.mask);
	}

	// Step 2: apply any address resolver functions
	for (auto& [id, addr] : m_scanResults)
	{
		auto resolverIt = m_resolvers.find(id);
		if (resolverIt != m_resolvers.end())
			addr = resolverIt->second(addr);

		m_addresses[id] = addr;
	}

	return true;
}

uintptr_t GlobalsManager::getAddress(EGlobalVar var) const
{
	auto it = m_addresses.find(var);
	return (it != m_addresses.end()) ? it->second : 0;
}

uintptr_t GlobalsManager::getOffset(EGlobalVar global) const
{
	auto it = m_addresses.find(global);
	if (it == m_addresses.end() || !it->second)
		return 0;

	if (it->second < Memory::getBaseAddress())
		return 0;

	return (it->second - Memory::getBaseAddress());
}

std::string GlobalsManager::generateOffsetDefines() const
{
	std::string out;

	size_t maxNameLen = getMaxDefineNameLength("_OFFSET");
	for (const auto& pair : GlobalsManager::m_enumNames)
	{
		std::string defineLine = generateOffsetDefine(pair.first, maxNameLen);
		if (!defineLine.empty())
			out += defineLine;
	}
	out += "\n";

	return out;
}

std::string GlobalsManager::generateOffsetDefine(EGlobalVar var, size_t maxNameLen) const
{
	auto it = m_enumNames.find(var);
	if (it == m_enumNames.end())
		return {};

	const std::string& name   = it->second;
	uintptr_t          offset = getOffset(var);

	return std::format("#define {0:<{1}} static_cast<uintptr_t>({2})\n",
	    Printer::ToUpper(name) + "_OFFSET",
	    maxNameLen + 2, // +2 for extra spacing between name and value
	    Printer::Hex(offset, sizeof(uintptr_t)));
}

std::string GlobalsManager::generateSignatureDefines() const
{
	std::string out;

	size_t maxNameLen = getMaxDefineNameLength("_SIG");
	for (const auto& pair : m_enumNames)
	{
		std::string defineLine = generateSignatureDefine(pair.first, maxNameLen);
		if (!defineLine.empty())
			out += defineLine;
	}
	out += "\n";

	return out;
}

std::string GlobalsManager::generateSignatureDefine(EGlobalVar var, size_t maxNameLen) const
{
	auto it = m_enumNames.find(var);
	if (it == m_enumNames.end())
		return {};

	const std::string& name = it->second;

	// get the actual pattern string (or empty if none)
	auto patternIt = m_patternStrings.find(var);
	if (patternIt == m_patternStrings.end() || patternIt->second.empty())
		return {};

	const std::string& sig = patternIt->second;

	return std::format("#define {0:<{1}} \"{2}\"\n", Printer::ToUpper(name) + "_SIG", maxNameLen + 2, sig);
}

// Find max width of enum strings + suffix for pretty printing
size_t GlobalsManager::getMaxDefineNameLength(const std::string& suffix) const
{
	size_t maxLen = 0;
	for (const auto& [var, str] : m_enumNames)
		maxLen = max(maxLen, str.size() + suffix.size());
	return maxLen;
}

void GlobalsManager::generateExternDeclarations(std::ofstream& definesFile)
{
	for (const auto& pair : m_enumNames)
	{
		auto it = m_enumTypes.find(pair.first);
		if (it == m_enumTypes.end())
			continue;

		definesFile << std::format("extern {} {};\n", it->second, pair.second);
	}
}

void GlobalsManager::generateExternDeclaration(std::ofstream& definesFile, EGlobalVar global)
{
	auto nameIt = m_enumNames.find(global);
	if (nameIt == m_enumNames.end())
		return;

	auto it = m_enumTypes.find(global);
	if (it == m_enumTypes.end())
		return;

	definesFile << std::format("extern {} {};\n", it->second, nameIt->second);
}

void GlobalsManager::generateDeclarations(std::ofstream& definesFile)
{
	for (const auto& pair : m_enumNames)
	{
		auto it = m_enumTypes.find(pair.first);
		if (it == m_enumTypes.end())
			continue;

		definesFile << std::format("{} {}{{}};\n", it->second, pair.second);
	}
	definesFile << "\n";
}

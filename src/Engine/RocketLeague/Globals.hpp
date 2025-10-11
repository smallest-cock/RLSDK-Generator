#pragma once
#include <cstddef>
#include <unordered_map>
#include <functional>
#include <string>
#include <cstdint>
#include "../Pattern.hpp"

// If you add values to this enum, make sure to update m_enumNames and m_enumTypes accordingly in Globals.cpp
enum class EGlobalVar
{
	GMalloc,
	GNames,
	GObjects,
};

using OffsetsList      = std::unordered_map<EGlobalVar, uintptr_t>;
using PatternsList     = std::unordered_map<EGlobalVar, std::string>;
using AddressResolvers = std::unordered_map<EGlobalVar, std::function<uintptr_t(uintptr_t)>>;

class GlobalsManager
{
public:
	GlobalsManager() = default;

public:
	// Static, user-configurable variables
	static bool m_useOffsetsInFinalSDK;
	static bool m_dumpOffsets;

	static PatternsList     m_patternStrings;
	static OffsetsList      m_offsets;
	static AddressResolvers m_resolvers;

	bool        initGlobals();
	bool        checkGlobals();
	bool        initPatterns();        // parse pretty pattern srings from m_patternStrings into useable data
	bool        resolveAllAddresses(); // apply offsets, do scans, etc.
	uintptr_t   getAddress(EGlobalVar var) const;
	uintptr_t   getOffset(EGlobalVar var) const;
	std::string generateOffsetDefines() const;
	std::string generateSignatureDefines() const;
	void        generateExternDeclarations(std::ofstream& definesFile);
	void generateExternDeclaration(std::ofstream& definesFile, EGlobalVar); // for finer control over where declarations are placed in
	                                                                        // GameDefines.hpp
	void generateDeclarations(std::ofstream& definesFile);

private:
	std::string generateOffsetDefine(EGlobalVar var, size_t maxNameLen) const;
	std::string generateSignatureDefine(EGlobalVar var, size_t maxNameLen) const;
	size_t      getMaxDefineNameLength(const std::string& suffix) const;
	void        setGlobals();

private:
	static const std::unordered_map<EGlobalVar, std::string>              m_enumNames;
	static const std::unordered_map<EGlobalVar, std::string>              m_enumTypes;
	static std::unordered_map<EGlobalVar, std::function<bool(uintptr_t)>> m_validators;
	static std::unordered_map<EGlobalVar, uintptr_t>                      m_scanResults;
	std::unordered_map<EGlobalVar, Memory::PatternData>                   m_patterns;
	std::unordered_map<EGlobalVar, uintptr_t>                             m_addresses;
};

extern GlobalsManager g_Globals;
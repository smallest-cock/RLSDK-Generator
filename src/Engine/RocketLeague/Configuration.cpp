#include "Configuration.hpp"

/*
#
=========================================================================================
# # Game Info #
=========================================================================================
#
*/

bool                  GConfig::addTimestampToGameVersion      = true;
bool                  GConfig::addTimestampToOutputFolderName = true;
std::string           GConfig::m_gameNameLong                 = "Rocket League";
std::string           GConfig::m_gameNameShort                = "RLSDK";
std::string           GConfig::m_gameVersion                  = "Season 20 (v2.56)"; // <-- update this every game update
std::string           GConfig::outputFolderName               = m_gameNameShort;
std::filesystem::path GConfig::m_outputPathParentDir          = "C:\\folder\\path\\where\\you\\want\\the\\SDK\\generated";
std::filesystem::path GConfig::m_outputPath                   = m_outputPathParentDir / outputFolderName;

const std::string&           GConfig::GetGameNameLong() { return m_gameNameLong; }
const std::string&           GConfig::GetGameNameShort() { return m_gameNameShort; }
const std::string&           GConfig::GetGameVersion() { return m_gameVersion; }
const std::filesystem::path& GConfig::GetOutputPath() { return m_outputPath; }
bool                         GConfig::HasOutputPath()
{
	return (!GetOutputPath().string().empty() &&
	        (GetOutputPath().string().find("C:\\folder\\path\\where\\you\\want\\the\\SDK\\generated") != 0));
}

void GConfig::AddTimestampToGameVersion(const std::string& timestamp)
{
	if (addTimestampToGameVersion)
		m_gameVersion += " " + timestamp;
}

void GConfig::SetOutputFolderName(const std::string& dateStr)
{
	// add date if necessary
	if (addTimestampToOutputFolderName)
		outputFolderName += "__" + dateStr;

	// replace spaces with underscores
	for (char& c : outputFolderName)
	{
		if (c == ' ')
			c = '_';
	}

	// update output path
	m_outputPath = m_outputPathParentDir / outputFolderName;
}

/*
#
=========================================================================================
# # Global Objects & Names #
=========================================================================================
#
*/

/*
m_useGnamesPatternForOffsets - If you want to use a pattern to find GNames address, and calculate GObjects as GNames + 0x48

m_dumpOffsets - Dump GNames/GObjects offsets and generation times to a file in the output directory

m_useOffsets - If want to use offsets or patterns to initialize global objects and names

m_gnameMask - Half byte mask, use question marks for unknown data

m_gobjectMask - Half byte mask, use question marks for unknown data

m_gnamePattern - First value is actual hex escaped pattern, second value is string version of it printed in the final sdk

m_gobjectPattern - First value is actual hex escaped pattern, second value is string version of it printed in the final sdk
*/

bool GConfig::m_useGnamesPatternForOffsets = true;
bool GConfig::m_useOffsets                 = false;
bool GConfig::m_dumpOffsets                = true;

uintptr_t GConfig::m_gnameOffset   = 0xFFFFFFF; // <--- should be updated every game update if using offsets
uintptr_t GConfig::m_gobjectOffset = 0xFFFFFFF; // <--- should be updated every game update if using offsets

std::pair<uint8_t*, std::string> GConfig::m_gnamePattern = {
    (uint8_t*)"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x35\x25\x02\x00", ""};
std::string GConfig::m_gnameMask = "??????xx??xxxxxx";

std::pair<uint8_t*, std::string> GConfig::m_gobjectPattern = {(uint8_t*)"", ""};
std::string                      GConfig::m_gobjectMask    = "";

bool               GConfig::UsingGnamesPatternForOffsets() { return m_useGnamesPatternForOffsets; }
bool               GConfig::DumpOffsets() { return m_dumpOffsets; }
bool               GConfig::UsingOffsets() { return m_useOffsets; }
uintptr_t          GConfig::GetGObjectOffset() { return m_gobjectOffset; }
uintptr_t          GConfig::GetGObjectsAddress() { return m_calculatedGobjectsAddress; }
uint8_t*           GConfig::GetGObjectPattern() { return m_gobjectPattern.first; }
const std::string& GConfig::GetGObjectStr() { return m_gobjectPattern.second; }
const std::string& GConfig::GetGObjectMask() { return m_gobjectMask; }
uintptr_t          GConfig::GetGNameOffset() { return m_gnameOffset; }
uintptr_t          GConfig::GetGNamesAddress() { return m_calculatedGnamesAddress; }
uint8_t*           GConfig::GetGNamePattern() { return m_gnamePattern.first; }
const std::string& GConfig::GetGNameStr() { return m_gnamePattern.second; }
const std::string& GConfig::GetGNameMask() { return m_gnameMask; }
void               GConfig::SetGNamesOffset(uintptr_t offset) { m_gnameOffset = offset; }
void               GConfig::SetGNamesAddress(uintptr_t address) { m_calculatedGnamesAddress = address; }
void               GConfig::SetGObjectsOffset(uintptr_t offset) { m_gobjectOffset = offset; }
void               GConfig::SetGObjectsAddress(uintptr_t address) { m_calculatedGobjectsAddress = address; }
void               GConfig::SetUseOffsets(bool useOffsets) { m_useOffsets = useOffsets; }

uintptr_t GConfig::m_calculatedGnamesAddress   = NULL;
uintptr_t GConfig::m_calculatedGobjectsAddress = NULL;

/*
#
=========================================================================================
# # Generator Settings #
=========================================================================================
#
*/

/*
m_createPchCopy - If set to true, a copy of the generated SDK will be created (after generation) with "pch.h" includes in all .cpp files

m_useWindows - If set to true this will auto include "Windows.h" in your sdk, along with renaming some windows functions that could conflict
with windows macros.

m_useConstants - If you want to use objects internal integer for finding static classes and functions, note that these will change every
time the game updates.

m_removeNativeIndex - If you want to remove the "iNative" index on functions before calling process event.

m_removeNativeFlags - If you want to remove the "FUNC_Native" flag on functions before calling process event.

m_printEnumFlags - If you want the EFunctionFlags, EPropertyFlags, and EObjectFlags enums so be printed in the final sdk.

m_useEnumClasses - If you want to use strongly typed enum classes over normal ones.

m_enumClassType - Underlying enum type if you set "m_useEnumClasses" to true.

m_gameAlignment - Used to calculate property sizes and missed offsets.

m_finalAlignment - Forced alignment used in the final sdk.

m_blacklistedTypes - Names of classes or structs you don't want generated in the final sdk.

m_typeOverrides - Names of classes or structs you want to override with your own custom one.
*/

bool        GConfig::m_createPchCopy     = true;
bool        GConfig::m_useWindows        = true;
bool        GConfig::m_useConstants      = false;
bool        GConfig::m_removeNativeIndex = false;
bool        GConfig::m_removeNativeFlags = false;
bool        GConfig::m_printEnumFlags    = true;
bool        GConfig::m_useEnumClasses    = true;
std::string GConfig::m_enumClassType     = "uint8_t";
uint32_t    GConfig::m_gameAlignment     = EAlignment::NONE;
uint32_t    GConfig::m_finalAlignment    = EAlignment::NONE;

std::vector<std::string> GConfig::m_blacklistedTypes = {"UGroup_ORS", "FPointer"}; // FPointer gets re-defined in Core_structs.hpp
                                                                                   // and it breaks everything motherfucker

std::map<std::string, const char*> GConfig::m_typeOverrides = {};

bool               GConfig::CreatePchCopy() { return m_createPchCopy; }
bool               GConfig::UsingWindows() { return m_useWindows; }
bool               GConfig::UsingConstants() { return m_useConstants; }
bool               GConfig::RemoveNativeIndex() { return m_removeNativeIndex; }
bool               GConfig::RemoveNativeFlags() { return m_removeNativeFlags; }
bool               GConfig::PrintEnumFlags() { return m_printEnumFlags; }
bool               GConfig::UsingEnumClasses() { return m_useEnumClasses; }
const std::string& GConfig::GetEnumClassType() { return m_enumClassType; }
uint32_t           GConfig::GetGameAlignment() { return m_gameAlignment; }
uint32_t           GConfig::GetFinalAlignment() { return m_finalAlignment; }

bool GConfig::IsTypeBlacklisted(const std::string& name)
{
	if (m_blacklistedTypes.empty() || name.empty())
		return false;

	return (std::find(m_blacklistedTypes.begin(), m_blacklistedTypes.end(), name) != m_blacklistedTypes.end());
}

bool GConfig::IsTypeOveridden(const std::string& name)
{
	if (name.empty())
		return false;

	return m_typeOverrides.contains(name);
}

std::string GConfig::GetTypeOverride(const std::string& name)
{
	auto it = m_typeOverrides.find(name);
	if (it != m_typeOverrides.end())
		return it->second;
	else
		return "";
}

/*
#
=========================================================================================
# # Process Event #
=========================================================================================
#
*/

/*
m_useIndex - If you want to use "m_peIndex" change this to true, if not virutal voids will be generated from "m_peMask" and "m_pePattern".

m_peIndex - Position where the process event function is in UObject's VfTable.

m_peMask - Half byte mask, use question marks for unknown data.

m_pePattern - First value is the actual hex escaped pattern, second value is the string version of it printed in the final sdk.
*/

bool                             GConfig::m_useIndex  = true;
int32_t                          GConfig::m_peIndex   = 67;
std::string                      GConfig::m_peMask    = "";
std::pair<uint8_t*, std::string> GConfig::m_pePattern = {(uint8_t*)"", ""};

bool               GConfig::UsingProcessEventIndex() { return (m_useIndex && (m_peIndex != -1)); }
int32_t            GConfig::GetProcessEventIndex() { return m_peIndex; }
const std::string& GConfig::GetProcessEventMask() { return m_peMask; }
uint8_t*           GConfig::GetProcessEventPattern() { return m_pePattern.first; }
const std::string& GConfig::GetProcessEventStr() { return m_pePattern.second; }

/*
#
=========================================================================================
# # Cosmetics #
=========================================================================================
#
*/

uint32_t GConfig::m_constantSpacing = 50;
uint32_t GConfig::m_commentSpacing  = 30;
uint32_t GConfig::m_enumSpacing     = 50;
uint32_t GConfig::m_classSpacing    = 50;
uint32_t GConfig::m_structSpacing   = 50;
uint32_t GConfig::m_functionSpacing = 50;

uint32_t GConfig::GetConstSpacing() { return m_constantSpacing; }
uint32_t GConfig::GetCommentSpacing() { return m_commentSpacing; }
uint32_t GConfig::GetEnumSpacing() { return m_enumSpacing; }
uint32_t GConfig::GetClassSpacing() { return m_classSpacing; }
uint32_t GConfig::GetStructSpacing() { return m_structSpacing; }
uint32_t GConfig::GetFunctionSpacing() { return m_functionSpacing; }

/*
#
=========================================================================================
#
#
#
=========================================================================================
#
*/

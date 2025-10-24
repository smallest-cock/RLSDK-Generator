#include "pch.hpp"
#include "Configuration.hpp"
#include "Globals.hpp"

// ###############################################################################################
// ######################################    Game Info    ########################################
// ###############################################################################################

bool        GConfig::m_addTimestampToHeader           = true;
bool        GConfig::m_addTimestampToOutputFolderName = true;
std::string GConfig::m_gameNameLong                   = "Rocket League";
std::string GConfig::m_gameNameShort                  = "RLSDK";
std::string GConfig::m_gameVersion                    = "Season 20 (v2.61)"; // <-- update this every game update
std::string GConfig::outputFolderName                 = m_gameNameShort;
fs::path    GConfig::m_outputPathParentDir            = "C:\\folder\\path\\where\\you\\want\\the\\SDK\\generated";
fs::path    GConfig::m_outputPath                     = m_outputPathParentDir / outputFolderName;

// ###############################################################################################
// ###########################    Globals (GNames, GObjects, etc.)    ############################
// ###############################################################################################
// clang-format off
/*
m_useOffsetsInFinalSDK - If you want the macros generated in the SDK for each global to be their offset value,
                         as opposed to a pattern string (if it exists)

m_dumpOffsets          - Dump the (found) offsets for each global and SDK generation time to a Offsets.txt file

m_offsets              - The offsets for each global. Can leave as 0x0000000 if not using offsets for that particular global.
                         Any globals omitted from this list wont make it into the final SDK, so leave them even if using patterns!

m_patternStrings       - Any patterns used to find globals. Patterns will take precedence over offsets when resolving addresses
                         (aka, the offset wont be used if a pattern exists for that global)

m_resolvers            - Optional lambda functions containing any extra steps used to resolve the final address of a global.
                         Input param is the raw address after the initial search (the result of adding offset or pattern scanning).
						 Return value should be a uintptr_t of the final address.
						 The initial search result addresses of other globals can be accessed in m_scanResults.
*/

bool GlobalsManager::m_useOffsetsInFinalSDK = true;
bool GlobalsManager::m_dumpOffsets          = true;

// these should be updated every game update if using offsets
OffsetsList GlobalsManager::m_offsets = {
	{EGlobalVar::BuildDate,       0x0000000},
	{EGlobalVar::GPsyonixBuildID, 0x0000000},
	{EGlobalVar::GNames,          0x0000000},
	{EGlobalVar::GObjects,        0x0000000},
	{EGlobalVar::GMalloc,         0x0000000},
};

PatternsList GlobalsManager::m_patternStrings = {
    {EGlobalVar::BuildDate,       "48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 3D ?? ?? ?? ?? 48 85 FF 74"},
    {EGlobalVar::GPsyonixBuildID, "4C 8B 0D ?? ?? ?? ?? 4C 8D 05 ?? ?? ?? ?? BA F8 02 00 00"},
    {EGlobalVar::GMalloc,         "48 89 0D ?? ?? ?? ?? 48 8B 01 FF 50 60"},
	{EGlobalVar::GNames,          "?? ?? ?? ?? ?? ?? 00 00 ?? ?? 01 00 35 25 02 00"},
};

AddressResolvers GlobalsManager::m_resolvers = {
    {EGlobalVar::GObjects,
        [](...) -> uintptr_t
        {
	        auto it = GlobalsManager::m_scanResults.find(EGlobalVar::GNames);
	        if (it != GlobalsManager::m_scanResults.end())
		        return it->second + 0x48; // GObjects is usually GNames + 0x48
	        return 0;
        }},
    {EGlobalVar::GMalloc, [](uintptr_t foundAddress) -> uintptr_t
        {
			return findRipRelativeAddr(foundAddress, 3);
        }},
    {EGlobalVar::GPsyonixBuildID, [](uintptr_t foundAddress) -> uintptr_t
        {
			return findRipRelativeAddr(foundAddress, 3);
        }},
    {EGlobalVar::BuildDate, [](uintptr_t foundAddress) -> uintptr_t
        {
			return findRipRelativeAddr(foundAddress, 15);
        }},
};

// ###############################################################################################
// #################################    Generator Settings    ####################################
// ###############################################################################################
/*
m_createPchCopy     - If set to true, a copy of the generated SDK will be created (after generation) with "pch.h" includes in all .cpp files
m_useWindows        - If set to true this will auto include "Windows.h" in your sdk, along with renaming some windows functions that could conflict with windows macros.
m_useConstants      - If you want to use objects internal integer for finding static classes and functions, note that these will change every time the game updates.
m_removeNativeIndex - If you want to remove the "iNative" index on functions before calling process event.
m_removeNativeFlags - If you want to remove the "FUNC_Native" flag on functions before calling process event.
m_printEnumFlags    - If you want the EFunctionFlags, EPropertyFlags, and EObjectFlags enums so be printed in the final sdk.
m_useEnumClasses    - If you want to use strongly typed enum classes over normal ones.
m_enumClassType     - Underlying enum type if you set "m_useEnumClasses" to true.
m_gameAlignment     - Used to calculate property sizes and missed offsets.
m_finalAlignment    - Forced alignment used in the final sdk.
m_blacklistedTypes  - Names of classes or structs you don't want generated in the final sdk.
m_typeOverrides     - Names of classes or structs you want to override with your own custom one.
*/
// clang-format on

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

// FPointer gets re-defined in Core_structs.hpp and it breaks everything bitch
std::vector<std::string> GConfig::m_blacklistedTypes = {"UGroup_ORS", "FPointer"};

std::map<std::string, const char*> GConfig::m_typeOverrides = {};

// ###############################################################################################
// ####################################    Process Event    ######################################
// ###############################################################################################
/*
m_useIndex  - If you want to use "m_peIndex" change this to true, if not virutal voids will be generated from "m_peMask" and "m_pePattern".
m_peIndex   - Position where the process event function is in UObject's VfTable.
m_peMask    - Half byte mask, use question marks for unknown data.
m_pePattern - First value is the actual hex escaped pattern, second value is the string version of it printed in the final sdk.
*/

bool                             GConfig::m_useIndex  = true;
int32_t                          GConfig::m_peIndex   = 67; // sIx sEvEnnn
std::string                      GConfig::m_peMask    = "";
std::pair<uint8_t*, std::string> GConfig::m_pePattern = {(uint8_t*)"", ""};

// ###############################################################################################
// ######################################    Cosmetics    ########################################
// ###############################################################################################

uint32_t GConfig::m_constantSpacing = 50;
uint32_t GConfig::m_commentSpacing  = 30;
uint32_t GConfig::m_enumSpacing     = 50;
uint32_t GConfig::m_classSpacing    = 50;
uint32_t GConfig::m_structSpacing   = 50;
uint32_t GConfig::m_functionSpacing = 50;

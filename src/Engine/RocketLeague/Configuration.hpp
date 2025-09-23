#pragma once
#include <map>
#include <string>
#include <filesystem>

/*
# ========================================================================================= #
# Configuration
# ========================================================================================= #
*/

// Uncomment this if you want to disable file logging during generation.
// #define NO_LOGGING

// Uncomment this if your game uses wide characters (UTF16), the default is UTF8!
#define UTF16

// Uncomment this if your game uses wide characters (UTF16) for FStrings!
#define UTF16_FSTRING

enum EAlignment : int32_t
{
	NONE   = 0x1,
	X32BIT = 0x4,
	X64BIT = 0x8
};

// Below are global variables for the generator, make changes in in the cpp file only!

class GConfig
{
private: // Cosmetics
	static uint32_t m_constantSpacing;
	static uint32_t m_commentSpacing;
	static uint32_t m_enumSpacing;
	static uint32_t m_classSpacing;
	static uint32_t m_structSpacing;
	static uint32_t m_functionSpacing;

public:
	static uint32_t GetConstSpacing();
	static uint32_t GetCommentSpacing();
	static uint32_t GetEnumSpacing();
	static uint32_t GetClassSpacing();
	static uint32_t GetStructSpacing();
	static uint32_t GetFunctionSpacing();

private:                                                       // Generator Settings
	static bool                               m_createPchCopy; // custom addon
	static bool                               m_useWindows;
	static bool                               m_useConstants;
	static bool                               m_removeNativeIndex;
	static bool                               m_removeNativeFlags;
	static bool                               m_printEnumFlags;
	static bool                               m_useEnumClasses;
	static std::string                        m_enumClassType;
	static uint32_t                           m_gameAlignment;
	static uint32_t                           m_finalAlignment;
	static std::vector<std::string>           m_blacklistedTypes;
	static std::map<std::string, const char*> m_typeOverrides;

public:
	static bool                                            CreatePchCopy(); // custom addon
	static bool                                            UsingWindows();
	static bool                                            UsingConstants();
	static bool                                            RemoveNativeIndex();
	static bool                                            RemoveNativeFlags();
	static bool                                            PrintEnumFlags();
	static bool                                            UsingEnumClasses();
	static const std::string&                              GetEnumClassType();
	static uint32_t                                        GetGameAlignment();
	static uint32_t                                        GetFinalAlignment();
	static bool                                            IsTypeBlacklisted(const std::string& name);
	static bool                                            IsTypeOveridden(const std::string& name);
	static std::string                                     GetTypeOverride(const std::string& name);
	static inline const std::map<std::string, const char*> GetTypeOverrideMap() { return m_typeOverrides; }

private: // Process Event
	static bool                             m_useIndex;
	static int32_t                          m_peIndex;
	static std::string                      m_peMask;
	static std::pair<uint8_t*, std::string> m_pePattern;

public:
	static bool               UsingProcessEventIndex();
	static int32_t            GetProcessEventIndex();
	static uint8_t*           GetProcessEventPattern();
	static const std::string& GetProcessEventStr();
	static const std::string& GetProcessEventMask();

private:                                                                  // Global Objects & Names
	static bool                             m_useGnamesPatternForOffsets; // custom addon
	static bool                             m_dumpOffsets;                // custom addon
	static bool                             m_useOffsets;
	static uintptr_t                        m_gobjectOffset;
	static std::string                      m_gobjectMask;
	static std::pair<uint8_t*, std::string> m_gobjectPattern;
	static uintptr_t                        m_gnameOffset;
	static std::string                      m_gnameMask;
	static std::pair<uint8_t*, std::string> m_gnamePattern;
	static uintptr_t                        m_calculatedGnamesAddress;   // custom addon
	static uintptr_t                        m_calculatedGobjectsAddress; // custom addon

public:
	static bool               UsingGnamesPatternForOffsets(); // custom addon
	static bool               DumpOffsets();                  // custom addon
	static bool               UsingOffsets();
	static uintptr_t          GetGObjectOffset();
	static uintptr_t          GetGObjectsAddress(); // custom addon
	static uint8_t*           GetGObjectPattern();
	static const std::string& GetGObjectStr();
	static const std::string& GetGObjectMask();
	static uintptr_t          GetGNameOffset();
	static uintptr_t          GetGNamesAddress(); // custom addon
	static uint8_t*           GetGNamePattern();
	static const std::string& GetGNameStr();
	static const std::string& GetGNameMask();
	static void               SetGNamesOffset(uintptr_t offset);     // custom addon
	static void               SetGNamesAddress(uintptr_t address);   // custom addon
	static void               SetGObjectsOffset(uintptr_t offset);   // custom addon
	static void               SetGObjectsAddress(uintptr_t address); // custom addon
	static void               SetUseOffsets(bool useOffsets);        // custom addon

private:                                                         // Game Info
	static bool                  addTimestampToGameVersion;      // custom addon
	static bool                  addTimestampToOutputFolderName; // custom addon
	static std::string           m_gameNameLong;
	static std::string           m_gameNameShort;
	static std::string           m_gameVersion;
	static std::string           outputFolderName;      // custom addon
	static std::filesystem::path m_outputPathParentDir; // custom addon
	static std::filesystem::path m_outputPath;

public:
	static const std::string&           GetGameNameLong();
	static const std::string&           GetGameNameShort();
	static const std::string&           GetGameVersion();
	static void                         AddTimestampToGameVersion(const std::string& timestamp); // custom addon
	static void                         SetOutputFolderName(const std::string& newFolderName);   // custom addon
	static const std::filesystem::path& GetOutputPath();
	static bool                         HasOutputPath();

public:
	GConfig() = delete;
};

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/

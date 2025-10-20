#pragma once
#include <map>
#include <string>
#include <vector>

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
	static uint32_t GetConstSpacing() { return m_constantSpacing; }
	static uint32_t GetCommentSpacing() { return m_commentSpacing; }
	static uint32_t GetEnumSpacing() { return m_enumSpacing; }
	static uint32_t GetClassSpacing() { return m_classSpacing; }
	static uint32_t GetStructSpacing() { return m_structSpacing; }
	static uint32_t GetFunctionSpacing() { return m_functionSpacing; }

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
	static bool                                     CreatePchCopy() { return m_createPchCopy; } // custom addon
	static bool                                     UsingWindows() { return m_useWindows; }
	static bool                                     UsingConstants() { return m_useConstants; }
	static bool                                     RemoveNativeIndex() { return m_removeNativeIndex; }
	static bool                                     RemoveNativeFlags() { return m_removeNativeFlags; }
	static bool                                     PrintEnumFlags() { return m_printEnumFlags; }
	static bool                                     UsingEnumClasses() { return m_useEnumClasses; }
	static const std::string&                       GetEnumClassType() { return m_enumClassType; }
	static uint32_t                                 GetGameAlignment() { return m_gameAlignment; }
	static uint32_t                                 GetFinalAlignment() { return m_finalAlignment; }
	static const std::map<std::string, const char*> GetTypeOverrideMap() { return m_typeOverrides; }

	static bool IsTypeBlacklisted(const std::string& name)
	{
		if (m_blacklistedTypes.empty() || name.empty())
			return false;

		return (std::find(m_blacklistedTypes.begin(), m_blacklistedTypes.end(), name) != m_blacklistedTypes.end());
	}

	static bool IsTypeOveridden(const std::string& name)
	{
		if (name.empty())
			return false;

		return m_typeOverrides.contains(name);
	}

	static std::string GetTypeOverride(const std::string& name)
	{
		auto it = m_typeOverrides.find(name);
		if (it != m_typeOverrides.end())
			return it->second;
		else
			return "";
	}

private: // Process Event
	static bool                             m_useIndex;
	static int32_t                          m_peIndex;
	static std::string                      m_peMask;
	static std::pair<uint8_t*, std::string> m_pePattern;

public:
	static bool               UsingProcessEventIndex() { return (m_useIndex && (m_peIndex != -1)); }
	static int32_t            GetProcessEventIndex() { return m_peIndex; }
	static const std::string& GetProcessEventMask() { return m_peMask; }
	static uint8_t*           GetProcessEventPattern() { return m_pePattern.first; }
	static const std::string& GetProcessEventStr() { return m_pePattern.second; }

private:                                                           // Game Info
	static bool                  m_addTimestampToHeader;           // custom addon
	static bool                  m_addTimestampToOutputFolderName; // custom addon
	static std::string           m_gameNameLong;
	static std::string           m_gameNameShort;
	static std::string           m_gameVersion;
	static std::string           outputFolderName;      // custom addon
	static std::filesystem::path m_outputPathParentDir; // custom addon
	static std::filesystem::path m_outputPath;

public:
	static const std::string&           GetGameNameLong() { return m_gameNameLong; }
	static const std::string&           GetGameNameShort() { return m_gameNameShort; }
	static const std::string&           GetGameVersion() { return m_gameVersion; }
	static const std::filesystem::path& GetOutputPath() { return m_outputPath; }
	static bool                         addTimestampToHeader() { return m_addTimestampToHeader; }

	static bool HasOutputPath()
	{
		return (!GetOutputPath().string().empty() &&
		        (GetOutputPath().string().find("C:\\folder\\path\\where\\you\\want\\the\\SDK\\generated") != 0));
	}

	static void SetOutputFolderName(const std::string& newFolderName) // custom addon
	{
		// add date if necessary
		if (m_addTimestampToOutputFolderName)
			outputFolderName += "__" + newFolderName;

		// replace spaces with underscores
		for (char& c : outputFolderName)
		{
			if (c == ' ')
				c = '_';
		}

		// update output path
		m_outputPath = m_outputPathParentDir / outputFolderName;
	}

public:
	GConfig() = delete;
};

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/

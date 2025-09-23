#pragma once
#include <string>


/*
# ========================================================================================= #
# Engine
# ========================================================================================= #
*/

// These are global variables for the generator, you do not need to change them; they are assigned in the cpp file.

class GEngine
{
private:
	static std::string m_name;
	static std::string m_version;
	static std::string m_credits;
	static std::string m_links;

public:
	static const std::string& GetName();
	static const std::string& GetVersion();
	static const std::string& GetCredits();
	static const std::string& GetLinks();

public:
	GEngine() = delete;
};

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/
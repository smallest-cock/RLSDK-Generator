#include "GameDefines.hpp"
#include <unordered_map>

/*
# ========================================================================================= #
# Initialize Globals
# ========================================================================================= #
*/

void*                            GMalloc  = nullptr;
class TArray<class UObject*>*    GObjects = nullptr;
class TArray<class FNameEntry*>* GNames   = nullptr;

/*
# ========================================================================================= #
# Class Functions
# ========================================================================================= #
*/

class TArray<class UObject*>* UObject::GObjObjects() { return reinterpret_cast<TArray<UObject*>*>(GObjects); }

std::string UObject::GetName() { return this->Name.ToString(); }

std::string UObject::GetNameCPP()
{
	std::string nameCPP;

	if (this->IsA<UClass>())
	{
		UClass* uClass = reinterpret_cast<UClass*>(this);

		while (uClass)
		{
			std::string className = uClass->GetName();

			if (className == "Actor")
			{
				nameCPP += "A";
				break;
			}
			else if (className == "Object")
			{
				nameCPP += "U";
				break;
			}

			uClass = reinterpret_cast<UClass*>(uClass->SuperField);
		}
	}
	else
		nameCPP += "F";

	nameCPP += this->GetName();
	return nameCPP;
}

std::string UObject::GetFullName()
{
	std::string fullName = this->GetName();

	for (UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer)
		fullName = (uOuter->GetName() + "." + fullName);

	fullName = (this->Class->GetName() + " " + fullName);
	return fullName;
}

class UObject* UObject::GetPackageObj()
{
	UObject* uPackage = nullptr;
	for (UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer)
		uPackage = uOuter;

	return uPackage;
}

class UClass* UObject::FindClass(const std::string& classFullName)
{
	static std::map<std::string, UClass*> classCache;

	if (classCache.empty())
	{
		for (int32_t i = 0; i < (UObject::GObjObjects()->size() - 1); ++i)
		{
			UObject* uObject = UObject::GObjObjects()->at(i);
			if (!uObject)
				continue;

			std::string objectFullName = uObject->GetFullName();
			if (objectFullName.find("Class") == 0)
				classCache[objectFullName] = reinterpret_cast<UClass*>(uObject);
		}
	}

	if (auto it = classCache.find(classFullName); it != classCache.end())
		return it->second;
	else
		return nullptr;
}

bool UObject::IsA(class UClass* uClass)
{
	if (!uClass)
		return false;

	for (UClass* uSuperClass = reinterpret_cast<UClass*>(this->Class); uSuperClass;
	    uSuperClass          = reinterpret_cast<UClass*>(uSuperClass->SuperField))
	{
		if (uSuperClass == uClass)
			return true;
	}

	return false;
}

bool UObject::IsA(int32_t objInternalInteger)
{
	UClass* uClass = reinterpret_cast<UClass*>(UObject::GObjObjects()->at(objInternalInteger)->Class);
	if (!uClass)
		return false;

	return this->IsA(uClass);
}

class UFunction* UFunction::FindFunction(const std::string& functionFullName)
{
	static std::unordered_map<std::string, UFunction*> functionCache;

	if (functionCache.empty())
	{
		for (int32_t i = 0; i < (UObject::GObjObjects()->size() - 1); ++i)
		{
			UObject* uObject = UObject::GObjObjects()->at(i);
			if (!uObject)
				continue;

			std::string objectFullName = uObject->GetFullName();
			if (objectFullName.find("Function") == 0)
				functionCache[objectFullName] = reinterpret_cast<UFunction*>(uObject);
		}
	}

	if (auto it = functionCache.find(functionFullName); it != functionCache.end())
		return it->second;
	else
		return nullptr;
}

class TArray<class FNameEntry*>* FName::Names() { return reinterpret_cast<TArray<FNameEntry*>*>(GNames); }

/*
# ========================================================================================= #
#
# ========================================================================================= #
*/
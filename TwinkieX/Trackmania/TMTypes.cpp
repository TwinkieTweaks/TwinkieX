// Precompiled Headers.
#include "pch.h"

// Various type definitions for Trackmania
#include <Trackmania/TMTypes.h>

CMwMemberInfo** CMwClassInfo::begin()
{
	return this->Members;
}

CMwMemberInfo** CMwClassInfo::end()
{
	return this->Members + this->MembersAmount;
}

CMwMemberInfo** CMwClassInfo::begin() const
{
	return this->Members;
}

CMwMemberInfo** CMwClassInfo::end() const
{
	return this->Members + this->MembersAmount;
}

std::string CMwClassInfo::GetName() const
{
	std::string TheClassName = this->ClassName;

	for (size_t Pos; (Pos = TheClassName.find("::")) != std::string::npos;)
		TheClassName.replace(Pos, 2, "_");

	if (TheClassName.empty()) return std::format("__{:08X}", this->ClassID);
	else return TheClassName;
}

std::string CMwMemberInfo::GetName() const
{
	std::string TheMemberName = this->MemberName;

	for (size_t Pos; (Pos = TheMemberName.find("::")) != std::string::npos;)
		TheMemberName.replace(Pos, 2, "_");
	for (char& Char : TheMemberName)
		if (!std::isalnum((unsigned char)Char) && Char != '_') Char = '_';
	if (!TheMemberName.empty() && std::isdigit((unsigned char)TheMemberName[0]))
		TheMemberName.insert(0, "_");

	if (TheMemberName.empty()) return std::format("__{:08X}", this->MemberID);
	else return TheMemberName;
}
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
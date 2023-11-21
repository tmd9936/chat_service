#pragma once

class NoCopyObject
{
protected:
	NoCopyObject() {};
	~NoCopyObject() {};
	NoCopyObject(const NoCopyObject& rhs) {};
	NoCopyObject* operator=(const NoCopyObject& ref) {};
};
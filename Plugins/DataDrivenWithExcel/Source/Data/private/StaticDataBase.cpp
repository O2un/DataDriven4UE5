#include "StaticDataBase.h"

void UStaticDataBase::Initialize(const FStaticDataInitStruct& data)
{
	if (_initialized)
	{
		ensureMsgf(false, TEXT("StaticData already Initialized"));
		return;
	}

	_key = data._key;
	_initialized = true;
}

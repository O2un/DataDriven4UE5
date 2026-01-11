#include "StaticData/testDataStaticData.h"

void UtestDataStaticData::Initialize(const FStaticDataInitStruct& data)
{
	Super::Initialize(data);

	const auto& initor = static_cast<const FtestDataInitStruct&>(data);

	_data = initor._data;
	_value = initor._value;

}

#include "StaticData/3rdSheetStaticData.h"

void U3rdSheetStaticData::Initialize(const FStaticDataInitStruct& data)
{
	Super::Initialize(data);

	const auto& initor = static_cast<const F3rdSheetInitStruct&>(data);

	_data = initor._data;

}

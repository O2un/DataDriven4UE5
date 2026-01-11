// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "StaticData/3rdSheetStaticData.h"

/**
 * Auto-generated Static Data Class
 * 수정 금지: 엑셀 파서에 의해 자동 생성된 파일입니다.
 */
void U3rdSheetStaticData::Initialize(const FStaticDataInitStruct& data)
{
	Super::Initialize(data);

	const auto& initor = static_cast<const F3rdSheetInitStruct&>(data);

	_data = initor._data;

}

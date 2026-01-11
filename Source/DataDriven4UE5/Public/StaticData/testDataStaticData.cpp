// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "StaticData/testDataStaticData.h"

/**
 * Auto-generated Static Data Class
 * 수정 금지: 엑셀 파서에 의해 자동 생성된 파일입니다.
 */
void UtestDataStaticData::Initialize(const FStaticDataInitStruct& data)
{
	Super::Initialize(data);

	const auto& initor = static_cast<const FtestDataInitStruct&>(data);

	_data = initor._data;
	_value = initor._value;

}

// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "DataAsset/testDataDataAsset.h"
#include "StaticData/testDataStaticData.h"

/**
 * Auto-generated Static Data Asset
 * 수정 금지: 엑셀 파서에 의해 자동 생성된 파일입니다.
 */
UtestDataStaticData* UtestDataDataAsset::GetData(FStaticDataKey id, bool& success)
{
	if (_dataList.Contains(id))
	{
		success = true;
		return static_cast<UtestDataStaticData*>(_dataList[id]);
	}

	success = false;
	return nullptr;
}

// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#include "DataAsset/3rdSheetDataAsset.h"
#include "StaticData/3rdSheetStaticData.h"

/**
 * Auto-generated Static Data Asset
 * 수정 금지: 엑셀 파서에 의해 자동 생성된 파일입니다.
 */
U3rdSheetStaticData* U3rdSheetDataAsset::GetData(FStaticDataKey id, bool& success)
{
	if (_dataList.Contains(id))
	{
		success = true;
		return static_cast<U3rdSheetStaticData*>(_dataList[id]);
	}

	success = false;
	return nullptr;
}

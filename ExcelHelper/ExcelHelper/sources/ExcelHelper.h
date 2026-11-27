// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once

#include <stdint.h>

#ifdef EXCELHELPER_EXPORTS
#define O2UN_API __declspec(dllexport)
#else
#define O2UN_API __declspec(dllimport)
#endif // EXCELHELPER_EXPORTS

// 로그 등급 (logCallback의 level 값)
#define O2UN_LOG_INFO    0
#define O2UN_LOG_WARNING 1
#define O2UN_LOG_ERROR   2
#define O2UN_LOG_FATAL   3

extern "C"
{
	// message는 UTF-8, 콜백 안에서만 유효하다
	typedef void(*logCallback)(int32_t level, const char* message);
	// 등급 추가로 시그니처가 바뀌어 이름을 바꿨다 (예전 DLL과 섞이면 로드 시점에 실패하도록)
	O2UN_API void SetLogHandlerWithLevel(logCallback c);
	O2UN_API void SetCodeGenerateInfo(const char* projectName, const char* root);
	O2UN_API void GenerateAllDataClass(const char* root);
	// 반환 문자열은 DLL 내부 버퍼 소유 — 다음 LoadExcel/GetExcelList 호출 전까지만 유효, 해제 금지
	O2UN_API const char* LoadExcel(const char* path, const char* className);
	O2UN_API const char* GetExcelList(const char* root);

	// ---- 데이터 API (abiVersion 1) — design/ai-fill.md §3 ----
	// 입력: UTF-8, NUL 종료, 호출 중에만 빌린다(DLL이 보관하지 않음). NULL이면 -1.
	// 경로: UTF-8 절대 경로.
	// 출력: *outJson에 DLL이 할당한 UTF-8 JSON을 성공·실패 모두 채운다. 호출자는 복사 후 반드시 O2un_FreeString으로 해제한다.
	//       outJson이 NULL이면 -1을 돌려주고 아무것도 할당하지 않는다.
	// 반환: 0 성공 · 1 검증 실패(details) · -1 잘못된 인자 · -2 파일/폴더 없음 · -3 시트 없음 · -4 입출력 실패/잠김
	//       · -5 입력 JSON 파싱/형식 오류 · -99 내부 오류
	// 실패 JSON: {"code":"<snake_case>","message":"<영문 한 줄>","details":[{"row":0,"column":"hp","code":"type_mismatch","message":"..."}]}
	//            row는 입력 배열 인덱스(행과 무관하면 생략), details는 최대 50개(넘으면 "totalDetails").
	// 스레드: 공유 버퍼는 없지만 로그 콜백이 전역이므로 호출자가 DLL 호출을 직렬화한다.
	// 쓰기: 같은 폴더의 "~&" 임시 파일에 저장하고 로더로 다시 읽어 확인한 뒤 MoveFileExW로 교체한다. 실패하면 원본은 그대로다.

	// {"abiVersion":1,"types":[{"name","cpp"}],"keyType":"key","keyColumns":{...},"reservedColumnNames":[...],"limits":{...}}
	O2UN_API int32_t O2un_GetCapabilities(char** outJson);
	// limit = 0이면 스키마만(rows 빈 배열). offset/limit은 로더가 읽는 데이터 행 기준
	O2UN_API int32_t O2un_ReadSheet(const char* xlsxPath, const char* sheet, int32_t offset, int32_t limit, char** outJson);
	// rowsJson = [{"id":101,"group":1,...}] (키 = 1행 열 이름). dryRun != 0이면 검증·왕복 확인만 하고 원본을 쓰지 않는다
	O2UN_API int32_t O2un_AppendRows(const char* xlsxPath, const char* sheet, const char* rowsJson, int32_t dryRun, char** outJson);
	// schemaJson = {"sheet":"Name","columns":[{"name","type"}],"rows":[...]}. 파일은 {root}/{sheet}.xlsx, 있으면 실패
	O2UN_API int32_t O2un_CreateSheet(const char* root, const char* schemaJson, int32_t dryRun, char** outJson);
	// O2un_* 함수가 돌려준 문자열을 해제한다. NULL 허용
	O2UN_API void O2un_FreeString(char* s);
}

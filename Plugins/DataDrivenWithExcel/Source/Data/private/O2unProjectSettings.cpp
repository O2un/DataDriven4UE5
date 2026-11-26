// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.

#include "O2unProjectSettings.h"
#include "Misc/Paths.h"

UO2unProjectSettings::UO2unProjectSettings()
{
#if WITH_EDITORONLY_DATA
	// 프로젝트 폴더 기준 상대 경로 (기존 기본값 FPaths::GameSourceDir()와 같은 위치)
	GeneratedSourceDirectory.Path = TEXT("Source");
#endif
}

#if WITH_EDITORONLY_DATA
FString UO2unProjectSettings::ResolveProjectRelativeDirectory(const FString& path)
{
	if (path.IsEmpty())
	{
		return FString();
	}

	const FString projectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FString result = FPaths::ConvertRelativePathToFull(projectDir, path);
	FPaths::NormalizeDirectoryName(result);
	return result;
}

FString UO2unProjectSettings::MakeProjectRelativeIfInside(const FString& path)
{
	if (path.IsEmpty() || FPaths::IsRelative(path))
	{
		return path;
	}

	FString fullPath = FPaths::ConvertRelativePathToFull(path);
	FPaths::NormalizeDirectoryName(fullPath);

	FString projectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	FPaths::NormalizeDirectoryName(projectDir);

	// 프로젝트 폴더 자체이거나 밖이면 그대로 둔다
	if (fullPath.Equals(projectDir, ESearchCase::IgnoreCase) || false == FPaths::IsUnderDirectory(fullPath, projectDir))
	{
		return path;
	}

	FString relativePath = fullPath;
	if (false == FPaths::MakePathRelativeTo(relativePath, *(projectDir / TEXT(""))))
	{
		return path;
	}
	return relativePath;
}
#endif

#if WITH_EDITOR
void UO2unProjectSettings::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
	const FName memberName = propertyChangedEvent.GetMemberPropertyName();
	if (memberName == GET_MEMBER_NAME_CHECKED(UO2unProjectSettings, RawExcelDirectory))
	{
		RawExcelDirectory.Path = MakeProjectRelativeIfInside(RawExcelDirectory.Path);
	}
	else if (memberName == GET_MEMBER_NAME_CHECKED(UO2unProjectSettings, GeneratedSourceDirectory))
	{
		GeneratedSourceDirectory.Path = MakeProjectRelativeIfInside(GeneratedSourceDirectory.Path);
	}

	// 설정 편집기는 PostEditChangeProperty 다음에 저장하므로 바꾼 값이 DefaultGame.ini에 들어간다
	Super::PostEditChangeProperty(propertyChangedEvent);
}
#endif

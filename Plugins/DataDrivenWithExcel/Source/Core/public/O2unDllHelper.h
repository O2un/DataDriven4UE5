// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.
#pragma once
#include "CoreMinimal.h"
#include <memory>

class IPlugin;

namespace O2un
{
	struct O2UNCORE_API FDllDeleter
	{
		void operator()(void* handle) const;
	};

	using FSmartDLLHandle = std::unique_ptr<void, FDllDeleter>;

	class FPluginHelper
	{
	public:
		static TSharedPtr<IPlugin> GetPlugin();
		static FString GetDllPath(const FString& dllName);
		static O2UNCORE_API FSmartDLLHandle LoadDLL(const FString& dllName);

	private: 
		static const FString puglinName;
	};
}

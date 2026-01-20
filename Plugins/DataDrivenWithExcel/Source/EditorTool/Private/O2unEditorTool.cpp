// SPDX-License-Identifier: AGPL-3.0-or-later
// Copyright (c) 2026 [Jaewon Cho]. All rights reserved.

#include "O2unEditorTool.h"

#include "ToolMenus.h"
#include "Misc/MessageDialog.h"

#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IMainFrameModule.h"
#include "Widgets/SWindow.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"

#include "O2unProjectSettings.h"
#include "ExcelHelperSubsystem.h"

void FO2unEditorToolModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FO2unEditorToolModule::RegisterMenus));
}

void FO2unEditorToolModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FO2unEditorToolModule::RegisterMenus()
{
	FToolMenuOwnerScoped ownerScoped(this);
	
	// Add menu entry to the main menu
	auto menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu");
	{
		FToolMenuSection& section = menu->FindOrAddSection("O2un");
		section.AddSubMenu(
			"O2un",
			FText::FromString(L"O2un"),
			FText::FromString(L"Tools developed by O2un"),
			FNewToolMenuDelegate::CreateRaw(this, &FO2unEditorToolModule::FillMenu)
		);
	}
}

void FO2unEditorToolModule::OnMenuClicked()
{
    if (_toolWindow.IsValid()) 
    { 
        _toolWindow.Pin()->BringToFront();
        return; 
    }

    TSharedRef<SWindow> newWindow = SAssignNew(_toolWindow, SWindow)
        .Title(FText::FromString(L"Data Driven Tool"))
        .ClientSize(FVector2D(600, 400))
        .SupportsMaximize(true)
        .SupportsMinimize(true)
        [
            SNew(SBorder)
                .Padding(10)
                .BorderImage(FAppStyle::GetBrush("Docking.Tab.ContentAreaBrush"))
                [
                    SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .Padding(0, 0, 0, 10)
                        [
                            SNew(SVerticalBox)
                                + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(2.0f)
                                [
                                    SNew(SButton)
                                        .Text(FText::FromString(L"데이터 스크립트 생성"))
                                        .OnClicked_Lambda([this]() -> FReply {
                                        if (GEditor)
                                        {
                                            if(auto* helper = GEditor->GetEditorSubsystem<UExcelHelperSubsystem>())
                                            {
                                                helper->GenerateDataScript();
                                            }
                                        }
                                        return FReply::Handled();
                                            })
                                ]
                            + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(2.0f)
                                [
                                    SNew(SButton)
                                        .Text(FText::FromString(L"데이터 에셋 로드"))
                                        .OnClicked_Lambda([this]() -> FReply {
                                        if (GEditor)
                                        {
                                            if (auto* helper = GEditor->GetEditorSubsystem<UExcelHelperSubsystem>())
                                            {
                                                helper->GenerateAllDataAsset();
                                            }
                                        }
                                        return FReply::Handled();
                                            })
                                ]
                            + SVerticalBox::Slot()
                                .AutoHeight()
                                .Padding(2.0f)
                                [
                                    SNew(SButton)
                                        .Text(FText::FromString(L"엑셀 리스트 로드"))
                                        .OnClicked_Lambda([this]() -> FReply {
                                        if (GEditor)
                                        {
                                            if (auto* helper = GEditor->GetEditorSubsystem<UExcelHelperSubsystem>())
                                            {
                                                helper->LoadAllExcelFileList();
                                            }
                                        }
                                        return FReply::Handled();
                                            })
                                ]
                        ]
                        + SVerticalBox::Slot()
                        .FillHeight(1.0f)
                        [
                            // 여기에 실제 작업할 UI(스크롤박스, 버튼 등)를 추가
                            SNew(SScrollBox)
                        ]
                        + SVerticalBox::Slot()
                        .AutoHeight()
                        .HAlign(HAlign_Right)
                        [
                            SNew(SButton)
                                .Text(FText::FromString(L"닫기"))
                                .OnClicked_Lambda([this]() -> FReply {
                                if (_toolWindow.IsValid()) _toolWindow.Pin()->RequestDestroyWindow();
                                return FReply::Handled();
                                    })
                        ]
                ]
        ];

    IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
    TSharedPtr<SWindow> ParentWindow = MainFrameModule.GetParentWindow();

    if (ParentWindow.IsValid())
    {
        FSlateApplication::Get().AddWindowAsNativeChild(newWindow, ParentWindow.ToSharedRef());
    }
    else
    {
        FSlateApplication::Get().AddWindow(newWindow);
    }
}

void FO2unEditorToolModule::FillMenu(UToolMenu* InMenu)
{
	FToolMenuSection& Section = InMenu->AddSection("MyItems", FText::FromString(L"Tool List"));

	// Button to open the window
	Section.AddMenuEntry(
		"ExecuteTool",
		FText::FromString(L"Data Driven Tool"),
		FText::FromString(L"Opens the Data Driven tool window."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FO2unEditorToolModule::OnMenuClicked))
	);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FO2unEditorToolModule, O2unEditorTool)
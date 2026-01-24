// Fill out your copyright notice in the Description page of Project Settings.


#include "SStaticDataVisualizer.h"
#include "StaticDataAssetBase.h"

#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Input/SComboBox.h"
#include "Rendering/DrawElements.h"

void SStaticDataGraph::Construct(const FArguments& InArgs)
{
	_dataPoints = InArgs._DataPoints;
}

int32 SStaticDataGraph::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float Margin = 30.0f;
	const FVector2D GraphArea = LocalSize - (Margin * 2.0f);
	FSlateFontInfo FontInfo = FCoreStyle::Get().GetFontStyle("NormalFont");
	FontInfo.Size = 8;

	FSlateDrawElement::MakeBox(OutDrawElements, LayerId++, AllottedGeometry.ToPaintGeometry(), FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(0.01f, 0.01f, 0.01f, 0.9f));

	if (!_dataPoints || _dataPoints->Num() < 1) return LayerId;

	float MaxVal = -FLT_MAX, MinVal = FLT_MAX;
	for (float V : *_dataPoints) { MaxVal = FMath::Max(MaxVal, V); MinVal = FMath::Min(MinVal, V); }
	float Range = (MaxVal - MinVal) < KINDA_SMALL_NUMBER ? 1.0f : (MaxVal - MinVal);

	const int32 Divisions = 10;
	for (int32 i = 0; i <= Divisions; ++i)
	{
		float Ratio = (float)i / Divisions;
		float YPos = LocalSize.Y - Margin - (Ratio * GraphArea.Y);
		TArray<FVector2D> YLine;
		YLine.Add(FVector2D(Margin, YPos));
		YLine.Add(FVector2D(LocalSize.X - Margin, YPos));

		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), YLine, ESlateDrawEffect::None, FLinearColor(0.2f, 0.2f, 0.2f, 0.5f), false);

		float LabelVal = MinVal + (Ratio * Range);
		FVector2D YLabelPos(5.0f, YPos - 7.0f);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.MakeChild(YLabelPos, FVector2D(100.0f, 20.0f)).ToPaintGeometry(),
			FString::Printf(TEXT("%.2f"), LabelVal),
			FontInfo,
			ESlateDrawEffect::None,
			FLinearColor::Gray
		);

		float XPos = Margin + (Ratio * GraphArea.X);
		TArray<FVector2D> XLine;
		XLine.Add(FVector2D(XPos, Margin));
		XLine.Add(FVector2D(XPos, LocalSize.Y - Margin));

		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), XLine, ESlateDrawEffect::None, FLinearColor(0.2f, 0.2f, 0.2f, 0.5f), false);

		int32 KeyIdx = FMath::RoundToInt(Ratio * (_dataPoints->Num() - 1));
		FVector2D XLabelPos(XPos - 5.0f, LocalSize.Y - 15.0f);
		FSlateDrawElement::MakeText(
			OutDrawElements,
			LayerId,
			AllottedGeometry.MakeChild(XLabelPos, FVector2D(50.0f, 20.0f)).ToPaintGeometry(),
			FString::FromInt(KeyIdx),
			FontInfo,
			ESlateDrawEffect::None,
			FLinearColor::Gray
		);
	}

	TArray<FVector2D> RenderPoints;
	for (int32 i = 0; i < _dataPoints->Num(); ++i)
	{
		float AlphaX = (float)i / (_dataPoints->Num() - 1);
		float AlphaY = ((*_dataPoints)[i] - MinVal) / Range;
		RenderPoints.Add(FVector2D(Margin + (AlphaX * GraphArea.X), LocalSize.Y - Margin - (AlphaY * GraphArea.Y)));
	}

	FSlateDrawElement::MakeLines(OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(), RenderPoints, ESlateDrawEffect::None, FLinearColor::Yellow, true, 2.0f);

	if (_bMouseInside && _dataPoints && _dataPoints->Num() > 0)
	{
		float RelativeX = (_mouseLocalPos.X - Margin) / GraphArea.X;
		int32 DataIdx = FMath::Clamp(FMath::RoundToInt(RelativeX * (_dataPoints->Num() - 1)), 0, _dataPoints->Num() - 1);
		float DataAlphaX = (float)DataIdx / (_dataPoints->Num() - 1);
		float DataValue = (*_dataPoints)[DataIdx];
		float DataAlphaY = (DataValue - MinVal) / Range;

		FVector2D CirclePos(Margin + (DataAlphaX * GraphArea.X), LocalSize.Y - Margin - (DataAlphaY * GraphArea.Y));

		TArray<FVector2D> GuideLine;
		GuideLine.Add(FVector2D(CirclePos.X, Margin));
		GuideLine.Add(FVector2D(CirclePos.X, LocalSize.Y - Margin));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), GuideLine, ESlateDrawEffect::None, FLinearColor::White.CopyWithNewOpacity(0.3f), false);

		FSlateDrawElement::MakeBox(
			OutDrawElements, LayerId + 2,
			AllottedGeometry.MakeChild(CirclePos - FVector2D(4, 4), FVector2D(8, 8)).ToPaintGeometry(),
			FAppStyle::GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor::Red
		);

		FString TooltipStr = FString::Printf(TEXT("Idx: %d\nVal: %.2f"), DataIdx, DataValue);
		FSlateDrawElement::MakeText(
			OutDrawElements, LayerId + 3,
			AllottedGeometry.MakeChild(CirclePos + FVector2D(10, -20), FVector2D(150, 40)).ToPaintGeometry(),
			TooltipStr, FontInfo, ESlateDrawEffect::None, FLinearColor::Yellow
		);
	}

	return LayerId + 3;
}

FReply SStaticDataGraph::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	_mouseLocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	_bMouseInside = true;

	return FReply::Handled();
}

void SStaticDataGraph::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	_bMouseInside = false;
}

void SStaticDataVisualizer::Construct(const FArguments& InArgs)
{
	_targetAsset = InArgs._TargetAsset;
	if (!_targetAsset.IsValid()) return;

	_numericPropertyNames.Empty();
	UClass* DataClass = _targetAsset->GetDataClass();
	if (!DataClass) return;

	for (TFieldIterator<FProperty> PropertyIt(DataClass); PropertyIt; ++PropertyIt)
	{
		FProperty* Prop = *PropertyIt;
		if (Prop->IsA<FNumericProperty>())
		{
			_numericPropertyNames.Add(MakeShared<FString>(Prop->GetName()));
		}
	}

	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&_numericPropertyNames)
						.OnSelectionChanged(this, &SStaticDataVisualizer::OnPropertySelectionChanged)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
							{
								return SNew(STextBlock)
									.Text(FText::FromString(*InItem))
									.Margin(FMargin(4.0f, 2.0f));
							})
						[SAssignNew(_selectedText, STextBlock).Text(FText::FromString(TEXT("Select Property")))]
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.Padding(10.0f)
				[
					SAssignNew(_graphWidget, SStaticDataGraph)
						.DataPoints(&_dataPoints)
				]
		];
}

void SStaticDataVisualizer::OnPropertySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo)
{
	if (!NewSelection.IsValid() || !_targetAsset.IsValid()) return;

	_selectedText->SetText(FText::FromString(*NewSelection));
	_dataPoints.Empty();

	FString PropName = *NewSelection;

	_targetAsset->EnumerateData([&](const UStaticDataBase* DataObj) {
		if (FProperty* Prop = DataObj->GetClass()->FindPropertyByName(*PropName))
		{
			if (FNumericProperty* NP = CastField<FNumericProperty>(Prop))
			{
				void* ValuePtr = NP->ContainerPtrToValuePtr<void>(const_cast<UStaticDataBase*>(DataObj));

				if (ValuePtr)
				{
					float FinalValue = 0.0f;

					if (NP->IsFloatingPoint())
					{
						FinalValue = (float)NP->GetFloatingPointPropertyValue(ValuePtr);
					}
					else if (NP->IsInteger())
					{
						FinalValue = (float)NP->GetSignedIntPropertyValue(ValuePtr);
					}

					_dataPoints.Add(FinalValue);
				}
			}
		}
		});

	if (_graphWidget.IsValid())
	{
		_graphWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

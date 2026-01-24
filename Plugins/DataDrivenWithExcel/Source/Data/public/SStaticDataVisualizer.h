// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"

class UStaticDataAssetBase;
/**
 * 
 */
class SStaticDataGraph : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SStaticDataGraph) {}
		SLATE_ARGUMENT(const TArray<float>*, DataPoints)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(100.0f, 100.0f); }

	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }

private:
	const TArray<float>* _dataPoints = nullptr;
	FVector2D _mouseLocalPos = FVector2D::ZeroVector;
	bool _bMouseInside = false;
};


class O2UNDATAHELPER_API SStaticDataVisualizer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStaticDataVisualizer) {}
		SLATE_ARGUMENT(UStaticDataAssetBase*, TargetAsset)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	void OnPropertySelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);

	TWeakObjectPtr<UStaticDataAssetBase> _targetAsset;
	TArray<TSharedPtr<FString>> _numericPropertyNames;
	TArray<float> _dataPoints;

	TSharedPtr<STextBlock> _selectedText;
	TSharedPtr<SStaticDataGraph> _graphWidget;
};

// Copyright Epic Games, Inc. All Rights Reserved.
// RainyWindow 蓝图函数库 - 提供蓝图可调用的函数

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RainyWindowBlueprintLibrary.generated.h"

/**
 * 雨窗效果蓝图函数库
 * 提供蓝图友好的接口来控制雨窗后处理效果
 */
UCLASS()
class RAINYWINDOW_API URainyWindowBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 设置雨窗效果是否启用
	 * @param bEnabled - true 启用，false 禁用
	 */
	UFUNCTION(BlueprintCallable, Category = "RainyWindow")
	static void SetRainEnabled(bool bEnabled);

	/**
	 * 设置雨量
	 * @param Amount - 雨量值 (0.0 - 1.0)
	 */
	UFUNCTION(BlueprintCallable, Category = "RainyWindow")
	static void SetRainAmount(float Amount);

	/**
	 * 重置为默认值（Enabled=false, Amount=0.7, CameraZoom=false）
	 * 建议在 EndPlay 时调用
	 */
	UFUNCTION(BlueprintCallable, Category = "RainyWindow")
	static void ResetRainToDefault();

	/**
	 * 设置是否启用相机伸缩效果
	 * @param bEnabled - true 启用，false 禁用
	 */
	UFUNCTION(BlueprintCallable, Category = "RainyWindow")
	static void SetCameraZoomEnabled(bool bEnabled);

	/**
	 * 获取是否启用相机伸缩效果
	 */
	UFUNCTION(BlueprintPure, Category = "RainyWindow")
	static bool IsCameraZoomEnabled();



	/**
	 * 获取当前雨窗效果是否启用
	 */
	UFUNCTION(BlueprintPure, Category = "RainyWindow")
	static bool IsRainEnabled();

	/**
	 * 获取当前雨量值
	 */
	UFUNCTION(BlueprintPure, Category = "RainyWindow")
	static float GetRainAmount();
};

// Copyright Epic Games, Inc. All Rights Reserved.
// 雨窗效果 SceneViewExtension

#pragma once

#include "CoreMinimal.h"
#include "RenderGraphUtils.h"
#include "SceneViewExtension.h"
#include "PostProcess/PostProcessMaterial.h"
#include "DataDrivenShaderPlatformInfo.h"

class FRainyWindowSceneViewExtension : public FSceneViewExtensionBase
{
public:
	FRainyWindowSceneViewExtension(const FAutoRegister& AutoRegister);

public:
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {}
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override {}

	// 订阅后处理Pass
	virtual void SubscribeToPostProcessingPass(EPostProcessingPass PassId, const FSceneView& View, FAfterPassCallbackDelegateArray& InOutPassCallbacks, bool bIsPassEnabled) override;

	// 后处理回调函数
	FScreenPassTexture RainyWindowPostProcessing(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessMaterialInputs& Inputs);

	// ============== 蓝图控制接口 ==============
	// 设置是否启用雨窗效果
	static void SetRainEnabled(bool bEnabled);
	
	// 设置雨量
	static void SetRainAmount(float Amount);
	
	// 设置是否启用相机伸缩效果
	static void SetCameraZoomEnabled(bool bEnabled);
	
	// 获取是否启用相机伸缩效果
	static bool IsCameraZoomEnabled();
	
	// 设置是否启用闪电效果
	static void SetLightningEnabled(bool bEnabled);
	
	// 获取是否启用闪电效果
	static bool IsLightningEnabled();
	
	// 重置为默认值（通常在 EndPlay 时调用）
	static void ResetToDefault();


	
	// 获取当前是否启用
	static bool IsRainEnabled();
	
	// 获取当前雨量
	static float GetRainAmount();

private:
	// 雨窗效果状态
	static bool bRainEnabled;       // 是否启用
	static float RainAmount;        // 雨量
	static bool bCameraZoomEnabled; // 是否启用相机伸缩效果
	static bool bLightningEnabled;  // 是否启用闪电效果


};


// 雨窗效果 Compute Shader
class RAINYWINDOW_API FRainyWindowCS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FRainyWindowCS)
	SHADER_USE_PARAMETER_STRUCT(FRainyWindowCS, FGlobalShader)

	// ========== Shader Permutation 定义 ==========
	class FCameraZoomDim : SHADER_PERMUTATION_BOOL("CAMERA_ZOOM_ENABLED");
	class FLightningDim : SHADER_PERMUTATION_BOOL("LIGHTNING_ENABLED");
	using FPermutationDomain = TShaderPermutationDomain<FCameraZoomDim, FLightningDim>;


	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
		SHADER_PARAMETER_STRUCT(FScreenPassTextureViewportParameters, SceneColorViewport)
		SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D, OriginalSceneColor)
		SHADER_PARAMETER_SAMPLER(SamplerState, OriginalSceneColorSampler)
		SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, Output)
		SHADER_PARAMETER(float, Time)
		SHADER_PARAMETER(float, RainAmount)
	END_SHADER_PARAMETER_STRUCT()




	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
	}
};


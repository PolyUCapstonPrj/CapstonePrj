// Copyright Epic Games, Inc. All Rights Reserved.

#include "RainyWindowSceneViewExtension.h"
#include "ScreenPass.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "RenderGraphUtils.h"
#include "GenerateMips.h"

// ============== 静态变量定义 ==============
bool FRainyWindowSceneViewExtension::bRainEnabled = false;
float FRainyWindowSceneViewExtension::RainAmount = 0.7f;
bool FRainyWindowSceneViewExtension::bCameraZoomEnabled = false;  // 默认关闭相机伸缩
bool FRainyWindowSceneViewExtension::bLightningEnabled = true;   // 默认开启闪电效果



// ============== 静态函数实现 ==============
void FRainyWindowSceneViewExtension::SetRainEnabled(bool bEnabled)
{
	bRainEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("RainyWindow: SetRainEnabled = %s"), bEnabled ? TEXT("true") : TEXT("false"));
}

void FRainyWindowSceneViewExtension::SetRainAmount(float Amount)
{
	RainAmount = FMath::Clamp(Amount, 0.0f, 1.0f);
	UE_LOG(LogTemp, Log, TEXT("RainyWindow: SetRainAmount = %f"), RainAmount);
}

void FRainyWindowSceneViewExtension::ResetToDefault()
{
	bRainEnabled = false;
	RainAmount = 0.7f;
	bCameraZoomEnabled = false;
	bLightningEnabled = true;
	UE_LOG(LogTemp, Log, TEXT("RainyWindow: Reset to default"));
}


void FRainyWindowSceneViewExtension::SetCameraZoomEnabled(bool bEnabled)
{
	bCameraZoomEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("RainyWindow: SetCameraZoomEnabled = %s"), bEnabled ? TEXT("true") : TEXT("false"));
}

bool FRainyWindowSceneViewExtension::IsCameraZoomEnabled()
{
	return bCameraZoomEnabled;
}

void FRainyWindowSceneViewExtension::SetLightningEnabled(bool bEnabled)
{
	bLightningEnabled = bEnabled;
	UE_LOG(LogTemp, Log, TEXT("RainyWindow: SetLightningEnabled = %s"), bEnabled ? TEXT("true") : TEXT("false"));
}

bool FRainyWindowSceneViewExtension::IsLightningEnabled()
{
	return bLightningEnabled;
}

bool FRainyWindowSceneViewExtension::IsRainEnabled()


{
	return bRainEnabled;
}

float FRainyWindowSceneViewExtension::GetRainAmount()
{
	return RainAmount;
}

// Implement Compute Shader
IMPLEMENT_GLOBAL_SHADER(FRainyWindowCS, "/Plugins/RainyWindow/Private/RainyWindow.usf", "MainCS", SF_Compute);

FRainyWindowSceneViewExtension::FRainyWindowSceneViewExtension(const FAutoRegister& AutoRegister)
	: FSceneViewExtensionBase(AutoRegister)
{
	UE_LOG(LogTemp, Log, TEXT("RainyWindow: SceneViewExtension registered"));
}

void FRainyWindowSceneViewExtension::SubscribeToPostProcessingPass(
	EPostProcessingPass PassId, 
	const FSceneView& View, 
	FAfterPassCallbackDelegateArray& InOutPassCallbacks, 
	bool bIsPassEnabled)
{
	// 检查是否启用 - 使用综合函数（蓝图优先）
	if (!IsRainEnabled())
	{
		return;
	}

	// 在 MotionBlur 之后注入效果
	if (PassId == EPostProcessingPass::MotionBlur)
	{
		InOutPassCallbacks.Add(FAfterPassCallbackDelegate::CreateRaw(
			this, &FRainyWindowSceneViewExtension::RainyWindowPostProcessing));
	}
}


FScreenPassTexture FRainyWindowSceneViewExtension::RainyWindowPostProcessing(
	FRDGBuilder& GraphBuilder, 
	const FSceneView& SceneView, 
	const FPostProcessMaterialInputs& Inputs)
{
	const FSceneViewFamily& ViewFamily = *SceneView.Family;

	// 使用 CopyFromSlice 获取场景颜色
	const FScreenPassTexture& SceneColor = FScreenPassTexture::CopyFromSlice(GraphBuilder, Inputs.GetInput(EPostProcessMaterialInput::SceneColor));

	// 检查是否启用或场景颜色无效 - 使用综合函数（蓝图优先）
	if (!SceneColor.IsValid() || !IsRainEnabled())
	{
		return SceneColor;
	}

	const FScreenPassTextureViewport SceneColorViewport(SceneColor);

	FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(ViewFamily.GetFeatureLevel());

	// RDG 事件作用域
	RDG_EVENT_SCOPE(GraphBuilder, "RainyWindow PostProcess Effect");
	
	// 第一步：创建带 Mipmap 的纹理副本 - 使用 ViewRect 尺寸
	FRDGTextureRef SceneColorWithMips;
	FIntPoint ViewSize = SceneColor.ViewRect.Size();
	{
		// 使用 ViewRect 尺寸创建 mip 纹理，而不是整个 SceneColor 纹理尺寸
		int32 NumMips = FMath::FloorLog2(FMath::Min(ViewSize.X, ViewSize.Y)) + 1;
		NumMips = FMath::Clamp(NumMips, 1, 8);  // 限制最大 mip 数量
		
		FRDGTextureDesc MipDesc = FRDGTextureDesc::Create2D(
			ViewSize,
			SceneColor.Texture->Desc.Format,
			FClearValueBinding::Black,
			TexCreate_ShaderResource | TexCreate_UAV,
			NumMips);
		
		SceneColorWithMips = GraphBuilder.CreateTexture(MipDesc, TEXT("RainyWindow SceneColor With Mips"));
		
		// 复制原始纹理到 mip 0 - 只复制 ViewRect 区域，并放到 (0,0)
		FRHICopyTextureInfo CopyToMipInfo;
		CopyToMipInfo.Size = FIntVector(ViewSize.X, ViewSize.Y, 1);
		CopyToMipInfo.SourcePosition = FIntVector(SceneColor.ViewRect.Min.X, SceneColor.ViewRect.Min.Y, 0);  // 从 ViewRect.Min 复制
		CopyToMipInfo.DestPosition = FIntVector(0, 0, 0);  // 复制到 (0,0)
		AddCopyTexturePass(GraphBuilder, SceneColor.Texture, SceneColorWithMips, CopyToMipInfo);
		
		// 使用 Compute Shader 生成 mipmap
		FGenerateMips::Execute(GraphBuilder, ViewFamily.GetFeatureLevel(), SceneColorWithMips, TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
	}


	// 第二步：创建输出纹理 - 使用 ViewRect 尺寸
	FRDGTextureRef OutputTexture;
	{
		FRDGTextureDesc OutputDesc = FRDGTextureDesc::Create2D(
			ViewSize,
			SceneColor.Texture->Desc.Format,
			FClearValueBinding::Black,
			TexCreate_UAV | TexCreate_ShaderResource);
		
		OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("RainyWindow Output Texture"));
	}



	// 第三步：执行 Compute Shader
	{
		FRainyWindowCS::FParameters* PassParameters = GraphBuilder.AllocParameters<FRainyWindowCS::FParameters>();
		PassParameters->OriginalSceneColor = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(SceneColorWithMips));  // 使用带 mip 的纹理
		PassParameters->OriginalSceneColorSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
		
		// Viewport 从 (0,0) 开始，因为 SceneColorWithMips 和 Output 纹理都是从 (0,0) 开始
		FScreenPassTextureViewport OutputViewport(ViewSize, FIntRect(FIntPoint::ZeroValue, ViewSize));
		PassParameters->SceneColorViewport = GetScreenPassTextureViewportParameters(OutputViewport);
		
		PassParameters->Output = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));
		PassParameters->Time = SceneView.Family->Time.GetRealTimeSeconds();
		PassParameters->RainAmount = GetRainAmount();  // 使用综合函数（蓝图优先）

		// 根据相机伸缩和闪电设置选择对应的 Shader 变体
		FRainyWindowCS::FPermutationDomain PermutationVector;
		PermutationVector.Set<FRainyWindowCS::FCameraZoomDim>(IsCameraZoomEnabled());
		PermutationVector.Set<FRainyWindowCS::FLightningDim>(IsLightningEnabled());


		FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(ViewSize, FComputeShaderUtils::kGolden2DGroupSize);

		TShaderMapRef<FRainyWindowCS> ComputeShader(GlobalShaderMap, PermutationVector);


		FComputeShaderUtils::AddPass(
			GraphBuilder,
			RDG_EVENT_NAME("RainyWindow CS %dx%d", ViewSize.X, ViewSize.Y),
			ComputeShader,
			PassParameters,
			GroupCount);
	}



	// 将输出复制回场景颜色 - 需要指定正确的区域
	{
		FRHICopyTextureInfo CopyInfo;
		CopyInfo.Size = FIntVector(SceneColor.ViewRect.Size().X, SceneColor.ViewRect.Size().Y, 1);
		CopyInfo.SourcePosition = FIntVector(0, 0, 0);  // Output 从 (0,0) 开始
		CopyInfo.DestPosition = FIntVector(SceneColor.ViewRect.Min.X, SceneColor.ViewRect.Min.Y, 0);  // 目标是 ViewRect.Min
		AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor.Texture, CopyInfo);
	}


	return SceneColor;
}
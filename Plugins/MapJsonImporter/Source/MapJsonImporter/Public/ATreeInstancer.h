// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ATreeInstancer.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * 从 JSON 文件读取点位，并用 HISM 批量实例化指定的 StaticMesh（适合树木/路灯等大批量相同模型）。
 *
 * 约定的 JSON 格式（单位默认 cm；可通过 PositionScale / 坐标轴开关调整）：
 * {
 *   "points": [
 *     { "x": 123.4, "y": 56.7, "z": 0.0, "yaw": 0, "scale": 1.0 },
 *     { "x": 200.0, "y": 80.0, "z": 0.0 }
 *   ]
 * }
 * yaw / scale 可选；也支持最简 [[x,y,z], [x,y,z]] 形式。
 */
UCLASS()
class MAPJSONIMPORTER_API AATreeInstancer : public AActor
{
	GENERATED_BODY()

public:
	AATreeInstancer();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	virtual void Tick(float DeltaTime) override;

	/** 用于实例化的 HISM 组件（层级剔除 + LOD，大批量场景性能最佳） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TreeInstancer")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISMComponent;

	/** JSON 文件绝对路径，例如 D:/TokyoMap/export/TreePoint/tree_points.json */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Input")
	FString JsonFilePath = TEXT("D:/TokyoMap/export/TreePoint/tree_points.json");

	/** 要实例化的静态网格数组（支持多种树，每个点位随机选一种） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Mesh")
	TArray<TObjectPtr<UStaticMesh>> TreeMeshes;

	/**
	 * StaticMesh 资产路径数组（备选）。当 TreeMeshes 中对应项为空时（如 Live Coding 后丢失），
	 * 会用此路径 LoadObject 加载。格式示例：/Game/Trees/Tree1.Tree1
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Mesh")
	TArray<FString> TreeMeshPaths;

	/** Fallback 网格：当 TreeMesh 为空时使用（默认引擎 Cube），方便先验证点位再换真实模型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Mesh")
	TObjectPtr<UStaticMesh> FallbackMesh;

	/** 使用 FallbackMesh 时的额外每实例缩放系数（作用在 InstanceScale 之上）。默认 (1,1,1) 不额外拉伸。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Mesh")
	FVector FallbackScale = FVector(1.f, 1.f, 1.f);

	/** 覆盖材质（可选，留空使用 Mesh 自带材质） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Mesh")
	TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;

	/** JSON 坐标 -> UE 世界坐标的统一缩放（若 JSON 单位是米，填 100） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	float PositionScale = 1.0f;

	/** 交换 Y 与 Z（某些 GIS/Y-up 数据用得上） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	bool bSwapYZ = false;

	/** 反转 X（右手坐标 -> 左手坐标时可用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	bool bFlipX = false;

	/** 反转 Y */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	bool bFlipY = false;

	/** 统一附加的偏移（世界坐标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	FVector PositionOffset = FVector::ZeroVector;

	/** 统一缩放（作用于每个实例） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	FVector InstanceScale = FVector(1.f, 1.f, 1.f);

	/** 最大绘制距离（0 = 无限远，不剔除）。如果远处的树看不见，请确保此值足够大或为 0 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Rendering")
	float EndCullDistance = 0.f;

	/** 每个实例完全随机旋转（Pitch/Yaw/Roll 全部随机，适合自然散布的树木） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	bool bFullRandomRotation = true;

	/** 仅随机 Yaw（bFullRandomRotation 为 false 时生效，JSON 未指定 yaw 时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform", meta=(EditCondition="!bFullRandomRotation"))
	bool bRandomYaw = false;

	/** 每个实例随机缩放范围（JSON 未指定 scale 时生效，X=最小 Y=最大） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Transform")
	FVector2D RandomScaleRange = FVector2D(0.85f, 1.15f);

	/** 运行时 BeginPlay 自动 Cook 一次（仅 PIE/打包场景才会触发，编辑器中不生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Input")
	bool bAutoCookOnBeginPlay = true;

	/** 编辑器里第一次把 Actor 拖进关卡时自动 Cook 一次（之后所有重建都必须显式点 Cook） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TreeInstancer|Input")
	bool bCookOnFirstPlacement = true;

	/** [只读] 自上次 Cook 以来是否有参数变更，true 表示需要重新 Cook 才能看到最新结果 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TreeInstancer|Status", meta=(DisplayName="Dirty (需重新 Cook)"))
	bool bDirty = false;

	/** [只读] 上次 Cook 时间戳（方便确认是否跑过） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TreeInstancer|Status")
	FString LastCookedAt;

	/** [只读] 上次 Cook 生成的实例数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TreeInstancer|Status")
	int32 LastCookedInstanceCount = 0;

	/**
	 * [主按钮] 显式 Cook：读 JSON → 生成 Transform → 绑 Mesh → 实例化。
	 * 任何参数改动都必须手动点一下它才会生效。
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TreeInstancer|Actions")
	int32 Cook();

	/** 清空所有实例（不清 UPROPERTY） */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TreeInstancer|Actions")
	void ClearInstances();

	/** 仅把当前 TreeMesh / FallbackMesh / OverrideMaterials 应用到 HISM 上，不重建实例 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TreeInstancer|Actions")
	void ApplyMeshOnly();

	/** 从 JSON 加载点位并重建所有实例；返回生成的实例数（= Cook 的内部实现，蓝图兼容保留） */
	UFUNCTION(BlueprintCallable, Category = "TreeInstancer")
	int32 LoadFromJson();

	/** 使用指定的点集重建实例（供蓝图/外部调用） */
	UFUNCTION(BlueprintCallable, Category = "TreeInstancer")
	int32 BuildFromTransforms(const TArray<FTransform>& Transforms);

	/** 把 Actor 自身位置移到所有实例的包围盒中心，方便在编辑器中按 F 聚焦 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TreeInstancer|Actions")
	void FocusOnInstancesCenter();

	/** 诊断：打印当前 TreeMesh / FallbackMesh / HISM 实际 Mesh 的状态到 Output Log */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "TreeInstancer|Actions")
	void DumpMeshStatus();

private:
	/** 解析 JSON 字符串得到 FTransform 列表 */
	bool ParseJson(const FString& JsonContent, TArray<FTransform>& OutTransforms) const;

	/** 应用坐标轴变换 */
	FVector ApplyAxis(const FVector& In) const;

	/** 解析 TreeMeshes 数组中指定索引的 Mesh：优先 UPROPERTY 引用，为空则用 TreeMeshPaths LoadObject */
	UStaticMesh* ResolveTreeMesh(int32 Index);

	/** 获取所有有效的 TreeMesh 列表（已解析） */
	TArray<UStaticMesh*> ResolveAllTreeMeshes();


};

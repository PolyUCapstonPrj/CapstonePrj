// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AStreetLampInstancer.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class URectLightComponent;
class USpotLightComponent;
class ULocalLightComponent;

/** 灯光类型（决定每个路灯实例生成哪种 LocalLight） */
UENUM(BlueprintType)
enum class ELampLightType : uint8
{
	/** 聚光灯：性能最优，推荐路灯/路灯杆等大批量光源场景 */
	SpotLight  UMETA(DisplayName = "SpotLight (聚光灯，性能优)"),

	/** 矩形面光源：光照质量最高，适合广告牌、霓虹灯、宽幅灯具，开销约为 SpotLight 的 2~3 倍 */
	RectLight  UMETA(DisplayName = "RectLight (矩形面光，质量优)")
};

/**
 * 从 JSON 文件读取点位，用 HISM 批量实例化路灯模型（高性能，适合 2000+ 路灯），
 * 灯光（RectLight）则单独为每个点位生成独立组件。
 *
 * 设计思路：
 * - 路灯模型：使用 HISM（与 ATreeInstancer 相同方案），一次 DrawCall 渲染所有路灯模型
 * - 灯光：逐个 Spawn URectLightComponent 挂在本 Actor 上
 * - 灯光支持距离剔除（MaxDrawDistance），远处的灯光自动关闭以节省性能
 *
 * 约定的 JSON 格式（与 ATreeInstancer 兼容）：
 * {
 *   "points": [
 *     { "x": 123.4, "y": 56.7, "z": 0.0, "yaw": 90 },
 *     { "x": 200.0, "y": 80.0, "z": 0.0 }
 *   ]
 * }
 * 也支持 [[x,y,z], ...] 形式。
 */
UCLASS()
class MAPJSONIMPORTER_API AAStreetLampInstancer : public AActor
{
	GENERATED_BODY()

public:
	AAStreetLampInstancer();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
	// ==================== HISM 组件 ====================

	/**
	 * 用于批量实例化路灯模型的 HISM 组件（内部使用，不在 Details 面板里显示）。
	 * 使用 BlueprintReadOnly 但不带 VisibleAnywhere/EditAnywhere，避免在 Details 顶部
	 * 暴露 HISM 自带的 "Static Mesh" 槽，与 LampMesh 字段产生混淆。
	 * 路灯模型请统一通过 'LampInstancer | Mesh' 分类下的 Lamp Mesh 字段来配置。
	 */
	UPROPERTY(BlueprintReadOnly, Category = "LampInstancer", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HISMComponent;

	// ==================== 输入 ====================

	/** JSON 文件绝对路径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Input")
	FString JsonFilePath = TEXT("D:/TokyoMap/export/StreetLamp/street_lamps.json");

	/** 运行时 BeginPlay 自动 Cook */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Input")
	bool bAutoCookOnBeginPlay = true;

	/** [已废弃] 编辑器自动 Cook 已被移除（避免在 LampMesh 未配置完毕时误触发），请始终手动点 'Cook' 按钮 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LampInstancer|Input")
	bool bCookOnFirstPlacement = false;

	// ==================== 路灯模型 ====================

	/** 路灯静态网格 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Mesh")
	TObjectPtr<UStaticMesh> LampMesh;

	/** 路灯模型资产路径（备选，当 LampMesh 为空时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Mesh")
	FString LampMeshPath;

	/** Fallback 网格（默认引擎 Cube） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Mesh")
	TObjectPtr<UStaticMesh> FallbackMesh;

	/** 覆盖材质（可选） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Mesh")
	TArray<TObjectPtr<UMaterialInterface>> OverrideMaterials;

	// ==================== 坐标变换 ====================

	/** JSON 坐标 -> UE 世界坐标的统一缩放（若 JSON 单位是米，填 100） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Transform")
	float PositionScale = 1.0f;

	/** 交换 Y 与 Z */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Transform")
	bool bSwapYZ = false;

	/** 反转 X */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Transform")
	bool bFlipX = false;

	/** 反转 Y */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Transform")
	bool bFlipY = false;

	/** 统一附加的偏移（世界坐标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Transform")
	FVector PositionOffset = FVector::ZeroVector;

	/** 统一缩放（作用于每个路灯实例） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Transform")
	FVector InstanceScale = FVector(1.f, 1.f, 1.f);

	/** 仅随机 Yaw（JSON 未指定 yaw 时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Transform")
	bool bRandomYaw = false;

	/** HISM 最大绘制距离（0 = 无限远） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Rendering")
	float MeshEndCullDistance = 0.f;

	// ==================== 灯光配置 ====================

	/**
	 * 【灯光类型】每个路灯实例生成哪种灯光组件。
	 *   - SpotLight  : 聚光灯，性能优；推荐
	 *   - RectLight  : 矩形面光源，光照质量更好但开销更大
	 * 切换后需要重新点 'Cook' 才会生效。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (DisplayName = "灯光类型 (Light Type)"))
	ELampLightType LightType = ELampLightType::SpotLight;

	/** 是否为每个路灯生成灯光组件 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light")
	bool bEnableRectLight = true;

	/**
	 * 是否使用 StaticMesh 上的 Socket 作为灯光生成位置。
	 * 启用后：每个 Socket 生成一盏灯，灯的朝向 = Socket 的 -Z 方向（即 Socket 的 Z 轴朝下作为光照方向）。
	 * 关闭后：使用 LightOffset / LightRotation 单灯生成。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light")
	bool bUseSocketsForLights = true;

	/**
	 * 仅匹配名称包含此前缀的 Socket（不区分大小写，留空则使用全部 Socket）。
	 * 例如 "Light" 只会拾取名为 Light、Light_01、LightL、LightR 等的 Socket。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (EditCondition = "bUseSocketsForLights"))
	FString SocketNameFilter = TEXT("");

	/** [仅在不使用 Socket 时生效] RectLight/SpotLight 相对于路灯模型原点的偏移（本地空间） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (EditCondition = "!bUseSocketsForLights"))
	FVector LightOffset = FVector(0.f, 0.f, 500.f);

	/** [仅在不使用 Socket 时生效] 灯光相对旋转（本地空间，默认朝下照射） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (EditCondition = "!bUseSocketsForLights"))
	FRotator LightRotation = FRotator(-90.f, 0.f, 0.f);

	/** [仅 SpotLight] 内锥角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (EditCondition = "LightType == ELampLightType::SpotLight", ClampMin = "0.0", ClampMax = "80.0"))
	float SpotInnerConeAngle = 20.f;

	/** [仅 SpotLight] 外锥角（度） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (EditCondition = "LightType == ELampLightType::SpotLight", ClampMin = "0.0", ClampMax = "80.0"))
	float SpotOuterConeAngle = 45.f;

	/** RectLight 强度 (cd) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light")
	float LightIntensity = 5000.f;

	/** RectLight 颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light")
	FLinearColor LightColor = FLinearColor(1.f, 0.95f, 0.8f, 1.f);

	/** [仅 RectLight] 矩形宽度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (EditCondition = "LightType == ELampLightType::RectLight"))
	float LightWidth = 100.f;

	/** [仅 RectLight] 矩形高度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (EditCondition = "LightType == ELampLightType::RectLight"))
	float LightHeight = 100.f;

	/** RectLight 衰减半径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light")
	float LightAttenuationRadius = 1000.f;

	/** RectLight 是否投射阴影 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light")
	bool bLightCastShadows = true;

	/** RectLight 最大绘制距离（超过此距离的灯光不渲染，节省性能；0 = 不剔除） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light")
	float LightMaxDrawDistance = 5000.f;

	/**
	 * 灯光淡出过渡距离（避免远处灯光"突然消失"）。
	 * 在 [LightMaxDrawDistance - LightMaxDistanceFadeRange, LightMaxDrawDistance] 区间内灯光强度线性渐隐到 0。
	 * 建议值：LightMaxDrawDistance 的 10%~20%。0 = 关闭渐隐（硬剔除）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light", meta = (ClampMin = "0.0"))
	float LightMaxDistanceFadeRange = 800.f;

	// ==================== 阴影性能控制 ====================

	/**
	 * 阴影分辨率缩放（仅当 bLightCastShadows = true 时生效）。
	 * 阴影是 LocalLight 最大的开销，路灯类灯光无需高分辨率阴影。
	 * 推荐值：0.25 ~ 0.5（即 1/4 ~ 1/2 分辨率），可显著降低 GPU 阴影开销。
	 * 1.0 = 默认（不缩放）。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LampInstancer|Light|Shadow", meta = (ClampMin = "0.0625", ClampMax = "1.0", EditCondition = "bLightCastShadows"))
	float LightShadowResolutionScale = 0.5f;

	// ==================== 状态 ====================

	/** [只读] 是否有参数变更需要重新 Cook */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LampInstancer|Status")
	bool bDirty = false;

	/** [只读] 数据是否已 Bake（Bake 后不再依赖 JSON 文件） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LampInstancer|Status")
	bool bIsBaked = false;

	/** [只读] 上次 Cook 时间戳 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LampInstancer|Status")
	FString LastCookedAt;

	/** [只读] 上次 Cook 生成的路灯实例数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LampInstancer|Status")
	int32 LastCookedInstanceCount = 0;

	/** [只读] 上次 Cook 生成的灯光数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LampInstancer|Status")
	int32 LastCookedLightCount = 0;

	// ==================== 操作 ====================

	/** [主按钮] Cook：读 JSON → HISM 实例化路灯模型 + 生成灯光 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LampInstancer|Actions")
	void Cook();

	/**
	 * [Bake] 将当前生成的 HISM 实例和灯光固化到关卡中。
	 * Bake 后保存关卡即可脱离 JSON 文件依赖，下次打开关卡无需重新 Cook。
	 * 注意：Bake 后如需修改，需先 Unbake 再重新 Cook。
	 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LampInstancer|Actions")
	void Bake();

	/** [Unbake] 取消固化状态，恢复为动态模式（可重新 Cook） */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LampInstancer|Actions")
	void Unbake();

	/** 清空所有实例和灯光 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LampInstancer|Actions")
	void ClearAll();

	/** 仅清除灯光（保留 HISM 模型实例） */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LampInstancer|Actions")
	void ClearLights();

	/** 从 JSON 加载并生成（内部由 Cook 调用） */
	UFUNCTION(BlueprintCallable, Category = "LampInstancer")
	int32 LoadFromJson();

	/** 把 Actor 位置移到实例中心 */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "LampInstancer|Actions")
	void FocusOnInstancesCenter();

private:
	/** 解析 JSON 字符串得到 FTransform 列表 */
	bool ParseJson(const FString& JsonContent, TArray<FTransform>& OutTransforms) const;

	/** 应用坐标轴变换 */
	FVector ApplyAxis(const FVector& In) const;

	/** 解析路灯 Mesh */
	UStaticMesh* ResolveLampMesh();

	/** 为指定的 Transform 列表生成灯光 */
	void SpawnLights(const TArray<FTransform>& Transforms);

	/** 已生成的灯光组件列表（RectLight 或 SpotLight，统一用基类管理） */
	UPROPERTY()
	TArray<TObjectPtr<ULocalLightComponent>> SpawnedLights;

	/** 缓存的 Transform 列表（用于内部使用） */
	UPROPERTY()
	TArray<FTransform> CachedTransforms;
};

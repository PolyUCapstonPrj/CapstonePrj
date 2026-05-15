// Fill out your copyright notice in the Description page of Project Settings.

#include "AStreetLampInstancer.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/RectLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

AAStreetLampInstancer::AAStreetLampInstancer()
{
	PrimaryActorTick.bCanEverTick = false;

	// 根组件
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// 默认添加 StreetLamp Tag，供 ADayNightCycle 自动查找并控制灯光开关
	Tags.Add(FName(TEXT("StreetLamp")));

	// HISM 组件（用于批量渲染路灯模型）
	HISMComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISM"));
	HISMComponent->SetupAttachment(RootComponent);
	HISMComponent->SetMobility(EComponentMobility::Static);
	HISMComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	HISMComponent->SetCollisionProfileName(TEXT("BlockAll"));
	HISMComponent->bUseAsOccluder = false;
	HISMComponent->SetCullDistances(0, 0);
	HISMComponent->bDisableCollision = false;
	HISMComponent->bNeverDistanceCull = true;
	HISMComponent->bEnableDensityScaling = false;
#if WITH_EDITORONLY_DATA
	// 在 Components 树中隐藏该组件，避免用户误把 Mesh 拖到 HISM 自带的 "Static Mesh" 槽里。
	// 路灯模型必须通过 LampMesh 字段配置（Cook 时由代码统一赋值给 HISM）。
	HISMComponent->bVisualizeComponent = false;
	HISMComponent->CreationMethod = EComponentCreationMethod::Native;
#endif

	// 默认 Fallback Mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultCubeFinder.Succeeded())
	{
		FallbackMesh = DefaultCubeFinder.Object;
	}
}

void AAStreetLampInstancer::BeginPlay()
{
	Super::BeginPlay();

	// 如果已经 Bake，数据已持久化在关卡中，无需重新 Cook
	if (bIsBaked)
	{
		UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 已 Bake，跳过 BeginPlay Cook。"));
		return;
	}

	if (bAutoCookOnBeginPlay)
	{
		if (!HISMComponent || HISMComponent->GetInstanceCount() == 0)
		{
			Cook();
		}
	}
}

void AAStreetLampInstancer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 注意：此处不再自动调用 Cook()。
	// 原因：UE 在属性修改时会重置/重建 Actor 并触发 OnConstruction，
	// 此时 LampMesh 等用户配置可能尚未应用，自动 Cook 会因 LampMesh 为空而误报错，
	// 同时也容易导致 Details 面板按钮渲染异常。请显式点击 'Cook' 按钮。
}

#if WITH_EDITOR
void AAStreetLampInstancer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	// 仅标记为 dirty，绝不在这里触发 Cook，避免与 OnConstruction 时序冲突。
	bDirty = true;
}
#endif

FVector AAStreetLampInstancer::ApplyAxis(const FVector& In) const
{
	FVector V = In;
	if (bSwapYZ)
	{
		Swap(V.Y, V.Z);
	}
	if (bFlipX) V.X = -V.X;
	if (bFlipY) V.Y = -V.Y;
	return V * PositionScale + PositionOffset;
}

UStaticMesh* AAStreetLampInstancer::ResolveLampMesh()
{
	if (LampMesh)
	{
		return LampMesh;
	}

	if (!LampMeshPath.IsEmpty())
	{
		UStaticMesh* Loaded = LoadObject<UStaticMesh>(nullptr, *LampMeshPath);
		if (Loaded)
		{
			LampMesh = Loaded;
			UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 通过 LampMeshPath 加载成功: %s"), *LampMeshPath);
			return Loaded;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[LampInstancer] LampMeshPath 加载失败: %s"), *LampMeshPath);
		}
	}

	return nullptr;
}

void AAStreetLampInstancer::ClearAll()
{
	// 清除 HISM 实例
	if (HISMComponent)
	{
		HISMComponent->ClearInstances();
	}

	// 清除灯光
	ClearLights();

	CachedTransforms.Empty();
	LastCookedInstanceCount = 0;
	LastCookedLightCount = 0;
	bDirty = true;
	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 已清除所有实例和灯光。"));
}

void AAStreetLampInstancer::ClearLights()
{
	for (ULocalLightComponent* Light : SpawnedLights)
	{
		if (Light && IsValid(Light))
		{
			Light->DestroyComponent();
		}
	}
	SpawnedLights.Empty();
	LastCookedLightCount = 0;
	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 已清除所有灯光。"));
}

void AAStreetLampInstancer::Cook()
{
	if (bIsBaked)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LampInstancer] 当前已处于 Bake 状态，请先点 'Unbake' 再重新 Cook。"));
		return;
	}

	// 明确检查当前 LampMesh 字段状态，方便用户诊断
	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] === Cook 开始 === LampMesh=%s, LampMeshPath='%s', bUseSocketsForLights=%s"),
		LampMesh ? *LampMesh->GetName() : TEXT("<None>"),
		*LampMeshPath,
		bUseSocketsForLights ? TEXT("true") : TEXT("false"));

	// 严格检查：必须设置 LampMesh（不再静默回退到 Cube）
	UStaticMesh* MeshToUse = ResolveLampMesh();
	if (!MeshToUse)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[LampInstancer] ❌ Cook 失败：LampMesh 未设置！请在 Details 面板的 'LampInstancer | Mesh' 分类下指定一个路灯 StaticMesh。"));
		return;
	}

	const int32 Count = LoadFromJson();
	bDirty = false;
	LastCookedInstanceCount = Count;
	LastCookedAt = FDateTime::Now().ToString();
	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] ✓ Cook 完成 @ %s —— 模型实例数: %d, 灯光数: %d"),
		*LastCookedAt, Count, LastCookedLightCount);
}

void AAStreetLampInstancer::Bake()
{
	if (bIsBaked)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LampInstancer] 已经处于 Bake 状态，无需重复操作。"));
		return;
	}

	// 检查是否有数据可以 Bake
	const int32 InstanceCount = HISMComponent ? HISMComponent->GetInstanceCount() : 0;
	if (InstanceCount == 0 && SpawnedLights.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LampInstancer] 无实例和灯光可 Bake，请先执行 Cook。"));
		return;
	}

	// 将灯光组件标记为 Instance 创建方式，这样 UE 序列化系统会将其保存到关卡
	for (ULocalLightComponent* Light : SpawnedLights)
	{
		if (Light && IsValid(Light))
		{
			Light->CreationMethod = EComponentCreationMethod::Instance;
		}
	}

	bIsBaked = true;
	bDirty = false;

	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] ✓ Bake 完成！模型实例: %d, 灯光: %d。保存关卡后即可脱离 JSON 文件依赖。"),
		InstanceCount, SpawnedLights.Num());
	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 提示：请执行 File → Save Current Level 保存关卡。"));
}

void AAStreetLampInstancer::Unbake()
{
	if (!bIsBaked)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LampInstancer] 当前未处于 Bake 状态。"));
		return;
	}

	bIsBaked = false;
	bDirty = true;

	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 已取消 Bake 状态，现在可以重新 Cook 或修改参数。"));
}

void AAStreetLampInstancer::SpawnLights(const TArray<FTransform>& Transforms)
{
	// 先清除旧灯光
	ClearLights();

	if (!bEnableRectLight)
	{
		UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 灯光已禁用，跳过灯光生成。"));
		return;
	}

	// ---------- 收集灯光局部变换列表（每个 Lamp 实例可能有多个 Socket → 多盏灯） ----------
	struct FLocalLightXform
	{
		FVector  RelLoc;
		FRotator RelRot;
	};
	TArray<FLocalLightXform> LocalLightXforms;

	if (bUseSocketsForLights)
	{
		// 注意：这里只能用 LampMesh（带 Socket 的真实路灯模型），
		// 不能 fallback 到 Cube，否则永远找不到 Socket。
		UStaticMesh* MeshForSocket = ResolveLampMesh();

		if (!MeshForSocket)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[LampInstancer] [Socket 模式] LampMesh 为空（且不允许 fallback 到 Cube），")
				TEXT("请在 Details 面板中正确指定 LampMesh 资产，或关闭 bUseSocketsForLights。"));
		}
		else
		{
			const TArray<TObjectPtr<UStaticMeshSocket>>& Sockets = MeshForSocket->Sockets;
			UE_LOG(LogTemp, Log,
				TEXT("[LampInstancer] [Socket 模式] 当前 Mesh: '%s'，Sockets.Num() = %d，过滤前缀: '%s'"),
				*MeshForSocket->GetName(), Sockets.Num(), *SocketNameFilter);

			for (const TObjectPtr<UStaticMeshSocket>& SocketPtr : Sockets)
			{
				const UStaticMeshSocket* Socket = SocketPtr.Get();
				if (!Socket) continue;

				const FString SockNameStr = Socket->SocketName.ToString();

				// 名称过滤（前缀匹配，大小写不敏感）
				if (!SocketNameFilter.IsEmpty()
					&& !SockNameStr.StartsWith(SocketNameFilter, ESearchCase::IgnoreCase))
				{
					UE_LOG(LogTemp, Verbose, TEXT("[LampInstancer]   跳过 Socket '%s'（不匹配前缀 '%s'）"),
						*SockNameStr, *SocketNameFilter);
					continue;
				}

				FLocalLightXform X;
				X.RelLoc = Socket->RelativeLocation;
				// 约定：用户已在 Mesh 资产中将 Socket 的 +Z 轴设为期望的发光方向（朝下）。
				// UE 中 Light 的发光方向是局部 +X 轴，因此需要把 "Light 的 +X" 对齐到 "Socket 的 +Z"。
				// 解决方法：在 Socket 的旋转之前，先让 Light 自身绕 Y 轴 Pitch +90°
				//   —— 这样 Light 的 +X 就指向了 +Z 方向；
				// 再左乘 Socket 的旋转，光线就会沿 Socket 的 +Z 射出。
				const FQuat AlignXToZ(FRotator(90.f, 0.f, 0.f));
				X.RelRot = (FQuat(Socket->RelativeRotation) * AlignXToZ).Rotator();
				LocalLightXforms.Add(X);

				UE_LOG(LogTemp, Log,
					TEXT("[LampInstancer]   ✓ 采用 Socket '%s'  Loc=%s  Rot=%s"),
					*SockNameStr, *X.RelLoc.ToString(), *X.RelRot.ToString());
			}
		}

		if (LocalLightXforms.Num() == 0)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[LampInstancer] ❌ 未采集到任何 Socket！请检查：")
				TEXT("1) LampMesh '%s' 是否真的在资产中添加了 Socket；")
				TEXT("2) SocketNameFilter='%s' 是否过严（留空可采集所有 Socket）。")
				TEXT("如不需要 Socket，请关闭 bUseSocketsForLights。"),
				MeshForSocket ? *MeshForSocket->GetName() : TEXT("<None>"),
				*SocketNameFilter);
			return;
		}
	}
	else
	{
		FLocalLightXform X;
		X.RelLoc = LightOffset;
		X.RelRot = LightRotation;
		LocalLightXforms.Add(X);
	}

	// ---------- 真正生成灯光 ----------
	int32 LightCount = 0;
	const FTransform ActorXform = GetActorTransform();

	for (int32 i = 0; i < Transforms.Num(); ++i)
	{
		// Transforms[i] 是 Actor 本地空间下的 HISM 实例变换（含 Scale/Rot/Loc）
		const FTransform& InstanceLocal = Transforms[i];

		for (int32 s = 0; s < LocalLightXforms.Num(); ++s)
		{
			const FLocalLightXform& LX = LocalLightXforms[s];

			// Socket 在 Mesh 自己的局部空间下的变换
			const FTransform SocketLocal(LX.RelRot, LX.RelLoc, FVector::OneVector);

			// 关键修正：用 FTransform 组合，UE 会自动处理 Scale/Rotation/Translation 的正确顺序
			// World = Actor * Instance * SocketLocal
			const FTransform LightWorldXform = SocketLocal * InstanceLocal * ActorXform;

			const FVector  LightWorldPos = LightWorldXform.GetLocation();
			const FRotator LightWorldRot = LightWorldXform.GetRotation().Rotator();

			ULocalLightComponent* Light = nullptr;

			if (LightType == ELampLightType::SpotLight)
			{
				FName Name = *FString::Printf(TEXT("SpotLight_%d_%d"), i, s);
				USpotLightComponent* Spot = NewObject<USpotLightComponent>(this, Name);
				Spot->SetInnerConeAngle(SpotInnerConeAngle);
				Spot->SetOuterConeAngle(SpotOuterConeAngle);
				Light = Spot;
			}
			else
			{
				FName Name = *FString::Printf(TEXT("RectLight_%d_%d"), i, s);
				URectLightComponent* Rect = NewObject<URectLightComponent>(this, Name);
				Rect->SetSourceWidth(LightWidth);
				Rect->SetSourceHeight(LightHeight);
				Light = Rect;
			}

			Light->SetupAttachment(RootComponent);
			Light->SetWorldLocation(LightWorldPos);
			Light->SetWorldRotation(LightWorldRot);
			Light->SetIntensity(LightIntensity);
			Light->SetLightColor(LightColor);
			Light->SetAttenuationRadius(LightAttenuationRadius);
			Light->SetCastShadows(bLightCastShadows);
			Light->SetMobility(EComponentMobility::Stationary);
			Light->CreationMethod = EComponentCreationMethod::Instance;

			// 距离剔除：远处的灯不渲染（核心性能优化）
			if (LightMaxDrawDistance > 0.f)
			{
				Light->MaxDrawDistance = LightMaxDrawDistance;
				// 渐隐过渡，避免突然消失（FadeRange 必须 < MaxDrawDistance）
				Light->MaxDistanceFadeRange = FMath::Min(LightMaxDistanceFadeRange, LightMaxDrawDistance - 1.f);
			}

			// 阴影分辨率缩放：路灯阴影无需高分辨率，显著降低 GPU 开销
			if (bLightCastShadows)
			{
				Light->ShadowResolutionScale = LightShadowResolutionScale;
			}

			Light->RegisterComponent();
			SpawnedLights.Add(Light);
			LightCount++;
		}
	}

	LastCookedLightCount = LightCount;
	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 已生成 %d 个 %s（每个实例 %d 盏灯）。"),
		LightCount,
		LightType == ELampLightType::SpotLight ? TEXT("SpotLight") : TEXT("RectLight"),
		LocalLightXforms.Num());
}

int32 AAStreetLampInstancer::LoadFromJson()
{
	// 清洗路径
	FString CleanPath = JsonFilePath;
	CleanPath.TrimStartAndEndInline();
	CleanPath.TrimQuotesInline();
	CleanPath.TrimStartAndEndInline();
	CleanPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	if (CleanPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LampInstancer] JsonFilePath 为空。"));
		return 0;
	}

	if (CleanPath != JsonFilePath)
	{
		UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 路径已自动规范化: %s -> %s"), *JsonFilePath, *CleanPath);
		JsonFilePath = CleanPath;
	}

	if (!FPaths::FileExists(CleanPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[LampInstancer] 文件不存在: %s"), *CleanPath);
		return 0;
	}

	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *CleanPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[LampInstancer] 读取 JSON 失败: %s"), *CleanPath);
		return 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 已读取 JSON: %s (%d 字符)"), *CleanPath, JsonContent.Len());

	TArray<FTransform> Transforms;
	if (!ParseJson(JsonContent, Transforms))
	{
		UE_LOG(LogTemp, Error, TEXT("[LampInstancer] 解析 JSON 失败: %s"), *CleanPath);
		return 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 解析到 %d 个点"), Transforms.Num());

	if (Transforms.Num() == 0)
	{
		return 0;
	}

	CachedTransforms = Transforms;

	// ========== 1. HISM 实例化路灯模型 ==========
	if (!HISMComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[LampInstancer] HISMComponent 为空！"));
		return 0;
	}

	UStaticMesh* MeshToUse = ResolveLampMesh();

	// Cook() 中已严格检查过 LampMesh，这里 MeshToUse 一定非空
	HISMComponent->SetStaticMesh(MeshToUse);

	// 覆盖材质
	for (int32 MatIdx = 0; MatIdx < OverrideMaterials.Num(); ++MatIdx)
	{
		if (OverrideMaterials[MatIdx])
		{
			HISMComponent->SetMaterial(MatIdx, OverrideMaterials[MatIdx]);
		}
	}

	// 剔除距离
	HISMComponent->SetCullDistances(0, MeshEndCullDistance > 0.f ? static_cast<int32>(MeshEndCullDistance) : 0);
	HISMComponent->bNeverDistanceCull = (MeshEndCullDistance <= 0.f);

	// 清空旧实例
	HISMComponent->ClearInstances();

	// 添加实例
	HISMComponent->PreAllocateInstancesMemory(Transforms.Num());
	HISMComponent->AddInstances(Transforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/false);

	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] HISM 实例化完成: %d 个路灯模型 (Mesh: %s)"),
		HISMComponent->GetInstanceCount(), *MeshToUse->GetName());

	// ========== 2. 生成灯光 ==========
	SpawnLights(Transforms);

	// 诊断日志
	if (Transforms.Num() > 0)
	{
		FBox LocalBox(ForceInit);
		for (const FTransform& T : Transforms)
		{
			LocalBox += T.GetLocation();
		}
		UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 点集本地中心: %s  尺寸: %s"),
			*LocalBox.GetCenter().ToString(), *LocalBox.GetSize().ToString());
	}

	return HISMComponent ? HISMComponent->GetInstanceCount() : 0;
}

void AAStreetLampInstancer::FocusOnInstancesCenter()
{
	if (!HISMComponent) return;

	const int32 TotalNum = HISMComponent->GetInstanceCount();
	if (TotalNum == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LampInstancer] 无实例，无法聚焦。"));
		return;
	}

	FBox LocalBox(ForceInit);
	for (int32 i = 0; i < TotalNum; ++i)
	{
		FTransform T;
		HISMComponent->GetInstanceTransform(i, T, /*bWorldSpace=*/false);
		LocalBox += T.GetLocation();
	}

	const FVector LocalCenter = LocalBox.GetCenter();
	const FVector DeltaWorld = GetActorTransform().TransformVector(LocalCenter);
	const FVector NewActorLoc = GetActorLocation() + DeltaWorld;

	// 把所有实例反向偏移
	for (int32 i = 0; i < TotalNum; ++i)
	{
		FTransform T;
		HISMComponent->GetInstanceTransform(i, T, /*bWorldSpace=*/false);
		T.AddToTranslation(-LocalCenter);
		HISMComponent->UpdateInstanceTransform(i, T, /*bWorldSpace=*/false, /*bMarkRenderStateDirty=*/false, /*bTeleport=*/true);
	}
	HISMComponent->MarkRenderStateDirty();

	// 同步移动灯光
	for (ULocalLightComponent* Light : SpawnedLights)
	{
		if (Light && IsValid(Light))
		{
			Light->SetWorldLocation(Light->GetComponentLocation() - DeltaWorld + (NewActorLoc - GetActorLocation()));
		}
	}

	SetActorLocation(NewActorLoc);
	UE_LOG(LogTemp, Log, TEXT("[LampInstancer] 已聚焦到中心: %s (共 %d 实例)"), *NewActorLoc.ToString(), TotalNum);
}

bool AAStreetLampInstancer::ParseJson(const FString& JsonContent, TArray<FTransform>& OutTransforms) const
{
	OutTransforms.Reset();

	// 先尝试对象格式 { "points": [...] }
	TSharedPtr<FJsonObject> RootObj;
	TSharedRef<TJsonReader<>> ObjReader = TJsonReaderFactory<>::Create(JsonContent);

	const TArray<TSharedPtr<FJsonValue>>* PointsArray = nullptr;
	TArray<TSharedPtr<FJsonValue>> TopLevelArray;

	if (FJsonSerializer::Deserialize(ObjReader, RootObj) && RootObj.IsValid())
	{
		// 常见字段名兼容
		static const TArray<FString> Candidates = {
			TEXT("points"), TEXT("Points"),
			TEXT("lamps"), TEXT("Lamps"),
			TEXT("lights"), TEXT("Lights"),
			TEXT("data")
		};
		for (const FString& Key : Candidates)
		{
			const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
			if (RootObj->TryGetArrayField(Key, Arr))
			{
				PointsArray = Arr;
				break;
			}
		}
	}
	else
	{
		// 尝试顶层数组格式
		TSharedRef<TJsonReader<>> ArrReader = TJsonReaderFactory<>::Create(JsonContent);
		if (FJsonSerializer::Deserialize(ArrReader, TopLevelArray))
		{
			PointsArray = &TopLevelArray;
		}
	}

	if (!PointsArray)
	{
		return false;
	}

	OutTransforms.Reserve(PointsArray->Num());

	for (const TSharedPtr<FJsonValue>& Val : *PointsArray)
	{
		if (!Val.IsValid()) continue;

		FVector Pos = FVector::ZeroVector;
		float Yaw = 0.f;
		bool bHasYaw = false;

		if (Val->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
			if (!Obj.IsValid()) continue;

			double X = 0, Y = 0, Z = 0;
			if (!Obj->TryGetNumberField(TEXT("x"), X)) Obj->TryGetNumberField(TEXT("X"), X);
			if (!Obj->TryGetNumberField(TEXT("y"), Y)) Obj->TryGetNumberField(TEXT("Y"), Y);
			if (!Obj->TryGetNumberField(TEXT("z"), Z)) Obj->TryGetNumberField(TEXT("Z"), Z);
			Pos = FVector(X, Y, Z);

			double DYaw = 0;
			if (Obj->TryGetNumberField(TEXT("yaw"), DYaw) || Obj->TryGetNumberField(TEXT("Yaw"), DYaw))
			{
				Yaw = (float)DYaw;
				bHasYaw = true;
			}
			// 注意：JSON 中的 scale 字段被忽略，路灯使用 StaticMesh 原始大小（仅受 InstanceScale 影响）
		}
		else if (Val->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& Arr = Val->AsArray();
			if (Arr.Num() >= 3)
			{
				Pos = FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
			}
			if (Arr.Num() >= 4) { Yaw = (float)Arr[3]->AsNumber(); bHasYaw = true; }
			// 注意：数组第 5 位（scale）被忽略
		}
		else
		{
			continue;
		}

		const FVector WorldPos = ApplyAxis(Pos);

		// 旋转
		FRotator Rot = FRotator::ZeroRotator;
		if (bHasYaw)
		{
			Rot.Yaw = Yaw;
		}
		else if (bRandomYaw)
		{
			Rot.Yaw = FMath::FRandRange(0.f, 360.f);
		}

		// 缩放：直接使用 InstanceScale（默认 1,1,1 = StaticMesh 原始大小），不再读取 JSON 中的 scale
		const FVector FinalScale = InstanceScale;

		OutTransforms.Emplace(Rot, WorldPos, FinalScale);
	}

	return true;
}

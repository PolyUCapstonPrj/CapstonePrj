// Fill out your copyright notice in the Description page of Project Settings.

#include "ATreeInstancer.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

AATreeInstancer::AATreeInstancer()
{
	PrimaryActorTick.bCanEverTick = false;

	// 创建一个默认的根组件（SceneComponent）
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// 创建单个 HISM 组件
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

	// 默认加载引擎自带的 Cube 作为 fallback，避免 TreeMesh 未设置时完全看不到东西
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultCubeFinder.Succeeded())
	{
		FallbackMesh = DefaultCubeFinder.Object;
	}
}

void AATreeInstancer::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoCookOnBeginPlay)
	{
		if (!HISMComponent || HISMComponent->GetInstanceCount() == 0)
		{
			Cook();
		}
	}
}

void AATreeInstancer::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

#if WITH_EDITOR
	if (bCookOnFirstPlacement && GetWorld() && !GetWorld()->IsGameWorld()
	    && LastCookedInstanceCount == 0)
	{
		if (!HISMComponent || HISMComponent->GetInstanceCount() == 0)
		{
			Cook();
		}
	}
#endif
}

void AATreeInstancer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

#if WITH_EDITOR
void AATreeInstancer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName Name = PropertyChangedEvent.GetPropertyName();

	// 只影响 Mesh 外观的属性：可以即时应用，不用重新实例化
	if (Name == GET_MEMBER_NAME_CHECKED(AATreeInstancer, TreeMeshes) ||
	    Name == GET_MEMBER_NAME_CHECKED(AATreeInstancer, TreeMeshPaths) ||
	    Name == GET_MEMBER_NAME_CHECKED(AATreeInstancer, OverrideMaterials) ||
	    Name == GET_MEMBER_NAME_CHECKED(AATreeInstancer, FallbackMesh))
	{
		bDirty = true;
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] Mesh 配置已修改，请点 'Cook' 按钮重新生成实例。"));
		return;
	}

	// 其它影响实例位置/数量/缩放的属性变更：仅标脏，不自动重建
	static const TSet<FName> DirtyTriggers = {
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, JsonFilePath),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, PositionScale),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, bSwapYZ),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, bFlipX),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, bFlipY),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, PositionOffset),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, InstanceScale),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, bRandomYaw),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, bFullRandomRotation),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, RandomScaleRange),
		GET_MEMBER_NAME_CHECKED(AATreeInstancer, FallbackScale),
	};
	if (DirtyTriggers.Contains(Name))
	{
		bDirty = true;
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] 参数 '%s' 已修改，请点 Details 面板的 'Cook' 按钮重新生成实例。"),
			*Name.ToString());
	}
}
#endif

FVector AATreeInstancer::ApplyAxis(const FVector& In) const
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

void AATreeInstancer::ClearInstances()
{
	if (HISMComponent)
	{
		HISMComponent->ClearInstances();
	}
	LastCookedInstanceCount = 0;
	bDirty = true;
}

void AATreeInstancer::ApplyMeshOnly()
{
	if (!HISMComponent) return;

	TArray<UStaticMesh*> Meshes = ResolveAllTreeMeshes();

	UStaticMesh* MeshToUse = nullptr;
	if (Meshes.Num() > 0)
	{
		MeshToUse = Meshes[0];
	}
	else if (FallbackMesh)
	{
		MeshToUse = FallbackMesh;
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] ApplyMeshOnly: 无有效 TreeMesh，使用 FallbackMesh。"));
	}

	if (MeshToUse)
	{
		HISMComponent->SetStaticMesh(MeshToUse);
		// 应用覆盖材质
		for (int32 MatIdx = 0; MatIdx < OverrideMaterials.Num(); ++MatIdx)
		{
			HISMComponent->SetMaterial(MatIdx, OverrideMaterials[MatIdx]);
		}
		HISMComponent->MarkRenderStateDirty();
	}

	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] ApplyMeshOnly: 已应用 Mesh '%s' 到 HISM 组件。"),
		MeshToUse ? *MeshToUse->GetName() : TEXT("<None>"));
}

int32 AATreeInstancer::Cook()
{
	const int32 Count = LoadFromJson();
	bDirty = false;
	LastCookedInstanceCount = Count;
	LastCookedAt = FDateTime::Now().ToString();
	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] Cook 完成 @ %s —— 实例数: %d"), *LastCookedAt, Count);
	return Count;
}

int32 AATreeInstancer::BuildFromTransforms(const TArray<FTransform>& Transforms)
{
	if (!HISMComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[TreeInstancer] HISMComponent 为空！"));
		return 0;
	}

	if (Transforms.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] 点集为空，未生成任何实例。"));
		return 0;
	}

	// 解析所有有效的 Mesh，取第一个作为 HISM 使用的 Mesh
	TArray<UStaticMesh*> ValidMeshes = ResolveAllTreeMeshes();
	const bool bUsingFallback = (ValidMeshes.Num() == 0) && (FallbackMesh != nullptr);

	if (ValidMeshes.Num() == 0 && !FallbackMesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] TreeMeshes 和 FallbackMesh 都未设置，跳过实例化。"));
		return 0;
	}

	UStaticMesh* MeshToUse = nullptr;
	if (bUsingFallback)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] 无有效 TreeMesh，使用 FallbackMesh 预览点位: %s"), *FallbackMesh->GetName());
		MeshToUse = FallbackMesh;
	}
	else
	{
		// 使用第一个有效的 Mesh
		MeshToUse = ValidMeshes[0];
	}

	// 绑定 Mesh
	HISMComponent->SetStaticMesh(MeshToUse);

	// 覆盖材质（仅非 Fallback 时）
	if (!bUsingFallback)
	{
		for (int32 MatIdx = 0; MatIdx < OverrideMaterials.Num(); ++MatIdx)
		{
			HISMComponent->SetMaterial(MatIdx, OverrideMaterials[MatIdx]);
		}
	}

	// 应用剔除距离
	HISMComponent->SetCullDistances(0, EndCullDistance > 0.f ? static_cast<int32>(EndCullDistance) : 0);
	HISMComponent->bNeverDistanceCull = (EndCullDistance <= 0.f);
	HISMComponent->bEnableDensityScaling = false;

	// 清空旧实例
	HISMComponent->ClearInstances();

	// 构建 Transform 列表（应用 FallbackScale）
	TArray<FTransform> FinalTransforms;
	FinalTransforms.Reserve(Transforms.Num());
	for (const FTransform& T : Transforms)
	{
		FTransform FinalT = T;
		if (bUsingFallback)
		{
			FinalT.SetScale3D(FinalT.GetScale3D() * FallbackScale);
		}
		FinalTransforms.Add(FinalT);
	}

	// 添加实例
	HISMComponent->PreAllocateInstancesMemory(FinalTransforms.Num());
	HISMComponent->AddInstances(FinalTransforms, /*bShouldReturnIndices=*/false, /*bWorldSpace=*/false);

	const int32 TotalInstances = HISMComponent->GetInstanceCount();

	// 诊断日志
	const FTransform& FirstT = Transforms[0];
	const FTransform& LastT = Transforms.Last();
	FBox LocalBox(ForceInit);
	for (const FTransform& T : Transforms)
	{
		LocalBox += T.GetLocation();
	}
	const FVector LocalCenter = LocalBox.GetCenter();
	const FVector LocalSize = LocalBox.GetSize();
	const FVector ActorLoc = GetActorLocation();

	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 总实例数: %d  Mesh: %s"), TotalInstances, *MeshToUse->GetName());
	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 首点(本地): %s    末点(本地): %s"),
		*FirstT.GetLocation().ToString(), *LastT.GetLocation().ToString());
	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 点集本地中心: %s  尺寸: %s"),
		*LocalCenter.ToString(), *LocalSize.ToString());
	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] Actor 位置: %s  =>  预计世界中心约: %s"),
		*ActorLoc.ToString(), *(ActorLoc + LocalCenter).ToString());

	return TotalInstances;
}

int32 AATreeInstancer::LoadFromJson()
{
	// 清洗路径：去掉首尾引号（Windows "复制为路径"常带双引号）、空白，并统一斜杠
	FString CleanPath = JsonFilePath;
	CleanPath.TrimStartAndEndInline();
	CleanPath.TrimQuotesInline();
	CleanPath.TrimStartAndEndInline();
	CleanPath.ReplaceInline(TEXT("\\"), TEXT("/"));

	if (CleanPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] JsonFilePath 为空。"));
		return 0;
	}

	if (CleanPath != JsonFilePath)
	{
		UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 路径已自动规范化: %s -> %s"), *JsonFilePath, *CleanPath);
		JsonFilePath = CleanPath;
	}

	if (!FPaths::FileExists(CleanPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[TreeInstancer] 文件不存在: %s"), *CleanPath);
		return 0;
	}

	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *CleanPath))
	{
		UE_LOG(LogTemp, Error, TEXT("[TreeInstancer] 读取 JSON 失败: %s"), *CleanPath);
		return 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 已读取 JSON: %s (%d 字符)"), *CleanPath, JsonContent.Len());

	TArray<FTransform> Transforms;
	if (!ParseJson(JsonContent, Transforms))
	{
		UE_LOG(LogTemp, Error, TEXT("[TreeInstancer] 解析 JSON 失败: %s"), *CleanPath);
		return 0;
	}

	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 解析到 %d 个点"), Transforms.Num());
	return BuildFromTransforms(Transforms);
}

void AATreeInstancer::FocusOnInstancesCenter()
{
	if (!HISMComponent) return;

	FBox LocalBox(ForceInit);
	const int32 TotalNum = HISMComponent->GetInstanceCount();

	if (TotalNum == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] 无实例，无法聚焦。"));
		return;
	}

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

	SetActorLocation(NewActorLoc);
	UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 已聚焦到中心: %s (共 %d 实例)"), *NewActorLoc.ToString(), TotalNum);
}

void AATreeInstancer::DumpMeshStatus()
{
	UE_LOG(LogTemp, Log, TEXT("=========== [TreeInstancer] Mesh 状态诊断 ==========="));

	TArray<UStaticMesh*> Resolved = ResolveAllTreeMeshes();
	UE_LOG(LogTemp, Log, TEXT("  TreeMeshes 数组大小: %d"), TreeMeshes.Num());
	UE_LOG(LogTemp, Log, TEXT("  有效解析 Mesh 数量: %d"), Resolved.Num());

	for (int32 i = 0; i < Resolved.Num(); ++i)
	{
		UE_LOG(LogTemp, Log, TEXT("  [%d] %s  (路径: %s)"), i, *Resolved[i]->GetName(), *Resolved[i]->GetPathName());
	}

	if (FallbackMesh)
	{
		UE_LOG(LogTemp, Log, TEXT("  FallbackMesh  : %s  (路径: %s)"), *FallbackMesh->GetName(), *FallbackMesh->GetPathName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("  FallbackMesh  : <None>"));
	}

	if (HISMComponent)
	{
		int32 Count = HISMComponent->GetInstanceCount();
		UStaticMesh* CurMesh = HISMComponent->GetStaticMesh();
		UE_LOG(LogTemp, Log, TEXT("  HISM: Mesh=%s  实例数=%d"),
			CurMesh ? *CurMesh->GetName() : TEXT("<None>"), Count);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("  HISMComponent: <None>"));
	}

	UE_LOG(LogTemp, Log, TEXT("======================================================="));
}

UStaticMesh* AATreeInstancer::ResolveTreeMesh(int32 Index)
{
	// 优先使用 UPROPERTY 引用
	if (TreeMeshes.IsValidIndex(Index) && TreeMeshes[Index])
	{
		return TreeMeshes[Index];
	}

	// 引用为空，尝试用字符串路径加载
	if (TreeMeshPaths.IsValidIndex(Index) && !TreeMeshPaths[Index].IsEmpty())
	{
		UStaticMesh* Loaded = LoadObject<UStaticMesh>(nullptr, *TreeMeshPaths[Index]);
		if (Loaded)
		{
			// 回写到 TreeMeshes 引用
			if (TreeMeshes.IsValidIndex(Index))
			{
				TreeMeshes[Index] = Loaded;
			}
			UE_LOG(LogTemp, Log, TEXT("[TreeInstancer] 通过 TreeMeshPaths[%d] 加载成功: %s"), Index, *TreeMeshPaths[Index]);
			return Loaded;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[TreeInstancer] TreeMeshPaths[%d] 加载失败: %s"), Index, *TreeMeshPaths[Index]);
		}
	}

	return nullptr;
}

TArray<UStaticMesh*> AATreeInstancer::ResolveAllTreeMeshes()
{
	TArray<UStaticMesh*> Result;
	const int32 MaxCount = FMath::Max(TreeMeshes.Num(), TreeMeshPaths.Num());
	for (int32 i = 0; i < MaxCount; ++i)
	{
		UStaticMesh* Mesh = ResolveTreeMesh(i);
		if (Mesh)
		{
			Result.Add(Mesh);
		}
	}
	return Result;
}

bool AATreeInstancer::ParseJson(const FString& JsonContent, TArray<FTransform>& OutTransforms) const
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
		static const TArray<FString> Candidates = { TEXT("points"), TEXT("Points"), TEXT("trees"), TEXT("Trees"), TEXT("data") };
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
		// 尝试顶层数组格式 [ {...}, {...} ] 或 [[x,y,z], ...]
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
		float Scale = 1.f;
		bool bHasYaw = false;
		bool bHasScale = false;

		if (Val->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
			if (!Obj.IsValid()) continue;

			double X = 0, Y = 0, Z = 0;
			if (!Obj->TryGetNumberField(TEXT("x"), X)) Obj->TryGetNumberField(TEXT("X"), X);
			if (!Obj->TryGetNumberField(TEXT("y"), Y)) Obj->TryGetNumberField(TEXT("Y"), Y);
			if (!Obj->TryGetNumberField(TEXT("z"), Z)) Obj->TryGetNumberField(TEXT("Z"), Z);
			Pos = FVector(X, Y, Z);

			double DYaw = 0, DScale = 1;
			if (Obj->TryGetNumberField(TEXT("yaw"), DYaw) || Obj->TryGetNumberField(TEXT("Yaw"), DYaw))
			{
				Yaw = (float)DYaw;
				bHasYaw = true;
			}
			if (Obj->TryGetNumberField(TEXT("scale"), DScale) || Obj->TryGetNumberField(TEXT("Scale"), DScale))
			{
				Scale = (float)DScale;
				bHasScale = true;
			}
		}
		else if (Val->Type == EJson::Array)
		{
			// [x, y, z] 或 [x, y, z, yaw, scale]
			const TArray<TSharedPtr<FJsonValue>>& Arr = Val->AsArray();
			if (Arr.Num() >= 3)
			{
				Pos = FVector(Arr[0]->AsNumber(), Arr[1]->AsNumber(), Arr[2]->AsNumber());
			}
			if (Arr.Num() >= 4) { Yaw = (float)Arr[3]->AsNumber(); bHasYaw = true; }
			if (Arr.Num() >= 5) { Scale = (float)Arr[4]->AsNumber(); bHasScale = true; }
		}
		else
		{
			continue;
		}

		const FVector WorldPos = ApplyAxis(Pos);

		// 旋转：优先 JSON 指定的 yaw，否则根据设置随机
		FRotator Rot = FRotator::ZeroRotator;
		if (bHasYaw)
		{
			Rot.Yaw = Yaw;
		}
		else if (bFullRandomRotation)
		{
			// 完全自由旋转：Pitch、Yaw、Roll 全部随机
			Rot.Pitch = FMath::FRandRange(-15.f, 15.f);  // Pitch 轻微倾斜（树不会完全倒下）
			Rot.Yaw = FMath::FRandRange(0.f, 360.f);
			Rot.Roll = FMath::FRandRange(-15.f, 15.f);   // Roll 轻微倾斜
		}
		else if (bRandomYaw)
		{
			Rot.Yaw = FMath::FRandRange(0.f, 360.f);
		}

		// 缩放：优先 JSON 指定，否则随机范围
		FVector FinalScale = InstanceScale;
		if (bHasScale)
		{
			FinalScale *= Scale;
		}
		else if (RandomScaleRange.X != RandomScaleRange.Y)
		{
			FinalScale *= FMath::FRandRange(RandomScaleRange.X, RandomScaleRange.Y);
		}

		OutTransforms.Emplace(Rot, WorldPos, FinalScale);
	}

	return true;
}
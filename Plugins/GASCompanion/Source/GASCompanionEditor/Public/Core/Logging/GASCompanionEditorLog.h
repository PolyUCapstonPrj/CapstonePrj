// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

GASCOMPANIONEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogGASCompanionEditor, Display, All);

#define GSC_EDITOR_LOG(Verbosity, Format, ...) \
{ \
    UE_LOG(LogGASCompanionEditor, Verbosity, Format, ##__VA_ARGS__); \
}

#define GSC_EDITOR_PLOG(Verbosity, Format, ...) \
{ \
	UE_LOG(LogGASCompanionEditor, Verbosity, TEXT("%s - %s"), *FString(__FUNCTION__), *FString::Printf(Format, ##__VA_ARGS__)); \
}

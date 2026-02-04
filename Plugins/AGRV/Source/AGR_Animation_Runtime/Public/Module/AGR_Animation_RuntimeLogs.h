// Copyright 2024 3S Game Studio OU. All Rights Reserved.

#pragma once
#include "Logging/LogMacros.h"

AGR_ANIMATION_RUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogAGR_Animation_Runtime, Log, All);

#ifndef AGR_LOG
#define AGR_LOG(CategoryName, Verbosity, Format, ...) UE_LOG(CategoryName, Verbosity, TEXT(Format), ##__VA_ARGS__);
#define AGR_LOG_COND(Condition, CategoryName, Verbosity, Format, ...) if(Condition) { UE_LOG(CategoryName, Verbosity, TEXT(Format), ##__VA_ARGS__); }
#endif
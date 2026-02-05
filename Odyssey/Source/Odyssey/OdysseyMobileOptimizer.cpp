#include "OdysseyMobileOptimizer.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformApplicationMisc.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/ConfigCacheIni.h"

UOdysseyMobileOptimizer::UOdysseyMobileOptimizer()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Default optimization settings
	TargetFPS = 30.0f;
	FPSThresholdForDowngrade = 25.0f;
	FPSThresholdForUpgrade = 35.0f;
	PerformanceCheckInterval = 2.0f;
	bEnableAutomaticOptimization = true;
	bEnablePerformanceLogging = false;

	// Dynamic optimization settings
	bEnableDynamicLOD = true;
	LODDistanceScale = 1.0f;
	MaxRenderTargets = 2;

	// Initialize performance settings
	InitializePerformanceSettings();

	// Performance monitoring
	FPSSampleCount = 0;
	PerformanceCheckTimer = 0.0f;
	CurrentPerformanceTier = EPerformanceTier::Medium;
}

void UOdysseyMobileOptimizer::BeginPlay()
{
	Super::BeginPlay();

	// Detect and optimize for current device
	OptimizeForCurrentDevice();

	UE_LOG(LogTemp, Warning, TEXT("Mobile optimizer initialized: Device: %s, Tier: %d"),
		*GetDeviceModel(), (int32)CurrentPerformanceTier);
}

void UOdysseyMobileOptimizer::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Update performance metrics
	UpdatePerformanceMetrics(DeltaTime);

	// Check performance thresholds periodically
	PerformanceCheckTimer += DeltaTime;
	if (PerformanceCheckTimer >= PerformanceCheckInterval)
	{
		CheckPerformanceThresholds();
		PerformanceCheckTimer = 0.0f;
	}

	// Dynamic LOD updates
	if (bEnableDynamicLOD)
	{
		UpdateDynamicLOD();
	}
}

void UOdysseyMobileOptimizer::SetPerformanceTier(EPerformanceTier NewTier)
{
	if (NewTier == CurrentPerformanceTier)
		return;

	EPerformanceTier OldTier = CurrentPerformanceTier;
	CurrentPerformanceTier = NewTier;

	// Apply appropriate settings
	switch (NewTier)
	{
		case EPerformanceTier::High:
			ApplyPerformanceSettings(HighPerformanceSettings);
			break;
		case EPerformanceTier::Medium:
			ApplyPerformanceSettings(MediumPerformanceSettings);
			break;
		case EPerformanceTier::Low:
			ApplyPerformanceSettings(LowPerformanceSettings);
			break;
	}

	OnPerformanceTierChanged(OldTier, NewTier);

	UE_LOG(LogTemp, Warning, TEXT("Performance tier changed from %d to %d"), (int32)OldTier, (int32)NewTier);
}

void UOdysseyMobileOptimizer::OptimizeForCurrentDevice()
{
	if (IsMobileDevice())
	{
		if (IsLowEndDevice())
		{
			SetPerformanceTier(EPerformanceTier::Low);
		}
		else
		{
			// Default to medium for mobile devices
			SetPerformanceTier(EPerformanceTier::Medium);
		}
	}
	else
	{
		// Desktop/high-end device
		SetPerformanceTier(EPerformanceTier::High);
	}

	// Apply memory optimizations
	OptimizeMemoryUsage();
}

void UOdysseyMobileOptimizer::ApplyPerformanceSettings(const FMobilePerformanceSettings& Settings)
{
	if (!GEngine || !GEngine->GameViewport)
		return;

	// Apply console variable settings
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.ViewDistanceScale %f"), Settings.ViewDistanceScale));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.Shadow.MaxResolution %d"), Settings.ShadowQuality * 256));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.ParticleLODBias %d"), Settings.TextureQuality));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.SkeletalMeshLODBias %d"), Settings.TextureQuality));

	// Apply render scale
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.MobileContentScaleFactor %f"), Settings.RenderScale));

	// Apply post-process settings
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.DefaultFeature.Bloom %d"), Settings.bEnableBloom ? 1 : 0));
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.DefaultFeature.AntiAliasing %d"), Settings.bEnableAntiAliasing ? 1 : 0));

	UE_LOG(LogTemp, Verbose, TEXT("Applied performance settings: ViewDistance=%f, RenderScale=%f"),
		Settings.ViewDistanceScale, Settings.RenderScale);
}

float UOdysseyMobileOptimizer::GetAverageFPS() const
{
	if (FPSSamples.Num() == 0)
		return 0.0f;

	float Total = 0.0f;
	for (float Sample : FPSSamples)
	{
		Total += Sample;
	}
	return Total / FPSSamples.Num();
}

bool UOdysseyMobileOptimizer::IsPerformanceAcceptable() const
{
	return GetAverageFPS() >= FPSThresholdForDowngrade;
}

void UOdysseyMobileOptimizer::UpdateDynamicLOD()
{
	float CurrentFPS = PerformanceMetrics.CurrentFPS;

	if (CurrentFPS < TargetFPS * 0.8f) // 80% of target
	{
		// Increase LOD bias to reduce detail
		LODDistanceScale = FMath::Min(LODDistanceScale + 0.1f, 2.0f);
	}
	else if (CurrentFPS > TargetFPS * 1.2f) // 120% of target
	{
		// Decrease LOD bias to increase detail
		LODDistanceScale = FMath::Max(LODDistanceScale - 0.1f, 0.5f);
	}

	// Apply LOD scale
	GEngine->Exec(GetWorld(), *FString::Printf(TEXT("r.ViewDistanceScale %f"), LODDistanceScale));
}

void UOdysseyMobileOptimizer::OptimizeRenderingForLowPerformance()
{
	// Aggressive optimizations for very low performance
	GEngine->Exec(GetWorld(), TEXT("r.Shadow.MaxResolution 128"));
	GEngine->Exec(GetWorld(), TEXT("r.ParticleLODBias 2"));
	GEngine->Exec(GetWorld(), TEXT("r.SkeletalMeshLODBias 2"));
	GEngine->Exec(GetWorld(), TEXT("r.ViewDistanceScale 0.5"));
	GEngine->Exec(GetWorld(), TEXT("r.MobileContentScaleFactor 0.75"));
	GEngine->Exec(GetWorld(), TEXT("r.DefaultFeature.Bloom 0"));
	GEngine->Exec(GetWorld(), TEXT("r.DefaultFeature.AntiAliasing 0"));

	UE_LOG(LogTemp, Warning, TEXT("Applied aggressive performance optimizations"));
}

void UOdysseyMobileOptimizer::RestoreNormalRendering()
{
	// Restore normal settings based on current tier
	ApplyPerformanceSettings(GetCurrentPerformanceSettings());

	UE_LOG(LogTemp, Warning, TEXT("Restored normal rendering settings"));
}

bool UOdysseyMobileOptimizer::IsMobileDevice() const
{
	FString PlatformName = UGameplayStatics::GetPlatformName();
	return (PlatformName == TEXT("Android") || PlatformName == TEXT("IOS"));
}

bool UOdysseyMobileOptimizer::IsLowEndDevice() const
{
	if (!IsMobileDevice())
		return false;

	// Simple heuristic: check available memory and device characteristics
	// This is a simplified check - real implementation would be more sophisticated
	return FPlatformMisc::GetPhysicalGBRam() < 4;
}

FString UOdysseyMobileOptimizer::GetDeviceModel() const
{
	return FPlatformMisc::GetDeviceModel();
}

void UOdysseyMobileOptimizer::OptimizeMemoryUsage()
{
	// Force garbage collection
	GEngine->ForceGarbageCollection(true);

	// Clear unused texture streaming pool
	GEngine->Exec(GetWorld(), TEXT("r.Streaming.PoolSize 64"));

	// Limit texture quality for mobile
	if (IsMobileDevice())
	{
		GEngine->Exec(GetWorld(), TEXT("r.Streaming.LimitPoolSizeToVRAM 1"));
		GEngine->Exec(GetWorld(), TEXT("r.Streaming.UseFixedPoolSize 1"));
	}

	UE_LOG(LogTemp, Verbose, TEXT("Applied memory optimizations"));
}

void UOdysseyMobileOptimizer::ClearUnusedAssets()
{
	// Clear unused assets from memory
	if (GEngine)
	{
		GEngine->TrimMemory();
		GEngine->ForceGarbageCollection(true);
	}

	UE_LOG(LogTemp, Verbose, TEXT("Cleared unused assets"));
}

void UOdysseyMobileOptimizer::InitializePerformanceSettings()
{
	// High performance settings (Desktop/High-end mobile)
	HighPerformanceSettings.ViewDistanceScale = 1.0f;
	HighPerformanceSettings.ShadowQuality = 4;
	HighPerformanceSettings.EffectQuality = 1.0f;
	HighPerformanceSettings.TextureQuality = 0;
	HighPerformanceSettings.bEnableBloom = true;
	HighPerformanceSettings.bEnableAntiAliasing = true;
	HighPerformanceSettings.RenderScale = 1.0f;

	// Medium performance settings (Standard mobile)
	MediumPerformanceSettings.ViewDistanceScale = 0.8f;
	MediumPerformanceSettings.ShadowQuality = 2;
	MediumPerformanceSettings.EffectQuality = 0.8f;
	MediumPerformanceSettings.TextureQuality = 1;
	MediumPerformanceSettings.bEnableBloom = true;
	MediumPerformanceSettings.bEnableAntiAliasing = false;
	MediumPerformanceSettings.RenderScale = 0.9f;

	// Low performance settings (Low-end mobile)
	LowPerformanceSettings.ViewDistanceScale = 0.6f;
	LowPerformanceSettings.ShadowQuality = 1;
	LowPerformanceSettings.EffectQuality = 0.6f;
	LowPerformanceSettings.TextureQuality = 2;
	LowPerformanceSettings.bEnableBloom = false;
	LowPerformanceSettings.bEnableAntiAliasing = false;
	LowPerformanceSettings.RenderScale = 0.8f;
}

void UOdysseyMobileOptimizer::UpdatePerformanceMetrics(float DeltaTime)
{
	// Calculate current FPS
	PerformanceMetrics.CurrentFPS = (DeltaTime > 0.0f) ? (1.0f / DeltaTime) : 0.0f;
	PerformanceMetrics.FrameTime = DeltaTime * 1000.0f; // Convert to milliseconds

	// Add to FPS samples array
	FPSSamples.Add(PerformanceMetrics.CurrentFPS);
	if (FPSSamples.Num() > MaxFPSSamples)
	{
		FPSSamples.RemoveAt(0);
	}

	// Update min/max FPS
	if (FPSSamples.Num() == 1)
	{
		PerformanceMetrics.MinFPS = PerformanceMetrics.CurrentFPS;
		PerformanceMetrics.MaxFPS = PerformanceMetrics.CurrentFPS;
	}
	else
	{
		PerformanceMetrics.MinFPS = FMath::Min(PerformanceMetrics.MinFPS, PerformanceMetrics.CurrentFPS);
		PerformanceMetrics.MaxFPS = FMath::Max(PerformanceMetrics.MaxFPS, PerformanceMetrics.CurrentFPS);
	}

	// Calculate average FPS
	PerformanceMetrics.AverageFPS = GetAverageFPS();
}

void UOdysseyMobileOptimizer::CheckPerformanceThresholds()
{
	if (!bEnableAutomaticOptimization)
		return;

	float AvgFPS = GetAverageFPS();

	if (AvgFPS < FPSThresholdForDowngrade)
	{
		// Performance is too low, downgrade if possible
		if (CurrentPerformanceTier == EPerformanceTier::High)
		{
			SetPerformanceTier(EPerformanceTier::Medium);
		}
		else if (CurrentPerformanceTier == EPerformanceTier::Medium)
		{
			SetPerformanceTier(EPerformanceTier::Low);
		}
		else
		{
			// Already at lowest tier, apply aggressive optimizations
			OptimizeRenderingForLowPerformance();
		}

		OnPerformanceThresholdReached(true);
	}
	else if (AvgFPS > FPSThresholdForUpgrade)
	{
		// Performance is good, upgrade if possible
		if (CurrentPerformanceTier == EPerformanceTier::Low)
		{
			SetPerformanceTier(EPerformanceTier::Medium);
		}
		else if (CurrentPerformanceTier == EPerformanceTier::Medium && !IsMobileDevice())
		{
			// Only upgrade to high on non-mobile devices
			SetPerformanceTier(EPerformanceTier::High);
		}

		OnPerformanceThresholdReached(false);
	}

	if (bEnablePerformanceLogging)
	{
		LogPerformanceData();
	}
}

void UOdysseyMobileOptimizer::AutoOptimizePerformance()
{
	EPerformanceTier OptimalTier = DetermineOptimalPerformanceTier();
	if (OptimalTier != CurrentPerformanceTier)
	{
		SetPerformanceTier(OptimalTier);
	}
}

void UOdysseyMobileOptimizer::LogPerformanceData()
{
	UE_LOG(LogTemp, Log, TEXT("Performance Metrics: FPS=%.1f (Avg=%.1f, Min=%.1f, Max=%.1f), FrameTime=%.2fms, Tier=%d"),
		PerformanceMetrics.CurrentFPS,
		PerformanceMetrics.AverageFPS,
		PerformanceMetrics.MinFPS,
		PerformanceMetrics.MaxFPS,
		PerformanceMetrics.FrameTime,
		(int32)CurrentPerformanceTier);
}

EPerformanceTier UOdysseyMobileOptimizer::DetermineOptimalPerformanceTier() const
{
	float AvgFPS = GetAverageFPS();

	if (AvgFPS >= FPSThresholdForUpgrade)
	{
		if (IsMobileDevice())
		{
			return EPerformanceTier::Medium; // Cap mobile devices at medium
		}
		else
		{
			return EPerformanceTier::High;
		}
	}
	else if (AvgFPS >= FPSThresholdForDowngrade)
	{
		return EPerformanceTier::Medium;
	}
	else
	{
		return EPerformanceTier::Low;
	}
}


const FMobilePerformanceSettings& UOdysseyMobileOptimizer::GetCurrentPerformanceSettings() const
{
	switch (CurrentPerformanceTier)
	{
		case EPerformanceTier::High:
			return HighPerformanceSettings;
		case EPerformanceTier::Medium:
			return MediumPerformanceSettings;
		case EPerformanceTier::Low:
		default:
			return LowPerformanceSettings;
	}
}

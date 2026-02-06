// ExplorationRewardSystem.h
// Discovery mechanics, scanning, exploration progress, and rare findings
// Part of the Odyssey Procedural Planet & Resource Generation System

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ProceduralTypes.h"
#include "OdysseyBiomeDefinitionSystem.h"
#include "OdysseyPlanetGenerator.h"
#include "ExplorationRewardSystem.generated.h"

// Delegates for exploration events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDiscoveryMade, const FDiscoveryData&, Discovery);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDiscoveryClaimed, const FDiscoveryData&, Discovery);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExplorationProgress, int32, PlanetID, float, NewProgress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMilestoneReached, const FExplorationMilestone&, Milestone);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnScanComplete, const FScanResult&, Result);

/**
 * UExplorationRewardSystem
 *
 * Manages exploration mechanics including discovery generation, scanning,
 * fog-of-war tracking, milestone progression, and reward distribution.
 * Drives the exploration loop by creating meaningful rewards for venturing
 * into uncharted territory.
 */
UCLASS(BlueprintType, Blueprintable)
class ODYSSEY_API UExplorationRewardSystem : public UObject
{
	GENERATED_BODY()

public:
	UExplorationRewardSystem();

	// ========================================================================
	// INITIALIZATION
	// ========================================================================

	UFUNCTION(BlueprintCallable, Category = "Exploration|Setup")
	void Initialize(UOdysseyBiomeDefinitionSystem* BiomeSystem);

	// ========================================================================
	// DISCOVERY GENERATION
	// ========================================================================

	/** Generate all discoveries for a planet based on its data */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Generation")
	TArray<FDiscoveryData> GenerateDiscoveriesForPlanet(
		const FGeneratedPlanetData& PlanetData,
		int32 DiscoveryCount);

	/** Generate a single discovery at a specific location */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Generation")
	FDiscoveryData GenerateDiscovery(
		int32 Seed,
		int32 PlanetID,
		const FVector& Location,
		EBiomeType Biome);

	/** Generate discoveries specifically for a biome region */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Generation")
	TArray<FDiscoveryData> GenerateDiscoveriesForRegion(
		int32 Seed,
		int32 PlanetID,
		const FPlanetBiomeRegion& Region,
		int32 Count);

	// ========================================================================
	// SCANNING
	// ========================================================================

	/** Perform a scan from a location and return results */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Scanning")
	TArray<FScanResult> PerformScan(
		int32 PlanetID,
		const FVector& ScanOrigin,
		EScanMode ScanMode,
		float ScannerPower = 1.0f);

	/** Get the effective scan radius for a given mode and power */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Scanning")
	float GetEffectiveScanRadius(EScanMode Mode, float ScannerPower) const;

	/** Check if a specific discovery can be detected by the given scan */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Scanning")
	bool CanDetectDiscovery(
		const FDiscoveryData& Discovery,
		EScanMode Mode,
		float Distance,
		float ScannerPower) const;

	// ========================================================================
	// DISCOVERY CLAIMING
	// ========================================================================

	/** Attempt to discover (reveal) a discovery at a location */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Discovery")
	bool TryDiscoverAtLocation(
		int32 PlanetID,
		const FVector& PlayerLocation,
		float InteractionRadius,
		FDiscoveryData& OutDiscovery);

	/** Claim rewards from a discovered item */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Discovery")
	bool ClaimDiscoveryRewards(
		int32 DiscoveryID,
		const FString& PlayerID,
		int32& OutOMEN,
		int32& OutExperience,
		TArray<FResourceStack>& OutResources);

	/** Check if a discovery has already been claimed */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Discovery")
	bool IsDiscoveryClaimed(int32 DiscoveryID) const;

	/** Check if a discovery has been revealed */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Discovery")
	bool IsDiscoveryRevealed(int32 DiscoveryID) const;

	// ========================================================================
	// EXPLORATION PROGRESS
	// ========================================================================

	/** Register a planet for exploration tracking */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Progress")
	void RegisterPlanet(int32 PlanetID, int32 TotalRegions, int32 GridResolution = 32);

	/** Update exploration based on player position (call periodically) */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Progress")
	void UpdateExploration(
		int32 PlanetID,
		const FVector& PlayerLocation,
		float RevealRadius,
		const FVector2D& WorldSize);

	/** Get exploration data for a planet */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Progress")
	FPlanetExplorationData GetExplorationData(int32 PlanetID) const;

	/** Get exploration status enum for a planet */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Progress")
	EExplorationStatus GetExplorationStatus(int32 PlanetID) const;

	/** Get exploration percentage for a planet */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Progress")
	float GetExplorationPercent(int32 PlanetID) const;

	/** Mark a biome region as explored */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Progress")
	void MarkRegionExplored(int32 PlanetID, int32 RegionIndex, EBiomeType BiomeType);

	// ========================================================================
	// MILESTONES
	// ========================================================================

	/** Get all milestones for a planet */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Milestones")
	TArray<FExplorationMilestone> GetMilestones(int32 PlanetID) const;

	/** Get uncompleted milestones */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Milestones")
	TArray<FExplorationMilestone> GetPendingMilestones(int32 PlanetID) const;

	/** Check and award milestones based on current progress */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Milestones")
	TArray<FExplorationMilestone> CheckMilestones(int32 PlanetID);

	// ========================================================================
	// QUERIES
	// ========================================================================

	/** Get all discoveries for a planet */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Query")
	TArray<FDiscoveryData> GetPlanetDiscoveries(int32 PlanetID) const;

	/** Get undiscovered discoveries (for debug/admin) */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Query")
	TArray<FDiscoveryData> GetUndiscoveredItems(int32 PlanetID) const;

	/** Get discoveries by type */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Query")
	TArray<FDiscoveryData> GetDiscoveriesByType(int32 PlanetID, EDiscoveryType Type) const;

	/** Get discoveries by rarity */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Query")
	TArray<FDiscoveryData> GetDiscoveriesByRarity(int32 PlanetID, EDiscoveryRarity MinRarity) const;

	/** Find nearest undiscovered item */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Query")
	FDiscoveryData FindNearestUndiscovered(
		int32 PlanetID,
		const FVector& FromLocation,
		float MaxDistance = -1.0f) const;

	/** Get total OMEN earned from exploration on a planet */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Query")
	int32 GetTotalExplorationRewards(int32 PlanetID) const;

	// ========================================================================
	// SERIALIZATION
	// ========================================================================

	/** Export save data for a planet (minimal footprint) */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Save")
	void ExportPlanetSaveData(int32 PlanetID, TArray<int32>& OutDiscoveredIDs, TArray<int32>& OutClaimedIDs) const;

	/** Import save data and restore state */
	UFUNCTION(BlueprintCallable, Category = "Exploration|Save")
	void ImportPlanetSaveData(int32 PlanetID, const TArray<int32>& DiscoveredIDs, const TArray<int32>& ClaimedIDs);

	// ========================================================================
	// UTILITY
	// ========================================================================

	UFUNCTION(BlueprintCallable, Category = "Exploration|Utility")
	static FText GetDiscoveryTypeDisplayName(EDiscoveryType Type);

	UFUNCTION(BlueprintCallable, Category = "Exploration|Utility")
	static FLinearColor GetDiscoveryRarityColor(EDiscoveryRarity Rarity);

	UFUNCTION(BlueprintCallable, Category = "Exploration|Utility")
	static FText GetExplorationStatusDisplayName(EExplorationStatus Status);

	UFUNCTION(BlueprintCallable, Category = "Exploration|Utility")
	static int32 CalculateDiscoveryOMENValue(EDiscoveryType Type, EDiscoveryRarity Rarity);

	// ========================================================================
	// DELEGATES
	// ========================================================================

	UPROPERTY(BlueprintAssignable, Category = "Exploration|Events")
	FOnDiscoveryMade OnDiscoveryMade;

	UPROPERTY(BlueprintAssignable, Category = "Exploration|Events")
	FOnDiscoveryClaimed OnDiscoveryClaimed;

	UPROPERTY(BlueprintAssignable, Category = "Exploration|Events")
	FOnExplorationProgress OnExplorationProgress;

	UPROPERTY(BlueprintAssignable, Category = "Exploration|Events")
	FOnMilestoneReached OnMilestoneReached;

	UPROPERTY(BlueprintAssignable, Category = "Exploration|Events")
	FOnScanComplete OnScanComplete;

protected:
	UPROPERTY()
	UOdysseyBiomeDefinitionSystem* BiomeDefinitionSystem;

	// All discoveries indexed by planet ID
	TMap<int32, TArray<FDiscoveryData>> PlanetDiscoveries;

	// Exploration progress per planet
	TMap<int32, FPlanetExplorationData> ExplorationProgress;

	// Milestones per planet
	TMap<int32, TArray<FExplorationMilestone>> PlanetMilestones;

	// Global discovery ID counter
	UPROPERTY()
	int32 NextDiscoveryID;

private:
	// Discovery generation helpers
	EDiscoveryType SelectDiscoveryType(int32 Seed, EBiomeType Biome) const;
	EDiscoveryRarity DetermineDiscoveryRarity(int32 Seed, EBiomeType Biome) const;
	FText GenerateDiscoveryName(int32 Seed, EDiscoveryType Type, EBiomeType Biome) const;
	FText GenerateDiscoveryDescription(EDiscoveryType Type, EDiscoveryRarity Rarity, EBiomeType Biome) const;
	FText GenerateLoreText(int32 Seed, EDiscoveryType Type) const;
	TArray<FResourceStack> GenerateResourceRewards(int32 Seed, EDiscoveryType Type, EDiscoveryRarity Rarity, EBiomeType Biome) const;
	EScanMode DetermineScanRequirement(EDiscoveryRarity Rarity) const;

	// Biome-to-discovery affinity tables
	TMap<EBiomeType, TArray<EDiscoveryType>> BiomeDiscoveryAffinity;
	void InitializeBiomeDiscoveryAffinity();

	// Default milestone definitions
	void InitializeDefaultMilestones(int32 PlanetID);

	// Name generation pools
	TArray<FString> RuinsPrefixes;
	TArray<FString> ArtifactNames;
	TArray<FString> WonderAdjectives;
	TArray<FString> OutpostDesignations;
	void InitializeNamePools();

	// Seeded random
	static uint32 HashSeed(int32 Seed);
	static float SeededRandom(int32 Seed);
	static int32 SeededRandomRange(int32 Seed, int32 Min, int32 Max);
	static float SeededRandomFloat(int32 Seed, float Min, float Max);
};


// ReputationSystem.cpp
// Implementation of faction and player reputation tracking

#include "ReputationSystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

UReputationSystem::UReputationSystem()
	: MaxHistoryPerPlayer(200)
	, MaxFeedbackPerPlayer(100)
	, GlobalDecayMultiplier(1.0f)
	, MinReputationValue(-1000.0f)
	, MaxReputationValue(1000.0f)
{
}

void UReputationSystem::Initialize()
{
	InitializeFactionDefinitions();
	InitializeFactionRelationships();
	InitializeTitleDefinitions();

	// Set up periodic decay processing (every 60 seconds of game time)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DecayTimerHandle,
			FTimerDelegate::CreateLambda([this]()
			{
				ProcessReputationDecay(60.0f);
			}),
			60.0f,
			true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("ReputationSystem initialized with %d factions, %d relationships, %d titles"),
		FactionDefinitions.Num(), FactionRelationships.Num(), TitleRequirements.Num());
}

// ==================== Faction Definitions ====================

void UReputationSystem::InitializeFactionDefinitions()
{
	auto AddFaction = [this](EFaction ID, const FString& Name, const FString& Desc,
		FLinearColor Color, float DefaultRep, float DecayRate)
	{
		FFactionDefinition Def;
		Def.FactionID = ID;
		Def.FactionName = Name;
		Def.Description = Desc;
		Def.FactionColor = Color;
		Def.DefaultReputation = DefaultRep;
		Def.DecayRatePerDay = DecayRate;
		Def.bDecayEnabled = true;
		Def.MinTierForMissions = EReputationTier::Amiable;
		Def.MinTierForVendor = EReputationTier::Friendly;
		Def.MinTierForHangar = EReputationTier::Honored;

		// Trade discounts per tier
		Def.TierTradeDiscounts.Add(EReputationTier::Reviled, 0.50f);      // 50% markup
		Def.TierTradeDiscounts.Add(EReputationTier::Hostile, 0.30f);      // 30% markup
		Def.TierTradeDiscounts.Add(EReputationTier::Unfriendly, 0.15f);   // 15% markup
		Def.TierTradeDiscounts.Add(EReputationTier::Wary, 0.05f);         // 5% markup
		Def.TierTradeDiscounts.Add(EReputationTier::Neutral, 0.0f);       // No modifier
		Def.TierTradeDiscounts.Add(EReputationTier::Amiable, -0.05f);     // 5% discount
		Def.TierTradeDiscounts.Add(EReputationTier::Friendly, -0.10f);    // 10% discount
		Def.TierTradeDiscounts.Add(EReputationTier::Honored, -0.15f);     // 15% discount
		Def.TierTradeDiscounts.Add(EReputationTier::Exalted, -0.20f);     // 20% discount

		FactionDefinitions.Add(ID, Def);
	};

	AddFaction(EFaction::Concordat, TEXT("The Concordat"),
		TEXT("Central governing body maintaining order across settled systems."),
		FLinearColor(0.2f, 0.4f, 0.8f), 0.0f, 2.0f);

	AddFaction(EFaction::VoidTraders, TEXT("Void Traders' Coalition"),
		TEXT("Merchant guild controlling major trade routes and commerce hubs."),
		FLinearColor(0.9f, 0.75f, 0.1f), 0.0f, 1.5f);

	AddFaction(EFaction::IronVanguard, TEXT("Iron Vanguard"),
		TEXT("Militaristic faction of mercenaries and defense contractors."),
		FLinearColor(0.7f, 0.1f, 0.1f), 0.0f, 2.5f);

	AddFaction(EFaction::StellarAcademy, TEXT("Stellar Academy"),
		TEXT("Scientific consortium dedicated to research and exploration."),
		FLinearColor(0.3f, 0.8f, 0.9f), 0.0f, 1.0f);

	AddFaction(EFaction::FreeHaven, TEXT("Free Haven Collective"),
		TEXT("Independent frontier settlers valuing freedom and self-reliance."),
		FLinearColor(0.4f, 0.7f, 0.3f), 10.0f, 0.5f);

	AddFaction(EFaction::ShadowSyndicate, TEXT("Shadow Syndicate"),
		TEXT("Underground network of smugglers and information brokers."),
		FLinearColor(0.3f, 0.1f, 0.4f), -50.0f, 3.0f);

	AddFaction(EFaction::AncientOrder, TEXT("Order of the Architects"),
		TEXT("Enigmatic cult studying precursor artifacts and technology."),
		FLinearColor(0.8f, 0.6f, 0.0f), -25.0f, 1.0f);

	AddFaction(EFaction::MinerGuild, TEXT("Deep Core Miner's Guild"),
		TEXT("Industrial cooperative controlling major mining operations."),
		FLinearColor(0.6f, 0.4f, 0.2f), 0.0f, 1.5f);

	AddFaction(EFaction::NomadFleet, TEXT("Nomad Fleet"),
		TEXT("Spacefaring nomads trading stories and salvage across the void."),
		FLinearColor(0.5f, 0.5f, 0.7f), 0.0f, 0.5f);
}

void UReputationSystem::InitializeFactionRelationships()
{
	// Allies (positive ripple: helping one helps the other)
	FactionRelationships.Add(FFactionRelationship(EFaction::Concordat, EFaction::VoidTraders, 50.0f, 0.15f));
	FactionRelationships.Add(FFactionRelationship(EFaction::Concordat, EFaction::StellarAcademy, 30.0f, 0.10f));
	FactionRelationships.Add(FFactionRelationship(EFaction::VoidTraders, EFaction::MinerGuild, 40.0f, 0.20f));
	FactionRelationships.Add(FFactionRelationship(EFaction::StellarAcademy, EFaction::AncientOrder, 20.0f, 0.10f));
	FactionRelationships.Add(FFactionRelationship(EFaction::FreeHaven, EFaction::NomadFleet, 60.0f, 0.25f));

	// Enemies (negative ripple: helping one hurts the other)
	FactionRelationships.Add(FFactionRelationship(EFaction::Concordat, EFaction::ShadowSyndicate, -70.0f, -0.30f));
	FactionRelationships.Add(FFactionRelationship(EFaction::Concordat, EFaction::FreeHaven, -20.0f, -0.05f));
	FactionRelationships.Add(FFactionRelationship(EFaction::IronVanguard, EFaction::NomadFleet, -30.0f, -0.10f));
	FactionRelationships.Add(FFactionRelationship(EFaction::VoidTraders, EFaction::ShadowSyndicate, -50.0f, -0.20f));
	FactionRelationships.Add(FFactionRelationship(EFaction::StellarAcademy, EFaction::IronVanguard, -15.0f, -0.05f));
	FactionRelationships.Add(FFactionRelationship(EFaction::MinerGuild, EFaction::StellarAcademy, -10.0f, -0.03f));

	// Neutral-ish
	FactionRelationships.Add(FFactionRelationship(EFaction::IronVanguard, EFaction::ShadowSyndicate, 10.0f, 0.05f));
	FactionRelationships.Add(FFactionRelationship(EFaction::ShadowSyndicate, EFaction::NomadFleet, 25.0f, 0.10f));
}

void UReputationSystem::InitializeTitleDefinitions()
{
	// Concordat titles
	TitleRequirements.Add(FName(TEXT("Citizen")), TPair<EFaction, EReputationTier>(EFaction::Concordat, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Consul")), TPair<EFaction, EReputationTier>(EFaction::Concordat, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Senator")), TPair<EFaction, EReputationTier>(EFaction::Concordat, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Archon")), TPair<EFaction, EReputationTier>(EFaction::Concordat, EReputationTier::Exalted));

	// Void Traders titles
	TitleRequirements.Add(FName(TEXT("Peddler")), TPair<EFaction, EReputationTier>(EFaction::VoidTraders, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Merchant")), TPair<EFaction, EReputationTier>(EFaction::VoidTraders, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Magnate")), TPair<EFaction, EReputationTier>(EFaction::VoidTraders, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Tycoon")), TPair<EFaction, EReputationTier>(EFaction::VoidTraders, EReputationTier::Exalted));

	// Iron Vanguard titles
	TitleRequirements.Add(FName(TEXT("Recruit")), TPair<EFaction, EReputationTier>(EFaction::IronVanguard, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Sergeant")), TPair<EFaction, EReputationTier>(EFaction::IronVanguard, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Commander")), TPair<EFaction, EReputationTier>(EFaction::IronVanguard, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Warlord")), TPair<EFaction, EReputationTier>(EFaction::IronVanguard, EReputationTier::Exalted));

	// Stellar Academy titles
	TitleRequirements.Add(FName(TEXT("Initiate")), TPair<EFaction, EReputationTier>(EFaction::StellarAcademy, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Scholar")), TPair<EFaction, EReputationTier>(EFaction::StellarAcademy, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Professor")), TPair<EFaction, EReputationTier>(EFaction::StellarAcademy, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Luminary")), TPair<EFaction, EReputationTier>(EFaction::StellarAcademy, EReputationTier::Exalted));

	// Free Haven titles
	TitleRequirements.Add(FName(TEXT("Settler")), TPair<EFaction, EReputationTier>(EFaction::FreeHaven, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Pioneer")), TPair<EFaction, EReputationTier>(EFaction::FreeHaven, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Pathfinder")), TPair<EFaction, EReputationTier>(EFaction::FreeHaven, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Trailblazer")), TPair<EFaction, EReputationTier>(EFaction::FreeHaven, EReputationTier::Exalted));

	// Shadow Syndicate titles
	TitleRequirements.Add(FName(TEXT("Runner")), TPair<EFaction, EReputationTier>(EFaction::ShadowSyndicate, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Operative")), TPair<EFaction, EReputationTier>(EFaction::ShadowSyndicate, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Broker")), TPair<EFaction, EReputationTier>(EFaction::ShadowSyndicate, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Phantom")), TPair<EFaction, EReputationTier>(EFaction::ShadowSyndicate, EReputationTier::Exalted));

	// Miner's Guild titles
	TitleRequirements.Add(FName(TEXT("Prospector")), TPair<EFaction, EReputationTier>(EFaction::MinerGuild, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Foreman")), TPair<EFaction, EReputationTier>(EFaction::MinerGuild, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Deep Driller")), TPair<EFaction, EReputationTier>(EFaction::MinerGuild, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Core Breaker")), TPair<EFaction, EReputationTier>(EFaction::MinerGuild, EReputationTier::Exalted));

	// Nomad Fleet titles
	TitleRequirements.Add(FName(TEXT("Drifter")), TPair<EFaction, EReputationTier>(EFaction::NomadFleet, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Wayfarer")), TPair<EFaction, EReputationTier>(EFaction::NomadFleet, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Star Walker")), TPair<EFaction, EReputationTier>(EFaction::NomadFleet, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Void Sage")), TPair<EFaction, EReputationTier>(EFaction::NomadFleet, EReputationTier::Exalted));

	// Ancient Order titles
	TitleRequirements.Add(FName(TEXT("Seeker")), TPair<EFaction, EReputationTier>(EFaction::AncientOrder, EReputationTier::Amiable));
	TitleRequirements.Add(FName(TEXT("Acolyte")), TPair<EFaction, EReputationTier>(EFaction::AncientOrder, EReputationTier::Friendly));
	TitleRequirements.Add(FName(TEXT("Keeper")), TPair<EFaction, EReputationTier>(EFaction::AncientOrder, EReputationTier::Honored));
	TitleRequirements.Add(FName(TEXT("Architect")), TPair<EFaction, EReputationTier>(EFaction::AncientOrder, EReputationTier::Exalted));
}

// ==================== Faction Reputation ====================

float UReputationSystem::ModifyReputation(const FString& PlayerID, EFaction Faction, float Amount,
	EReputationChangeSource Source, const FString& Description)
{
	FScopeLock Lock(&ReputationLock);

	FPlayerReputationProfile& Profile = GetOrCreateProfile(PlayerID);

	FFactionStanding* Standing = Profile.FactionStandings.Find(Faction);
	if (!Standing)
	{
		// Initialize standing with default reputation
		const FFactionDefinition* FactionDef = FactionDefinitions.Find(Faction);
		float DefaultRep = FactionDef ? FactionDef->DefaultReputation : 0.0f;
		Profile.FactionStandings.Add(Faction, FFactionStanding(Faction, DefaultRep));
		Standing = Profile.FactionStandings.Find(Faction);
	}

	if (!Standing || Standing->bIsLocked)
	{
		return Standing ? Standing->CurrentReputation : 0.0f;
	}

	float OldReputation = Standing->CurrentReputation;
	EReputationTier OldTier = Standing->CurrentTier;

	// Apply the change
	Standing->CurrentReputation = FMath::Clamp(
		Standing->CurrentReputation + Amount, MinReputationValue, MaxReputationValue);

	// Track lifetime totals
	if (Amount > 0.0f) Standing->LifetimePositive += Amount;
	else Standing->LifetimeNegative += FMath::Abs(Amount);

	Standing->LastChangeTime = FDateTime::Now();
	Standing->RecalculateTier();

	// Record the change
	FReputationChangeRecord Record;
	Record.PlayerID = PlayerID;
	Record.Faction = Faction;
	Record.Amount = Amount;
	Record.NewTotal = Standing->CurrentReputation;
	Record.OldTier = OldTier;
	Record.NewTier = Standing->CurrentTier;
	Record.Source = Source;
	Record.Description = Description;
	Profile.RecentChanges.Insert(Record, 0);

	TrimHistory(Profile);

	// Fire events
	OnReputationChanged.Broadcast(PlayerID, Faction, OldReputation, Standing->CurrentReputation, Source);

	if (OldTier != Standing->CurrentTier)
	{
		OnReputationTierChanged.Broadcast(PlayerID, Faction, OldTier, Standing->CurrentTier);
		CheckTitleUnlocks(PlayerID, Profile);

		UE_LOG(LogTemp, Log, TEXT("Player %s tier changed with %s: %d -> %d (rep: %.1f)"),
			*PlayerID, *GetFactionName(Faction),
			static_cast<int32>(OldTier), static_cast<int32>(Standing->CurrentTier),
			Standing->CurrentReputation);
	}

	// Apply ripple effects to related factions
	ApplyRippleEffects(PlayerID, Faction, Amount, Source);

	return Standing->CurrentReputation;
}

void UReputationSystem::SetReputation(const FString& PlayerID, EFaction Faction, float NewValue)
{
	FScopeLock Lock(&ReputationLock);

	FPlayerReputationProfile& Profile = GetOrCreateProfile(PlayerID);

	FFactionStanding* Standing = Profile.FactionStandings.Find(Faction);
	if (!Standing)
	{
		Profile.FactionStandings.Add(Faction, FFactionStanding(Faction, NewValue));
		Standing = Profile.FactionStandings.Find(Faction);
	}

	if (Standing)
	{
		EReputationTier OldTier = Standing->CurrentTier;
		float OldRep = Standing->CurrentReputation;
		Standing->CurrentReputation = FMath::Clamp(NewValue, MinReputationValue, MaxReputationValue);
		Standing->LastChangeTime = FDateTime::Now();
		Standing->RecalculateTier();

		OnReputationChanged.Broadcast(PlayerID, Faction, OldRep, Standing->CurrentReputation, EReputationChangeSource::AdminAction);

		if (OldTier != Standing->CurrentTier)
		{
			OnReputationTierChanged.Broadcast(PlayerID, Faction, OldTier, Standing->CurrentTier);
			CheckTitleUnlocks(PlayerID, Profile);
		}
	}
}

float UReputationSystem::GetReputation(const FString& PlayerID, EFaction Faction) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (!Profile) return 0.0f;

	const FFactionStanding* Standing = Profile->FactionStandings.Find(Faction);
	if (!Standing)
	{
		const FFactionDefinition* FactionDef = FactionDefinitions.Find(Faction);
		return FactionDef ? FactionDef->DefaultReputation : 0.0f;
	}

	return Standing->CurrentReputation;
}

EReputationTier UReputationSystem::GetReputationTier(const FString& PlayerID, EFaction Faction) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (!Profile) return EReputationTier::Neutral;

	return Profile->GetTier(Faction);
}

TMap<EFaction, FFactionStanding> UReputationSystem::GetAllStandings(const FString& PlayerID) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (Profile)
	{
		return Profile->FactionStandings;
	}
	return TMap<EFaction, FFactionStanding>();
}

bool UReputationSystem::GetReputationProfile(const FString& PlayerID, FPlayerReputationProfile& OutProfile) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (Profile)
	{
		OutProfile = *Profile;
		return true;
	}
	return false;
}

float UReputationSystem::GetTierProgress(const FString& PlayerID, EFaction Faction) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (!Profile) return 0.5f;

	const FFactionStanding* Standing = Profile->FactionStandings.Find(Faction);
	if (!Standing) return 0.5f;

	return Standing->GetProgressToNextTier();
}

void UReputationSystem::SetReputationLocked(const FString& PlayerID, EFaction Faction, bool bLocked)
{
	FScopeLock Lock(&ReputationLock);

	FPlayerReputationProfile& Profile = GetOrCreateProfile(PlayerID);
	FFactionStanding* Standing = Profile.FactionStandings.Find(Faction);
	if (Standing)
	{
		Standing->bIsLocked = bLocked;
	}
}

bool UReputationSystem::MeetsTierRequirement(const FString& PlayerID, EFaction Faction, EReputationTier RequiredTier) const
{
	EReputationTier PlayerTier = GetReputationTier(PlayerID, Faction);
	return static_cast<uint8>(PlayerTier) >= static_cast<uint8>(RequiredTier);
}

// ==================== Trade Price Modifiers ====================

float UReputationSystem::GetTradeModifier(const FString& PlayerID, EFaction Faction) const
{
	FScopeLock Lock(&ReputationLock);

	EReputationTier Tier = GetReputationTier(PlayerID, Faction);

	const FFactionDefinition* FactionDef = FactionDefinitions.Find(Faction);
	if (!FactionDef) return 0.0f;

	const float* Modifier = FactionDef->TierTradeDiscounts.Find(Tier);
	return Modifier ? *Modifier : 0.0f;
}

float UReputationSystem::GetCombinedTradeModifier(const FString& PlayerID, EFaction Faction) const
{
	float PersonalModifier = GetTradeModifier(PlayerID, Faction);

	// Could be extended to include guild reputation bonuses here
	// For now return just personal modifier
	return PersonalModifier;
}

// ==================== Player Social Reputation ====================

bool UReputationSystem::SubmitPlayerFeedback(const FString& FromPlayerID, const FString& ToPlayerID,
	EPlayerFeedbackType FeedbackType, const FString& Context)
{
	FScopeLock Lock(&ReputationLock);

	if (FromPlayerID == ToPlayerID)
	{
		return false; // Cannot rate yourself
	}

	FPlayerReputationProfile& Profile = GetOrCreateProfile(ToPlayerID);

	// Check for duplicate feedback from same player in recent window (24h)
	FDateTime CutoffTime = FDateTime::Now() - FTimespan::FromDays(1);
	for (const FPlayerFeedback& Existing : Profile.ReceivedFeedback)
	{
		if (Existing.FromPlayerID == FromPlayerID && Existing.Timestamp > CutoffTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("Duplicate feedback from %s to %s within 24h"), *FromPlayerID, *ToPlayerID);
			return false;
		}
	}

	FPlayerFeedback Feedback;
	Feedback.FromPlayerID = FromPlayerID;
	Feedback.ToPlayerID = ToPlayerID;
	Feedback.FeedbackType = FeedbackType;
	Feedback.Context = Context;

	// Weight feedback by reporter's own trust score
	float ReporterTrust = GetPlayerTrustScore(FromPlayerID);
	Feedback.Weight = FMath::Clamp(ReporterTrust / 50.0f, 0.5f, 2.0f);

	Profile.ReceivedFeedback.Insert(Feedback, 0);

	// Trim old feedback
	while (Profile.ReceivedFeedback.Num() > MaxFeedbackPerPlayer)
	{
		Profile.ReceivedFeedback.RemoveAt(Profile.ReceivedFeedback.Num() - 1);
	}

	// Update counts
	switch (FeedbackType)
	{
	case EPlayerFeedbackType::Positive: Profile.SocialReputation.PositiveCount++; break;
	case EPlayerFeedbackType::Neutral:  Profile.SocialReputation.NeutralCount++; break;
	case EPlayerFeedbackType::Negative: Profile.SocialReputation.NegativeCount++; break;
	}

	float OldScore = Profile.SocialReputation.TrustScore;
	RecalculateTrustScore(Profile.SocialReputation);
	float NewScore = Profile.SocialReputation.TrustScore;

	if (FMath::Abs(OldScore - NewScore) > 0.5f)
	{
		OnPlayerTrustScoreChanged.Broadcast(ToPlayerID, OldScore, NewScore);
	}

	return true;
}

void UReputationSystem::RecordContractOutcome(const FString& PlayerID, bool bCompleted, float Rating)
{
	FScopeLock Lock(&ReputationLock);

	FPlayerReputationProfile& Profile = GetOrCreateProfile(PlayerID);

	if (bCompleted)
	{
		Profile.SocialReputation.ContractsCompleted++;

		// Update average rating
		int32 TotalCompleted = Profile.SocialReputation.ContractsCompleted;
		float PrevAvg = Profile.SocialReputation.AverageContractRating;
		Profile.SocialReputation.AverageContractRating =
			((PrevAvg * (TotalCompleted - 1)) + Rating) / static_cast<float>(TotalCompleted);
	}
	else
	{
		Profile.SocialReputation.ContractsFailed++;
	}

	RecalculateTrustScore(Profile.SocialReputation);
}

void UReputationSystem::RecordTradeOutcome(const FString& PlayerID)
{
	FScopeLock Lock(&ReputationLock);

	FPlayerReputationProfile& Profile = GetOrCreateProfile(PlayerID);
	Profile.SocialReputation.TradesCompleted++;
	RecalculateTrustScore(Profile.SocialReputation);
}

void UReputationSystem::RecordGuildContribution(const FString& PlayerID)
{
	FScopeLock Lock(&ReputationLock);

	FPlayerReputationProfile& Profile = GetOrCreateProfile(PlayerID);
	Profile.SocialReputation.GuildContributions++;
	RecalculateTrustScore(Profile.SocialReputation);
}

float UReputationSystem::GetPlayerTrustScore(const FString& PlayerID) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (!Profile) return 50.0f; // Default trust

	return Profile->SocialReputation.TrustScore;
}

bool UReputationSystem::GetPlayerSocialReputation(const FString& PlayerID, FPlayerSocialReputation& OutRep) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (Profile)
	{
		OutRep = Profile->SocialReputation;
		return true;
	}
	return false;
}

TArray<FPlayerFeedback> UReputationSystem::GetPlayerFeedbackHistory(const FString& PlayerID, int32 MaxCount) const
{
	FScopeLock Lock(&ReputationLock);

	TArray<FPlayerFeedback> Result;
	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (Profile)
	{
		int32 Count = FMath::Min(MaxCount, Profile->ReceivedFeedback.Num());
		for (int32 i = 0; i < Count; ++i)
		{
			Result.Add(Profile->ReceivedFeedback[i]);
		}
	}
	return Result;
}

// ==================== Cross-Faction Ripple Effects ====================

float UReputationSystem::GetFactionRippleMultiplier(EFaction SourceFaction, EFaction TargetFaction) const
{
	for (const FFactionRelationship& Rel : FactionRelationships)
	{
		if ((Rel.FactionA == SourceFaction && Rel.FactionB == TargetFaction) ||
			(Rel.FactionA == TargetFaction && Rel.FactionB == SourceFaction))
		{
			return Rel.RippleMultiplier;
		}
	}
	return 0.0f;
}

bool UReputationSystem::AreFactionAllies(EFaction FactionA, EFaction FactionB) const
{
	for (const FFactionRelationship& Rel : FactionRelationships)
	{
		if ((Rel.FactionA == FactionA && Rel.FactionB == FactionB) ||
			(Rel.FactionA == FactionB && Rel.FactionB == FactionA))
		{
			return Rel.BaseRelationship > 0.0f;
		}
	}
	return false;
}

bool UReputationSystem::AreFactionEnemies(EFaction FactionA, EFaction FactionB) const
{
	for (const FFactionRelationship& Rel : FactionRelationships)
	{
		if ((Rel.FactionA == FactionA && Rel.FactionB == FactionB) ||
			(Rel.FactionA == FactionB && Rel.FactionB == FactionA))
		{
			return Rel.BaseRelationship < 0.0f;
		}
	}
	return false;
}

void UReputationSystem::ApplyRippleEffects(const FString& PlayerID, EFaction SourceFaction,
	float BaseAmount, EReputationChangeSource Source)
{
	for (const FFactionRelationship& Rel : FactionRelationships)
	{
		EFaction RippleFaction = EFaction::None;

		if (Rel.FactionA == SourceFaction)
		{
			RippleFaction = Rel.FactionB;
		}
		else if (Rel.FactionB == SourceFaction)
		{
			RippleFaction = Rel.FactionA;
		}

		if (RippleFaction == EFaction::None || RippleFaction == SourceFaction)
		{
			continue;
		}

		float RippleAmount = BaseAmount * Rel.RippleMultiplier;
		if (FMath::Abs(RippleAmount) < 0.1f)
		{
			continue;
		}

		// Apply ripple without recursive ripple effects
		FPlayerReputationProfile& Profile = GetOrCreateProfile(PlayerID);

		FFactionStanding* Standing = Profile.FactionStandings.Find(RippleFaction);
		if (!Standing)
		{
			const FFactionDefinition* FactionDef = FactionDefinitions.Find(RippleFaction);
			float DefaultRep = FactionDef ? FactionDef->DefaultReputation : 0.0f;
			Profile.FactionStandings.Add(RippleFaction, FFactionStanding(RippleFaction, DefaultRep));
			Standing = Profile.FactionStandings.Find(RippleFaction);
		}

		if (Standing && !Standing->bIsLocked)
		{
			float OldRep = Standing->CurrentReputation;
			EReputationTier OldTier = Standing->CurrentTier;

			Standing->CurrentReputation = FMath::Clamp(
				Standing->CurrentReputation + RippleAmount, MinReputationValue, MaxReputationValue);

			if (RippleAmount > 0.0f) Standing->LifetimePositive += RippleAmount;
			else Standing->LifetimeNegative += FMath::Abs(RippleAmount);

			Standing->LastChangeTime = FDateTime::Now();
			Standing->RecalculateTier();

			if (OldTier != Standing->CurrentTier)
			{
				OnReputationTierChanged.Broadcast(PlayerID, RippleFaction, OldTier, Standing->CurrentTier);
			}
		}
	}
}

// ==================== Titles ====================

TArray<FName> UReputationSystem::GetAvailableTitles(const FString& PlayerID) const
{
	FScopeLock Lock(&ReputationLock);

	TArray<FName> Result;

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (!Profile) return Result;

	for (const auto& TitlePair : TitleRequirements)
	{
		EFaction RequiredFaction = TitlePair.Value.Key;
		EReputationTier RequiredTier = TitlePair.Value.Value;

		const FFactionStanding* Standing = Profile->FactionStandings.Find(RequiredFaction);
		if (Standing && static_cast<uint8>(Standing->CurrentTier) >= static_cast<uint8>(RequiredTier))
		{
			Result.Add(TitlePair.Key);
		}
	}

	return Result;
}

bool UReputationSystem::SetActiveTitle(const FString& PlayerID, FName TitleID)
{
	FScopeLock Lock(&ReputationLock);

	FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (!Profile) return false;

	// Verify player has this title
	if (TitleID != NAME_None && !Profile->UnlockedTitles.Contains(TitleID))
	{
		return false;
	}

	Profile->ActiveTitle = TitleID;
	return true;
}

FName UReputationSystem::GetActiveTitle(const FString& PlayerID) const
{
	FScopeLock Lock(&ReputationLock);

	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (Profile)
	{
		return Profile->ActiveTitle;
	}
	return NAME_None;
}

void UReputationSystem::CheckTitleUnlocks(const FString& PlayerID, FPlayerReputationProfile& Profile)
{
	for (const auto& TitlePair : TitleRequirements)
	{
		FName TitleID = TitlePair.Key;

		// Skip already unlocked
		if (Profile.UnlockedTitles.Contains(TitleID))
		{
			continue;
		}

		EFaction RequiredFaction = TitlePair.Value.Key;
		EReputationTier RequiredTier = TitlePair.Value.Value;

		const FFactionStanding* Standing = Profile.FactionStandings.Find(RequiredFaction);
		if (Standing && static_cast<uint8>(Standing->CurrentTier) >= static_cast<uint8>(RequiredTier))
		{
			Profile.UnlockedTitles.Add(TitleID);
			OnTitleUnlocked.Broadcast(PlayerID, TitleID, RequiredFaction);

			UE_LOG(LogTemp, Log, TEXT("Player %s unlocked title '%s' from faction %s"),
				*PlayerID, *TitleID.ToString(), *GetFactionName(RequiredFaction));
		}
	}
}

// ==================== Reputation History ====================

TArray<FReputationChangeRecord> UReputationSystem::GetReputationHistory(const FString& PlayerID,
	int32 MaxEntries) const
{
	FScopeLock Lock(&ReputationLock);

	TArray<FReputationChangeRecord> Result;
	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (Profile)
	{
		int32 Count = FMath::Min(MaxEntries, Profile->RecentChanges.Num());
		for (int32 i = 0; i < Count; ++i)
		{
			Result.Add(Profile->RecentChanges[i]);
		}
	}
	return Result;
}

TArray<FReputationChangeRecord> UReputationSystem::GetFactionHistory(const FString& PlayerID,
	EFaction Faction, int32 MaxEntries) const
{
	FScopeLock Lock(&ReputationLock);

	TArray<FReputationChangeRecord> Result;
	const FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (!Profile) return Result;

	for (const FReputationChangeRecord& Record : Profile->RecentChanges)
	{
		if (Record.Faction == Faction)
		{
			Result.Add(Record);
			if (Result.Num() >= MaxEntries)
			{
				break;
			}
		}
	}
	return Result;
}

// ==================== Faction Data ====================

bool UReputationSystem::GetFactionDefinition(EFaction Faction, FFactionDefinition& OutDefinition) const
{
	const FFactionDefinition* Def = FactionDefinitions.Find(Faction);
	if (Def)
	{
		OutDefinition = *Def;
		return true;
	}
	return false;
}

TArray<FFactionDefinition> UReputationSystem::GetAllFactions() const
{
	TArray<FFactionDefinition> Result;
	for (const auto& Pair : FactionDefinitions)
	{
		Result.Add(Pair.Value);
	}
	return Result;
}

FString UReputationSystem::GetFactionName(EFaction Faction) const
{
	const FFactionDefinition* Def = FactionDefinitions.Find(Faction);
	if (Def)
	{
		return Def->FactionName;
	}
	return TEXT("Unknown");
}

// ==================== Reputation Decay ====================

void UReputationSystem::ProcessReputationDecay(float DeltaTime)
{
	FScopeLock Lock(&ReputationLock);

	float DayFraction = (DeltaTime / 86400.0f) * GlobalDecayMultiplier;
	if (DayFraction <= 0.0f) return;

	for (auto& ProfilePair : PlayerProfiles)
	{
		FPlayerReputationProfile& Profile = ProfilePair.Value;

		for (auto& StandingPair : Profile.FactionStandings)
		{
			FFactionStanding& Standing = StandingPair.Value;

			if (Standing.bIsLocked) continue;

			const FFactionDefinition* FactionDef = FactionDefinitions.Find(Standing.Faction);
			if (!FactionDef || !FactionDef->bDecayEnabled) continue;

			float DefaultRep = FactionDef->DefaultReputation;
			float DecayAmount = FactionDef->DecayRatePerDay * DayFraction;

			if (FMath::IsNearlyEqual(Standing.CurrentReputation, DefaultRep, 0.1f))
			{
				continue; // Already at default
			}

			// Decay toward default
			if (Standing.CurrentReputation > DefaultRep)
			{
				Standing.CurrentReputation = FMath::Max(
					Standing.CurrentReputation - DecayAmount, DefaultRep);
			}
			else
			{
				Standing.CurrentReputation = FMath::Min(
					Standing.CurrentReputation + DecayAmount, DefaultRep);
			}

			Standing.RecalculateTier();
		}

		Profile.LastDecayProcessed = FDateTime::Now();
	}
}

void UReputationSystem::SetDecayRateMultiplier(float Multiplier)
{
	GlobalDecayMultiplier = FMath::Max(0.0f, Multiplier);
}

// ==================== Player Profile Management ====================

void UReputationSystem::EnsurePlayerProfile(const FString& PlayerID, const FString& PlayerName)
{
	FScopeLock Lock(&ReputationLock);
	GetOrCreateProfile(PlayerID, PlayerName);
}

void UReputationSystem::RemovePlayerProfile(const FString& PlayerID)
{
	FScopeLock Lock(&ReputationLock);
	PlayerProfiles.Remove(PlayerID);
}

bool UReputationSystem::HasPlayerProfile(const FString& PlayerID) const
{
	FScopeLock Lock(&ReputationLock);
	return PlayerProfiles.Contains(PlayerID);
}

FPlayerReputationProfile& UReputationSystem::GetOrCreateProfile(const FString& PlayerID, const FString& PlayerName)
{
	// Caller must hold lock

	FPlayerReputationProfile* Profile = PlayerProfiles.Find(PlayerID);
	if (Profile)
	{
		if (!PlayerName.IsEmpty())
		{
			Profile->PlayerName = PlayerName;
		}
		return *Profile;
	}

	FPlayerReputationProfile NewProfile;
	NewProfile.PlayerID = PlayerID;
	NewProfile.PlayerName = PlayerName;
	NewProfile.SocialReputation.PlayerID = PlayerID;
	NewProfile.SocialReputation.PlayerName = PlayerName;

	// Initialize all faction standings to defaults
	for (const auto& FactionPair : FactionDefinitions)
	{
		FFactionStanding Standing(FactionPair.Key, FactionPair.Value.DefaultReputation);
		NewProfile.FactionStandings.Add(FactionPair.Key, Standing);
	}

	PlayerProfiles.Add(PlayerID, NewProfile);
	return *PlayerProfiles.Find(PlayerID);
}

void UReputationSystem::TrimHistory(FPlayerReputationProfile& Profile)
{
	while (Profile.RecentChanges.Num() > MaxHistoryPerPlayer)
	{
		Profile.RecentChanges.RemoveAt(Profile.RecentChanges.Num() - 1);
	}
}

void UReputationSystem::RecalculateTrustScore(FPlayerSocialReputation& SocialRep)
{
	// Trust score formula:
	// Base: 50
	// + Feedback reliability score (0-20)
	// + Contract completion rate (0-15)
	// + Average contract rating (0-10)
	// + Trade activity bonus (0-5)

	float Score = 50.0f;

	// Feedback component
	int32 TotalFeedback = SocialRep.PositiveCount + SocialRep.NeutralCount + SocialRep.NegativeCount;
	if (TotalFeedback > 0)
	{
		float FeedbackRatio = static_cast<float>(SocialRep.PositiveCount) / static_cast<float>(TotalFeedback);
		Score += FeedbackRatio * 20.0f;

		// Penalty for negative feedback
		float NegativeRatio = static_cast<float>(SocialRep.NegativeCount) / static_cast<float>(TotalFeedback);
		Score -= NegativeRatio * 25.0f;
	}

	// Contract completion component
	float CompletionRate = SocialRep.GetCompletionRate();
	Score += CompletionRate * 15.0f;

	// Contract rating component
	if (SocialRep.AverageContractRating > 0.0f)
	{
		Score += (SocialRep.AverageContractRating / 5.0f) * 10.0f;
	}

	// Trade activity bonus (diminishing returns)
	float TradeBonus = FMath::Min(5.0f, FMath::Sqrt(static_cast<float>(SocialRep.TradesCompleted)) * 0.5f);
	Score += TradeBonus;

	SocialRep.TrustScore = FMath::Clamp(Score, 0.0f, 100.0f);
	SocialRep.LastUpdated = FDateTime::Now();
}

// ==================== NPC Behavior Integration ====================

bool UReputationSystem::ShouldNPCAttackPlayer(const FString& PlayerID, EFaction NPCFaction) const
{
	EReputationTier Tier = GetReputationTier(PlayerID, NPCFaction);
	return Tier == EReputationTier::Hostile || Tier == EReputationTier::Reviled;
}

bool UReputationSystem::ShouldNPCRefuseService(const FString& PlayerID, EFaction NPCFaction) const
{
	EReputationTier Tier = GetReputationTier(PlayerID, NPCFaction);
	return static_cast<uint8>(Tier) <= static_cast<uint8>(EReputationTier::Unfriendly);
}

float UReputationSystem::GetNPCDispositionModifier(const FString& PlayerID, EFaction NPCFaction) const
{
	float Reputation = GetReputation(PlayerID, NPCFaction);

	// Map -1000..1000 to -1.0..1.0
	return FMath::Clamp(Reputation / MaxReputationValue, -1.0f, 1.0f);
}

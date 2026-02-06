// GuildEconomyComponent.cpp
// Implementation of guild-level economic activities and shared resources

#include "GuildEconomyComponent.h"
#include "OdysseyGuildManager.h"
#include "TimerManager.h"
#include "Engine/World.h"

UGuildEconomyComponent::UGuildEconomyComponent()
	: GuildManager(nullptr)
	, MaxTransactionHistory(500)
	, MaxEconomySnapshots(90) // ~3 months of daily snapshots
{
}

void UGuildEconomyComponent::Initialize(UOdysseyGuildManager* InGuildManager)
{
	GuildManager = InGuildManager;

	if (UWorld* World = GetWorld())
	{
		// Weekly maintenance timer
		World->GetTimerManager().SetTimer(
			WeeklyMaintenanceTimer,
			this,
			&UGuildEconomyComponent::ProcessWeeklyMaintenance,
			604800.0f, // 7 days in seconds
			true
		);

		// Daily snapshot timer
		World->GetTimerManager().SetTimer(
			DailySnapshotTimer,
			FTimerDelegate::CreateLambda([this]()
			{
				FScopeLock Lock(&EconomyLock);
				for (auto& Pair : GuildEconomies)
				{
					TakeEconomySnapshot(Pair.Key);
				}
			}),
			86400.0f, // 24 hours
			true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("GuildEconomyComponent initialized"));
}

// ==================== Treasury Operations ====================

bool UGuildEconomyComponent::CollectTax(const FGuid& GuildID, const FString& MemberPlayerID,
	const FString& MemberName, EResourceType ResourceType, int64 EarningAmount)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	FGuildData GuildData;
	if (!GuildManager->GetGuildData(GuildID, GuildData)) return false;

	// Calculate tax amount based on policy-adjusted rate
	float TaxRate = GetEffectiveTaxRate(GuildID);
	int64 TaxAmount = static_cast<int64>(static_cast<float>(EarningAmount) * TaxRate);

	if (TaxAmount <= 0) return false;

	// Apply trade facility bonus (reduces effective tax for member, not for guild income)
	// The guild still collects full tax, this is just tracking

	// Deposit tax to treasury via guild bank
	if (!GuildManager->DepositToBank(GuildID, TEXT("SYSTEM_TAX"), ResourceType, TaxAmount))
	{
		return false;
	}

	// Record the transaction
	int64 NewBalance = GuildManager->GetBankBalance(GuildID, ResourceType);
	RecordTransaction(*EconData, ETreasuryTransactionType::TaxCollection, ResourceType,
		TaxAmount, NewBalance, MemberPlayerID, MemberName,
		FString::Printf(TEXT("Tax on %lld %s earnings"), EarningAmount, *UEnum::GetValueAsString(ResourceType)));

	// Update member contribution
	UpdateMemberContribution(*EconData, MemberPlayerID, MemberName,
		ETreasuryTransactionType::TaxCollection, TaxAmount);

	// Track income
	EconData->PeriodIncome += TaxAmount;

	// Update goal progress
	UpdateGoalProgress(GuildID, ResourceType, TaxAmount);

	OnTreasuryChanged.Broadcast(GuildID, ResourceType, TaxAmount, ETreasuryTransactionType::TaxCollection);

	return true;
}

bool UGuildEconomyComponent::TreasuryDeposit(const FGuid& GuildID, const FString& PlayerID,
	const FString& PlayerName, EResourceType ResourceType, int64 Amount,
	const FString& Description)
{
	FScopeLock Lock(&EconomyLock);

	if (Amount <= 0) return false;

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	// Check treasury capacity for OMEN
	if (ResourceType == EResourceType::OMEN)
	{
		int64 CurrentBalance = GuildManager->GetBankBalance(GuildID, EResourceType::OMEN);
		int64 Capacity = EconData->GetTreasuryCapacity();
		if (CurrentBalance + Amount > Capacity)
		{
			UE_LOG(LogTemp, Warning, TEXT("Treasury deposit rejected: would exceed capacity (%lld + %lld > %lld)"),
				CurrentBalance, Amount, Capacity);
			return false;
		}
	}

	if (!GuildManager->DepositToBank(GuildID, PlayerID, ResourceType, Amount))
	{
		return false;
	}

	int64 NewBalance = GuildManager->GetBankBalance(GuildID, ResourceType);
	FString Desc = Description.IsEmpty() ? TEXT("Direct deposit") : Description;

	RecordTransaction(*EconData, ETreasuryTransactionType::Deposit, ResourceType,
		Amount, NewBalance, PlayerID, PlayerName, Desc);

	UpdateMemberContribution(*EconData, PlayerID, PlayerName,
		ETreasuryTransactionType::Deposit, Amount);

	EconData->PeriodIncome += Amount;

	UpdateGoalProgress(GuildID, ResourceType, Amount);

	OnTreasuryChanged.Broadcast(GuildID, ResourceType, Amount, ETreasuryTransactionType::Deposit);

	return true;
}

bool UGuildEconomyComponent::TreasuryWithdraw(const FGuid& GuildID, const FString& PlayerID,
	EResourceType ResourceType, int64 Amount, const FString& Description)
{
	FScopeLock Lock(&EconomyLock);

	if (Amount <= 0) return false;

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	if (!GuildManager->WithdrawFromBank(GuildID, PlayerID, ResourceType, Amount))
	{
		return false;
	}

	int64 NewBalance = GuildManager->GetBankBalance(GuildID, ResourceType);
	FString Desc = Description.IsEmpty() ? TEXT("Withdrawal") : Description;

	RecordTransaction(*EconData, ETreasuryTransactionType::Withdrawal, ResourceType,
		-Amount, NewBalance, PlayerID, TEXT(""), Desc);

	UpdateMemberContribution(*EconData, PlayerID, TEXT(""),
		ETreasuryTransactionType::Withdrawal, Amount);

	EconData->PeriodExpenses += Amount;

	OnTreasuryChanged.Broadcast(GuildID, ResourceType, -Amount, ETreasuryTransactionType::Withdrawal);

	return true;
}

bool UGuildEconomyComponent::FundProject(const FGuid& GuildID, const FString& PlayerID,
	const FGuid& ProjectID, EResourceType ResourceType, int64 Amount)
{
	FScopeLock Lock(&EconomyLock);

	if (Amount <= 0) return false;

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	// Check permission
	if (!GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ManageProjects))
	{
		UE_LOG(LogTemp, Warning, TEXT("FundProject: Player %s lacks ManageProjects permission"), *PlayerID);
		return false;
	}

	// Withdraw from guild bank
	if (!GuildManager->WithdrawFromBank(GuildID, PlayerID, ResourceType, Amount))
	{
		return false;
	}

	int64 NewBalance = GuildManager->GetBankBalance(GuildID, ResourceType);

	RecordTransaction(*EconData, ETreasuryTransactionType::ProjectFunding, ResourceType,
		-Amount, NewBalance, PlayerID, TEXT(""),
		FString::Printf(TEXT("Project funding: %s"), *ProjectID.ToString()));

	EconData->PeriodExpenses += Amount;

	OnTreasuryChanged.Broadcast(GuildID, ResourceType, -Amount, ETreasuryTransactionType::ProjectFunding);

	return true;
}

int64 UGuildEconomyComponent::GetTreasuryBalance(const FGuid& GuildID, EResourceType ResourceType) const
{
	if (!GuildManager) return 0;
	return GuildManager->GetBankBalance(GuildID, ResourceType);
}

TMap<EResourceType, int64> UGuildEconomyComponent::GetAllTreasuryBalances(const FGuid& GuildID) const
{
	if (!GuildManager) return TMap<EResourceType, int64>();
	return GuildManager->GetAllBankResources(GuildID);
}

int64 UGuildEconomyComponent::GetTreasuryCapacity(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return 100000; // Default

	return EconData->GetTreasuryCapacity();
}

TArray<FTreasuryTransaction> UGuildEconomyComponent::GetTransactionHistory(const FGuid& GuildID,
	int32 MaxEntries) const
{
	FScopeLock Lock(&EconomyLock);

	TArray<FTreasuryTransaction> Result;
	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return Result;

	int32 Count = FMath::Min(MaxEntries, EconData->TransactionHistory.Num());
	for (int32 i = EconData->TransactionHistory.Num() - 1;
		 i >= EconData->TransactionHistory.Num() - Count; --i)
	{
		Result.Add(EconData->TransactionHistory[i]);
	}
	return Result;
}

TArray<FTreasuryTransaction> UGuildEconomyComponent::GetTransactionsByType(const FGuid& GuildID,
	ETreasuryTransactionType Type, int32 MaxEntries) const
{
	FScopeLock Lock(&EconomyLock);

	TArray<FTreasuryTransaction> Result;
	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return Result;

	for (int32 i = EconData->TransactionHistory.Num() - 1; i >= 0 && Result.Num() < MaxEntries; --i)
	{
		if (EconData->TransactionHistory[i].Type == Type)
		{
			Result.Add(EconData->TransactionHistory[i]);
		}
	}
	return Result;
}

// ==================== Facility Management ====================

bool UGuildEconomyComponent::BuildFacility(const FGuid& GuildID, const FString& PlayerID,
	EGuildFacilityType FacilityType, const FString& FacilityName)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	// Check permission
	if (!GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ManageFacilities))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildFacility: Player %s lacks ManageFacilities permission"), *PlayerID);
		return false;
	}

	// Check facility count limit
	int32 MaxFac = GetMaxFacilities(GuildID);
	if (EconData->GetActiveFacilityCount() >= MaxFac)
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildFacility: Max facilities (%d) reached"), MaxFac);
		return false;
	}

	// Get guild level for cost calculation
	FGuildData GuildData;
	int32 GuildLevel = 1;
	if (GuildManager->GetGuildData(GuildID, GuildData))
	{
		GuildLevel = GuildData.Level;
	}

	// Check cost
	TMap<EResourceType, int64> Cost = GetFacilityBuildCost(FacilityType, GuildLevel);
	if (!CanAfford(GuildID, Cost))
	{
		UE_LOG(LogTemp, Warning, TEXT("BuildFacility: Insufficient funds"));
		return false;
	}

	// Deduct cost
	if (!DeductCost(*EconData, Cost, PlayerID, TEXT(""),
		FString::Printf(TEXT("Build %s"), *FacilityName)))
	{
		return false;
	}

	// Create facility
	FGuildFacility NewFacility = CreateDefaultFacility(FacilityType, FacilityName, GuildLevel);
	EconData->Facilities.Add(NewFacility);

	OnFacilityBuilt.Broadcast(GuildID, FacilityType, 1);

	UE_LOG(LogTemp, Log, TEXT("Built facility '%s' (type %d) for guild %s"),
		*FacilityName, static_cast<int32>(FacilityType), *GuildID.ToString());

	return true;
}

bool UGuildEconomyComponent::UpgradeFacility(const FGuid& GuildID, const FString& PlayerID,
	const FGuid& FacilityID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	if (!GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ManageFacilities))
	{
		return false;
	}

	FGuildFacility* Facility = nullptr;
	for (FGuildFacility& F : EconData->Facilities)
	{
		if (F.FacilityID == FacilityID)
		{
			Facility = &F;
			break;
		}
	}

	if (!Facility)
	{
		return false;
	}

	if (Facility->Level >= Facility->MaxLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("UpgradeFacility: Already at max level"));
		return false;
	}

	// Get upgrade cost
	TMap<EResourceType, int64> Cost = GetFacilityUpgradeCost(GuildID, FacilityID);
	if (!CanAfford(GuildID, Cost))
	{
		return false;
	}

	if (!DeductCost(*EconData, Cost, PlayerID, TEXT(""),
		FString::Printf(TEXT("Upgrade %s to level %d"), *Facility->FacilityName, Facility->Level + 1)))
	{
		return false;
	}

	Facility->Level++;

	// Update bonus based on level
	switch (Facility->FacilityType)
	{
	case EGuildFacilityType::Warehouse:
		Facility->CapacityBonus = 25000 * Facility->Level;
		break;
	case EGuildFacilityType::TradingPost:
		Facility->BonusMultiplier = 1.0f + (0.05f * Facility->Level);
		break;
	case EGuildFacilityType::Refinery:
		Facility->BonusMultiplier = 1.0f + (0.08f * Facility->Level);
		break;
	case EGuildFacilityType::Workshop:
		Facility->BonusMultiplier = 1.0f + (0.06f * Facility->Level);
		break;
	case EGuildFacilityType::ResearchLab:
		Facility->BonusMultiplier = 1.0f + (0.10f * Facility->Level);
		break;
	default:
		Facility->BonusMultiplier = 1.0f + (0.05f * Facility->Level);
		break;
	}

	// Update upkeep
	for (auto& UpkeepPair : Facility->DailyUpkeep)
	{
		UpkeepPair.Value = static_cast<int64>(UpkeepPair.Value * 1.5f);
	}

	OnFacilityUpgraded.Broadcast(GuildID, FacilityID, Facility->Level);

	return true;
}

bool UGuildEconomyComponent::DeactivateFacility(const FGuid& GuildID, const FString& PlayerID,
	const FGuid& FacilityID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager || !GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ManageFacilities))
	{
		return false;
	}

	for (FGuildFacility& F : EconData->Facilities)
	{
		if (F.FacilityID == FacilityID)
		{
			F.bIsActive = false;
			return true;
		}
	}
	return false;
}

bool UGuildEconomyComponent::ActivateFacility(const FGuid& GuildID, const FString& PlayerID,
	const FGuid& FacilityID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager || !GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ManageFacilities))
	{
		return false;
	}

	for (FGuildFacility& F : EconData->Facilities)
	{
		if (F.FacilityID == FacilityID)
		{
			F.bIsActive = true;
			return true;
		}
	}
	return false;
}

bool UGuildEconomyComponent::DemolishFacility(const FGuid& GuildID, const FString& PlayerID,
	const FGuid& FacilityID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager || !GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ManageFacilities))
	{
		return false;
	}

	for (int32 i = 0; i < EconData->Facilities.Num(); ++i)
	{
		if (EconData->Facilities[i].FacilityID == FacilityID)
		{
			UE_LOG(LogTemp, Log, TEXT("Demolished facility '%s'"), *EconData->Facilities[i].FacilityName);
			EconData->Facilities.RemoveAt(i);
			return true;
		}
	}
	return false;
}

TArray<FGuildFacility> UGuildEconomyComponent::GetFacilities(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (EconData)
	{
		return EconData->Facilities;
	}
	return TArray<FGuildFacility>();
}

float UGuildEconomyComponent::GetFacilityBonus(const FGuid& GuildID, EGuildFacilityType Type) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (EconData)
	{
		return EconData->GetFacilityBonus(Type);
	}
	return 1.0f;
}

TMap<EResourceType, int64> UGuildEconomyComponent::GetFacilityBuildCost(EGuildFacilityType Type, int32 GuildLevel) const
{
	TMap<EResourceType, int64> Cost;

	// Base costs scale with guild level (higher level = more expensive but more powerful)
	float LevelMultiplier = 1.0f + (static_cast<float>(GuildLevel - 1) * 0.2f);

	switch (Type)
	{
	case EGuildFacilityType::Warehouse:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(5000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(50 * LevelMultiplier));
		break;
	case EGuildFacilityType::TradingPost:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(8000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(30 * LevelMultiplier));
		break;
	case EGuildFacilityType::Refinery:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(10000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(80 * LevelMultiplier));
		Cost.Add(EResourceType::RefinedSilicate, static_cast<int64>(100 * LevelMultiplier));
		break;
	case EGuildFacilityType::Workshop:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(7000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(60 * LevelMultiplier));
		break;
	case EGuildFacilityType::ResearchLab:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(15000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(100 * LevelMultiplier));
		break;
	case EGuildFacilityType::DefensePlatform:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(12000 * LevelMultiplier));
		Cost.Add(EResourceType::RefinedSilicate, static_cast<int64>(200 * LevelMultiplier));
		Cost.Add(EResourceType::RefinedCarbon, static_cast<int64>(150 * LevelMultiplier));
		break;
	case EGuildFacilityType::ShipYard:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(20000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(150 * LevelMultiplier));
		break;
	case EGuildFacilityType::MarketTerminal:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(6000 * LevelMultiplier));
		break;
	case EGuildFacilityType::Beacon:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(3000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(20 * LevelMultiplier));
		break;
	case EGuildFacilityType::Embassy:
		Cost.Add(EResourceType::OMEN, static_cast<int64>(10000 * LevelMultiplier));
		Cost.Add(EResourceType::CompositeMaterial, static_cast<int64>(40 * LevelMultiplier));
		break;
	}

	return Cost;
}

TMap<EResourceType, int64> UGuildEconomyComponent::GetFacilityUpgradeCost(const FGuid& GuildID,
	const FGuid& FacilityID) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return TMap<EResourceType, int64>();

	for (const FGuildFacility& F : EconData->Facilities)
	{
		if (F.FacilityID == FacilityID)
		{
			// Upgrade cost = build cost * (currentLevel + 1) * 0.75
			int32 GuildLevel = 1;
			if (GuildManager)
			{
				FGuildData GuildData;
				if (GuildManager->GetGuildData(GuildID, GuildData))
				{
					GuildLevel = GuildData.Level;
				}
			}

			TMap<EResourceType, int64> BaseCost = GetFacilityBuildCost(F.FacilityType, GuildLevel);
			float UpgradeMultiplier = static_cast<float>(F.Level + 1) * 0.75f;

			TMap<EResourceType, int64> UpgradeCost;
			for (const auto& Pair : BaseCost)
			{
				UpgradeCost.Add(Pair.Key, static_cast<int64>(Pair.Value * UpgradeMultiplier));
			}
			return UpgradeCost;
		}
	}

	return TMap<EResourceType, int64>();
}

void UGuildEconomyComponent::ProcessFacilityUpkeep(const FGuid& GuildID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return;

	for (FGuildFacility& Facility : EconData->Facilities)
	{
		if (!Facility.bIsActive) continue;

		bool bCanPayUpkeep = true;
		for (const auto& UpkeepPair : Facility.DailyUpkeep)
		{
			int64 Balance = GetTreasuryBalance(GuildID, UpkeepPair.Key);
			if (Balance < UpkeepPair.Value)
			{
				bCanPayUpkeep = false;
				break;
			}
		}

		if (bCanPayUpkeep)
		{
			for (const auto& UpkeepPair : Facility.DailyUpkeep)
			{
				if (GuildManager)
				{
					GuildManager->WithdrawFromBank(GuildID, TEXT("SYSTEM_UPKEEP"), UpkeepPair.Key, UpkeepPair.Value);
				}

				EconData->PeriodExpenses += UpkeepPair.Value;
			}

			Facility.LastUpkeepPaid = FDateTime::Now();
		}
		else
		{
			// Cannot afford upkeep - deactivate facility
			Facility.bIsActive = false;
			UE_LOG(LogTemp, Warning, TEXT("Facility '%s' deactivated due to insufficient upkeep"), *Facility.FacilityName);
		}
	}
}

int32 UGuildEconomyComponent::GetMaxFacilities(const FGuid& GuildID) const
{
	if (!GuildManager) return 3;

	FGuildData GuildData;
	if (GuildManager->GetGuildData(GuildID, GuildData))
	{
		// Base 3 + 1 per 2 guild levels
		return 3 + (GuildData.Level / 2);
	}
	return 3;
}

// ==================== Economic Goals ====================

FGuid UGuildEconomyComponent::CreateGoal(const FGuid& GuildID, const FString& CreatorPlayerID,
	const FString& GoalName, const FString& Description,
	const TMap<EResourceType, int64>& TargetResources,
	int32 TargetTrades, int32 DaysToComplete)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return FGuid();

	if (!GuildManager || !GuildManager->HasPermission(GuildID, CreatorPlayerID, EGuildPermission::ManageProjects))
	{
		return FGuid();
	}

	FGuildEconomicGoal Goal;
	Goal.GoalName = GoalName;
	Goal.Description = Description;
	Goal.TargetResources = TargetResources;
	Goal.TargetTradeCount = TargetTrades;
	Goal.Deadline = FDateTime::Now() + FTimespan::FromDays(DaysToComplete);
	Goal.CreatorPlayerID = CreatorPlayerID;

	// Initialize progress tracking
	for (const auto& Pair : TargetResources)
	{
		Goal.CurrentProgress.Add(Pair.Key, 0);
	}

	FGuid GoalID = Goal.GoalID;
	EconData->Goals.Add(Goal);

	UE_LOG(LogTemp, Log, TEXT("Created economic goal '%s' for guild %s"), *GoalName, *GuildID.ToString());

	return GoalID;
}

void UGuildEconomyComponent::UpdateGoalProgress(const FGuid& GuildID, EResourceType ResourceType, int64 Amount)
{
	// Caller should hold lock or this should be called from locked context
	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return;

	for (FGuildEconomicGoal& Goal : EconData->Goals)
	{
		if (Goal.Status != EGuildGoalStatus::Active) continue;

		int64* CurrentAmount = Goal.CurrentProgress.Find(ResourceType);
		if (CurrentAmount)
		{
			*CurrentAmount += Amount;
		}
	}
}

void UGuildEconomyComponent::IncrementGoalTradeCount(const FGuid& GuildID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return;

	for (FGuildEconomicGoal& Goal : EconData->Goals)
	{
		if (Goal.Status == EGuildGoalStatus::Active && Goal.TargetTradeCount > 0)
		{
			Goal.CurrentTradeCount++;
		}
	}

	CheckGoalCompletion(GuildID);
}

bool UGuildEconomyComponent::CancelGoal(const FGuid& GuildID, const FString& PlayerID, const FGuid& GoalID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager || !GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ManageProjects))
	{
		return false;
	}

	for (FGuildEconomicGoal& Goal : EconData->Goals)
	{
		if (Goal.GoalID == GoalID && Goal.Status == EGuildGoalStatus::Active)
		{
			Goal.Status = EGuildGoalStatus::Failed;
			return true;
		}
	}
	return false;
}

TArray<FGuildEconomicGoal> UGuildEconomyComponent::GetActiveGoals(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	TArray<FGuildEconomicGoal> Result;
	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return Result;

	for (const FGuildEconomicGoal& Goal : EconData->Goals)
	{
		if (Goal.Status == EGuildGoalStatus::Active)
		{
			Result.Add(Goal);
		}
	}
	return Result;
}

TArray<FGuildEconomicGoal> UGuildEconomyComponent::GetAllGoals(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (EconData)
	{
		return EconData->Goals;
	}
	return TArray<FGuildEconomicGoal>();
}

void UGuildEconomyComponent::CheckGoalCompletion(const FGuid& GuildID)
{
	// Caller should hold lock

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return;

	for (FGuildEconomicGoal& Goal : EconData->Goals)
	{
		if (Goal.Status != EGuildGoalStatus::Active) continue;

		bool bResourcesMet = Goal.GetResourceProgress() >= 1.0f;
		bool bTradesMet = Goal.GetTradeProgress() >= 1.0f;

		if (bResourcesMet && bTradesMet)
		{
			Goal.Status = EGuildGoalStatus::Completed;
			Goal.CompletedAt = FDateTime::Now();

			// Award guild experience
			if (GuildManager && Goal.GuildExperienceReward > 0)
			{
				GuildManager->AddGuildExperience(GuildID, Goal.GuildExperienceReward);
			}

			OnGoalCompleted.Broadcast(GuildID, Goal.GoalName);

			UE_LOG(LogTemp, Log, TEXT("Guild goal '%s' completed!"), *Goal.GoalName);
		}
	}
}

void UGuildEconomyComponent::ProcessExpiredGoals(const FGuid& GuildID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return;

	for (FGuildEconomicGoal& Goal : EconData->Goals)
	{
		if (Goal.IsExpired())
		{
			Goal.Status = EGuildGoalStatus::Expired;
		}
	}
}

// ==================== Dividend Distribution ====================

bool UGuildEconomyComponent::DistributeEqualDividend(const FGuid& GuildID, const FString& AuthorizerPlayerID,
	EResourceType ResourceType, int64 TotalAmount)
{
	FScopeLock Lock(&EconomyLock);

	if (TotalAmount <= 0) return false;

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	// Permission check
	if (!GuildManager->HasPermission(GuildID, AuthorizerPlayerID, EGuildPermission::ManageTaxes))
	{
		return false;
	}

	// Check treasury has enough
	int64 Balance = GuildManager->GetBankBalance(GuildID, ResourceType);
	if (Balance < TotalAmount) return false;

	FGuildData GuildData;
	if (!GuildManager->GetGuildData(GuildID, GuildData)) return false;

	int32 MemberCount = GuildData.GetMemberCount();
	if (MemberCount <= 0) return false;

	int64 PerMember = TotalAmount / static_cast<int64>(MemberCount);
	if (PerMember <= 0) return false;

	// Withdraw total from treasury
	int64 ActualTotal = PerMember * static_cast<int64>(MemberCount);
	GuildManager->WithdrawFromBank(GuildID, TEXT("SYSTEM_DIVIDEND"), ResourceType, ActualTotal);

	// Record
	FDividendRecord Record;
	Record.AuthorizerPlayerID = AuthorizerPlayerID;
	Record.RecipientCount = MemberCount;
	Record.TotalDistributed.Add(ResourceType, ActualTotal);
	Record.Method = TEXT("Equal");
	EconData->DividendHistory.Add(Record);

	// Update member tracking
	for (const FGuildMember& Member : GuildData.Members)
	{
		FMemberEconomicContribution* MemberEcon = EconData->MemberContributions.Find(Member.PlayerID);
		if (MemberEcon)
		{
			MemberEcon->TotalDividendsReceived += PerMember;
			MemberEcon->RecalculateNet();
		}
	}

	int64 NewBalance = GuildManager->GetBankBalance(GuildID, ResourceType);
	RecordTransaction(*EconData, ETreasuryTransactionType::DividendPayout, ResourceType,
		-ActualTotal, NewBalance, AuthorizerPlayerID, TEXT(""),
		FString::Printf(TEXT("Equal dividend: %lld each to %d members"), PerMember, MemberCount));

	EconData->PeriodExpenses += ActualTotal;

	OnDividendDistributed.Broadcast(GuildID, MemberCount, ActualTotal);

	return true;
}

bool UGuildEconomyComponent::DistributeContributionDividend(const FGuid& GuildID, const FString& AuthorizerPlayerID,
	EResourceType ResourceType, int64 TotalAmount)
{
	FScopeLock Lock(&EconomyLock);

	if (TotalAmount <= 0) return false;

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	if (!GuildManager->HasPermission(GuildID, AuthorizerPlayerID, EGuildPermission::ManageTaxes))
	{
		return false;
	}

	int64 Balance = GuildManager->GetBankBalance(GuildID, ResourceType);
	if (Balance < TotalAmount) return false;

	// Calculate total contributions
	int64 TotalContributions = 0;
	for (const auto& Pair : EconData->MemberContributions)
	{
		int64 Contribution = Pair.Value.TotalDeposited + Pair.Value.TotalTaxesPaid + Pair.Value.TotalProjectContributions;
		if (Contribution > 0) TotalContributions += Contribution;
	}

	if (TotalContributions <= 0) return false;

	GuildManager->WithdrawFromBank(GuildID, TEXT("SYSTEM_DIVIDEND"), ResourceType, TotalAmount);

	int32 RecipientCount = 0;
	int64 Distributed = 0;

	for (auto& Pair : EconData->MemberContributions)
	{
		FMemberEconomicContribution& MemberEcon = Pair.Value;
		int64 MemberContrib = MemberEcon.TotalDeposited + MemberEcon.TotalTaxesPaid + MemberEcon.TotalProjectContributions;

		if (MemberContrib <= 0) continue;

		float Ratio = static_cast<float>(MemberContrib) / static_cast<float>(TotalContributions);
		int64 Share = static_cast<int64>(static_cast<float>(TotalAmount) * Ratio);

		MemberEcon.TotalDividendsReceived += Share;
		MemberEcon.RecalculateNet();

		Distributed += Share;
		RecipientCount++;
	}

	FDividendRecord Record;
	Record.AuthorizerPlayerID = AuthorizerPlayerID;
	Record.RecipientCount = RecipientCount;
	Record.TotalDistributed.Add(ResourceType, Distributed);
	Record.Method = TEXT("Contribution-based");
	EconData->DividendHistory.Add(Record);

	int64 NewBalance = GuildManager->GetBankBalance(GuildID, ResourceType);
	RecordTransaction(*EconData, ETreasuryTransactionType::DividendPayout, ResourceType,
		-Distributed, NewBalance, AuthorizerPlayerID, TEXT(""),
		FString::Printf(TEXT("Contribution-based dividend to %d members"), RecipientCount));

	EconData->PeriodExpenses += Distributed;

	OnDividendDistributed.Broadcast(GuildID, RecipientCount, Distributed);

	return true;
}

bool UGuildEconomyComponent::DistributeRankDividend(const FGuid& GuildID, const FString& AuthorizerPlayerID,
	EResourceType ResourceType, int64 TotalAmount)
{
	FScopeLock Lock(&EconomyLock);

	if (TotalAmount <= 0) return false;

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager) return false;

	if (!GuildManager->HasPermission(GuildID, AuthorizerPlayerID, EGuildPermission::ManageTaxes))
	{
		return false;
	}

	int64 Balance = GuildManager->GetBankBalance(GuildID, ResourceType);
	if (Balance < TotalAmount) return false;

	FGuildData GuildData;
	if (!GuildManager->GetGuildData(GuildID, GuildData)) return false;

	// Calculate weighted total based on rank priority
	float TotalWeight = 0.0f;
	TMap<FString, float> MemberWeights;

	for (const FGuildMember& Member : GuildData.Members)
	{
		const FGuildRole* Role = GuildData.GetRole(Member.RoleID);
		float Weight = Role ? static_cast<float>(Role->RankPriority + 1) : 1.0f;
		MemberWeights.Add(Member.PlayerID, Weight);
		TotalWeight += Weight;
	}

	if (TotalWeight <= 0.0f) return false;

	GuildManager->WithdrawFromBank(GuildID, TEXT("SYSTEM_DIVIDEND"), ResourceType, TotalAmount);

	int32 RecipientCount = 0;
	int64 Distributed = 0;

	for (const auto& WeightPair : MemberWeights)
	{
		float Ratio = WeightPair.Value / TotalWeight;
		int64 Share = static_cast<int64>(static_cast<float>(TotalAmount) * Ratio);

		FMemberEconomicContribution* MemberEcon = EconData->MemberContributions.Find(WeightPair.Key);
		if (MemberEcon)
		{
			MemberEcon->TotalDividendsReceived += Share;
			MemberEcon->RecalculateNet();
		}

		Distributed += Share;
		RecipientCount++;
	}

	FDividendRecord Record;
	Record.AuthorizerPlayerID = AuthorizerPlayerID;
	Record.RecipientCount = RecipientCount;
	Record.TotalDistributed.Add(ResourceType, Distributed);
	Record.Method = TEXT("Rank-based");
	EconData->DividendHistory.Add(Record);

	int64 NewBalance = GuildManager->GetBankBalance(GuildID, ResourceType);
	RecordTransaction(*EconData, ETreasuryTransactionType::DividendPayout, ResourceType,
		-Distributed, NewBalance, AuthorizerPlayerID, TEXT(""),
		FString::Printf(TEXT("Rank-based dividend to %d members"), RecipientCount));

	EconData->PeriodExpenses += Distributed;

	OnDividendDistributed.Broadcast(GuildID, RecipientCount, Distributed);

	return true;
}

TArray<FDividendRecord> UGuildEconomyComponent::GetDividendHistory(const FGuid& GuildID, int32 MaxEntries) const
{
	FScopeLock Lock(&EconomyLock);

	TArray<FDividendRecord> Result;
	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return Result;

	int32 Count = FMath::Min(MaxEntries, EconData->DividendHistory.Num());
	for (int32 i = EconData->DividendHistory.Num() - 1;
		 i >= EconData->DividendHistory.Num() - Count; --i)
	{
		Result.Add(EconData->DividendHistory[i]);
	}
	return Result;
}

// ==================== Economic Policy ====================

bool UGuildEconomyComponent::SetEconomicPolicy(const FGuid& GuildID, const FString& PlayerID,
	EGuildEconomicPolicy Policy)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	if (!GuildManager || !GuildManager->HasPermission(GuildID, PlayerID, EGuildPermission::ModifyGuildSettings))
	{
		return false;
	}

	EconData->Policy = Policy;
	OnPolicyChanged.Broadcast(GuildID, Policy);

	return true;
}

EGuildEconomicPolicy UGuildEconomyComponent::GetEconomicPolicy(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (EconData)
	{
		return EconData->Policy;
	}
	return EGuildEconomicPolicy::Cooperative;
}

float UGuildEconomyComponent::GetEffectiveTaxRate(const FGuid& GuildID) const
{
	if (!GuildManager) return 0.05f;

	FGuildData GuildData;
	if (!GuildManager->GetGuildData(GuildID, GuildData))
	{
		return 0.05f;
	}

	float BaseTax = GuildData.TaxRate;

	FScopeLock Lock(&EconomyLock);
	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return BaseTax;

	// Policy modifiers
	switch (EconData->Policy)
	{
	case EGuildEconomicPolicy::FreeMarket:
		BaseTax *= 0.5f;  // Half tax
		break;
	case EGuildEconomicPolicy::Cooperative:
		// No modifier
		break;
	case EGuildEconomicPolicy::Collectivist:
		BaseTax *= 2.0f;  // Double tax
		break;
	case EGuildEconomicPolicy::MilitaryEconomy:
		BaseTax *= 1.5f;  // 50% more tax
		break;
	case EGuildEconomicPolicy::Research:
		BaseTax *= 1.25f; // 25% more tax
		break;
	default:
		break;
	}

	return FMath::Clamp(BaseTax, 0.0f, 0.5f); // Cap at 50%
}

// ==================== Member Economic Data ====================

bool UGuildEconomyComponent::GetMemberContribution(const FGuid& GuildID, const FString& PlayerID,
	FMemberEconomicContribution& OutContribution) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return false;

	const FMemberEconomicContribution* Contrib = EconData->MemberContributions.Find(PlayerID);
	if (Contrib)
	{
		OutContribution = *Contrib;
		return true;
	}
	return false;
}

TArray<FMemberEconomicContribution> UGuildEconomyComponent::GetMemberContributions(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	TArray<FMemberEconomicContribution> Result;
	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return Result;

	for (const auto& Pair : EconData->MemberContributions)
	{
		Result.Add(Pair.Value);
	}

	Result.Sort([](const FMemberEconomicContribution& A, const FMemberEconomicContribution& B) {
		return A.NetContribution > B.NetContribution;
	});

	return Result;
}

TArray<FMemberEconomicContribution> UGuildEconomyComponent::GetTopContributors(const FGuid& GuildID, int32 Count) const
{
	TArray<FMemberEconomicContribution> All = GetMemberContributions(GuildID);
	if (All.Num() > Count)
	{
		All.SetNum(Count);
	}
	return All;
}

// ==================== Analytics ====================

void UGuildEconomyComponent::TakeEconomySnapshot(const FGuid& GuildID)
{
	// Caller may or may not hold lock

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return;

	FGuildEconomySnapshot Snapshot;
	Snapshot.TreasuryBalances = GetAllTreasuryBalances(GuildID);
	Snapshot.TotalIncome = EconData->PeriodIncome;
	Snapshot.TotalExpenses = EconData->PeriodExpenses;
	Snapshot.ActiveFacilityCount = EconData->GetActiveFacilityCount();

	int32 ActiveGoals = 0;
	for (const FGuildEconomicGoal& G : EconData->Goals)
	{
		if (G.Status == EGuildGoalStatus::Active) ActiveGoals++;
	}
	Snapshot.ActiveProjectCount = ActiveGoals;

	if (GuildManager)
	{
		FGuildData GuildData;
		if (GuildManager->GetGuildData(GuildID, GuildData))
		{
			Snapshot.ActiveMemberCount = GuildData.GetOnlineMemberCount();
		}
	}

	EconData->EconomyHistory.Add(Snapshot);

	// Trim old snapshots
	while (EconData->EconomyHistory.Num() > MaxEconomySnapshots)
	{
		EconData->EconomyHistory.RemoveAt(0);
	}
}

TArray<FGuildEconomySnapshot> UGuildEconomyComponent::GetEconomyHistory(const FGuid& GuildID, int32 MaxEntries) const
{
	FScopeLock Lock(&EconomyLock);

	TArray<FGuildEconomySnapshot> Result;
	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return Result;

	int32 Count = FMath::Min(MaxEntries, EconData->EconomyHistory.Num());
	int32 Start = EconData->EconomyHistory.Num() - Count;
	for (int32 i = Start; i < EconData->EconomyHistory.Num(); ++i)
	{
		Result.Add(EconData->EconomyHistory[i]);
	}
	return Result;
}

int64 UGuildEconomyComponent::GetPeriodIncome(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	return EconData ? EconData->PeriodIncome : 0;
}

int64 UGuildEconomyComponent::GetPeriodExpenses(const FGuid& GuildID) const
{
	FScopeLock Lock(&EconomyLock);

	const FGuildEconomyData* EconData = GetEconomyData(GuildID);
	return EconData ? EconData->PeriodExpenses : 0;
}

void UGuildEconomyComponent::ResetPeriodTracking(const FGuid& GuildID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (EconData)
	{
		EconData->PeriodIncome = 0;
		EconData->PeriodExpenses = 0;
		EconData->PeriodStart = FDateTime::Now();
	}
}

// ==================== Guild Economy Lifecycle ====================

void UGuildEconomyComponent::InitializeGuildEconomy(const FGuid& GuildID)
{
	FScopeLock Lock(&EconomyLock);

	if (GuildEconomies.Contains(GuildID))
	{
		return;
	}

	FGuildEconomyData NewEconomy;
	NewEconomy.GuildID = GuildID;

	GuildEconomies.Add(GuildID, NewEconomy);

	UE_LOG(LogTemp, Log, TEXT("Initialized economy for guild %s"), *GuildID.ToString());
}

void UGuildEconomyComponent::RemoveGuildEconomy(const FGuid& GuildID)
{
	FScopeLock Lock(&EconomyLock);
	GuildEconomies.Remove(GuildID);
}

void UGuildEconomyComponent::RegisterMember(const FGuid& GuildID, const FString& PlayerID, const FString& PlayerName)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (!EconData) return;

	if (!EconData->MemberContributions.Contains(PlayerID))
	{
		FMemberEconomicContribution NewContrib;
		NewContrib.PlayerID = PlayerID;
		NewContrib.PlayerName = PlayerName;
		EconData->MemberContributions.Add(PlayerID, NewContrib);
	}
}

void UGuildEconomyComponent::UnregisterMember(const FGuid& GuildID, const FString& PlayerID)
{
	FScopeLock Lock(&EconomyLock);

	FGuildEconomyData* EconData = GetEconomyData(GuildID);
	if (EconData)
	{
		// Keep the data for historical purposes, don't remove
		// Just mark inactive if needed
	}
}

void UGuildEconomyComponent::ProcessWeeklyMaintenance()
{
	FScopeLock Lock(&EconomyLock);

	for (auto& Pair : GuildEconomies)
	{
		FGuildEconomyData& EconData = Pair.Value;

		// Reset weekly counters
		for (auto& MemberPair : EconData.MemberContributions)
		{
			MemberPair.Value.CurrentWeekContribution = 0;
		}

		// Process facility upkeep (7 days worth)
		for (int32 Day = 0; Day < 7; ++Day)
		{
			ProcessFacilityUpkeep(Pair.Key);
		}

		// Check for expired goals
		ProcessExpiredGoals(Pair.Key);

		// Take snapshot
		TakeEconomySnapshot(Pair.Key);

		// Reset period tracking
		ResetPeriodTracking(Pair.Key);
	}
}

// ==================== Internal Helpers ====================

FGuildEconomyData* UGuildEconomyComponent::GetEconomyData(const FGuid& GuildID)
{
	return GuildEconomies.Find(GuildID);
}

const FGuildEconomyData* UGuildEconomyComponent::GetEconomyData(const FGuid& GuildID) const
{
	return GuildEconomies.Find(GuildID);
}

void UGuildEconomyComponent::RecordTransaction(FGuildEconomyData& EconData, ETreasuryTransactionType Type,
	EResourceType ResourceType, int64 Amount, int64 BalanceAfter,
	const FString& PlayerID, const FString& PlayerName, const FString& Description)
{
	FTreasuryTransaction Transaction;
	Transaction.Type = Type;
	Transaction.ResourceType = ResourceType;
	Transaction.Amount = Amount;
	Transaction.BalanceAfter = BalanceAfter;
	Transaction.InitiatorPlayerID = PlayerID;
	Transaction.InitiatorName = PlayerName;
	Transaction.Description = Description;

	EconData.TransactionHistory.Add(Transaction);
	TrimTransactionHistory(EconData);
}

void UGuildEconomyComponent::UpdateMemberContribution(FGuildEconomyData& EconData,
	const FString& PlayerID, const FString& PlayerName,
	ETreasuryTransactionType Type, int64 Amount)
{
	FMemberEconomicContribution* Contrib = EconData.MemberContributions.Find(PlayerID);
	if (!Contrib)
	{
		FMemberEconomicContribution NewContrib;
		NewContrib.PlayerID = PlayerID;
		NewContrib.PlayerName = PlayerName;
		EconData.MemberContributions.Add(PlayerID, NewContrib);
		Contrib = EconData.MemberContributions.Find(PlayerID);
	}

	if (!Contrib) return;

	switch (Type)
	{
	case ETreasuryTransactionType::TaxCollection:
		Contrib->TotalTaxesPaid += Amount;
		break;
	case ETreasuryTransactionType::Deposit:
		Contrib->TotalDeposited += Amount;
		break;
	case ETreasuryTransactionType::Withdrawal:
		Contrib->TotalWithdrawn += Amount;
		break;
	case ETreasuryTransactionType::ProjectFunding:
		Contrib->TotalProjectContributions += Amount;
		break;
	default:
		break;
	}

	Contrib->CurrentWeekContribution += Amount;
	Contrib->LastContribution = FDateTime::Now();
	Contrib->RecalculateNet();
}

FGuildFacility UGuildEconomyComponent::CreateDefaultFacility(EGuildFacilityType Type,
	const FString& Name, int32 GuildLevel)
{
	FGuildFacility Facility;
	Facility.FacilityType = Type;
	Facility.FacilityName = Name;
	Facility.Level = 1;
	Facility.bIsActive = true;

	// Set max level based on guild level
	Facility.MaxLevel = FMath::Min(5, 2 + (GuildLevel / 3));

	// Set type-specific defaults
	switch (Type)
	{
	case EGuildFacilityType::Warehouse:
		Facility.CapacityBonus = 25000;
		Facility.BonusMultiplier = 1.0f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 100);
		break;
	case EGuildFacilityType::TradingPost:
		Facility.BonusMultiplier = 1.05f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 200);
		break;
	case EGuildFacilityType::Refinery:
		Facility.BonusMultiplier = 1.08f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 300);
		break;
	case EGuildFacilityType::Workshop:
		Facility.BonusMultiplier = 1.06f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 250);
		break;
	case EGuildFacilityType::ResearchLab:
		Facility.BonusMultiplier = 1.10f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 400);
		break;
	case EGuildFacilityType::DefensePlatform:
		Facility.BonusMultiplier = 1.15f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 350);
		break;
	case EGuildFacilityType::ShipYard:
		Facility.BonusMultiplier = 1.10f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 500);
		break;
	case EGuildFacilityType::MarketTerminal:
		Facility.BonusMultiplier = 1.03f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 150);
		break;
	case EGuildFacilityType::Beacon:
		Facility.BonusMultiplier = 1.0f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 50);
		break;
	case EGuildFacilityType::Embassy:
		Facility.BonusMultiplier = 1.05f;
		Facility.DailyUpkeep.Add(EResourceType::OMEN, 200);
		break;
	}

	// Set upgrade cost (same as build cost for first upgrade)
	Facility.UpgradeCost = GetFacilityBuildCost(Type, GuildLevel);

	return Facility;
}

bool UGuildEconomyComponent::CanAfford(const FGuid& GuildID, const TMap<EResourceType, int64>& Cost) const
{
	if (!GuildManager) return false;

	for (const auto& Pair : Cost)
	{
		int64 Balance = GuildManager->GetBankBalance(GuildID, Pair.Key);
		if (Balance < Pair.Value)
		{
			return false;
		}
	}
	return true;
}

bool UGuildEconomyComponent::DeductCost(FGuildEconomyData& EconData, const TMap<EResourceType, int64>& Cost,
	const FString& PlayerID, const FString& PlayerName, const FString& Description)
{
	if (!GuildManager) return false;

	// Verify all resources available first
	if (!CanAfford(EconData.GuildID, Cost)) return false;

	// Deduct each resource
	for (const auto& Pair : Cost)
	{
		if (!GuildManager->WithdrawFromBank(EconData.GuildID, PlayerID, Pair.Key, Pair.Value))
		{
			// Partial failure - this shouldn't happen since we checked above
			UE_LOG(LogTemp, Error, TEXT("DeductCost: Partial deduction failure for resource %d"),
				static_cast<int32>(Pair.Key));
			return false;
		}

		int64 NewBalance = GuildManager->GetBankBalance(EconData.GuildID, Pair.Key);
		RecordTransaction(EconData, ETreasuryTransactionType::FacilityPurchase, Pair.Key,
			-Pair.Value, NewBalance, PlayerID, PlayerName, Description);

		EconData.PeriodExpenses += Pair.Value;
	}

	return true;
}

void UGuildEconomyComponent::TrimTransactionHistory(FGuildEconomyData& EconData)
{
	while (EconData.TransactionHistory.Num() > MaxTransactionHistory)
	{
		EconData.TransactionHistory.RemoveAt(0);
	}
}

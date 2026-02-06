// SocialContractSystem.h
// Player-to-player service contracts and escrow system
// Enables secure transactions for escort, crafting, transport, and other services

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataTable.h"
#include "OdysseyInventoryComponent.h"
#include "SocialContractSystem.generated.h"

// Forward declarations
class UOdysseyGuildManager;
class UReputationSystem;

/**
 * Contract service types
 */
UENUM(BlueprintType)
enum class EContractType : uint8
{
	Escort,             // Protection/escort services
	Transport,          // Cargo transport
	Crafting,           // Item crafting services
	Mining,             // Mining operations
	Combat,             // Combat assistance
	Exploration,        // Scouting/mapping
	Training,           // Skill training
	Repair,             // Ship/equipment repair
	Trade,              // Trading services
	Custom              // User-defined
};

/**
 * Contract status
 */
UENUM(BlueprintType)
enum class EContractStatus : uint8
{
	Draft,              // Being created
	Open,               // Available for acceptance
	Pending,            // Waiting for contractor acceptance
	Active,             // Work in progress
	AwaitingConfirmation, // Work done, awaiting client confirmation
	Disputed,           // Under dispute resolution
	Completed,          // Successfully finished
	Cancelled,          // Cancelled before completion
	Expired,            // Deadline passed
	Failed              // Failed to complete
};

/**
 * Escrow status
 */
UENUM(BlueprintType)
enum class EEscrowStatus : uint8
{
	Pending,            // Awaiting deposit
	Funded,             // Funds deposited
	Releasing,          // In process of release
	Released,           // Released to contractor
	Refunded,           // Returned to client
	Disputed            // Held for dispute resolution
};

/**
 * Rating category for feedback
 */
UENUM(BlueprintType)
enum class ERatingCategory : uint8
{
	Overall,
	Communication,
	Timeliness,
	Quality,
	Professionalism,
	Value
};

/**
 * Payment terms for contracts
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FContractPaymentTerms
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment Terms")
	TMap<EResourceType, int64> TotalPayment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment Terms")
	float UpfrontPercentage; // 0.0 - 1.0, portion paid upfront

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment Terms")
	float MilestonePercentage; // Portion paid at milestones

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment Terms")
	float CompletionPercentage; // Portion paid on completion

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment Terms")
	bool bUseEscrow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Payment Terms")
	float PlatformFeePercentage; // System fee

	FContractPaymentTerms()
		: UpfrontPercentage(0.0f)
		, MilestonePercentage(0.0f)
		, CompletionPercentage(1.0f)
		, bUseEscrow(true)
		, PlatformFeePercentage(0.05f) // 5% platform fee
	{
	}

	int64 GetTotalOMEN() const
	{
		const int64* Amount = TotalPayment.Find(EResourceType::OMEN);
		return Amount ? *Amount : 0;
	}

	int64 GetUpfrontAmount(EResourceType Type) const
	{
		const int64* Amount = TotalPayment.Find(Type);
		if (Amount)
		{
			return static_cast<int64>(*Amount * UpfrontPercentage);
		}
		return 0;
	}

	int64 GetCompletionAmount(EResourceType Type) const
	{
		const int64* Amount = TotalPayment.Find(Type);
		if (Amount)
		{
			return static_cast<int64>(*Amount * CompletionPercentage);
		}
		return 0;
	}
};

/**
 * Contract milestone
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FContractMilestone
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Milestone")
	FGuid MilestoneID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Milestone")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Milestone")
	int32 OrderIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Milestone")
	float PaymentPercentage; // Portion of total payment

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Milestone")
	bool bIsComplete;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Milestone")
	FDateTime CompletedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Milestone")
	bool bClientConfirmed;

	FContractMilestone()
		: Description(TEXT(""))
		, OrderIndex(0)
		, PaymentPercentage(0.0f)
		, bIsComplete(false)
		, bClientConfirmed(false)
	{
		MilestoneID = FGuid::NewGuid();
	}
};

/**
 * Escrow account for contract
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FContractEscrow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escrow")
	FGuid EscrowID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escrow")
	EEscrowStatus Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escrow")
	TMap<EResourceType, int64> HeldFunds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escrow")
	TMap<EResourceType, int64> ReleasedFunds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escrow")
	FDateTime CreatedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escrow")
	FDateTime LastUpdated;

	FContractEscrow()
		: Status(EEscrowStatus::Pending)
	{
		EscrowID = FGuid::NewGuid();
		CreatedAt = FDateTime::Now();
		LastUpdated = FDateTime::Now();
	}

	int64 GetHeldAmount(EResourceType Type) const
	{
		const int64* Amount = HeldFunds.Find(Type);
		return Amount ? *Amount : 0;
	}

	int64 GetTotalHeld() const
	{
		int64 Total = 0;
		for (const auto& Pair : HeldFunds)
		{
			Total += Pair.Value;
		}
		return Total;
	}
};

/**
 * Rating/feedback for a contract
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FContractRating
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Rating")
	FString RaterPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Rating")
	FString RatedPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Rating")
	bool bIsClientRating; // True if client rating contractor

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Rating")
	TMap<ERatingCategory, int32> Ratings; // 1-5 stars

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Rating")
	FString Review;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Rating")
	FDateTime RatingDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Rating")
	bool bIsPublic;

	FContractRating()
		: RaterPlayerID(TEXT(""))
		, RatedPlayerID(TEXT(""))
		, bIsClientRating(true)
		, Review(TEXT(""))
		, bIsPublic(true)
	{
		RatingDate = FDateTime::Now();
	}

	float GetAverageRating() const
	{
		if (Ratings.Num() == 0) return 0.0f;

		float Total = 0.0f;
		for (const auto& Pair : Ratings)
		{
			Total += static_cast<float>(Pair.Value);
		}
		return Total / static_cast<float>(Ratings.Num());
	}

	int32 GetOverallRating() const
	{
		const int32* Rating = Ratings.Find(ERatingCategory::Overall);
		return Rating ? *Rating : 0;
	}
};

/**
 * Dispute information
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FContractDispute
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	FGuid DisputeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	FString InitiatorPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	FString Reason;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	FString Evidence; // Description of evidence

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	FDateTime FiledAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	FString Resolution;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	float ClientRefundPercentage; // 0.0 - 1.0

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	bool bIsResolved;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Dispute")
	FDateTime ResolvedAt;

	FContractDispute()
		: InitiatorPlayerID(TEXT(""))
		, Reason(TEXT(""))
		, Evidence(TEXT(""))
		, Resolution(TEXT(""))
		, ClientRefundPercentage(0.0f)
		, bIsResolved(false)
	{
		DisputeID = FGuid::NewGuid();
		FiledAt = FDateTime::Now();
	}
};

/**
 * Contract message/communication
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FContractMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Message")
	FGuid MessageID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Message")
	FString SenderPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Message")
	FString SenderName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Message")
	FString Content;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Message")
	FDateTime Timestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Message")
	bool bIsSystemMessage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Message")
	bool bIsRead;

	FContractMessage()
		: SenderPlayerID(TEXT(""))
		, SenderName(TEXT(""))
		, Content(TEXT(""))
		, bIsSystemMessage(false)
		, bIsRead(false)
	{
		MessageID = FGuid::NewGuid();
		Timestamp = FDateTime::Now();
	}
};

/**
 * Complete social contract data
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FSocialContract
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FGuid ContractID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FString Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	EContractType ContractType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	EContractStatus Status;

	// Parties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FString ClientPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FString ClientName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FString ContractorPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FString ContractorName;

	// Timing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FDateTime CreatedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FDateTime AcceptedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FDateTime Deadline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FDateTime CompletedAt;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	float EstimatedHours;

	// Payment
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FContractPaymentTerms PaymentTerms;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FContractEscrow Escrow;

	// Progress
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	TArray<FContractMilestone> Milestones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	int32 CurrentMilestoneIndex;

	// Requirements
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	int32 MinContractorLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	float MinContractorRating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	int32 MinCompletedContracts;

	// Location/details
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FVector StartLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FVector EndLocation; // For transport/escort

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	TMap<FString, FString> CustomTerms; // Key-value custom requirements

	// Communication
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	TArray<FContractMessage> Messages;

	// Ratings and feedback
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FContractRating ClientRating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FContractRating ContractorRating;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	bool bClientRated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	bool bContractorRated;

	// Dispute
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FContractDispute Dispute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	bool bHasDispute;

	// Visibility
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	bool bIsPublic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract")
	FGuid RestrictedToGuildID; // If set, only guild members can accept

	FSocialContract()
		: Title(TEXT("New Contract"))
		, Description(TEXT(""))
		, ContractType(EContractType::Custom)
		, Status(EContractStatus::Draft)
		, ClientPlayerID(TEXT(""))
		, ClientName(TEXT(""))
		, ContractorPlayerID(TEXT(""))
		, ContractorName(TEXT(""))
		, EstimatedHours(1.0f)
		, CurrentMilestoneIndex(0)
		, MinContractorLevel(1)
		, MinContractorRating(0.0f)
		, MinCompletedContracts(0)
		, StartLocation(FVector::ZeroVector)
		, EndLocation(FVector::ZeroVector)
		, bClientRated(false)
		, bContractorRated(false)
		, bHasDispute(false)
		, bIsPublic(true)
	{
		ContractID = FGuid::NewGuid();
		CreatedAt = FDateTime::Now();
		Deadline = FDateTime::Now() + FTimespan::FromDays(7);
	}

	bool IsExpired() const
	{
		return FDateTime::Now() > Deadline &&
			Status != EContractStatus::Completed &&
			Status != EContractStatus::Cancelled;
	}

	float GetProgress() const
	{
		if (Milestones.Num() == 0) return 0.0f;

		int32 Completed = 0;
		for (const FContractMilestone& M : Milestones)
		{
			if (M.bIsComplete && M.bClientConfirmed) Completed++;
		}
		return static_cast<float>(Completed) / static_cast<float>(Milestones.Num());
	}

	bool CanBeAcceptedBy(const FString& PlayerID) const
	{
		// Cannot accept own contract
		if (ClientPlayerID == PlayerID) return false;
		// Cannot accept if already has contractor
		if (!ContractorPlayerID.IsEmpty()) return false;
		// Must be open
		if (Status != EContractStatus::Open) return false;
		return true;
	}
};

/**
 * Contract offer (for bidding system)
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FContractOffer
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	FGuid OfferID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	FGuid ContractID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	FString OffererPlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	FString OffererName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	TMap<EResourceType, int64> ProposedPayment;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	FString Pitch; // Why they should be chosen

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	float EstimatedCompletionHours;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	FDateTime OfferDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	FDateTime ExpirationDate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	bool bIsAccepted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Contract Offer")
	bool bIsRejected;

	FContractOffer()
		: OffererPlayerID(TEXT(""))
		, OffererName(TEXT(""))
		, Pitch(TEXT(""))
		, EstimatedCompletionHours(1.0f)
		, bIsAccepted(false)
		, bIsRejected(false)
	{
		OfferID = FGuid::NewGuid();
		OfferDate = FDateTime::Now();
		ExpirationDate = FDateTime::Now() + FTimespan::FromDays(3);
	}
};

/**
 * Player service profile
 */
USTRUCT(BlueprintType)
struct ODYSSEY_API FServiceProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	FString PlayerID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	FString PlayerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	TArray<EContractType> OfferedServices;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	FString Bio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	TMap<ERatingCategory, float> AverageRatings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	int32 TotalContractsCompleted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	int32 TotalContractsFailed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	float CompletionRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	int64 TotalEarnings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	FDateTime ProfileCreated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Service Profile")
	bool bIsAvailable;

	FServiceProfile()
		: PlayerID(TEXT(""))
		, PlayerName(TEXT(""))
		, Bio(TEXT(""))
		, TotalContractsCompleted(0)
		, TotalContractsFailed(0)
		, CompletionRate(1.0f)
		, TotalEarnings(0)
		, bIsAvailable(true)
	{
		ProfileCreated = FDateTime::Now();
	}

	float GetOverallRating() const
	{
		const float* Rating = AverageRatings.Find(ERatingCategory::Overall);
		return Rating ? *Rating : 0.0f;
	}
};

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnContractCreated, const FGuid&, ContractID, const FString&, ClientID, const FString&, Title);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnContractAccepted, const FGuid&, ContractID, const FString&, ContractorID, const FString&, ContractorName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnContractStatusChanged, const FGuid&, ContractID, EContractStatus, OldStatus, EContractStatus, NewStatus);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnContractMilestoneCompleted, const FGuid&, ContractID, int32, MilestoneIndex, const FString&, Description);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnContractCompleted, const FGuid&, ContractID, const FString&, ClientID, const FString&, ContractorID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPaymentReleased, const FGuid&, ContractID, const FString&, RecipientID, int64, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDisputeFiled, const FGuid&, ContractID, const FString&, InitiatorID, const FString&, Reason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnRatingSubmitted, const FGuid&, ContractID, const FString&, RaterID, const FString&, RatedID, int32, Rating);

/**
 * USocialContractSystem
 * 
 * Manages player-to-player service contracts.
 * Provides escrow, rating, and dispute resolution systems.
 */
UCLASS(Blueprintable, BlueprintType)
class ODYSSEY_API USocialContractSystem : public UObject
{
	GENERATED_BODY()

public:
	USocialContractSystem();

	// Initialize the system
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	void Initialize(UOdysseyGuildManager* InGuildManager, UReputationSystem* InReputationSystem);

	// ==================== Contract Lifecycle ====================

	/** Create a new contract as client */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	FGuid CreateContract(const FString& ClientPlayerID, const FString& ClientName,
		const FString& Title, const FString& Description, EContractType Type,
		const FContractPaymentTerms& Payment);

	/** Post contract to public market */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool PostContract(const FGuid& ContractID, const FString& ClientPlayerID);

	/** Accept a contract as contractor */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool AcceptContract(const FGuid& ContractID, const FString& ContractorPlayerID,
		const FString& ContractorName);

	/** Cancel a contract */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool CancelContract(const FGuid& ContractID, const FString& PlayerID, const FString& Reason);

	/** Get contract data */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool GetContractData(const FGuid& ContractID, FSocialContract& OutContract) const;

	// ==================== Milestone Management ====================

	/** Add milestone to contract (client only, before start) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool AddMilestone(const FGuid& ContractID, const FString& ClientPlayerID,
		const FContractMilestone& Milestone);

	/** Mark milestone as complete (contractor) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool CompleteMilestone(const FGuid& ContractID, const FString& ContractorPlayerID,
		int32 MilestoneIndex);

	/** Confirm milestone completion (client) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool ConfirmMilestone(const FGuid& ContractID, const FString& ClientPlayerID,
		int32 MilestoneIndex);

	/** Reject milestone (client) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool RejectMilestone(const FGuid& ContractID, const FString& ClientPlayerID,
		int32 MilestoneIndex, const FString& Reason);

	// ==================== Escrow System ====================

	/** Fund the escrow (client deposits payment) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool FundEscrow(const FGuid& ContractID, const FString& ClientPlayerID);

	/** Release escrow payment (automatic on milestone/completion) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool ReleaseEscrow(const FGuid& ContractID, float Percentage);

	/** Refund escrow to client */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool RefundEscrow(const FGuid& ContractID);

	/** Get escrow status */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	FContractEscrow GetEscrowStatus(const FGuid& ContractID) const;

	// ==================== Ratings & Feedback ====================

	/** Submit rating for the other party */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool SubmitRating(const FGuid& ContractID, const FString& RaterPlayerID,
		const TMap<ERatingCategory, int32>& Ratings, const FString& Review, bool bIsPublic);

	/** Get a player's service profile */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool GetServiceProfile(const FString& PlayerID, FServiceProfile& OutProfile) const;

	/** Update own service profile */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool UpdateServiceProfile(const FString& PlayerID, const TArray<EContractType>& Services,
		const FString& Bio, bool bAvailable);

	/** Get ratings for a player */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	TArray<FContractRating> GetPlayerRatings(const FString& PlayerID, int32 MaxCount = 20) const;

	// ==================== Disputes ====================

	/** File a dispute */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool FileDispute(const FGuid& ContractID, const FString& InitiatorPlayerID,
		const FString& Reason, const FString& Evidence);

	/** Resolve a dispute (system/moderator) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool ResolveDispute(const FGuid& ContractID, const FString& Resolution,
		float ClientRefundPercentage);

	/** Get dispute details */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool GetDisputeDetails(const FGuid& ContractID, FContractDispute& OutDispute) const;

	// ==================== Communication ====================

	/** Send message in contract */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool SendMessage(const FGuid& ContractID, const FString& SenderPlayerID,
		const FString& SenderName, const FString& Content);

	/** Get contract messages */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	TArray<FContractMessage> GetMessages(const FGuid& ContractID, int32 MaxCount = 50) const;

	/** Mark messages as read */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	void MarkMessagesRead(const FGuid& ContractID, const FString& ReaderPlayerID);

	// ==================== Bidding System ====================

	/** Submit an offer/bid for a contract */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	FGuid SubmitOffer(const FGuid& ContractID, const FString& OffererPlayerID,
		const FString& OffererName, const TMap<EResourceType, int64>& ProposedPayment,
		const FString& Pitch, float EstimatedHours);

	/** Accept an offer (client) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool AcceptOffer(const FGuid& ContractID, const FString& ClientPlayerID, const FGuid& OfferID);

	/** Reject an offer (client) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	bool RejectOffer(const FGuid& ContractID, const FString& ClientPlayerID, const FGuid& OfferID);

	/** Get offers for a contract */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	TArray<FContractOffer> GetContractOffers(const FGuid& ContractID) const;

	// ==================== Search & Discovery ====================

	/** Search for available contracts */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	TArray<FSocialContract> SearchContracts(const FString& SearchQuery, EContractType TypeFilter,
		bool bOpenOnly = true, int32 MaxResults = 20) const;

	/** Get contracts for a player (as client or contractor) */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	TArray<FSocialContract> GetPlayerContracts(const FString& PlayerID, bool bActiveOnly = false) const;

	/** Find service providers */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	TArray<FServiceProfile> FindServiceProviders(EContractType ServiceType, float MinRating = 0.0f,
		int32 MaxResults = 20) const;

	// ==================== Statistics ====================

	/** Get player contract statistics */
	UFUNCTION(BlueprintCallable, Category = "Social Contract")
	void GetPlayerStats(const FString& PlayerID, int32& OutCompleted, int32& OutFailed,
		int32& OutActive, int64& OutTotalEarned) const;

	// ==================== Events ====================

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnContractCreated OnContractCreated;

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnContractAccepted OnContractAccepted;

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnContractStatusChanged OnContractStatusChanged;

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnContractMilestoneCompleted OnContractMilestoneCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnContractCompleted OnContractCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnPaymentReleased OnPaymentReleased;

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnDisputeFiled OnDisputeFiled;

	UPROPERTY(BlueprintAssignable, Category = "Social Contract Events")
	FOnRatingSubmitted OnRatingSubmitted;

protected:
	// External system references
	UPROPERTY()
	UOdysseyGuildManager* GuildManager;

	UPROPERTY()
	UReputationSystem* ReputationSystem;

	// All contracts
	UPROPERTY()
	TMap<FGuid, FSocialContract> Contracts;

	// Contract offers
	UPROPERTY()
	TMap<FGuid, TArray<FContractOffer>> ContractOffers;

	// Service profiles
	UPROPERTY()
	TMap<FString, FServiceProfile> ServiceProfiles;

	// Player ratings history
	UPROPERTY()
	TMap<FString, TArray<FContractRating>> PlayerRatings;

	// Thread safety
	mutable FCriticalSection ContractLock;

	// Internal helpers
	void ChangeContractStatus(FSocialContract& Contract, EContractStatus NewStatus);
	void AddSystemMessage(FSocialContract& Contract, const FString& Content);
	void UpdateServiceProfileStats(const FString& PlayerID, bool bCompleted, int64 Earned);
	void RecalculatePlayerRatings(const FString& PlayerID);
	bool CheckContractCompletion(FSocialContract& Contract);
	bool MeetsContractorRequirements(const FSocialContract& Contract, const FString& PlayerID) const;
	void ProcessExpiredContracts();

	FTimerHandle ExpirationTimerHandle;
};

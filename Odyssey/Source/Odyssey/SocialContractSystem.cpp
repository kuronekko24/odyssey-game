// SocialContractSystem.cpp
// Implementation of player-to-player contract system

#include "SocialContractSystem.h"
#include "OdysseyGuildManager.h"
#include "Social/ReputationSystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

USocialContractSystem::USocialContractSystem()
	: GuildManager(nullptr)
	, ReputationSystem(nullptr)
{
}

void USocialContractSystem::Initialize(UOdysseyGuildManager* InGuildManager, UReputationSystem* InReputationSystem)
{
	GuildManager = InGuildManager;
	ReputationSystem = InReputationSystem;

	// Set up expiration check timer
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ExpirationTimerHandle,
			this,
			&USocialContractSystem::ProcessExpiredContracts,
			3600.0f, // Check every hour
			true
		);
	}
}

// ==================== Contract Lifecycle ====================

FGuid USocialContractSystem::CreateContract(const FString& ClientPlayerID, const FString& ClientName,
	const FString& Title, const FString& Description, EContractType Type,
	const FContractPaymentTerms& Payment)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract NewContract;
	NewContract.Title = Title;
	NewContract.Description = Description;
	NewContract.ContractType = Type;
	NewContract.ClientPlayerID = ClientPlayerID;
	NewContract.ClientName = ClientName;
	NewContract.PaymentTerms = Payment;
	NewContract.Status = EContractStatus::Draft;

	// Initialize escrow
	NewContract.Escrow.Status = EEscrowStatus::Pending;

	FGuid ContractID = NewContract.ContractID;
	Contracts.Add(ContractID, NewContract);

	// Ensure client has a service profile
	if (!ServiceProfiles.Contains(ClientPlayerID))
	{
		FServiceProfile Profile;
		Profile.PlayerID = ClientPlayerID;
		Profile.PlayerName = ClientName;
		ServiceProfiles.Add(ClientPlayerID, Profile);
	}

	UE_LOG(LogTemp, Log, TEXT("Contract '%s' created by %s"), *Title, *ClientPlayerID);

	return ContractID;
}

bool USocialContractSystem::PostContract(const FGuid& ContractID, const FString& ClientPlayerID)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->ClientPlayerID != ClientPlayerID)
	{
		return false;
	}

	if (Contract->Status != EContractStatus::Draft)
	{
		return false;
	}

	ChangeContractStatus(*Contract, EContractStatus::Open);
	Contract->bIsPublic = true;

	OnContractCreated.Broadcast(ContractID, ClientPlayerID, Contract->Title);

	return true;
}

bool USocialContractSystem::AcceptContract(const FGuid& ContractID, const FString& ContractorPlayerID,
	const FString& ContractorName)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (!Contract->CanBeAcceptedBy(ContractorPlayerID))
	{
		return false;
	}

	// Check contractor requirements
	if (!MeetsContractorRequirements(*Contract, ContractorPlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("AcceptContract: Contractor does not meet requirements"));
		return false;
	}

	Contract->ContractorPlayerID = ContractorPlayerID;
	Contract->ContractorName = ContractorName;
	Contract->AcceptedAt = FDateTime::Now();

	// Ensure contractor has a service profile
	if (!ServiceProfiles.Contains(ContractorPlayerID))
	{
		FServiceProfile Profile;
		Profile.PlayerID = ContractorPlayerID;
		Profile.PlayerName = ContractorName;
		ServiceProfiles.Add(ContractorPlayerID, Profile);
	}

	// If escrow is required, move to pending status until funded
	if (Contract->PaymentTerms.bUseEscrow)
	{
		ChangeContractStatus(*Contract, EContractStatus::Pending);
		AddSystemMessage(*Contract, FString::Printf(TEXT("%s accepted the contract. Awaiting escrow funding."),
			*ContractorName));
	}
	else
	{
		ChangeContractStatus(*Contract, EContractStatus::Active);
		AddSystemMessage(*Contract, FString::Printf(TEXT("%s accepted the contract. Work can begin."),
			*ContractorName));
	}

	OnContractAccepted.Broadcast(ContractID, ContractorPlayerID, ContractorName);

	return true;
}

bool USocialContractSystem::CancelContract(const FGuid& ContractID, const FString& PlayerID, const FString& Reason)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	// Only client or contractor can cancel
	if (Contract->ClientPlayerID != PlayerID && Contract->ContractorPlayerID != PlayerID)
	{
		return false;
	}

	// Cannot cancel completed contracts
	if (Contract->Status == EContractStatus::Completed)
	{
		return false;
	}

	// Handle escrow refund
	if (Contract->Escrow.Status == EEscrowStatus::Funded)
	{
		// If contractor cancels, full refund to client
		// If client cancels after work started, partial compensation may apply
		if (PlayerID == Contract->ContractorPlayerID || Contract->Status == EContractStatus::Open)
		{
			RefundEscrow(ContractID);
		}
		else
		{
			// Client cancelling active contract - contractor may get partial payment
			float Progress = Contract->GetProgress();
			if (Progress > 0.0f)
			{
				ReleaseEscrow(ContractID, Progress);
			}
			RefundEscrow(ContractID);
		}
	}

	AddSystemMessage(*Contract, FString::Printf(TEXT("Contract cancelled by %s. Reason: %s"),
		PlayerID == Contract->ClientPlayerID ? TEXT("client") : TEXT("contractor"), *Reason));

	ChangeContractStatus(*Contract, EContractStatus::Cancelled);

	return true;
}

bool USocialContractSystem::GetContractData(const FGuid& ContractID, FSocialContract& OutContract) const
{
	FScopeLock Lock(&ContractLock);

	const FSocialContract* Contract = Contracts.Find(ContractID);
	if (Contract)
	{
		OutContract = *Contract;
		return true;
	}
	return false;
}

// ==================== Milestone Management ====================

bool USocialContractSystem::AddMilestone(const FGuid& ContractID, const FString& ClientPlayerID,
	const FContractMilestone& Milestone)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->ClientPlayerID != ClientPlayerID)
	{
		return false;
	}

	// Can only add milestones before contract is active
	if (Contract->Status != EContractStatus::Draft && Contract->Status != EContractStatus::Open)
	{
		return false;
	}

	FContractMilestone NewMilestone = Milestone;
	NewMilestone.MilestoneID = FGuid::NewGuid();
	NewMilestone.OrderIndex = Contract->Milestones.Num();
	NewMilestone.bIsComplete = false;
	NewMilestone.bClientConfirmed = false;

	Contract->Milestones.Add(NewMilestone);

	return true;
}

bool USocialContractSystem::CompleteMilestone(const FGuid& ContractID, const FString& ContractorPlayerID,
	int32 MilestoneIndex)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->ContractorPlayerID != ContractorPlayerID)
	{
		return false;
	}

	if (Contract->Status != EContractStatus::Active)
	{
		return false;
	}

	if (MilestoneIndex < 0 || MilestoneIndex >= Contract->Milestones.Num())
	{
		return false;
	}

	FContractMilestone& Milestone = Contract->Milestones[MilestoneIndex];
	if (Milestone.bIsComplete)
	{
		return false;
	}

	Milestone.bIsComplete = true;
	Milestone.CompletedAt = FDateTime::Now();

	AddSystemMessage(*Contract, FString::Printf(TEXT("Milestone '%s' marked as complete by contractor."),
		*Milestone.Description));

	// Move to awaiting confirmation
	if (!Milestone.bClientConfirmed)
	{
		ChangeContractStatus(*Contract, EContractStatus::AwaitingConfirmation);
	}

	return true;
}

bool USocialContractSystem::ConfirmMilestone(const FGuid& ContractID, const FString& ClientPlayerID,
	int32 MilestoneIndex)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->ClientPlayerID != ClientPlayerID)
	{
		return false;
	}

	if (MilestoneIndex < 0 || MilestoneIndex >= Contract->Milestones.Num())
	{
		return false;
	}

	FContractMilestone& Milestone = Contract->Milestones[MilestoneIndex];
	if (!Milestone.bIsComplete || Milestone.bClientConfirmed)
	{
		return false;
	}

	Milestone.bClientConfirmed = true;

	// Release milestone payment
	if (Contract->PaymentTerms.bUseEscrow && Milestone.PaymentPercentage > 0.0f)
	{
		ReleaseEscrow(ContractID, Milestone.PaymentPercentage);
	}

	AddSystemMessage(*Contract, FString::Printf(TEXT("Milestone '%s' confirmed by client."),
		*Milestone.Description));

	OnContractMilestoneCompleted.Broadcast(ContractID, MilestoneIndex, Milestone.Description);

	// Check if all milestones are complete
	CheckContractCompletion(*Contract);

	return true;
}

bool USocialContractSystem::RejectMilestone(const FGuid& ContractID, const FString& ClientPlayerID,
	int32 MilestoneIndex, const FString& Reason)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->ClientPlayerID != ClientPlayerID)
	{
		return false;
	}

	if (MilestoneIndex < 0 || MilestoneIndex >= Contract->Milestones.Num())
	{
		return false;
	}

	FContractMilestone& Milestone = Contract->Milestones[MilestoneIndex];
	if (!Milestone.bIsComplete || Milestone.bClientConfirmed)
	{
		return false;
	}

	// Mark milestone as not complete (needs rework)
	Milestone.bIsComplete = false;

	AddSystemMessage(*Contract, FString::Printf(TEXT("Milestone '%s' rejected by client. Reason: %s"),
		*Milestone.Description, *Reason));

	// Return to active status
	ChangeContractStatus(*Contract, EContractStatus::Active);

	return true;
}

// ==================== Escrow System ====================

bool USocialContractSystem::FundEscrow(const FGuid& ContractID, const FString& ClientPlayerID)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->ClientPlayerID != ClientPlayerID)
	{
		return false;
	}

	if (Contract->Escrow.Status != EEscrowStatus::Pending)
	{
		return false;
	}

	// TODO: Actually deduct from player inventory
	// For now, just mark as funded
	Contract->Escrow.HeldFunds = Contract->PaymentTerms.TotalPayment;
	Contract->Escrow.Status = EEscrowStatus::Funded;
	Contract->Escrow.LastUpdated = FDateTime::Now();

	AddSystemMessage(*Contract, TEXT("Escrow funded. Contract is now active."));

	// If contract was pending, activate it
	if (Contract->Status == EContractStatus::Pending)
	{
		ChangeContractStatus(*Contract, EContractStatus::Active);
	}

	return true;
}

bool USocialContractSystem::ReleaseEscrow(const FGuid& ContractID, float Percentage)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->Escrow.Status != EEscrowStatus::Funded)
	{
		return false;
	}

	Percentage = FMath::Clamp(Percentage, 0.0f, 1.0f);

	int64 TotalReleased = 0;

	for (auto& HeldPair : Contract->Escrow.HeldFunds)
	{
		int64 ReleaseAmount = static_cast<int64>(HeldPair.Value * Percentage);
		if (ReleaseAmount > 0)
		{
			HeldPair.Value -= ReleaseAmount;

			if (!Contract->Escrow.ReleasedFunds.Contains(HeldPair.Key))
			{
				Contract->Escrow.ReleasedFunds.Add(HeldPair.Key, 0);
			}
			Contract->Escrow.ReleasedFunds[HeldPair.Key] += ReleaseAmount;

			if (HeldPair.Key == EResourceType::OMEN)
			{
				TotalReleased = ReleaseAmount;
			}
		}
	}

	Contract->Escrow.LastUpdated = FDateTime::Now();

	// TODO: Actually transfer to contractor inventory

	if (TotalReleased > 0)
	{
		OnPaymentReleased.Broadcast(ContractID, Contract->ContractorPlayerID, TotalReleased);
	}

	// Check if all funds released
	if (Contract->Escrow.GetTotalHeld() <= 0)
	{
		Contract->Escrow.Status = EEscrowStatus::Released;
	}
	else
	{
		Contract->Escrow.Status = EEscrowStatus::Releasing;
	}

	return true;
}

bool USocialContractSystem::RefundEscrow(const FGuid& ContractID)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (Contract->Escrow.Status != EEscrowStatus::Funded &&
		Contract->Escrow.Status != EEscrowStatus::Releasing)
	{
		return false;
	}

	// TODO: Actually transfer remaining funds back to client

	int64 RefundedOMEN = Contract->Escrow.GetHeldAmount(EResourceType::OMEN);

	Contract->Escrow.HeldFunds.Empty();
	Contract->Escrow.Status = EEscrowStatus::Refunded;
	Contract->Escrow.LastUpdated = FDateTime::Now();

	AddSystemMessage(*Contract, FString::Printf(TEXT("Escrow refunded. %lld OMEN returned to client."),
		RefundedOMEN));

	return true;
}

FContractEscrow USocialContractSystem::GetEscrowStatus(const FGuid& ContractID) const
{
	FScopeLock Lock(&ContractLock);

	const FSocialContract* Contract = Contracts.Find(ContractID);
	if (Contract)
	{
		return Contract->Escrow;
	}
	return FContractEscrow();
}

// ==================== Ratings & Feedback ====================

bool USocialContractSystem::SubmitRating(const FGuid& ContractID, const FString& RaterPlayerID,
	const TMap<ERatingCategory, int32>& Ratings, const FString& Review, bool bIsPublic)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	// Must be completed
	if (Contract->Status != EContractStatus::Completed)
	{
		return false;
	}

	// Determine who is being rated
	bool bIsClientRating = (RaterPlayerID == Contract->ClientPlayerID);
	bool bIsContractorRating = (RaterPlayerID == Contract->ContractorPlayerID);

	if (!bIsClientRating && !bIsContractorRating)
	{
		return false;
	}

	// Check if already rated
	if (bIsClientRating && Contract->bClientRated)
	{
		return false;
	}
	if (bIsContractorRating && Contract->bContractorRated)
	{
		return false;
	}

	FContractRating NewRating;
	NewRating.RaterPlayerID = RaterPlayerID;
	NewRating.RatedPlayerID = bIsClientRating ? Contract->ContractorPlayerID : Contract->ClientPlayerID;
	NewRating.bIsClientRating = bIsClientRating;
	NewRating.Ratings = Ratings;
	NewRating.Review = Review;
	NewRating.bIsPublic = bIsPublic;

	// Validate ratings are 1-5
	for (auto& RatingPair : NewRating.Ratings)
	{
		RatingPair.Value = FMath::Clamp(RatingPair.Value, 1, 5);
	}

	// Store rating
	if (bIsClientRating)
	{
		Contract->ClientRating = NewRating;
		Contract->bClientRated = true;
	}
	else
	{
		Contract->ContractorRating = NewRating;
		Contract->bContractorRated = true;
	}

	// Add to player's rating history
	if (!PlayerRatings.Contains(NewRating.RatedPlayerID))
	{
		PlayerRatings.Add(NewRating.RatedPlayerID, TArray<FContractRating>());
	}
	PlayerRatings[NewRating.RatedPlayerID].Add(NewRating);

	// Recalculate player's average ratings
	RecalculatePlayerRatings(NewRating.RatedPlayerID);

	OnRatingSubmitted.Broadcast(ContractID, RaterPlayerID, NewRating.RatedPlayerID,
		NewRating.GetOverallRating());

	return true;
}

bool USocialContractSystem::GetServiceProfile(const FString& PlayerID, FServiceProfile& OutProfile) const
{
	FScopeLock Lock(&ContractLock);

	const FServiceProfile* Profile = ServiceProfiles.Find(PlayerID);
	if (Profile)
	{
		OutProfile = *Profile;
		return true;
	}
	return false;
}

bool USocialContractSystem::UpdateServiceProfile(const FString& PlayerID, const TArray<EContractType>& Services,
	const FString& Bio, bool bAvailable)
{
	FScopeLock Lock(&ContractLock);

	FServiceProfile* Profile = ServiceProfiles.Find(PlayerID);
	if (!Profile)
	{
		// Create new profile
		FServiceProfile NewProfile;
		NewProfile.PlayerID = PlayerID;
		ServiceProfiles.Add(PlayerID, NewProfile);
		Profile = ServiceProfiles.Find(PlayerID);
	}

	if (Profile)
	{
		Profile->OfferedServices = Services;
		Profile->Bio = Bio;
		Profile->bIsAvailable = bAvailable;
		return true;
	}

	return false;
}

TArray<FContractRating> USocialContractSystem::GetPlayerRatings(const FString& PlayerID, int32 MaxCount) const
{
	FScopeLock Lock(&ContractLock);

	TArray<FContractRating> Result;
	const TArray<FContractRating>* Ratings = PlayerRatings.Find(PlayerID);
	if (Ratings)
	{
		// Return most recent ratings
		int32 StartIndex = FMath::Max(0, Ratings->Num() - MaxCount);
		for (int32 i = Ratings->Num() - 1; i >= StartIndex; --i)
		{
			if ((*Ratings)[i].bIsPublic)
			{
				Result.Add((*Ratings)[i]);
			}
		}
	}
	return Result;
}

// ==================== Disputes ====================

bool USocialContractSystem::FileDispute(const FGuid& ContractID, const FString& InitiatorPlayerID,
	const FString& Reason, const FString& Evidence)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	// Must be party to contract
	if (Contract->ClientPlayerID != InitiatorPlayerID &&
		Contract->ContractorPlayerID != InitiatorPlayerID)
	{
		return false;
	}

	// Cannot dispute already resolved contracts
	if (Contract->Status == EContractStatus::Completed || Contract->Status == EContractStatus::Cancelled)
	{
		return false;
	}

	// Cannot file multiple disputes
	if (Contract->bHasDispute)
	{
		return false;
	}

	Contract->Dispute.InitiatorPlayerID = InitiatorPlayerID;
	Contract->Dispute.Reason = Reason;
	Contract->Dispute.Evidence = Evidence;
	Contract->Dispute.FiledAt = FDateTime::Now();
	Contract->bHasDispute = true;

	// Freeze escrow
	if (Contract->Escrow.Status == EEscrowStatus::Funded ||
		Contract->Escrow.Status == EEscrowStatus::Releasing)
	{
		Contract->Escrow.Status = EEscrowStatus::Disputed;
	}

	ChangeContractStatus(*Contract, EContractStatus::Disputed);

	AddSystemMessage(*Contract, FString::Printf(TEXT("Dispute filed by %s. Reason: %s"),
		InitiatorPlayerID == Contract->ClientPlayerID ? TEXT("client") : TEXT("contractor"), *Reason));

	OnDisputeFiled.Broadcast(ContractID, InitiatorPlayerID, Reason);

	return true;
}

bool USocialContractSystem::ResolveDispute(const FGuid& ContractID, const FString& Resolution,
	float ClientRefundPercentage)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	if (!Contract->bHasDispute || Contract->Dispute.bIsResolved)
	{
		return false;
	}

	Contract->Dispute.Resolution = Resolution;
	Contract->Dispute.ClientRefundPercentage = FMath::Clamp(ClientRefundPercentage, 0.0f, 1.0f);
	Contract->Dispute.bIsResolved = true;
	Contract->Dispute.ResolvedAt = FDateTime::Now();

	// Distribute escrow based on resolution
	float ContractorPercentage = 1.0f - ClientRefundPercentage;
	if (ContractorPercentage > 0.0f)
	{
		Contract->Escrow.Status = EEscrowStatus::Funded;
		ReleaseEscrow(ContractID, ContractorPercentage);
	}
	if (ClientRefundPercentage > 0.0f)
	{
		Contract->Escrow.Status = EEscrowStatus::Funded;
		RefundEscrow(ContractID);
	}

	AddSystemMessage(*Contract, FString::Printf(TEXT("Dispute resolved. Resolution: %s. Client refund: %.0f%%"),
		*Resolution, ClientRefundPercentage * 100.0f));

	// Mark contract as complete or failed based on outcome
	if (ClientRefundPercentage >= 0.5f)
	{
		ChangeContractStatus(*Contract, EContractStatus::Failed);
		UpdateServiceProfileStats(Contract->ContractorPlayerID, false, 0);
	}
	else
	{
		ChangeContractStatus(*Contract, EContractStatus::Completed);
		int64 Earned = static_cast<int64>(Contract->PaymentTerms.GetTotalOMEN() * ContractorPercentage);
		UpdateServiceProfileStats(Contract->ContractorPlayerID, true, Earned);
	}

	return true;
}

bool USocialContractSystem::GetDisputeDetails(const FGuid& ContractID, FContractDispute& OutDispute) const
{
	FScopeLock Lock(&ContractLock);

	const FSocialContract* Contract = Contracts.Find(ContractID);
	if (Contract && Contract->bHasDispute)
	{
		OutDispute = Contract->Dispute;
		return true;
	}
	return false;
}

// ==================== Communication ====================

bool USocialContractSystem::SendMessage(const FGuid& ContractID, const FString& SenderPlayerID,
	const FString& SenderName, const FString& Content)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return false;
	}

	// Must be party to contract
	if (Contract->ClientPlayerID != SenderPlayerID &&
		Contract->ContractorPlayerID != SenderPlayerID)
	{
		return false;
	}

	FContractMessage Message;
	Message.SenderPlayerID = SenderPlayerID;
	Message.SenderName = SenderName;
	Message.Content = Content;
	Message.bIsSystemMessage = false;

	Contract->Messages.Add(Message);

	return true;
}

TArray<FContractMessage> USocialContractSystem::GetMessages(const FGuid& ContractID, int32 MaxCount) const
{
	FScopeLock Lock(&ContractLock);

	TArray<FContractMessage> Result;
	const FSocialContract* Contract = Contracts.Find(ContractID);
	if (Contract)
	{
		int32 StartIndex = FMath::Max(0, Contract->Messages.Num() - MaxCount);
		for (int32 i = StartIndex; i < Contract->Messages.Num(); ++i)
		{
			Result.Add(Contract->Messages[i]);
		}
	}
	return Result;
}

void USocialContractSystem::MarkMessagesRead(const FGuid& ContractID, const FString& ReaderPlayerID)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return;
	}

	for (FContractMessage& Msg : Contract->Messages)
	{
		if (Msg.SenderPlayerID != ReaderPlayerID)
		{
			Msg.bIsRead = true;
		}
	}
}

// ==================== Bidding System ====================

FGuid USocialContractSystem::SubmitOffer(const FGuid& ContractID, const FString& OffererPlayerID,
	const FString& OffererName, const TMap<EResourceType, int64>& ProposedPayment,
	const FString& Pitch, float EstimatedHours)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract)
	{
		return FGuid();
	}

	if (Contract->Status != EContractStatus::Open)
	{
		return FGuid();
	}

	// Cannot bid on own contract
	if (Contract->ClientPlayerID == OffererPlayerID)
	{
		return FGuid();
	}

	// Check requirements
	if (!MeetsContractorRequirements(*Contract, OffererPlayerID))
	{
		return FGuid();
	}

	FContractOffer Offer;
	Offer.ContractID = ContractID;
	Offer.OffererPlayerID = OffererPlayerID;
	Offer.OffererName = OffererName;
	Offer.ProposedPayment = ProposedPayment;
	Offer.Pitch = Pitch;
	Offer.EstimatedCompletionHours = EstimatedHours;

	if (!ContractOffers.Contains(ContractID))
	{
		ContractOffers.Add(ContractID, TArray<FContractOffer>());
	}
	ContractOffers[ContractID].Add(Offer);

	return Offer.OfferID;
}

bool USocialContractSystem::AcceptOffer(const FGuid& ContractID, const FString& ClientPlayerID, const FGuid& OfferID)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract || Contract->ClientPlayerID != ClientPlayerID)
	{
		return false;
	}

	TArray<FContractOffer>* Offers = ContractOffers.Find(ContractID);
	if (!Offers)
	{
		return false;
	}

	for (FContractOffer& Offer : *Offers)
	{
		if (Offer.OfferID == OfferID && !Offer.bIsRejected)
		{
			Offer.bIsAccepted = true;

			// Update contract payment terms if offer differs
			Contract->PaymentTerms.TotalPayment = Offer.ProposedPayment;
			Contract->EstimatedHours = Offer.EstimatedCompletionHours;

			// Accept the contract
			return AcceptContract(ContractID, Offer.OffererPlayerID, Offer.OffererName);
		}
	}

	return false;
}

bool USocialContractSystem::RejectOffer(const FGuid& ContractID, const FString& ClientPlayerID, const FGuid& OfferID)
{
	FScopeLock Lock(&ContractLock);

	FSocialContract* Contract = Contracts.Find(ContractID);
	if (!Contract || Contract->ClientPlayerID != ClientPlayerID)
	{
		return false;
	}

	TArray<FContractOffer>* Offers = ContractOffers.Find(ContractID);
	if (!Offers)
	{
		return false;
	}

	for (FContractOffer& Offer : *Offers)
	{
		if (Offer.OfferID == OfferID)
		{
			Offer.bIsRejected = true;
			return true;
		}
	}

	return false;
}

TArray<FContractOffer> USocialContractSystem::GetContractOffers(const FGuid& ContractID) const
{
	FScopeLock Lock(&ContractLock);

	const TArray<FContractOffer>* Offers = ContractOffers.Find(ContractID);
	if (Offers)
	{
		return *Offers;
	}
	return TArray<FContractOffer>();
}

// ==================== Search & Discovery ====================

TArray<FSocialContract> USocialContractSystem::SearchContracts(const FString& SearchQuery,
	EContractType TypeFilter, bool bOpenOnly, int32 MaxResults) const
{
	FScopeLock Lock(&ContractLock);

	TArray<FSocialContract> Results;

	for (const auto& Pair : Contracts)
	{
		const FSocialContract& Contract = Pair.Value;

		if (!Contract.bIsPublic)
		{
			continue;
		}

		if (bOpenOnly && Contract.Status != EContractStatus::Open)
		{
			continue;
		}

		if (TypeFilter != EContractType::Custom && Contract.ContractType != TypeFilter)
		{
			continue;
		}

		if (!SearchQuery.IsEmpty())
		{
			if (!Contract.Title.Contains(SearchQuery, ESearchCase::IgnoreCase) &&
				!Contract.Description.Contains(SearchQuery, ESearchCase::IgnoreCase))
			{
				continue;
			}
		}

		Results.Add(Contract);

		if (Results.Num() >= MaxResults)
		{
			break;
		}
	}

	return Results;
}

TArray<FSocialContract> USocialContractSystem::GetPlayerContracts(const FString& PlayerID,
	bool bActiveOnly) const
{
	FScopeLock Lock(&ContractLock);

	TArray<FSocialContract> Results;

	for (const auto& Pair : Contracts)
	{
		const FSocialContract& Contract = Pair.Value;

		if (Contract.ClientPlayerID != PlayerID && Contract.ContractorPlayerID != PlayerID)
		{
			continue;
		}

		if (bActiveOnly)
		{
			if (Contract.Status != EContractStatus::Active &&
				Contract.Status != EContractStatus::Pending &&
				Contract.Status != EContractStatus::AwaitingConfirmation)
			{
				continue;
			}
		}

		Results.Add(Contract);
	}

	return Results;
}

TArray<FServiceProfile> USocialContractSystem::FindServiceProviders(EContractType ServiceType,
	float MinRating, int32 MaxResults) const
{
	FScopeLock Lock(&ContractLock);

	TArray<FServiceProfile> Results;

	for (const auto& Pair : ServiceProfiles)
	{
		const FServiceProfile& Profile = Pair.Value;

		if (!Profile.bIsAvailable)
		{
			continue;
		}

		if (!Profile.OfferedServices.Contains(ServiceType))
		{
			continue;
		}

		if (Profile.GetOverallRating() < MinRating)
		{
			continue;
		}

		Results.Add(Profile);

		if (Results.Num() >= MaxResults)
		{
			break;
		}
	}

	// Sort by rating descending
	Results.Sort([](const FServiceProfile& A, const FServiceProfile& B) {
		return A.GetOverallRating() > B.GetOverallRating();
	});

	return Results;
}

// ==================== Statistics ====================

void USocialContractSystem::GetPlayerStats(const FString& PlayerID, int32& OutCompleted, int32& OutFailed,
	int32& OutActive, int64& OutTotalEarned) const
{
	FScopeLock Lock(&ContractLock);

	OutCompleted = 0;
	OutFailed = 0;
	OutActive = 0;
	OutTotalEarned = 0;

	for (const auto& Pair : Contracts)
	{
		const FSocialContract& Contract = Pair.Value;

		if (Contract.ContractorPlayerID != PlayerID)
		{
			continue;
		}

		switch (Contract.Status)
		{
		case EContractStatus::Completed:
			OutCompleted++;
			OutTotalEarned += Contract.PaymentTerms.GetTotalOMEN();
			break;
		case EContractStatus::Failed:
		case EContractStatus::Cancelled:
			if (!Contract.ContractorPlayerID.IsEmpty())
			{
				OutFailed++;
			}
			break;
		case EContractStatus::Active:
		case EContractStatus::Pending:
		case EContractStatus::AwaitingConfirmation:
			OutActive++;
			break;
		default:
			break;
		}
	}
}

// ==================== Internal Helpers ====================

void USocialContractSystem::ChangeContractStatus(FSocialContract& Contract, EContractStatus NewStatus)
{
	EContractStatus OldStatus = Contract.Status;
	Contract.Status = NewStatus;
	OnContractStatusChanged.Broadcast(Contract.ContractID, OldStatus, NewStatus);
}

void USocialContractSystem::AddSystemMessage(FSocialContract& Contract, const FString& Content)
{
	FContractMessage Message;
	Message.SenderPlayerID = TEXT("System");
	Message.SenderName = TEXT("System");
	Message.Content = Content;
	Message.bIsSystemMessage = true;
	Contract.Messages.Add(Message);
}

void USocialContractSystem::UpdateServiceProfileStats(const FString& PlayerID, bool bCompleted, int64 Earned)
{
	FServiceProfile* Profile = ServiceProfiles.Find(PlayerID);
	if (!Profile)
	{
		return;
	}

	if (bCompleted)
	{
		Profile->TotalContractsCompleted++;
		Profile->TotalEarnings += Earned;
	}
	else
	{
		Profile->TotalContractsFailed++;
	}

	int32 Total = Profile->TotalContractsCompleted + Profile->TotalContractsFailed;
	if (Total > 0)
	{
		Profile->CompletionRate = static_cast<float>(Profile->TotalContractsCompleted) /
			static_cast<float>(Total);
	}
}

void USocialContractSystem::RecalculatePlayerRatings(const FString& PlayerID)
{
	const TArray<FContractRating>* Ratings = PlayerRatings.Find(PlayerID);
	if (!Ratings || Ratings->Num() == 0)
	{
		return;
	}

	FServiceProfile* Profile = ServiceProfiles.Find(PlayerID);
	if (!Profile)
	{
		return;
	}

	// Reset averages
	TMap<ERatingCategory, float> Totals;
	TMap<ERatingCategory, int32> Counts;

	for (const FContractRating& Rating : *Ratings)
	{
		for (const auto& RatingPair : Rating.Ratings)
		{
			if (!Totals.Contains(RatingPair.Key))
			{
				Totals.Add(RatingPair.Key, 0.0f);
				Counts.Add(RatingPair.Key, 0);
			}
			Totals[RatingPair.Key] += static_cast<float>(RatingPair.Value);
			Counts[RatingPair.Key]++;
		}
	}

	Profile->AverageRatings.Empty();
	for (const auto& TotalPair : Totals)
	{
		int32 Count = Counts[TotalPair.Key];
		if (Count > 0)
		{
			Profile->AverageRatings.Add(TotalPair.Key, TotalPair.Value / static_cast<float>(Count));
		}
	}
}

bool USocialContractSystem::CheckContractCompletion(FSocialContract& Contract)
{
	// Check if all milestones are complete and confirmed
	if (Contract.Milestones.Num() == 0)
	{
		// No milestones - check if work is marked done
		return false;
	}

	for (const FContractMilestone& Milestone : Contract.Milestones)
	{
		if (!Milestone.bIsComplete || !Milestone.bClientConfirmed)
		{
			return false;
		}
	}

	// All milestones complete - finalize contract
	Contract.CompletedAt = FDateTime::Now();

	// Release remaining escrow
	if (Contract.Escrow.Status == EEscrowStatus::Funded ||
		Contract.Escrow.Status == EEscrowStatus::Releasing)
	{
		ReleaseEscrow(Contract.ContractID, 1.0f);
	}

	ChangeContractStatus(Contract, EContractStatus::Completed);

	// Update contractor stats
	UpdateServiceProfileStats(Contract.ContractorPlayerID, true, Contract.PaymentTerms.GetTotalOMEN());

	OnContractCompleted.Broadcast(Contract.ContractID, Contract.ClientPlayerID, Contract.ContractorPlayerID);

	AddSystemMessage(Contract, TEXT("Contract completed successfully!"));

	return true;
}

bool USocialContractSystem::MeetsContractorRequirements(const FSocialContract& Contract,
	const FString& PlayerID) const
{
	const FServiceProfile* Profile = ServiceProfiles.Find(PlayerID);

	// Check minimum rating
	if (Contract.MinContractorRating > 0.0f)
	{
		if (!Profile || Profile->GetOverallRating() < Contract.MinContractorRating)
		{
			return false;
		}
	}

	// Check minimum completed contracts
	if (Contract.MinCompletedContracts > 0)
	{
		if (!Profile || Profile->TotalContractsCompleted < Contract.MinCompletedContracts)
		{
			return false;
		}
	}

	// Check guild restriction
	if (Contract.RestrictedToGuildID.IsValid() && GuildManager)
	{
		FGuid PlayerGuild = GuildManager->GetPlayerGuild(PlayerID);
		if (PlayerGuild != Contract.RestrictedToGuildID)
		{
			// Check for allied guilds
			EGuildRelationship Rel = GuildManager->GetGuildRelationship(
				Contract.RestrictedToGuildID, PlayerGuild);
			if (Rel != EGuildRelationship::Allied)
			{
				return false;
			}
		}
	}

	return true;
}

void USocialContractSystem::ProcessExpiredContracts()
{
	FScopeLock Lock(&ContractLock);

	for (auto& Pair : Contracts)
	{
		FSocialContract& Contract = Pair.Value;

		if (Contract.IsExpired() && Contract.Status == EContractStatus::Open)
		{
			// Expire unaccepted contracts
			ChangeContractStatus(Contract, EContractStatus::Expired);
			AddSystemMessage(Contract, TEXT("Contract expired without acceptance."));
		}
		else if (Contract.IsExpired() && Contract.Status == EContractStatus::Active)
		{
			// Active contracts past deadline go to dispute
			if (!Contract.bHasDispute)
			{
				FileDispute(Contract.ContractID, Contract.ClientPlayerID,
					TEXT("Contract deadline exceeded"), TEXT("Auto-generated: deadline passed"));
			}
		}
	}
}

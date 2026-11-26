#include "Logging/FactoryLogComponent.h"
#include "GameFramework/Actor.h"

UFactoryLogComponent::UFactoryLogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UFactoryLogComponent::ReportFailure(FName Operation, const FString& Reason, const FString& Details, EFactoryLogLevel Level) const
{
	const UObject* Context = GetOwner() ? static_cast<const UObject*>(GetOwner()) : this;
	return OperationLog.Fail(Context, Operation, Reason, Details, Level);
}

void UFactoryLogComponent::ClearFailure(FName Operation) const
{
	OperationLog.Succeed(Operation);
}

void UFactoryLogComponent::WriteLog(const FString& Message, EFactoryLogLevel Level) const
{
	const UObject* Context = GetOwner() ? static_cast<const UObject*>(GetOwner()) : this;
	switch (Level)
	{
	case EFactoryLogLevel::Verbose:
		UE_LOG(LogFactoryGame, Verbose, TEXT("[Context=%s] %s"), *GetPathNameSafe(Context), *Message);
		break;
	case EFactoryLogLevel::Log:
		UE_LOG(LogFactoryGame, Log, TEXT("[Context=%s] %s"), *GetPathNameSafe(Context), *Message);
		break;
	case EFactoryLogLevel::Warning:
		UE_LOG(LogFactoryGame, Warning, TEXT("[Context=%s] %s"), *GetPathNameSafe(Context), *Message);
		break;
	case EFactoryLogLevel::Error:
		UE_LOG(LogFactoryGame, Error, TEXT("[Context=%s] %s"), *GetPathNameSafe(Context), *Message);
		break;
	default:
		OperationLog.Fail(Context, TEXT("WriteLog"), TEXT("Unsupported log level."), FString::FromInt(int32(Level)));
		return;
	}
	OperationLog.Succeed(TEXT("WriteLog"));
}

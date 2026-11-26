#pragma once

#include "CoreMinimal.h"
#include "FactoryGameLog.generated.h"

PROJECT_FACTORY_API DECLARE_LOG_CATEGORY_EXTERN(LogFactoryGame, Log, All);

UENUM(BlueprintType)
enum class EFactoryLogLevel : uint8
{
	Log = 0,
	Warning = 1,
	Error = 2,
	Verbose = 3
};

/** 반복 실패는 원인 또는 수준이 바뀔 때 기록한다. 사용자 요청의 Log는 매번 기록한다. */
struct FFactoryOperationLog
{
	bool Fail(const UObject* Context, FName Operation, const FString& Reason, const FString& Details = FString(),
		EFactoryLogLevel Level = EFactoryLogLevel::Warning) const
	{
		const FFailureRecord* Previous = FailureReasons.Find(Operation);
		if (Level == EFactoryLogLevel::Log || !Previous || Previous->Reason != Reason || Previous->Level != Level)
		{
			FailureReasons.Add(Operation, {Reason, Level});
			switch (Level)
			{
			case EFactoryLogLevel::Verbose:
				UE_LOG(LogFactoryGame, Verbose, TEXT("%s failed [Context=%s]: %s %s"),
					*Operation.ToString(), *GetPathNameSafe(Context), *Reason, *Details);
				break;
			case EFactoryLogLevel::Log:
				UE_LOG(LogFactoryGame, Log, TEXT("%s failed [Context=%s]: %s %s"),
					*Operation.ToString(), *GetPathNameSafe(Context), *Reason, *Details);
				break;
			case EFactoryLogLevel::Error:
				UE_LOG(LogFactoryGame, Error, TEXT("%s failed [Context=%s]: %s %s"),
					*Operation.ToString(), *GetPathNameSafe(Context), *Reason, *Details);
				break;
			default:
				UE_LOG(LogFactoryGame, Warning, TEXT("%s failed [Context=%s]: %s %s"),
					*Operation.ToString(), *GetPathNameSafe(Context), *Reason, *Details);
				break;
			}
		}
		return false;
	}

	void Succeed(FName Operation) const
	{
		FailureReasons.Remove(Operation);
	}

private:
	struct FFailureRecord
	{
		FString Reason;
		EFactoryLogLevel Level;
	};
	mutable TMap<FName, FFailureRecord> FailureReasons;
};

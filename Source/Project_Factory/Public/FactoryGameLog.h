#pragma once

#include "CoreMinimal.h"

PROJECT_FACTORY_API DECLARE_LOG_CATEGORY_EXTERN(LogFactoryGame, Log, All);

/** 반복 호출에서는 기능별 실패 사유가 바뀔 때만 기록한다. 성공하면 해당 기록을 해제한다. */
struct FFactoryOperationLog
{
	bool Fail(const UObject* Context, FName Operation, const FString& Reason, const FString& Details = FString()) const
	{
		const FString* Previous = FailureReasons.Find(Operation);
		if (!Previous || *Previous != Reason)
		{
			FailureReasons.Add(Operation, Reason);
			UE_LOG(LogFactoryGame, Warning, TEXT("%s failed [Context=%s]: %s %s"),
				*Operation.ToString(), *GetPathNameSafe(Context), *Reason, *Details);
		}
		return false;
	}

	void Succeed(FName Operation) const
	{
		FailureReasons.Remove(Operation);
	}

private:
	mutable TMap<FName, FString> FailureReasons;
};

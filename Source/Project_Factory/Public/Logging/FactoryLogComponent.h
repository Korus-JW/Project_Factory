#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FactoryGameLog.h"
#include "FactoryLogComponent.generated.h"

/** 블루프린트와 C++에서 공통 카테고리 출력과 기능별 반복 실패 억제를 사용한다. */
UCLASS(ClassGroup = (Factory), meta = (BlueprintSpawnableComponent))
class PROJECT_FACTORY_API UFactoryLogComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFactoryLogComponent();

	/** 실패 원인과 수준에 따라 반복을 억제한다. 사용자 요청의 Log는 매번 기록하고 false를 반환한다. */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Factory|Logging")
	bool ReportFailure(FName Operation, const FString& Reason, const FString& Details = FString(),
		EFactoryLogLevel Level = EFactoryLogLevel::Warning) const;

	/** 기능 성공 시 이전 실패를 해제하여 같은 실패가 재발하면 다시 기록한다. */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Factory|Logging")
	void ClearFailure(FName Operation) const;

	/** 반복 억제 없이 지정한 수준으로 메시지를 출력한다. 메시지는 영어로 전달한다. */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Factory|Logging")
	void WriteLog(const FString& Message, EFactoryLogLevel Level = EFactoryLogLevel::Log) const;

private:
	FFactoryOperationLog OperationLog;
};

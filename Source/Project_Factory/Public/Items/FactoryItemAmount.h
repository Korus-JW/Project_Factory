#pragma once

#include "CoreMinimal.h"
#include "FactoryItemAmount.generated.h"

/** 제작법의 입력 또는 출력에 사용하는 아이템 식별자와 수량이다. */
USTRUCT(BlueprintType)
struct PROJECT_FACTORY_API FFactoryItemAmount
{
	GENERATED_BODY()

	/** 아이템 정보 데이터 테이블의 행 이름을 참조한다. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Factory|Item", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;
};

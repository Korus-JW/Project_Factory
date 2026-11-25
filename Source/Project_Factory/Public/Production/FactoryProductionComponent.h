#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Production/FactoryRecipeDefinition.h"
#include "FactoryGameLog.h"
#include "FactoryProductionComponent.generated.h"

class UFactoryInventoryComponent;

UENUM(BlueprintType)
enum class EFactoryProductionState : uint8
{
	NoRecipe, Paused, MissingInventory, WaitingForMaterials, WaitingForOutputSpace, Producing
};

/** 재료를 작업 시작 시 소비하고 완성품은 공간이 생길 때까지 보존한다. */
UCLASS(ClassGroup = (Factory), meta = (BlueprintSpawnableComponent))
class PROJECT_FACTORY_API UFactoryProductionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UFactoryProductionComponent();

	/** 건물 기본값에서 제작법 데이터 테이블과 행을 지정한다. */
	UPROPERTY(EditDefaultsOnly, Category = "Factory|Production")
	FDataTableRowHandle DefaultRecipe;

	/** 이 건물에서 선택할 수 있는 제작법이다. 같은 데이터 테이블의 여러 행을 등록한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factory|Production")
	TArray<FDataTableRowHandle> AvailableRecipes;

	/** 작업 중이면 입고 완료 후 전환한다. 연속 요청은 마지막 유효한 선택을 사용한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Production")
	bool SelectRecipe(FDataTableRowHandle Recipe);

	UFUNCTION(BlueprintPure, Category = "Factory|Production")
	FDataTableRowHandle GetSelectedRecipe() const { return SelectedRecipe; }

	/** 변경 예약이 없으면 빈 핸들을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Production")
	FDataTableRowHandle GetPendingRecipe() const { return PendingRecipe; }

	/** 목록을 사용하지 않는 기존 건물용 설정이다. 목록이 있거나 작업 중이면 거부한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Production")
	bool SetRecipe(const FFactoryRecipeDefinition& Recipe);

	UFUNCTION(BlueprintCallable, Category = "Factory|Production")
	void SetProductionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Factory|Production")
	EFactoryProductionState GetProductionState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Factory|Production")
	float GetProgress() const;

	/** 아직 출력 재고에 넣지 못한 완성품을 반환한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Production")
	TArray<FFactoryItemAmount> GetPendingOutputs() const;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	FFactoryOperationLog OperationLog;
	void SetState(EFactoryProductionState NewState, const FString& Reason);
	friend class FFactoryProductionTest;
	friend class FFactoryRecipeSelectionTest;
	UPROPERTY(Transient) 
	FDataTableRowHandle SelectedRecipe;

	UPROPERTY(Transient) 
	FDataTableRowHandle PendingRecipe;

	UPROPERTY(Transient) 
	FFactoryRecipeDefinition PendingRecipeDefinition;

	bool bHasPendingRecipe = false;

	void ApplyPendingRecipe();

	UPROPERTY(Transient) 
	TObjectPtr<UFactoryInventoryComponent> Inventory;

	UPROPERTY(Transient)
	FFactoryRecipeDefinition ActiveRecipe;

	UPROPERTY(Transient)
	TArray<FFactoryItemAmount> PendingOutputs;

	UPROPERTY(EditDefaultsOnly, Category = "Factory|Production")
	bool bProductionEnabled = true;

	EFactoryProductionState State = EFactoryProductionState::NoRecipe;
	bool bHasRecipe = false;
	bool bWorking = false;
	bool bAdvancing = false;
	float ElapsedSeconds = 0.0f;
	void AdvanceProduction(float DeltaTime);
	bool HasOutputSpace() const;
};

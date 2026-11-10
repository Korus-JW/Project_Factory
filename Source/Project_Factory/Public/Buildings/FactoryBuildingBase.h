#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Terrain/TerrainChunkData.h"
#include "FactoryBuildingBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/** 건물의 외형과 공통 정보를 제공한다. 생성 위치와 셀 점유는 배치 시스템에서 관리한다. */
UCLASS(Blueprintable)
class PROJECT_FACTORY_API AFactoryBuildingBase : public AActor
{
	GENERATED_BODY()

public:
	AFactoryBuildingBase();

	/** 각 축을 1~1024로 보정한 기본 점유 크기다. 한 셀은 500cm다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Building")
	FIntPoint GetFootprintSize() const;

	/** 90도 회전 횟수가 홀수이면 가로와 세로 크기를 교환한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Building|Placement")
	FIntPoint GetRotatedFootprintSize(int32 QuarterTurns = 0) const;

	/** 기준 좌표는 회전 후 점유 사각형의 최소 X/Y 셀이다. +X/+Y 방향의 모든 셀을 반환한다.
	 * 액터의 회전이나 위치는 바꾸지 않으며 실제 점유 등록도 수행하지 않는다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Building|Placement")
	TArray<FIntPoint> GetFootprintCells(FIntPoint AnchorGridCoordinate, int32 QuarterTurns = 0) const;

	/** 모든 점유 예정 셀이 생성된 지역에 있고 허용된 지형인지 검사한다.
	 * 다른 건물의 점유와 비용, 자원 조건은 별도 배치 시스템에서 검사해야 한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Building|Placement")
	bool CanPlaceOnTerrain(FIntPoint AnchorGridCoordinate, int32 QuarterTurns = 0) const;

	/*건설 가능 여부를 업데이트 한다.*/
	UFUNCTION(BlueprintCallable, Category = "Factory|Building|Preview")
	void UpdateCanBuild(bool bInCanBuild);

	/** 단일 셀의 지형 조건만 검사한다. 자원, 점유, 비용 검사는 포함하지 않는다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Building")
	bool IsTerrainAllowed(ETerrainType TerrainType) const;

	/** 미리보기 중에는 액터 충돌을 끄고, 종료 시 진입 전 충돌 설정을 복원한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Building|Preview")
	void SetPlacementPreview(bool bPreview);

	UFUNCTION(BlueprintPure, Category = "Factory|Building|Preview")
	bool IsPlacementPreview() const { return bPlacementPreview; }

	UFUNCTION(BlueprintCallable, Category = "Factory|Building")
	void RotateBuilding();

	/** 실제 배치 액터에 미리보기의 회전 단계를 전달할 때도 사용한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Building")
	void SetRotationStep(int32 NewRotationStep);

	/** 메시의 수평 중심과 회전 후 점유 영역을 맞춘다. 메시 교체 후에도 호출할 수 있다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Building")
	void RefreshBuildingTransform();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factory|Building|Components")
	TObjectPtr<USceneComponent> BuildingRoot;

	/** 점유 영역의 중심에 위치하며 건물의 수평 회전을 담당한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factory|Building|Components")
	TObjectPtr<USceneComponent> RotationRoot;

	/** 자동 중심 정렬 후 추가할 수평 보정값이다. 메시 높이는 기존 상대 Z를 유지한다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factory|Building")
	FVector2D MeshCenterOffset = FVector2D::ZeroVector;

	/** 자식 블루프린트에서 메시와 상대 위치를 설정한다. 루트는 격자 기준점으로 유지한다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Factory|Building|Components")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factory|Building")
	FName BuildingId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factory|Building")
	FText BuildingName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factory|Building")
	FIntPoint FootprintSize = FIntPoint(1, 1);

	/** 일반 건물은 땅만 허용한다. 항구 등은 자식 클래스에서 조건을 확장할 수 있다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factory|Building")
	TArray<ETerrainType> AllowedTerrainTypes;

	UPROPERTY(BlueprintReadOnly, Category = "Factory|Building")
	bool bCanBuild = false;

	/*건물의 회전 횟수 (0~3)*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factory|Building")
	int32 RotationStep = 0;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Destroyed() override;

	/** 자식 블루프린트에서 미리보기 재질 적용과 원래 재질 복원을 구현한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Factory|Building|Preview")
	void OnPlacementPreviewChanged(bool bPreview);

	/** 자식 블루프린트에서 건설 가능 여부에 따라 미리보기 재질을 변경한다. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Factory|Building|Preview")
	void OnCanBuildChanged(bool bChangedCanBuild);


private:
	UPROPERTY(Transient)
	bool bPlacementPreview = false;

	bool bCollisionEnabledBeforePreview = true;
};

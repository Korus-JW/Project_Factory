#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TerrainChunkData.h"
#include "TerrainWorldSubsystem.generated.h"

class AFactoryBuildingBase;
class ARoadNetworkActor;

/** 현재 월드의 청크 원본을 보관한다. 좌표는 월드 원점에서 +X, +Y 방향으로 증가한다. */
UCLASS(BlueprintType)
class PROJECT_FACTORY_API UTerrainWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 해당 도로 셀에서 등록된 건물 입구·출구를 향하는 연결 비트다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain|Road")
	int32 GetBuildingRoadConnectionMask(FIntPoint RoadCell) const;

	/** 등록 당시의 입구 또는 출구 앞 셀을 반환한다. 도로가 없어도 위치는 조회할 수 있다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain|Road")
	bool GetRegisteredBuildingRoadPort(AFactoryBuildingBase* Building, bool bEntrance, FIntPoint& OutRoadCell) const;
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain|Road")
	ARoadNetworkActor* GetRoadNetworkAtCell(FIntPoint GridCoordinate) const;

	/** 생성된 땅이며 건물과 도로 모두 없는지 검사한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain|Road")
	bool CanPlaceRoad(FIntPoint GridCoordinate) const;

	/** 전체 격자 좌표의 점유 건물을 반환한다. 비어 있으면 nullptr이다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain|Building")
	AFactoryBuildingBase* GetBuildingAtCell(FIntPoint GridCoordinate) const;

	/** 빈 배열은 실패한다. 지형 조건은 검사하지 않고 점유 여부만 검사한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain|Building")
	bool IsAreaFree(const TArray<FIntPoint>& GridCoordinates) const;

	/** 건물의 전체 영역에 대해 지형과 점유를 함께 검사한다. 미리보기에도 사용할 수 있다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain|Building")
	bool CanPlaceBuilding(AFactoryBuildingBase* Building, FIntPoint AnchorGridCoordinate, int32 QuarterTurns = 0) const;

	/** 전체 검사 성공 시에만 등록한다. 미리보기와 이미 등록된 건물은 실패한다.
	 * 액터 생성이나 이동은 수행하지 않는다. 실패 시 호출자가 새 액터를 정리해야 한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Terrain|Building")
	bool TryRegisterBuilding(AFactoryBuildingBase* Building, FIntPoint AnchorGridCoordinate, int32 QuarterTurns = 0);

	/** 해당 건물이 등록했던 셀만 해제한다. 건물 액터는 파괴하지 않는다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Terrain|Building")
	bool UnregisterBuilding(AFactoryBuildingBase* Building);

	/** 호출 객체가 속한 월드의 청크 서브시스템을 가져온다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain", meta = (WorldContext = "WorldContextObject"))
	static UTerrainWorldSubsystem* GetTerrainWorldSubsystem(const UObject* WorldContextObject);

	/** 테스트 청크를 생성하고 저장한다. 최초 청크의 크기가 월드의 공통 청크 크기가 된다.
	 * 다른 크기나 중복 좌표는 실패하며 기존 데이터를 유지한다. 교체는 명시적으로 허용해야 한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Terrain")
	bool GenerateTestChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData,
		int32 Width = 10, int32 Height = 10, int32 WaterRows = 2, bool bReplaceExisting = false);

	/** 조회 결과는 복사본이며 수정해도 저장된 원본은 변경되지 않는다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	bool GetChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData) const;

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	bool HasChunk(FIntPoint ChunkCoordinate) const;

	/** 지정한 청크 내부의 셀을 조회한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	bool GetCell(FIntPoint ChunkCoordinate, int32 X, int32 Y, FGridCellData& OutCell) const;

	/** 음수 좌표를 포함한 전체 격자 좌표로 셀을 조회한다. 미생성 지역은 실패한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	bool GetCellAtGridCoordinate(FIntPoint GridCoordinate, FGridCellData& OutCell) const;

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	bool GetCellAtWorldLocation(FVector WorldLocation, FGridCellData& OutCell) const;

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	FIntPoint WorldToGrid(FVector WorldLocation) const;

	/** 전체 격자의 모서리 또는 중심을 월드 좌표로 반환한다. 지표면 높이는 호출자가 지정한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	FVector GridToWorld(FIntPoint GridCoordinate, bool bCellCenter = false, float SurfaceZ = 0.0f) const;

	/** 청크 액터를 배치할 월드 원점이다. 청크 크기 확정 전에 호출하면 실패한다. */
	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	bool GetChunkWorldOrigin(FIntPoint ChunkCoordinate, FVector& OutOrigin) const;

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	FIntPoint GetChunkDimensions() const { return ChunkDimensions; }

	UFUNCTION(BlueprintPure, Category = "Factory|Terrain")
	int32 GetChunkCount() const { return Chunks.Num(); }

	/** 데이터만 초기화한다. 화면의 청크 액터와 ISM은 별도로 정리해야 한다. */
	UFUNCTION(BlueprintCallable, Category = "Factory|Terrain")
	void ClearMap();

	virtual void Deinitialize() override;

private:
	struct FRegisteredRoadPort
	{
		TWeakObjectPtr<AFactoryBuildingBase> Building;
		bool bEntrance = false;
		int32 Mask = 0;
	};
	TMap<FIntPoint, TArray<FRegisteredRoadPort>> BuildingRoadPorts;
	TMap<TWeakObjectPtr<AFactoryBuildingBase>, TArray<FIntPoint>> RegisteredPortCells;
	void RefreshRoadsAtPorts(const TArray<FIntPoint>& Cells);
	friend class ARoadNetworkActor;
	bool TryOccupyRoadCell(FIntPoint Cell, ARoadNetworkActor* Owner);
	void ReleaseRoadCell(FIntPoint Cell, ARoadNetworkActor* Owner);
	TMap<FIntPoint, TWeakObjectPtr<ARoadNetworkActor>> RoadOwners;
	// 약한 참조로 건물 수명을 연장하지 않는다. 조회는 전체 건물을 순회하지 않는다.
	TMap<FIntPoint, TWeakObjectPtr<AFactoryBuildingBase>> OccupiedCells;

	// 해제 시 현재 크기 대신 등록 당시 좌표를 사용해 크기 변경에도 안전하게 정리한다.
	TMap<TWeakObjectPtr<AFactoryBuildingBase>, TArray<FIntPoint>> RegisteredBuildingCells;

	UPROPERTY(Transient)
	TMap<FIntPoint, FTerrainChunkData> Chunks;

	UPROPERTY(Transient)
	FIntPoint ChunkDimensions = FIntPoint::ZeroValue;
};


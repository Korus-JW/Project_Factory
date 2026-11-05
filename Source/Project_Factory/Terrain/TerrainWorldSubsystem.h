#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TerrainChunkData.h"
#include "TerrainWorldSubsystem.generated.h"

/** 현재 월드의 청크 원본을 보관한다. 좌표는 월드 원점에서 +X, +Y 방향으로 증가한다. */
UCLASS(BlueprintType)
class PROJECT_FACTORY_API UTerrainWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
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
	UPROPERTY(Transient)
	TMap<FIntPoint, FTerrainChunkData> Chunks;

	UPROPERTY(Transient)
	FIntPoint ChunkDimensions = FIntPoint::ZeroValue;
};


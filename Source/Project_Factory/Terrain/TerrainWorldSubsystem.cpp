#include "TerrainWorldSubsystem.h"

#include "TerrainDataLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	// 정수 나눗셈의 0 방향 절삭을 보정해 음수 격자도 올바른 청크로 분류한다.
	int32 DivideFloor(int32 Value, int32 Divisor)
	{
		return Value / Divisor - (Value % Divisor < 0 ? 1 : 0);
	}

	int32 PositiveRemainder(int32 Value, int32 Divisor)
	{
		const int32 Remainder = Value % Divisor;
		return Remainder < 0 ? Remainder + Divisor : Remainder;
	}
}

UTerrainWorldSubsystem* UTerrainWorldSubsystem::GetTerrainWorldSubsystem(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World ? World->GetSubsystem<UTerrainWorldSubsystem>() : nullptr;
}

bool UTerrainWorldSubsystem::GenerateTestChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData,
	int32 Width, int32 Height, int32 WaterRows, bool bReplaceExisting)
{
	OutData = FTerrainChunkData();
	if ((!bReplaceExisting && Chunks.Contains(ChunkCoordinate))
		|| (Chunks.Num() > 0 && ChunkDimensions != FIntPoint(Width, Height)))
	{
		return false;
	}

	if (!UTerrainDataLibrary::GenerateTestChunk(ChunkCoordinate, OutData, Width, Height, WaterRows))
	{
		return false;
	}

	ChunkDimensions = FIntPoint(Width, Height);
	Chunks.Add(ChunkCoordinate, OutData);
	return true;
}

bool UTerrainWorldSubsystem::GetChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData) const
{
	const FTerrainChunkData* Chunk = Chunks.Find(ChunkCoordinate);
	OutData = Chunk ? *Chunk : FTerrainChunkData();
	return Chunk != nullptr;
}

bool UTerrainWorldSubsystem::HasChunk(FIntPoint ChunkCoordinate) const
{
	return Chunks.Contains(ChunkCoordinate);
}

bool UTerrainWorldSubsystem::GetCell(FIntPoint ChunkCoordinate, int32 X, int32 Y, FGridCellData& OutCell) const
{
	OutCell = FGridCellData();
	const FTerrainChunkData* Chunk = Chunks.Find(ChunkCoordinate);
	return Chunk && UTerrainDataLibrary::GetCell(*Chunk, X, Y, OutCell);
}

bool UTerrainWorldSubsystem::GetCellAtGridCoordinate(FIntPoint GridCoordinate, FGridCellData& OutCell) const
{
	OutCell = FGridCellData();
	if (ChunkDimensions.X <= 0 || ChunkDimensions.Y <= 0)
	{
		return false;
	}

	const FIntPoint ChunkCoordinate(DivideFloor(GridCoordinate.X, ChunkDimensions.X),
		DivideFloor(GridCoordinate.Y, ChunkDimensions.Y));
	return GetCell(ChunkCoordinate, PositiveRemainder(GridCoordinate.X, ChunkDimensions.X),
		PositiveRemainder(GridCoordinate.Y, ChunkDimensions.Y), OutCell);
}

bool UTerrainWorldSubsystem::GetCellAtWorldLocation(FVector WorldLocation, FGridCellData& OutCell) const
{
	return GetCellAtGridCoordinate(WorldToGrid(WorldLocation), OutCell);
}

FIntPoint UTerrainWorldSubsystem::WorldToGrid(FVector WorldLocation) const
{
	return FIntPoint(FMath::FloorToInt(WorldLocation.X / UTerrainDataLibrary::CellSize),
		FMath::FloorToInt(WorldLocation.Y / UTerrainDataLibrary::CellSize));
}

FVector UTerrainWorldSubsystem::GridToWorld(FIntPoint GridCoordinate, bool bCellCenter, float SurfaceZ) const
{
	FVector Location = bCellCenter
		? UTerrainDataLibrary::GetCellLocalCenter(GridCoordinate.X, GridCoordinate.Y)
		: UTerrainDataLibrary::GetCellLocalOrigin(GridCoordinate.X, GridCoordinate.Y);
	Location.Z = SurfaceZ;
	return Location;
}

bool UTerrainWorldSubsystem::GetChunkWorldOrigin(FIntPoint ChunkCoordinate, FVector& OutOrigin) const
{
	OutOrigin = FVector::ZeroVector;
	if (ChunkDimensions.X <= 0 || ChunkDimensions.Y <= 0)
	{
		return false;
	}
	OutOrigin = FVector(static_cast<double>(ChunkCoordinate.X) * ChunkDimensions.X * UTerrainDataLibrary::CellSize,
		static_cast<double>(ChunkCoordinate.Y) * ChunkDimensions.Y * UTerrainDataLibrary::CellSize, 0.0);
	return true;
}

void UTerrainWorldSubsystem::ClearMap()
{
	Chunks.Empty();
	ChunkDimensions = FIntPoint::ZeroValue;
}

void UTerrainWorldSubsystem::Deinitialize()
{
	ClearMap();
	Super::Deinitialize();
}

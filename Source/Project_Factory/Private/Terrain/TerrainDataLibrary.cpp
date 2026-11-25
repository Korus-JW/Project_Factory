#include "Terrain/TerrainDataLibrary.h"
#include "FactoryGameLog.h"

namespace
{
	constexpr int32 MaxChunkDimension = 1024;
	FFactoryOperationLog OperationLog;

	bool AreDimensionsValid(int32 Width, int32 Height)
	{
		return Width > 0 && Height > 0 && Width <= MaxChunkDimension && Height <= MaxChunkDimension;
	}
}

bool UTerrainDataLibrary::GenerateTestChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData,
	int32 Width, int32 Height, int32 WaterRows)
{
	OutData = FTerrainChunkData();
	OutData.ChunkCoordinate = ChunkCoordinate;
	if (!AreDimensionsValid(Width, Height))
	{
		return OperationLog.Fail(GetDefault<UTerrainDataLibrary>(), TEXT("GenerateTestChunk"), TEXT("Chunk dimensions must be within 1..1024."), FString::Printf(TEXT("Chunk=%s Width=%d Height=%d"), *ChunkCoordinate.ToString(), Width, Height));
	}
	OperationLog.Succeed(TEXT("GenerateTestChunk"));

	OutData.Width = Width;
	OutData.Height = Height;
	OutData.Cells.SetNum(Width * Height);
	const int32 ClampedWaterRows = FMath::Clamp(WaterRows, 0, Height);
	TArray<int32> LandIndices;
	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			OutData.Cells[Y * Width + X].TerrainType =
				Y < ClampedWaterRows ? ETerrainType::Water : ETerrainType::Land;
			if (Y >= ClampedWaterRows) LandIndices.Add(Y * Width + X);
		}
	}
	// 청크 좌표로 고정한 난수를 사용해 재생성 시에도 같은 자원 분포를 유지한다.
	const uint32 Seed = uint32(ChunkCoordinate.X) * 73856093u
		^ uint32(ChunkCoordinate.Y) * 19349663u ^ 0x46AC219Bu;
	FRandomStream Random(static_cast<int32>(Seed));
	for (int32 I = LandIndices.Num() - 1; I > 0; --I)
	{
		LandIndices.Swap(I, Random.RandRange(0, I));
	}
	// 땅의 약 10%씩 나무와 돌로 지정한다. 땅이 두 칸 이상이면 각 자원을 최소 한 칸 보장한다.
	const int32 CountPerResource = FMath::Min(LandIndices.Num() / 2, FMath::Max(1, LandIndices.Num() / 10));
	for (int32 I = 0; I < CountPerResource; ++I)
	{
		OutData.Cells[LandIndices[I]].ResourceType = ECellResourceType::Wood;
		OutData.Cells[LandIndices[CountPerResource + I]].ResourceType = ECellResourceType::Stone;
	}
	return true;
}

bool UTerrainDataLibrary::IsValidChunk(const FTerrainChunkData& Data)
{
	return AreDimensionsValid(Data.Width, Data.Height)
		&& Data.Cells.Num() == Data.Width * Data.Height;
}

int32 UTerrainDataLibrary::GetCellIndex(const FTerrainChunkData& Data, int32 X, int32 Y)
{
	if (!IsValidChunk(Data) || X < 0 || Y < 0 || X >= Data.Width || Y >= Data.Height)
	{
		OperationLog.Fail(GetDefault<UTerrainDataLibrary>(), TEXT("GetCellIndex"), TEXT("Invalid chunk data or cell coordinates."), FString::Printf(TEXT("Chunk=%s Cell=(%d,%d) Dimensions=(%d,%d) Cells=%d"), *Data.ChunkCoordinate.ToString(), X, Y, Data.Width, Data.Height, Data.Cells.Num()));
		return INDEX_NONE;
	}
	OperationLog.Succeed(TEXT("GetCellIndex"));
	return Y * Data.Width + X;
}

bool UTerrainDataLibrary::GetCell(const FTerrainChunkData& Data, int32 X, int32 Y, FGridCellData& OutCell)
{
	OutCell = FGridCellData();
	const int32 Index = GetCellIndex(Data, X, Y);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutCell = Data.Cells[Index];
	return true;
}

FVector UTerrainDataLibrary::GetCellLocalCenter(int32 X, int32 Y)
{
	return FVector((static_cast<double>(X) + 0.5) * CellSize,
		(static_cast<double>(Y) + 0.5) * CellSize, 0.0);
}

FVector UTerrainDataLibrary::GetCellLocalOrigin(int32 X, int32 Y)
{
	return FVector(static_cast<double>(X) * CellSize,
		static_cast<double>(Y) * CellSize, 0.0);
}

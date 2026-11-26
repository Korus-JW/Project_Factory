#include "Terrain/TerrainWorldSubsystem.h"
#include "Roads/RoadNetworkActor.h"

#include "Terrain/TerrainDataLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Buildings/FactoryBuildingBase.h"

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
	UTerrainWorldSubsystem* Subsystem = World ? World->GetSubsystem<UTerrainWorldSubsystem>() : nullptr;
	static FFactoryOperationLog ContextLog;
	if (!Subsystem)
	{
		ContextLog.Fail(WorldContextObject, TEXT("GetTerrainWorldSubsystem"), TEXT("World context or terrain subsystem is unavailable."));
	}
	else
	{
		ContextLog.Succeed(TEXT("GetTerrainWorldSubsystem"));
	}
	return Subsystem;
}

bool UTerrainWorldSubsystem::GenerateTestChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData,
	int32 Width, int32 Height, int32 WaterRows, bool bReplaceExisting)
{
	OutData = FTerrainChunkData();
	if (!bReplaceExisting && Chunks.Contains(ChunkCoordinate))
	{
		return OperationLog.Fail(this, TEXT("GenerateTestChunk"), TEXT("Chunk already exists and replacement is disabled."), ChunkCoordinate.ToString());
	}
	if (Chunks.Num() > 0 && ChunkDimensions != FIntPoint(Width, Height))
	{
		return OperationLog.Fail(this, TEXT("GenerateTestChunk"), TEXT("Dimensions differ from the existing map."), FString::Printf(TEXT("Requested=(%d,%d) Existing=%s"), Width, Height, *ChunkDimensions.ToString()));
	}

	if (!UTerrainDataLibrary::GenerateTestChunk(ChunkCoordinate, OutData, Width, Height, WaterRows))
	{
		return false;
	}

	ChunkDimensions = FIntPoint(Width, Height);
	Chunks.Add(ChunkCoordinate, OutData);
	OperationLog.Succeed(TEXT("GenerateTestChunk"));
	UE_LOG(LogFactoryGame, Log, TEXT("Terrain chunk generated [Chunk=%s Dimensions=(%d,%d)]"), *ChunkCoordinate.ToString(), Width, Height);
	return true;
}

bool UTerrainWorldSubsystem::GetChunk(FIntPoint ChunkCoordinate, FTerrainChunkData& OutData) const
{
	const FTerrainChunkData* Chunk = Chunks.Find(ChunkCoordinate);
	OutData = Chunk ? *Chunk : FTerrainChunkData();
	if (!Chunk) return OperationLog.Fail(this, TEXT("GetChunk"), TEXT("Chunk has not been generated."), ChunkCoordinate.ToString(), EFactoryLogLevel::Verbose);
	OperationLog.Succeed(TEXT("GetChunk"));
	return true;
}

bool UTerrainWorldSubsystem::HasChunk(FIntPoint ChunkCoordinate) const
{
	return Chunks.Contains(ChunkCoordinate);
}

bool UTerrainWorldSubsystem::GetCell(FIntPoint ChunkCoordinate, int32 X, int32 Y, FGridCellData& OutCell) const
{
	OutCell = FGridCellData();
	const FTerrainChunkData* Chunk = Chunks.Find(ChunkCoordinate);
	if (!Chunk) return OperationLog.Fail(this, TEXT("GetCell"), TEXT("Chunk has not been generated."), ChunkCoordinate.ToString(), EFactoryLogLevel::Verbose);
	OperationLog.Succeed(TEXT("GetCell"));
	return UTerrainDataLibrary::GetCell(*Chunk, X, Y, OutCell);
}

bool UTerrainWorldSubsystem::GetCellAtGridCoordinate(FIntPoint GridCoordinate, FGridCellData& OutCell) const
{
	OutCell = FGridCellData();
	if (ChunkDimensions.X <= 0 || ChunkDimensions.Y <= 0)
	{
		return OperationLog.Fail(this, TEXT("GetCellAtGridCoordinate"), TEXT("Chunk dimensions are not initialized."), FString(), EFactoryLogLevel::Verbose);
	}

	OperationLog.Succeed(TEXT("GetCellAtGridCoordinate"));
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
		return OperationLog.Fail(this, TEXT("GetChunkWorldOrigin"), TEXT("Chunk dimensions are not initialized."));
	}
	OperationLog.Succeed(TEXT("GetChunkWorldOrigin"));
	OutOrigin = FVector(static_cast<double>(ChunkCoordinate.X) * ChunkDimensions.X * UTerrainDataLibrary::CellSize,
		static_cast<double>(ChunkCoordinate.Y) * ChunkDimensions.Y * UTerrainDataLibrary::CellSize, 0.0);
	return true;
}

void UTerrainWorldSubsystem::ClearMap()
{
	// 도로 표시와 데이터를 먼저 지워 점유 초기화 이후에도 길이 남는 것을 방지한다.
	TSet<TWeakObjectPtr<ARoadNetworkActor>> Owners;
	for (const auto& Pair : RoadOwners) Owners.Add(Pair.Value);
	for (const auto& Owner : Owners)
	{
		if (Owner.IsValid()) Owner->ClearRoads();
	}
	RoadOwners.Empty();
	OccupiedCells.Empty();
	RegisteredBuildingCells.Empty();
	BuildingRoadPorts.Empty();
	RegisteredPortCells.Empty();
	Chunks.Empty();
	ChunkDimensions = FIntPoint::ZeroValue;
}

AFactoryBuildingBase* UTerrainWorldSubsystem::GetBuildingAtCell(FIntPoint GridCoordinate) const
{
	const TWeakObjectPtr<AFactoryBuildingBase>* Building = OccupiedCells.Find(GridCoordinate);
	return Building ? Building->Get() : nullptr;
}

bool UTerrainWorldSubsystem::IsAreaFree(const TArray<FIntPoint>& GridCoordinates, EFactoryLogLevel FailureLevel) const
{
	if (GridCoordinates.IsEmpty())
	{
		return OperationLog.Fail(this, TEXT("IsAreaFree"), TEXT("Footprint has no cells."));
	}
	for (const FIntPoint& Coordinate : GridCoordinates)
	{
		if (GetBuildingAtCell(Coordinate) || GetRoadNetworkAtCell(Coordinate))
		{
			return OperationLog.Fail(this, TEXT("IsAreaFree"), TEXT("Cell is occupied by a building or road."), Coordinate.ToString(), FailureLevel);
		}
	}
	OperationLog.Succeed(TEXT("IsAreaFree"));
	return true;
}

bool UTerrainWorldSubsystem::CanPlaceBuilding(AFactoryBuildingBase* Building,
	FIntPoint AnchorGridCoordinate, int32 QuarterTurns, EFactoryLogLevel FailureLevel) const
{
	if (!IsValid(Building)) return OperationLog.Fail(this, TEXT("BuildingValidation"), TEXT("Building is null or invalid."));
	for (bool bEntrance : {true, false})
	{
		const auto& Port = bEntrance ? Building->EntrancePort : Building->ExitPort;
		FIntPoint RoadCell;
		int32 Mask;
		if (Port.bEnabled && !Building->GetRoadPort(bEntrance, AnchorGridCoordinate, QuarterTurns, RoadCell, Mask)) return false;
	}
	if (Building->GetWorld() != GetWorld())
	{
		return OperationLog.Fail(this, TEXT("BuildingValidation"), TEXT("Building belongs to a different world."), Building->GetPathName());
	}
	OperationLog.Succeed(TEXT("BuildingValidation"));
	return Building->CanPlaceOnTerrain(AnchorGridCoordinate, QuarterTurns, FailureLevel)
		&& IsAreaFree(Building->GetFootprintCells(AnchorGridCoordinate, QuarterTurns), FailureLevel);
}

bool UTerrainWorldSubsystem::TryRegisterBuilding(AFactoryBuildingBase* Building,
	FIntPoint AnchorGridCoordinate, int32 QuarterTurns)
{
	if (!IsValid(Building)) return OperationLog.Fail(this, TEXT("TryRegisterBuilding"), TEXT("Building is null or invalid."));
	if (Building->IsPlacementPreview()) return OperationLog.Fail(this, TEXT("TryRegisterBuilding"), TEXT("Placement previews cannot be registered."), Building->GetPathName());
	if (RegisteredBuildingCells.Contains(TWeakObjectPtr<AFactoryBuildingBase>(Building)))
	{
		return OperationLog.Fail(this, TEXT("TryRegisterBuilding"), TEXT("Building is already registered."), Building->GetPathName());
	}
	if (!CanPlaceBuilding(Building, AnchorGridCoordinate, QuarterTurns, EFactoryLogLevel::Log)) return false;
	OperationLog.Succeed(TEXT("TryRegisterBuilding"));

	// 활성화한 연결점이 잘못된 경우 점유를 변경하기 전에 등록을 거절한다.
	TArray<FIntPoint> PortCells;
	TArray<FRegisteredRoadPort> Ports;
	for (bool bEntrance : {true, false})
	{
		const FBuildingRoadPort& Config = bEntrance ? Building->EntrancePort : Building->ExitPort;
		if (!Config.bEnabled) continue;
		FIntPoint RoadCell;
		int32 Mask;
		if (!Building->GetRoadPort(bEntrance, AnchorGridCoordinate, QuarterTurns, RoadCell, Mask)) return false;
		PortCells.Add(RoadCell);
		FRegisteredRoadPort Port;
		Port.Building = Building;
		Port.bEntrance = bEntrance;
		Port.Mask = Mask;
		Ports.Add(Port);
	}
	// 검사가 끝난 뒤에만 모든 셀을 기록하므로 실패 시 부분 점유가 남지 않는다.
	TArray<FIntPoint> Cells = Building->GetFootprintCells(AnchorGridCoordinate, QuarterTurns);
	for (const FIntPoint& Coordinate : Cells)
	{
		OccupiedCells.Add(Coordinate, Building);
	}
	// 이벤트 처리 중 등록 정보가 변경되어도 전달 배열이 유효하도록 지역 복사본을 유지한다.
	RegisteredBuildingCells.Add(Building, Cells);
	for (int32 I = 0; I < PortCells.Num(); ++I) BuildingRoadPorts.FindOrAdd(PortCells[I]).Add(Ports[I]);
	RegisteredPortCells.Add(Building, PortCells);
	RefreshRoadsAtPorts(PortCells);
	UE_LOG(LogFactoryGame, Log, TEXT("Building registered [Building=%s Anchor=%s Rotation=%d Cells=%d]"), *Building->GetPathName(), *AnchorGridCoordinate.ToString(), QuarterTurns, Cells.Num());
	OnCellOccupancyChanged.Broadcast(Cells);
	return true;
}

bool UTerrainWorldSubsystem::UnregisterBuilding(AFactoryBuildingBase* Building)
{
	if (!Building) return OperationLog.Fail(this, TEXT("UnregisterBuilding"), TEXT("Building is null."));
	const TWeakObjectPtr<AFactoryBuildingBase> Key(Building);
	const TArray<FIntPoint>* Cells = RegisteredBuildingCells.Find(Key);
	if (!Cells)
	{
		OperationLog.Fail(this, TEXT("UnregisterBuilding"), TEXT("Building has no registered occupancy."), Building->GetPathName(), EFactoryLogLevel::Verbose);
		return false;
	}
	OperationLog.Succeed(TEXT("UnregisterBuilding"));
	const TArray<FIntPoint> ChangedCells = *Cells;
	for (const FIntPoint& Coordinate : ChangedCells)
	{
		const TWeakObjectPtr<AFactoryBuildingBase>* Occupant = OccupiedCells.Find(Coordinate);
		if (Occupant && *Occupant == Key)
		{
			OccupiedCells.Remove(Coordinate);
		}
	}
	RegisteredBuildingCells.Remove(Key);
	TArray<FIntPoint> PortCells;
	RegisteredPortCells.RemoveAndCopyValue(Key, PortCells);
	for (const FIntPoint& Cell : PortCells)
	{
		if (auto* Ports = BuildingRoadPorts.Find(Cell))
		{
			Ports->RemoveAll([Key](const FRegisteredRoadPort& Port) { return Port.Building == Key; });
			if (Ports->IsEmpty()) BuildingRoadPorts.Remove(Cell);
		}
	}
	RefreshRoadsAtPorts(PortCells);
	UE_LOG(LogFactoryGame, Log, TEXT("Building unregistered [Building=%s Cells=%d]"), *Building->GetPathName(), ChangedCells.Num());
	OnCellOccupancyChanged.Broadcast(ChangedCells);
	return true;
}

int32 UTerrainWorldSubsystem::GetBuildingRoadConnectionMask(FIntPoint RoadCell) const
{
	int32 Mask = 0;
	if (const auto* Ports = BuildingRoadPorts.Find(RoadCell))
	{
		for (const auto& Port : *Ports) if (Port.Building.IsValid()) Mask |= Port.Mask;
	}
	return Mask;
}

bool UTerrainWorldSubsystem::GetRegisteredBuildingRoadPort(AFactoryBuildingBase* Building,
	bool bEntrance, FIntPoint& OutRoadCell) const
{
	OutRoadCell = FIntPoint::ZeroValue;
	if (!IsValid(Building)) return OperationLog.Fail(this, TEXT("BuildingValidation"), TEXT("Building is null or invalid."));
	if (const auto* Cells = RegisteredPortCells.Find(TWeakObjectPtr<AFactoryBuildingBase>(Building)))
	{
		for (const auto& Cell : *Cells)
		{
			if (const auto* Ports = BuildingRoadPorts.Find(Cell))
			{
				for (const auto& Port : *Ports)
				{
					if (Port.Building.Get() == Building && Port.bEntrance == bEntrance)
					{
						OperationLog.Succeed(TEXT("GetRegisteredBuildingRoadPort"));
						OutRoadCell = Cell;
						return true;
					}
				}
			}
		}
	}
	return OperationLog.Fail(this, TEXT("GetRegisteredBuildingRoadPort"), TEXT("Requested entrance or exit is not registered."), FString::Printf(TEXT("Building=%s Entrance=%d"), *Building->GetPathName(), bEntrance), EFactoryLogLevel::Verbose);
}

void UTerrainWorldSubsystem::RefreshRoadsAtPorts(const TArray<FIntPoint>& Cells)
{
	TSet<ARoadNetworkActor*> Owners;
	for (const auto& Cell : Cells) if (auto* Owner = GetRoadNetworkAtCell(Cell)) Owners.Add(Owner);
	for (auto* Owner : Owners) Owner->RebuildVisuals();
}

void UTerrainWorldSubsystem::Deinitialize()
{
	ClearMap();
	Super::Deinitialize();
}

ARoadNetworkActor* UTerrainWorldSubsystem::GetRoadNetworkAtCell(FIntPoint GridCoordinate) const
{
 const auto* Entry = RoadOwners.Find(GridCoordinate);
 ARoadNetworkActor* Owner = Entry ? Entry->Get() : nullptr;
 return Owner && Owner->HasRoad(GridCoordinate) ? Owner : nullptr;
}

bool UTerrainWorldSubsystem::SetCellResource(FIntPoint GridCoordinate, ECellResourceType ResourceType)
{
	if (ResourceType != ECellResourceType::None && ResourceType != ECellResourceType::Wood
		&& ResourceType != ECellResourceType::Stone) return OperationLog.Fail(this, TEXT("SetCellResource"), TEXT("Unsupported resource type."), FString::FromInt(int32(ResourceType)));
	FGridCellData Existing;
	if (!GetCellAtGridCoordinate(GridCoordinate, Existing))
	{
		return OperationLog.Fail(this, TEXT("SetCellResource"), TEXT("Requested cell has not been generated."), GridCoordinate.ToString(), EFactoryLogLevel::Log);
	}
	if (Existing.TerrainType != ETerrainType::Land || GetBuildingAtCell(GridCoordinate) || GetRoadNetworkAtCell(GridCoordinate))
	{
		return OperationLog.Fail(this, TEXT("SetCellResource"), TEXT("Resource requires an unoccupied land cell."), GridCoordinate.ToString(), EFactoryLogLevel::Log);
	}
	const FIntPoint ChunkCoordinate(DivideFloor(GridCoordinate.X, ChunkDimensions.X),
		DivideFloor(GridCoordinate.Y, ChunkDimensions.Y));
	FTerrainChunkData& Chunk = Chunks.FindChecked(ChunkCoordinate);
	const int32 Index = UTerrainDataLibrary::GetCellIndex(Chunk,
		PositiveRemainder(GridCoordinate.X, ChunkDimensions.X), PositiveRemainder(GridCoordinate.Y, ChunkDimensions.Y));
	if (Index == INDEX_NONE) return false;
	Chunk.Cells[Index].ResourceType = ResourceType;
	OperationLog.Succeed(TEXT("SetCellResource"));
	UE_LOG(LogFactoryGame, Log, TEXT("Cell resource changed [Cell=%s Resource=%d]"), *GridCoordinate.ToString(), int32(ResourceType));
	return true;
}

bool UTerrainWorldSubsystem::CanPlaceRoad(FIntPoint GridCoordinate, EFactoryLogLevel FailureLevel) const
{
 FGridCellData Cell;
 if (!GetCellAtGridCoordinate(GridCoordinate, Cell))
 {
  return OperationLog.Fail(this, TEXT("CanPlaceRoad"), TEXT("Road cell has not been generated."), GridCoordinate.ToString(), FailureLevel);
 }
 if (Cell.TerrainType != ETerrainType::Land || Cell.ResourceType != ECellResourceType::None
  || GetBuildingAtCell(GridCoordinate) || GetRoadNetworkAtCell(GridCoordinate))
 {
  return OperationLog.Fail(this, TEXT("CanPlaceRoad"), TEXT("Road requires unoccupied land without resources."), GridCoordinate.ToString(), FailureLevel);
 }
 OperationLog.Succeed(TEXT("CanPlaceRoad"));
 return true;
}

bool UTerrainWorldSubsystem::TryOccupyRoadCell(FIntPoint Cell, ARoadNetworkActor* Owner)
{
 if (!IsValid(Owner) || Owner->GetWorld() != GetWorld())
 {
  return OperationLog.Fail(this, TEXT("TryOccupyRoadCell"), TEXT("Road owner is invalid or belongs to another world."), Cell.ToString());
 }
 if (!CanPlaceRoad(Cell, EFactoryLogLevel::Log)) return false;
 OperationLog.Succeed(TEXT("TryOccupyRoadCell"));
 RoadOwners.Add(Cell, Owner);
 return true;
}

void UTerrainWorldSubsystem::ReleaseRoadCell(FIntPoint Cell, ARoadNetworkActor* Owner)
{
 const auto* Entry = RoadOwners.Find(Cell);
 if (Entry && *Entry == TWeakObjectPtr<ARoadNetworkActor>(Owner)) RoadOwners.Remove(Cell);
}

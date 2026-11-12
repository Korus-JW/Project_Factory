#include "Roads/RoadNetworkActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Terrain/TerrainDataLibrary.h"
#include "Terrain/TerrainWorldSubsystem.h"

namespace
{
	const FIntPoint Steps[] = { FIntPoint(0,1), FIntPoint(1,0), FIntPoint(0,-1), FIntPoint(-1,0) };
	bool Neighbor(FIntPoint Cell, int32 Direction, FIntPoint& Out)
	{
		const int64 X = int64(Cell.X) + Steps[Direction].X;
		const int64 Y = int64(Cell.Y) + Steps[Direction].Y;
		if (X < MIN_int32 || X > MAX_int32 || Y < MIN_int32 || Y > MAX_int32) return false;
		Out = FIntPoint(int32(X), int32(Y)); return true;
	}
	// 양의 Yaw는 동쪽을 북쪽으로 회전시킨다.
	int32 RotateMask(int32 Mask) { return ((Mask >> 1) | ((Mask & 1) << 3)) & 15; }
	// 신규 도로와 방향이 바뀐 도로 모두 같은 연결 규칙을 사용한다.
	bool CanConnect(bool bHorizontal, const FRoadCellData& Other, int32 Direction)
	{
		const bool AlongNew = bHorizontal == (Direction % 2 == 1);
		const bool AlongOther = Other.bHorizontal == (Direction % 2 == 1);
		return Other.bHorizontal == bHorizontal ? AlongNew
			: (Other.ConnectionMask == 0 ? AlongNew : (AlongNew || AlongOther));
	}
}
ARoadNetworkActor::ARoadNetworkActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("RoadRoot")));
	const TCHAR* Names[] = { TEXT("StraightISM"),TEXT("CornerISM"),TEXT("TJunctionISM"),TEXT("CrossISM") };
	for (const TCHAR* Name : Names)
	{
		auto* ISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		ISM->SetupAttachment(RootComponent);
		ISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		RoadInstances.Add(ISM);
	}
}
bool ARoadNetworkActor::AddRoad(FIntPoint Cell, bool bHorizontal)
{
	if (Roads.Contains(Cell)) return false;
	auto* Terrain = UTerrainWorldSubsystem::GetTerrainWorldSubsystem(this);
	if (!Terrain || !Terrain->TryOccupyRoadCell(Cell, this)) return false;
	FRoadCellData NewRoad; NewRoad.bHorizontal = bHorizontal;
	TArray<int32> Connections;
	TArray<FIntPoint> ChangedDirections;
	for (int32 D = 0; D < 4; ++D)
	{
		FIntPoint OtherCell;
		if (!Neighbor(Cell, D, OtherCell)) continue;
		const FRoadCellData* Other = Roads.Find(OtherCell);
		if (!Other) continue;
		// 평행 도로의 옆면은 연결하지 않는다. 독립된 다른 방향 도로에는 새 도로 방향을 우선한다.
		if (CanConnect(bHorizontal, *Other, D)) Connections.Add(D);
	}
	for (int32 D : Connections)
	{
		FIntPoint OtherCell; Neighbor(Cell, D, OtherCell);
		NewRoad.ConnectionMask |= 1 << D;
		FRoadCellData& Other = Roads.FindChecked(OtherCell);
		// 고립된 도로는 새 연결 축으로 기본 방향도 바꿔 이후 배치와 철거에 반영한다.
		if (Other.ConnectionMask == 0 && Other.bHorizontal != (D % 2 == 1))
		{
			Other.bHorizontal = (D % 2 == 1);
			ChangedDirections.Add(OtherCell);
		}
		Other.ConnectionMask |= 1 << ((D + 2) % 4);
	}
	Roads.Add(Cell, NewRoad);
	// 방향 변경으로 새로 마주 보게 된 기존 이웃도 연결한다. 추가 방향 변경은 큐로 전파한다.
	// 고립된 도로만 방향을 바꾸고 즉시 연결 비트를 기록하므로 같은 도로가 반복 회전하지 않는다.
	for (int32 Index = 0; Index < ChangedDirections.Num(); ++Index)
	{
		const FIntPoint ChangedCell = ChangedDirections[Index];
		FRoadCellData& Changed = Roads.FindChecked(ChangedCell);
		TArray<int32> Reconnect;
		for (int32 D = 0; D < 4; ++D)
		{
			FIntPoint OtherCell;
			if ((Changed.ConnectionMask & (1 << D)) || !Neighbor(ChangedCell, D, OtherCell)) continue;
			const auto* Other = Roads.Find(OtherCell);
			if (Other && CanConnect(Changed.bHorizontal, *Other, D)) Reconnect.Add(D);
		}
		for (int32 D : Reconnect)
		{
			FIntPoint OtherCell;
			Neighbor(ChangedCell, D, OtherCell);
			auto& Other = Roads.FindChecked(OtherCell);
			if (Other.ConnectionMask == 0 && Other.bHorizontal != (D % 2 == 1))
			{
				Other.bHorizontal = (D % 2 == 1);
				ChangedDirections.Add(OtherCell);
			}
			Changed.ConnectionMask |= 1 << D;
			Other.ConnectionMask |= 1 << ((D + 2) % 4);
		}
	}
	RebuildVisuals(); return true;
}
bool ARoadNetworkActor::RemoveRoad(FIntPoint Cell)
{
	if (!Roads.Remove(Cell)) return false;
	if (auto* Terrain = UTerrainWorldSubsystem::GetTerrainWorldSubsystem(this)) Terrain->ReleaseRoadCell(Cell, this);
	for (int32 D = 0; D < 4; ++D)
	{
		FIntPoint OtherCell;
		if (!Neighbor(Cell, D, OtherCell)) continue;
		if (auto* Other = Roads.Find(OtherCell)) Other->ConnectionMask &= ~(1 << ((D + 2) % 4));
	}
	RebuildVisuals(); return true;
}
bool ARoadNetworkActor::GetRoad(FIntPoint Cell, FRoadCellData& OutData) const
{
	const auto* Data = Roads.Find(Cell); OutData = Data ? *Data : FRoadCellData(); return Data != nullptr;
}
bool ARoadNetworkActor::HasRoad(FIntPoint Cell) const { return Roads.Contains(Cell); }
void ARoadNetworkActor::ClearRoads()
{
	if (auto* Terrain = UTerrainWorldSubsystem::GetTerrainWorldSubsystem(this))
	{
		for (const auto& Pair : Roads) Terrain->ReleaseRoadCell(Pair.Key, this);
	}
	Roads.Empty(); RebuildVisuals();
}
void ARoadNetworkActor::EndPlay(const EEndPlayReason::Type Reason)
{
	ClearRoads(); Super::EndPlay(Reason);
}
void ARoadNetworkActor::Destroyed() 
{ 
	ClearRoads(); Super::Destroyed(); 
}
void ARoadNetworkActor::ResolveVisual(int32 Mask, bool bHorizontal, int32& OutType, float& OutYaw)
{
	Mask &= 15; OutType = 0; OutYaw = bHorizontal ? 90.f : 0.f;
	if (Mask == 0) return;
	if (Mask == 1 || Mask == 4) { OutYaw = 0; return; }
	if (Mask == 2 || Mask == 8) { OutYaw = 90; return; }
	const int32 Bases[] = { 5,6,11,15 };
	for (int32 Type = 0; Type < 4; ++Type)
	{
		int32 Rotated = Bases[Type];
		for (int32 Q = 0; Q < 4; ++Q)
		{
			if (Rotated == Mask) { OutType = Type; OutYaw = 90.f * Q; return; }
			Rotated = RotateMask(Rotated);
		}
	}
}
void ARoadNetworkActor::RebuildVisuals()
{
	UStaticMesh* Meshes[] = { StraightMesh,CornerMesh,TJunctionMesh,CrossMesh };
	for (int32 I = 0; I < 4; ++I) { RoadInstances[I]->ClearInstances(); RoadInstances[I]->SetStaticMesh(Meshes[I]); }
	const double CellSize = UTerrainDataLibrary::CellSize;
	const auto* Terrain = UTerrainWorldSubsystem::GetTerrainWorldSubsystem(this);
	for (auto& Pair : Roads)
	{
		Pair.Value.BuildingConnectionMask = Terrain ? Terrain->GetBuildingRoadConnectionMask(Pair.Key) : 0;
		Pair.Value.VisualMask = Pair.Value.ConnectionMask | Pair.Value.BuildingConnectionMask;
		int32 Type; float Yaw; ResolveVisual(Pair.Value.VisualMask, Pair.Value.bHorizontal, Type, Yaw);
		UStaticMesh* Mesh = Meshes[Type]; if (!Mesh) continue;
		const FBox Box = Mesh->GetBoundingBox();
		const FVector MeshOriginOffset(250.0f, 250.0f, Box.Min.Z);
		const FQuat Rotation = FRotator(0, Yaw, 0).Quaternion();
		const FVector Center((double(Pair.Key.X) + 0.5) * CellSize, (double(Pair.Key.Y) + 0.5) * CellSize, SurfaceZ);
		RoadInstances[Type]->AddInstance(FTransform(Rotation, Center - Rotation.RotateVector(MeshOriginOffset)), true);
	}
}
void ARoadNetworkActor::OnConstruction(const FTransform& Transform) 
{ 
	Super::OnConstruction(Transform); RebuildVisuals(); 
}




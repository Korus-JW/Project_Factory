#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RoadNetworkActor.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FRoadCellData
{
 GENERATED_BODY()
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) bool bHorizontal = false;
 // 북=1, 동=2, 남=4, 서=8이며 실제 연결만 기록한다.
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 ConnectionMask = 0;
};

/** 도로 데이터와 표시를 관리한다. 같은 도로망은 하나의 액터에 등록한다. */
UCLASS(Blueprintable)
class PROJECT_FACTORY_API ARoadNetworkActor : public AActor
{
 GENERATED_BODY()
public:
 ARoadNetworkActor();
 // 전체 격자 좌표를 사용하며 지형과 공통 점유를 검사한다. 비용은 호출자가 처리한다.
 UFUNCTION(BlueprintCallable, Category="Factory|Road") bool AddRoad(FIntPoint Cell, bool bHorizontal);
 UFUNCTION(BlueprintCallable, Category="Factory|Road") bool RemoveRoad(FIntPoint Cell);
 UFUNCTION(BlueprintPure, Category="Factory|Road") bool GetRoad(FIntPoint Cell, FRoadCellData& OutData) const;
 UFUNCTION(BlueprintPure, Category="Factory|Road") bool HasRoad(FIntPoint Cell) const;
 UFUNCTION(BlueprintCallable, Category="Factory|Road") void ClearRoads();
 // 메시 설정을 바꾼 뒤에도 호출할 수 있다. 프로토타입에서는 전체 표시를 재구성한다.
 UFUNCTION(BlueprintCallable, Category="Factory|Road") void RebuildVisuals();
 // 마스크에 해당하는 메시 종류(0=직선,1=코너,2=T,3=십자)와 회전을 계산한다.
 UFUNCTION(BlueprintPure, Category="Factory|Road") static void ResolveVisual(int32 Mask, bool bHorizontal, int32& OutType, float& OutYaw);
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Factory|Road|Mesh") TObjectPtr<UStaticMesh> StraightMesh;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Factory|Road|Mesh") TObjectPtr<UStaticMesh> CornerMesh;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Factory|Road|Mesh") TObjectPtr<UStaticMesh> TJunctionMesh;
 UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Factory|Road|Mesh") TObjectPtr<UStaticMesh> CrossMesh;
 // 메시 크기는 자동 변경하지 않고 수평 중심과 바닥 높이만 정렬한다.
 UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Factory|Road") float SurfaceZ = 25.0f;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Factory|Road") TArray<TObjectPtr<UInstancedStaticMeshComponent>> RoadInstances;
protected:
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 virtual void Destroyed() override;
 virtual void OnConstruction(const FTransform& Transform) override;
private:
 UPROPERTY(Transient, VisibleAnywhere, Category="Factory|Road") TMap<FIntPoint, FRoadCellData> Roads;
};

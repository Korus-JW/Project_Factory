#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Inventory/FactoryInventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryInventoryTest, "ProjectFactory.Inventory.CapacityAndConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryInventoryTest::RunTest(const FString& Parameters)
{
	UFactoryInventoryComponent* Inventory = NewObject<UFactoryInventoryComponent>();
	const auto Input = EFactoryInventorySlot::Input;
	const auto Output = EFactoryInventorySlot::Output;
	const FName Stone(TEXT("Stone"));
	const FName Wood(TEXT("Wood"));
	TestEqual(TEXT("미설정 품목 입고 차단"), Inventory->AddItems(Input, Stone, 1), 0);
	Inventory->SetCapacity(Input, Stone, 10);
	Inventory->SetCapacity(Input, Wood, 10);
	Inventory->SetCapacity(Output, Stone, 5);
	TestEqual(TEXT("용량까지만 입고"), Inventory->AddItems(Input, Stone, 20), 10);
	TestEqual(TEXT("입출력 독립"), Inventory->GetQuantity(Output, Stone), 0);
	TestEqual(TEXT("출력 입고"), Inventory->AddItems(Output, Stone, 3), 3);
	TestEqual(TEXT("음수 요청 차단"), Inventory->RemoveItems(Input, Stone, -1), 0);
	TestFalse(TEXT("빈 식별자 차단"), Inventory->SetCapacity(Input, NAME_None, 10));

	FFactoryItemAmount A;
	A.ItemId = Stone;
	A.Quantity = 6;
	TArray<FFactoryItemAmount> Requirements = { A, A };
	TestFalse(TEXT("중복 품목 합산"), Inventory->TryConsumeItems(Input, Requirements));
	TestEqual(TEXT("실패 시 재고 보존"), Inventory->GetQuantity(Input, Stone), 10);
	Requirements[1].ItemId = Wood;
	TestFalse(TEXT("다른 재료 부족 시 전량 미차감"), Inventory->TryConsumeItems(Input, Requirements));
	TestEqual(TEXT("충분한 재료도 보존"), Inventory->GetQuantity(Input, Stone), 10);
	Inventory->AddItems(Input, Wood, 6);
	TestTrue(TEXT("모든 재료 일괄 소비"), Inventory->TryConsumeItems(Input, Requirements));
	TestEqual(TEXT("돌 잔량"), Inventory->GetQuantity(Input, Stone), 4);
	TestEqual(TEXT("목재 소진"), Inventory->GetQuantity(Input, Wood), 0);

	Inventory->SetCapacity(Input, Stone, 2);
	TestEqual(TEXT("용량 축소 시 보존"), Inventory->GetQuantity(Input, Stone), 4);
	TestEqual(TEXT("초과 재고 추가 입고 차단"), Inventory->AddItems(Input, Stone, 1), 0);
	TestEqual(TEXT("초과 제거 요청은 실제 재고만 제거"), Inventory->RemoveItems(Input, Stone, 9), 4);
	TestEqual(TEXT("공간 회복"), Inventory->GetFreeSpace(Input, Stone), 2);
	TestEqual(TEXT("출력 재고 유지"), Inventory->GetQuantity(Output, Stone), 3);
	return true;
}

#endif

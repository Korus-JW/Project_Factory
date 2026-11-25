#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FactoryGameLog.h"
#include "Logging/FactoryLogComponent.h"
#include "Buildings/FactoryBuildingBase.h"
#include "Roads/RoadNetworkActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryGameLogTest, "ProjectFactory.Logging.FailureTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryGameLogTest::RunTest(const FString& Parameters)
{
	FFactoryOperationLog Log;
	// 같은 실패는 억제하고 원인 변경 및 성공 후 재실패는 다시 기록한다.
	AddExpectedMessagePlain(TEXT("Reason A"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);
	AddExpectedMessagePlain(TEXT("Reason B"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestFalse(TEXT("실패 결과 유지"), Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason A")));
	Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason A"));
	Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason B"));
	Log.Succeed(TEXT("TestOperation"));
	Log.Fail(nullptr, TEXT("TestOperation"), TEXT("Reason A"));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactoryLogComponentTest, "ProjectFactory.Logging.Component",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFactoryLogComponentTest::RunTest(const FString& Parameters)
{
	auto* First = NewObject<UFactoryLogComponent>();
	auto* Second = NewObject<UFactoryLogComponent>();
	// 객체별 독립 기록과 성공 후 재실패, 원인 변경을 검증한다.
	AddExpectedMessagePlain(TEXT("Component reason A"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 3);
	AddExpectedMessagePlain(TEXT("Component reason B"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	TestFalse(TEXT("컴포넌트 실패 반환"), First->ReportFailure(TEXT("Test"), TEXT("Component reason A")));
	First->ReportFailure(TEXT("Test"), TEXT("Component reason A"), TEXT("Changed details"));
	Second->ReportFailure(TEXT("Test"), TEXT("Component reason A"));
	First->ReportFailure(TEXT("Test"), TEXT("Component reason B"));
	First->ClearFailure(TEXT("Test"));
	First->ReportFailure(TEXT("Test"), TEXT("Component reason A"));

	AddExpectedMessagePlain(TEXT("Component warning message"), ELogVerbosity::Warning);
	AddExpectedMessagePlain(TEXT("Component error message"), ELogVerbosity::Error);
	First->WriteLog(TEXT("Component info message"));
	First->WriteLog(TEXT("Component warning message"), EFactoryLogLevel::Warning);
	First->WriteLog(TEXT("Component error message"), EFactoryLogLevel::Error);
	TestFalse(TEXT("틱 비활성화"), First->PrimaryComponentTick.bCanEverTick);
	TestNotNull(TEXT("건물 기본 로그 컴포넌트"), GetDefault<AFactoryBuildingBase>()->FactoryLog.Get());
	TestNotNull(TEXT("도로 기본 로그 컴포넌트"), GetDefault<ARoadNetworkActor>()->FactoryLog.Get());
	return true;
}

#endif

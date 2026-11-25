#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "FactoryGameLog.h"

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

#endif


#include "pch.h"
#include <iostream>
#include <assert.h>
#include <Windows.h>
#include <conio.h>
#include <ctime> 
#include <Cstring>
#include <eyex\EyeX.h>
using namespace std;

#pragma comment (lib, "Tobii.EyeX.Client.lib")
// ID of the global interactor that provides our data stream; must be unique within the application.
char * name = new char[11];
static const TX_STRING InteractorId = name;

// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

/*
 * Initializes g_hGlobalInteractorSnapshot with an interactor that has the Eye Position behavior.
 */
BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_HANDLE hBehaviorWithoutParameters = TX_EMPTY_HANDLE;

	BOOL success;

	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateInteractorBehavior(hInteractor, &hBehaviorWithoutParameters, TX_BEHAVIORTYPE_EYEPOSITIONDATA) == TX_RESULT_OK;

	txReleaseObject(&hInteractor);

	return success;
}

/*
 * Callback function invoked when a snapshot has been committed.
 */
void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

/*
 * Callback function invoked when the status of the connection to the EyeX Engine has changed.
 */
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState) {
	case TX_CONNECTIONSTATE_CONNECTED: {
			BOOL success;
			printf("The connection state is now CONNECTED (We are connected to the EyeX Engine)\n");
			// commit the snapshot with the global interactor as soon as the connection to the engine is established.
			// (it cannot be done earlier because committing means "send to the engine".)
			success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
			if (!success) {
				printf("Failed to initialize the data stream.\n");
			}
			else {
				printf("Waiting for eye position data to start streaming...\n");
			}
		}
		break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		printf("The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		printf("The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		printf("The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		printf("The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.\n");
		break;
	}
}

/*
 * 計算時間
 * 輸入:第一固定為hBehavior，aroundtime是使用時間，resttime是休息時間
 * 此函式會讀取眼睛狀態，使用時間超過aroundtime，便會進入休息模式，待休息過(眼睛不注視螢幕)resttime才會回到使用模式
 */
void Checkeye(TX_HANDLE hEyePositionDataBehavior,int aroundtime,int resttime) 
{
	static bool timesup = false;
	static bool isstop = false;
	static time_t origin = time(0);
	static int starttime = origin;
	static int noeyetime = 0;
	static int stoptime = 0;
	static int retime = 0;
	static int noeyecounter = 0;
	static int counter = 0;
	static int stopcounter = 0;
	static int restcounter = 0;
	COORD position = { 0,8 };
	TX_EYEPOSITIONDATAEVENTPARAMS eventParams;
	if (txGetEyePositionDataEventParams(hEyePositionDataBehavior, &eventParams) == TX_RESULT_OK) {
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), position);
		if (!timesup && !isstop) {//正常計時
			cout << "Counting...." << endl;
			if ((eventParams.LeftEyeX == 0 && eventParams.LeftEyeY == 0 || eventParams.LeftEyeZ == 0) &&
				(eventParams.RightEyeX == 0 && eventParams.RightEyeY == 0 && eventParams.RightEyeZ == 0))
			{
				//printf("NOEYE      \n");
				if (noeyetime == 0) {//初始化
					noeyetime = starttime + counter;
				}
				time_t now = time(0);
				noeyecounter = now - noeyetime;
				//cout << noeyecounter << endl;
				if (noeyecounter >= 5) {
					isstop = true;
					noeyecounter = 0;
				}
			}
			else {
				//printf("EYE        \n");
				if (noeyetime != 0) {//初始化
					starttime += noeyecounter;
					noeyetime = 0;
					noeyecounter = 0;
				}
				time_t now = time(0);
				counter = now - starttime;
				//cout << counter << "         "<< endl;
				if (counter >= aroundtime) {
					timesup = true;
				}
			}
		}
		else if(isstop){//計時暫停
			cout << "count stop" << endl;
			if ((eventParams.LeftEyeX == 0 && eventParams.LeftEyeY == 0 || eventParams.LeftEyeZ == 0) &&
				(eventParams.RightEyeX == 0 && eventParams.RightEyeY == 0 && eventParams.RightEyeZ == 0)){
				//printf("NOEYE     \n");
				stoptime = 0;
			}
			else {
				//printf("EYE       \n");
				if (stoptime == 0) {
					stoptime = time(0);
				}
				time_t now = time(0);
				stopcounter = now - stoptime;
				//cout << stopcounter <<"   " << endl;
				if (stopcounter >= 4) {
					stoptime = 0;
					starttime = time(0) - counter;
					isstop = false;
				}
			}
		}
		else if (timesup) {//眼睛休息
			cout << "Time's up" << endl;
			if ((eventParams.LeftEyeX == 0 && eventParams.LeftEyeY == 0 || eventParams.LeftEyeZ == 0) &&
				(eventParams.RightEyeX == 0 && eventParams.RightEyeY == 0 && eventParams.RightEyeZ == 0)) {
				//printf("NOEYE     \n");
				if (retime == 0) {
					retime = time(0);
				}
				time_t now = time(0);
				restcounter = now - retime;
				//cout << restcounter << "   " << endl;
				if (restcounter >= resttime) {
					retime = 0;
					starttime = time(0);
					counter = 0;
					timesup = false;
				}
			}
			else {
				//printf("EYE       \n");
				retime = 0;
			}
		}
	}
	else {
		printf("Failed to interpret eye position data event packet.\n");
	}
}
/*
 * Callback function invoked when an event has been received from the EyeX Engine.這邊在初始化完eyetracker便會重複執行
 */
void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

	txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));
	
	if (txGetEventBehavior(hEvent, &hBehavior, TX_BEHAVIORTYPE_EYEPOSITIONDATA) == TX_RESULT_OK) {
		Checkeye(hBehavior,10,5);
		txReleaseObject(&hBehavior);
	}
	// NOTE since this is a very simple application with a single interactor and a single data stream, 
	// our event handling code can be very simple too. A more complex application would typically have to 
	// check for multiple behaviors and route events based on interactor IDs.
	txReleaseObject(&hEvent);
}
void Track() {
	TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	BOOL success;
	// initialize and enable the context that is our link to the EyeX Engine.
	success = txInitializeEyeX(TX_EYEXCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&hContext, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(hContext);
	success &= txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;
	success &= txEnableConnection(hContext) == TX_RESULT_OK;

	// let the events flow until a key is pressed.
	if (success) {
		printf("Initialization was successful.\n");
	}
	else {
		printf("Initialization failed.\n");
	}
	printf("Press any key to exit...\n");
	_getch();
	printf("Exiting.\n");
	// disable and delete the context.
	txDisableConnection(hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	success = txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE) == TX_RESULT_OK;
	success &= txReleaseContext(&hContext) == TX_RESULT_OK;
	success &= txUninitializeEyeX() == TX_RESULT_OK;
	if (!success) {
		printf("EyeX could not be shut down cleanly. Did you remember to release all handles?\n");
	}
}
int main()
{
	Track();
}

// 執行程式: Ctrl + F5 或 [偵錯] > [啟動但不偵錯] 功能表
// 偵錯程式: F5 或 [偵錯] > [啟動偵錯] 功能表

// 開始使用的秘訣: 
//   1. 使用 [方案總管] 視窗，新增/管理檔案
//   2. 使用 [Team Explorer] 視窗，連線到原始檔控制
//   3. 使用 [輸出] 視窗，參閱組建輸出與其他訊息
//   4. 使用 [錯誤清單] 視窗，檢視錯誤
//   5. 前往 [專案] > [新增項目]，建立新的程式碼檔案，或是前往 [專案] > [新增現有項目]，將現有程式碼檔案新增至專案
//   6. 之後要再次開啟此專案時，請前往 [檔案] > [開啟] > [專案]，然後選取 .sln 檔案

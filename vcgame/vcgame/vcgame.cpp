// vcgame.cpp : 애플리케이션에 대한 진입점을 정의합니다.
//

#include "framework.h"
#include "vcgame.h"
#define TIMER_ID_GAME_TICK 1001 // 게임 타이머 ID
#define GAME_DURATION_SEC 60    // 총 게임 시간 (초)
#define CIRCLE_RADIUS 20        // 동그라미 반지름
#define CIRCLE_SPAWN_INTERVAL 1000 // 동그라미 생성 주기 (ms)
#define MAX_CIRCLES 10          // 화면에 동시에 존재할 수 있는 최대 동그라미 수

// 동그라미 정보를 담을 구조체
typedef struct {
    RECT rect;       // 동그라미의 경계 사각형 (IntersectRect 사용 용도)
    DWORD spawnTime; // 동그라미가 생성된 시간 (GetTickCount() 값)
    int lifetimeMS;  // 동그라미가 사라지는 데 걸리는 시간 (밀리초)
    BOOL isActive;   // 활성화 상태
} CIRCLE;

// 게임 상태 변수
CIRCLE g_Circles[MAX_CIRCLES];
int g_SuccessCount = 0;
int g_FailCount = 0;
DWORD g_GameStartTime = 0;   // 게임 시작 시간
DWORD g_CurrentGameTime = 0; // 현재 게임 경과 시간 (ms)
BOOL g_GameActive = FALSE;   // 게임 진행 여부

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

// 이 코드 모듈에 포함된 함수의 선언을 전달합니다:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 여기에 코드를 입력합니다.

    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_VCGAME, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_VCGAME));

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  함수: MyRegisterClass()
//
//  용도: 창 클래스를 등록합니다.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VCGAME));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_VCGAME);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   함수: InitInstance(HINSTANCE, int)
//
//   용도: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   주석:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  용도: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND  - 애플리케이션 메뉴를 처리합니다.
//  WM_PAINT    - 주 창을 그립니다.
//  WM_DESTROY  - 종료 메시지를 게시하고 반환합니다.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
#define IDC_START_BUTTON 100 // 버튼 컨트롤 ID

    case WM_CREATE:
    {
        // 1. "시작" 버튼 생성
        // 버튼 핸들을 전역 변수 HMENU hStartButton; 등에 저장하여 나중에 제어할 수 있습니다.
        HWND hStartButton = CreateWindow(
            L"BUTTON",                  // 버튼 클래스 이름
            L"시작",                     // 버튼 텍스트
            WS_CHILD | WS_VISIBLE,      // 자식 윈도우, 보이게 설정
            100, 100,                   // 버튼의 X, Y 좌표 (임의 지정)
            100, 40,                    // 버튼의 너비, 높이
            hWnd,                       // 부모 윈도우 핸들
            (HMENU)IDC_START_BUTTON,    // 버튼의 고유 ID (WM_COMMAND에서 사용)
            (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
            NULL
        );

        // 2. 초기 게임 상태 설정 (선택 사항)
        // g_GameActive = FALSE; // 전역 변수 초기 상태 확인
        // SetRect(&g_Circles[i].rect, 0, 0, 0, 0); // 동그라미 배열 초기화

        // 3. 랜덤 시드 설정
        // rand() 함수를 사용하기 전에 현재 시간을 시드로 설정하여 매번 다른 패턴의 난수가 생성되도록 합니다.
        srand((unsigned int)GetTickCount64());
    }
    return 0;
    case WM_TIMER:
        if (wParam == TIMER_ID_GAME_TICK && g_GameActive) {
            DWORD currentTime = GetTickCount64();
            g_CurrentGameTime = currentTime - g_GameStartTime;

            // 1. **게임 종료 확인**
            if (g_CurrentGameTime >= GAME_DURATION_SEC * 1000)
            {
                // A. 게임 루프 중지 및 상태 변경
                KillTimer(hWnd, TIMER_ID_GAME_TICK);
                g_GameActive = FALSE;

                // B. 결과 메세지 박스 출력 (원하는 형식으로)
                TCHAR resultMsg[64];
                wsprintf(resultMsg, L"게임 종료!\n성공: %d | 실패: %d", g_SuccessCount, g_FailCount);
                MessageBox(hWnd, resultMsg, L"결과", MB_OK);

                // C. [핵심] UI 버튼들 다시 보이기 (초기 화면 복구)
                // C-1. 시작 버튼 보이기
                ShowWindow(GetDlgItem(hWnd, IDC_START_BUTTON), SW_SHOW);

                // C-2. 단계 선택 버튼들 보이기 (반복문 사용)
                /*for (int i = 1; i <= MAX_STAGES; i++) {
                    ShowWindow(GetDlgItem(hWnd, IDC_STAGE_BUTTON_BASE + i), SW_SHOW);
                }*/

                // D. 데이터 완전 초기화 (화면에 남은 동그라미 제거)
                for (int i = 0; i < MAX_CIRCLES; i++) {
                    g_Circles[i].isActive = FALSE;
                }

                // 점수 및 시간 변수는 게임 시작(WM_COMMAND)할 때 초기화하므로 여기서는 놔둬도 됩니다.

                // E. 화면 전체 갱신 (잔상 제거 및 초기 배경 그리기)
                // TRUE를 넣어 배경을 지우고 깨끗하게 다시 그립니다.
                InvalidateRect(hWnd, NULL, TRUE);
            }

            // 2. **동그라미 생성** (예: 1초마다)
            // 1초 주기로 생성 로직을 넣거나, 별도의 타이머를 사용할 수 있습니다.
            // 여기서는 매 1초(1000ms)마다 생성 로직을 실행한다고 가정합니다.
            if ((g_CurrentGameTime / CIRCLE_SPAWN_INTERVAL) != ((g_CurrentGameTime - 50) / CIRCLE_SPAWN_INTERVAL)) {
                // 새 동그라미 생성
                for (int i = 0; i < MAX_CIRCLES; i++) {
                    if (!g_Circles[i].isActive) {
                        RECT clientRect;
                        GetClientRect(hWnd, &clientRect);

                        int x = rand() % (clientRect.right - 2 * CIRCLE_RADIUS) + CIRCLE_RADIUS;
                        int y = rand() % (clientRect.bottom - 2 * CIRCLE_RADIUS) + CIRCLE_RADIUS;

                        // RECT 설정 (경계 사각형)
                        SetRect(&g_Circles[i].rect, x - CIRCLE_RADIUS, y - CIRCLE_RADIUS,
                            x + CIRCLE_RADIUS, y + CIRCLE_RADIUS);

                        g_Circles[i].spawnTime = currentTime;

                        // 시간이 지남에 따라 사라지는 속도가 빨라지게 설정
                        // 예: 2000ms (2초)에서 300ms (0.3초)까지 감소
                        int minLifetime = 300;
                        int maxLifetime = 2000;
                        double progress = (double)g_CurrentGameTime / (GAME_DURATION_SEC * 1000);

                        g_Circles[i].lifetimeMS = maxLifetime - (int)((maxLifetime - minLifetime) * progress);

                        g_Circles[i].isActive = TRUE;
                        break;
                    }
                }
            }

            // 3. **동그라미 수명 확인 및 제거**
            for (int i = 0; i < MAX_CIRCLES; i++) {
                if (g_Circles[i].isActive) {
                    if (currentTime - g_Circles[i].spawnTime > g_Circles[i].lifetimeMS) {
                        g_Circles[i].isActive = FALSE; // 시간 초과로 제거
                        g_FailCount++;
                    }
                }
            }

            InvalidateRect(hWnd, NULL, TRUE); // 화면 업데이트 요청 (WM_PAINT 발생)
        }
        break;

    case WM_LBUTTONDOWN:
        if (g_GameActive) {
            int mouseX = LOWORD(lParam); // 클릭한 X 좌표
            int mouseY = HIWORD(lParam); // 클릭한 Y 좌표

            POINT clickPoint = { mouseX, mouseY };
            BOOL hit = FALSE;

            for (int i = 0; i < MAX_CIRCLES; i++) {
                if (g_Circles[i].isActive) {
                    // IntersectRect는 두 RECT가 겹치는지를 확인하지만,
                    // 점(POINT)이 RECT 내부에 있는지 확인하는 함수는 PtInRect입니다.
                    // 요청하신 RECT 기반의 충돌 판정을 위해 PtInRect를 사용합니다.
                    if (PtInRect(&g_Circles[i].rect, clickPoint)) {
                        // 성공!
                        g_SuccessCount++;
                        g_Circles[i].isActive = FALSE; // 동그라미 제거
                        hit = TRUE;
                        break;
                    }
                }
            }

            if (!hit) {
                // 동그라미가 아닌 곳을 클릭
                g_FailCount++;
            }

            InvalidateRect(hWnd, NULL, TRUE); // 점수 업데이트를 위해 화면 갱신
        }
        // TODO: 시작 버튼 클릭 처리 로직 추가 (버튼 핸들을 사용해야 함)
        // 현재는 게임 시작 버튼 클릭 처리가 생략되어 있습니다.
        break;
    case WM_COMMAND:
        {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDC_START_BUTTON: // 사용자가 "시작" 버튼을 클릭했을 때
            if (!g_GameActive) // 게임이 현재 비활성화 상태라면 시작
            {
                // 1. 버튼 숨기기
                // 시작 버튼이 사라지게 하여 게임 진행을 막지 않도록 합니다.
                HWND hButton = GetDlgItem(hWnd, IDC_START_BUTTON);
                ShowWindow(hButton, SW_HIDE);

                // 2. 게임 상태 초기화 및 활성화
                g_SuccessCount = 0;
                g_FailCount = 0;
                g_GameStartTime = GetTickCount64(); // 게임 시작 시간 기록
                g_GameActive = TRUE;

                // 3. 타이머 설정
                // 50ms (0.05초)마다 WM_TIMER 메시지를 발생시켜 게임 로직을 실행합니다.
                SetTimer(hWnd, TIMER_ID_GAME_TICK, 50, NULL);

                // 4. 화면 갱신
                InvalidateRect(hWnd, NULL, TRUE);
            }
            break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        // 1. 남은 시간 및 점수 표시
        int remainingTime = GAME_DURATION_SEC - (g_CurrentGameTime / 1000);
        if (remainingTime < 0 || !g_GameActive) remainingTime = 0;

        TCHAR buffer[100];
        _stprintf_s(buffer, 100, TEXT("남은 시간: %d초 | 성공: %d | 실패: %d"),
            remainingTime, g_SuccessCount, g_FailCount);
        TextOut(hdc, 10, 10, buffer, _tcslen(buffer));

        // 2. 동그라미 그리기 
        if (g_GameActive) {
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0)); // 빨간색 브러시
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

            for (int i = 0; i < MAX_CIRCLES; i++) {
                if (g_Circles[i].isActive) {
                    // 동그라미의 경계 사각형을 사용하여 그리기
                    Ellipse(hdc, g_Circles[i].rect.left, g_Circles[i].rect.top,
                        g_Circles[i].rect.right, g_Circles[i].rect.bottom);
                }
            }

            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
        }

        // 3. (옵션) 게임 시작 버튼 그리기 (g_GameActive가 FALSE일 때)

        EndPaint(hWnd, &ps);
    }
    break;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 정보 대화 상자의 메시지 처리기입니다.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

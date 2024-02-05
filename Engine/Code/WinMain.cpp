#include <Application/Application.h>
#include <Framework/Common.h>

#include <ShellScalingAPI.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    ProfileSetThreadName("Main thread");

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    InitializeLogger();

    Application application(L"Engine demo");

    application.Run();

    return 0;
}

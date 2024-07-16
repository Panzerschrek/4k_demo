#include <windows.h>

int main()
{
	static const char message[] = "Hello, world!";
	DWORD written = 0;
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, DWORD(sizeof(message) - 1), &written, NULL);
}

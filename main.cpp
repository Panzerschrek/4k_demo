#include <windows.h>

int main()
{
	DWORD written = 0;
	const char message[] = "hello world";
	WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), message, DWORD(sizeof(message) - 1), &written, NULL);
}

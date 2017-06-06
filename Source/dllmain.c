// dllmain.cpp : Defines the entry point for the DLL application.
__declspec(dllexport)
BOOL WINAPI DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		ul_reason_for_call = ul_reason_for_call;
		break;
	case DLL_THREAD_ATTACH:
		ul_reason_for_call = ul_reason_for_call;
		break;
	case DLL_THREAD_DETACH:
		ul_reason_for_call = ul_reason_for_call;
		break;
	case DLL_PROCESS_DETACH:
		ul_reason_for_call = ul_reason_for_call;
		break;
	}
	return TRUE;
}
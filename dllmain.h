// dllmain.h : Declaration of module class.

class CoAuthModule : public ATL::CAtlDllModuleT< CoAuthModule >
{
public :
	DECLARE_LIBID(LIBID_oAuthLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_OAUTH, "{26994C40-1DA1-4466-97D9-160FF331D921}")
};

extern class CoAuthModule _AtlModule;

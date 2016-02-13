#pragma once

// CoAuthRenewTokenAsync command target
class CoAuthRenewTokenAsync : public CCmdTarget
{
public:
	CoAuthRenewTokenAsync();

protected:
	DECLARE_DYNCREATE(CoAuthRenewTokenAsync)
	DECLARE_MESSAGE_MAP()
};

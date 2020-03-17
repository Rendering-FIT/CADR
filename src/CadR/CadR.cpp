#include <CadR/CadR.h>

static bool _leakHandles = false;


bool CadR::leakHandles()
{
	return _leakHandles;
}


void CadR::setLeakHandles(bool v)
{
	_leakHandles=v;
}

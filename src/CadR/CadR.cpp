#include <CadR/CadR.h>
#include <CadR/Renderer.h>
#include <memory>

using namespace std;
using namespace CadR;


void CadR::init()
{
	Renderer::set(make_unique<Renderer>());
}


void CadR::finalize()
{
}

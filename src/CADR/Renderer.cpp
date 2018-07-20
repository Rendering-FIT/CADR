#include <CADR/Renderer.h>

using namespace std;
using namespace cd;


Renderer::Renderer()
	: _nullAttribStorage(new AttribStorage(this,AttribConfig()));
{
}


Renderer::~Renderer()
{
	delete _nullAttribStorage;
}

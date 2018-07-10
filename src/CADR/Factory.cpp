#include <CADR/Factory.h>

using namespace std;
using namespace cd;


static shared_ptr<Factory> _factory;


const shared_ptr<Factory>& Factory::get()
{
	return _factory;
}


void Factory::set(const shared_ptr<Factory>& f)
{
	_factory=f;
}

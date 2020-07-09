#include <CadR/ParentChildList.h>
#include <CadR/StateSet.h>

using namespace CadR;


int main(int,char**)
{
	StateSet ss1,ss2;

	ss1.childList.size();
	ss1.childList.empty();
	ChildList<StateSet,StateSet::parentChildListOffsets>::iterator it1=ss1.childList.append(&ss2);
	ss1.childList.remove(it1);
	ss1.childList.clear();
	for(const StateSet& child : ss1.childList)
		child.renderer();
	ss1.childList.front();
	ss1.childList.back();

	ss2.parentList.size();
	ss2.parentList.empty();
	ParentList<StateSet,StateSet::parentChildListOffsets>::iterator it2=ss2.parentList.append(&ss1);
	ss2.parentList.remove(it2);
	ss2.parentList.clear();
	for(const StateSet& parent : ss2.parentList)
		parent.renderer();
	ss2.parentList.front();
	ss2.parentList.back();

	return 0;
}

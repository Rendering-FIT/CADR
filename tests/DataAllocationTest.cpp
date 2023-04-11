#include <CadR/DataStorage.h>
#include <CadR/Renderer.h>
#include <CadR/VulkanDevice.h>
#include <CadR/VulkanInstance.h>
#include <CadR/VulkanLibrary.h>
#include <sstream>
#include <vector>

using namespace std;
using namespace CadR;


class DataMemoryTest : public DataMemory {
protected:
	void verifyAllocationBlockEmpty() const {
		if(!_allocationBlockList2.empty() && _usedBlock2EndAllocation != _allocationBlockList2.front().allocations.begin()+1)
			throw runtime_error("DataMemory's allocationBlock2 is not empty.");
		if(!_allocationBlockList1.empty() && _usedBlock1EndAllocation != _allocationBlockList1.front().allocations.begin()+1)
			throw runtime_error("DataMemory's allocationBlock1 is not empty.");
	}
public:
	static void verifyAllocationBlockEmpty(const DataMemory& m) {
		if(m.usedBytes() != 0)
			throw runtime_error("Not all memory was released.");
		static_cast<const DataMemoryTest&>(m).verifyAllocationBlockEmpty();
	}
	static void verifyDataStorageEmpty(const DataStorage& ds) {
		for(const DataMemory* m : ds.dataMemoryList()) 
			DataMemoryTest::verifyAllocationBlockEmpty(*m);
	}
};


int main(int,char**)
{
	// init Vulkan
	VulkanLibrary lib;
	lib.load();
	VulkanInstance instance(lib, nullptr, 0, nullptr, 0, VK_API_VERSION_1_2);
	vk::PhysicalDevice physicalDevice;
	uint32_t graphicsQueueFamily;
	tie(physicalDevice, graphicsQueueFamily) = instance.chooseDeviceAndQueueFamily();
	VulkanDevice device(instance, physicalDevice, graphicsQueueFamily, graphicsQueueFamily,
	                    nullptr, nullptr, VulkanDevice::defaultFeatures());
	Renderer r(device, instance, physicalDevice, graphicsQueueFamily);

	// free on nullptr, alloc and destroy single allocation of size 10
	DataStorage ds(r);
	DataMemoryTest::verifyDataStorageEmpty(ds);
	ds.free(nullptr);
	DataAllocation* a = ds.alloc(10, nullptr, nullptr);
	DataMemory& m = *ds.dataMemoryList().front();
	vk::DeviceAddress firstAddress = a->deviceAddress();
	if(firstAddress != m.bufferDeviceAddress())
		throw runtime_error("Allocation is not on the beginning of the buffer");
	ds.free(a);
	ds.free(nullptr);
	DataMemoryTest::verifyDataStorageEmpty(ds);

	// single allocation of size 0..260 - create and destroy
	for(size_t s=0; s<260; s++) {
		a = ds.alloc(s, nullptr, nullptr);
		if(a->deviceAddress() != firstAddress)
			throw runtime_error("Allocation is not on the beginning of the buffer");
		ds.free(a);
		DataMemoryTest::verifyDataStorageEmpty(ds);
	}

	// two allocations of size 0..260, first one released first
	for(size_t s=0; s<260; s++) {
		DataAllocation* a1 = ds.alloc(s, nullptr, nullptr);
		DataAllocation* a2 = ds.alloc(s, nullptr, nullptr);
		if(a1->deviceAddress() != firstAddress)
			throw runtime_error("Allocation is not on the beginning of the buffer");
		size_t offset = (s==0) ? 0 : (s<=16) ? 16 : (s<=32) ? 32 : (s<=48) ? 48 : (s<=64) ? 64 : (s<=128) ? 128 : (s+63)&~63;
		if(a2->deviceAddress() != firstAddress+offset)
			throw runtime_error("Allocation is not on the the proper place in the buffer");
		ds.free(a1);
		ds.free(a2);
		DataMemoryTest::verifyDataStorageEmpty(ds);
	}

	// two allocations of size 0..260, second one released first
	for(size_t s=0; s<260; s++) {
		DataAllocation* a1 = ds.alloc(s, nullptr, nullptr);
		DataAllocation* a2 = ds.alloc(s, nullptr, nullptr);
		if(a1->deviceAddress() != firstAddress)
			throw runtime_error("Allocation is not on the beginning of the buffer");
		size_t offset = (s==0) ? 0 : (s<=16) ? 16 : (s<=32) ? 32 : (s<=48) ? 48 : (s<=64) ? 64 : (s<=128) ? 128 : (s+63)&~63;
		if(a2->deviceAddress() != firstAddress+offset)
			throw runtime_error("Allocation is not on the the proper place in the buffer");
		ds.free(a2);
		ds.free(a1);
		DataMemoryTest::verifyDataStorageEmpty(ds);
	}

	{
		size_t s = 1;
		size_t offset = (s==0) ? 0 : (s<=16) ? 16 : (s<=32) ? 32 : (s<=48) ? 48 : (s<=64) ? 64 : (s<=128) ? 128 : (s+63)&~63;
		for(size_t n=198; n<1000; n++) {
			vector<DataAllocation*> a;
			a.reserve(1000);
			for(size_t i=0; i<n; i++)
				a.push_back(ds.alloc(s, nullptr, nullptr));
			for(size_t i=0; i<n; i++)
				if(a[i]->deviceAddress() != firstAddress+(offset*i))
					throw runtime_error("Allocation is not on the the proper place in the buffer");
			for(size_t i=0; i<n; i++)
				ds.free(a[i]);
			DataMemoryTest::verifyDataStorageEmpty(ds);
		}
	}

	// 0..1000 allocations of size 0..260, released in the order of their allocation
	for(size_t s=0; s<260; s++) {
		size_t offset = (s==0) ? 0 : (s<=16) ? 16 : (s<=32) ? 32 : (s<=48) ? 48 : (s<=64) ? 64 : (s<=128) ? 128 : (s+63)&~63;
		for(size_t n=0; n<1024; n++) {

			// skip the cases that require 64KiB+ memory
			if(offset*n >= 65536)
				continue;

			vector<DataAllocation*> a;
			a.reserve(1000);
			for(size_t i=0; i<n; i++)
				a.push_back(ds.alloc(s, nullptr, nullptr));
			for(size_t i=0; i<n; i++)
				if(a[i]->deviceAddress() != firstAddress+(offset*i))
					throw runtime_error("Allocation is not on the the proper place in the buffer. " +
					                    static_cast<ostringstream&>(ostringstream()
					                    << "(Details: AllocationSize = " << s <<
					                    ", offset = " << offset << ", total number of allocations = "
					                    << n << ", problematic allocation index = " << i <<
					                    " expected allocation place = " << (firstAddress+(offset*i))
					                    << ", real allocation place = " << a[i]->deviceAddress()
					                    << ")").str());
			for(size_t i=0; i<n; i++)
				ds.free(a[i]);
			DataMemoryTest::verifyDataStorageEmpty(ds);
		}
	}

	return 0;
}

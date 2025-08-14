#ifndef CADR_HANDLE_TABLE_HEADER
# define CADR_HANDLE_TABLE_HEADER

# ifndef CADR_NO_INLINE_FUNCTIONS
#  define CADR_NO_INLINE_FUNCTIONS
#  include <CadR/DataAllocation.h>
#  undef CADR_NO_INLINE_FUNCTIONS
# else
#  include <CadR/DataAllocation.h>
# endif

namespace CadR {


class CADR_EXPORT HandleTable {
public:

	static inline constexpr const unsigned numHandlesPerTable = 2048;
	static inline constexpr const unsigned handleBitsLevelShift = 11;
	static inline constexpr const unsigned handleBitsLevelMask = 0x07ff;
	static inline constexpr const unsigned maxHandleLevels = 1;
	static inline constexpr const unsigned minHandleLevels = 1;

protected:

	struct LastLevelTable { // ~16KiB on both CPU and GPU
		HandlelessAllocation allocation;  ///< Allocation of memory for storing copy of addrList on GPU.
		std::array<uint64_t,numHandlesPerTable> addrList;  ///< Device address list.

		LastLevelTable(DataStorage& storage) noexcept;
		void init(HandleTable& handleTable);
		void finalize(HandleTable& handleTable) noexcept;
		bool setValue(unsigned index, uint64_t value);
	};
	struct RoutingTable;
	union Pointer {
		void* value;
		LastLevelTable* lastLevelTable;
		RoutingTable* routingTable;
	};
	struct RoutingTable {  // ~32KiB on CPU, ~16KiB on GPU
		HandlelessAllocation allocation;  ///< Allocation of memory for storing copy of addrList on GPU.
		std::array<uint64_t,numHandlesPerTable> addrList;  ///< Device address list.
		std::array<Pointer,numHandlesPerTable> childTableList;  ///< List of child tables.
		RoutingTable(DataStorage& storage) noexcept;
		void init(HandleTable& handleTable);
		void finalize(HandleTable& handleTable) noexcept;
		bool setValue(unsigned index, uint64_t value);
	};

	union {
		LastLevelTable* _level0 = nullptr;
		RoutingTable* _level1;
		RoutingTable* _level2;
	};
	uint64_t _highestHandle = 0;
	DataStorage* _storage;
	unsigned _handleLevel = 0;

	using CreateHandleFunc = uint64_t (HandleTable::*)();
	CreateHandleFunc _createHandle = &HandleTable::createHandle0;
	using SetHandleFunc = void (HandleTable::*)(uint64_t handle, uint64_t addr);
	SetHandleFunc _setHandle = &HandleTable::setHandle0;
	using RootTableDeviceAddressFunc = uint64_t (HandleTable::*)() const;
	RootTableDeviceAddressFunc _rootTableDeviceAddress = &HandleTable::rootTableDeviceAddress0;

	uint64_t createHandle0();
	uint64_t createHandle1();
	uint64_t createHandle2();
	uint64_t createHandle3();
	void setHandle0(uint64_t handle, uint64_t addr);
	void setHandle1(uint64_t handle, uint64_t addr);
	void setHandle2(uint64_t handle, uint64_t addr);
	void setHandle3(uint64_t handle, uint64_t addr);
	uint64_t rootTableDeviceAddress0() const;
	uint64_t rootTableDeviceAddress1() const;
	uint64_t rootTableDeviceAddress2() const;
	uint64_t rootTableDeviceAddress3() const;

public:
	HandleTable(DataStorage& storage);
	~HandleTable() noexcept;
	inline uint64_t create();
	inline uint64_t create(vk::DeviceAddress deviceAddress);
	void destroy(uint64_t handle) noexcept  { if(handle==0) return; }
	void destroyAll() noexcept;
	inline void set(uint64_t handle, uint64_t addr);
	inline unsigned handleLevel() const;
	inline uint64_t rootTableDeviceAddress() const;
};


}

#endif


// inline methods
#if !defined(CADR_HANDLE_TABLE_INLINE_FUNCTIONS) && !defined(CADR_NO_INLINE_FUNCTIONS)
# define CADR_HANDLE_TABLE_INLINE_FUNCTIONS
namespace CadR {

inline uint64_t HandleTable::create()  { return (this->*_createHandle)(); }
inline uint64_t HandleTable::create(vk::DeviceAddress deviceAddress)  { uint64_t r = create(); set(r, deviceAddress); return r; }
inline void HandleTable::set(uint64_t handle, uint64_t addr)  { (this->*_setHandle)(handle, addr); }
inline unsigned HandleTable::handleLevel() const  { return _handleLevel; }
inline uint64_t HandleTable::rootTableDeviceAddress() const  { return (this->*_rootTableDeviceAddress)(); }

}
#endif

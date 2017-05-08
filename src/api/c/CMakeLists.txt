FIND_PACKAGE(Threads REQUIRED)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
IF (NOT BUILD_VIRT_GUEST)
ADD_LIBRARY(qot SHARED qot.h qot.c)
ELSE ()
add_definitions(-DPARAVIRT_GUEST)
ADD_LIBRARY(qot SHARED qot.h qot.c 
			../../virt/qot_virtguest.c ../../virt/qot_virtguest.h 
			../../virt/pci_mmio/upci.c ../../virt/pci_mmio/upci.h
			../../virt/pci_mmio/qot_pci_ivshmem.c ../../virt/pci_mmio/qot_pci_ivshmem.h)
ENDIF ()
TARGET_LINK_LIBRARIES(qot ${CMAKE_THREAD_LIBS_INIT})
INSTALL(FILES qot.h DESTINATION include COMPONENT headers)
INSTALL(TARGETS qot DESTINATION lib COMPONENT libraries)


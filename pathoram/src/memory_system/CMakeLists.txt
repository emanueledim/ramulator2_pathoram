add_library(ramulator-memorysystem OBJECT)

target_sources(
  ramulator-memorysystem PRIVATE
  bh_memory_system.h
  memory_system.h

  impl/bh_DRAM_system.cpp
  impl/dummy_memory_system.cpp
  impl/generic_DRAM_system.cpp
  impl/path_ORAM_system.cpp

  impl/oram/components/interfaces/iposition_map.h
  impl/oram/components/interfaces/iaddress_logic.h
  impl/oram/components/interfaces/iintegrity_controller.h
  impl/oram/components/interfaces/imee.h
  impl/oram/components/interfaces/ioram_controller.h
  impl/oram/components/interfaces/istash.h
  impl/oram/components/inc/oram_tree_info.h
  impl/oram/oob/bucket.h
  impl/oram/oob/oob_tree.h   impl/oram/oob/oob_tree.cpp
  impl/oram/oram_controller.h      impl/oram/oram_controller.cpp
  impl/oram/components/inc/position_map.h      impl/oram/components/impl/position_map.cpp
  impl/oram/components/inc/stash.h      impl/oram/components/impl/stash.cpp
  impl/oram/components/inc/mee.h
  impl/oram/components/inc/address_logic_double_tree.h      impl/oram/components/impl/address_logic_double_tree.cpp
  impl/oram/components/inc/integrity_controller.h   impl/oram/components/impl/integrity_controller.cpp
  

)

target_link_libraries(
  ramulator
  PRIVATE
  ramulator-memorysystem
)
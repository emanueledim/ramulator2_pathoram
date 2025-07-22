add_library(ramulator-memorysystem OBJECT)

target_sources(
  ramulator-memorysystem PRIVATE
  bh_memory_system.h
  memory_system.h

  impl/bh_DRAM_system.cpp
  impl/dummy_memory_system.cpp
  impl/generic_DRAM_system.cpp
  impl/path_ORAM_system.cpp

  impl/oram/oob/oob_tree.h   impl/oram/oob/oob_tree.cpp
  impl/oram/oram_controller.h      impl/oram/oram_controller.cpp
  impl/oram/components/inc/position_map.h      impl/oram/components/impl/position_map.cpp
  impl/oram/components/inc/stash.h      impl/oram/components/impl/stash.cpp
  impl/oram/components/inc/access_logic.h      impl/oram/components/impl/access_logic.cpp
  impl/oram/components/inc/integrity_checker.h      impl/oram/components/impl/integrity_checker.cpp
  impl/oram/oob/bucket.h

)

target_link_libraries(
  ramulator
  PRIVATE
  ramulator-memorysystem
)
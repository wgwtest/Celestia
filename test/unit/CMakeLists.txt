set(UNIT_TEST_SOURCES
  array_view_test.cpp
  associativearray_test.cpp
  category_test.cpp
  constellation_test.cpp
  greek_test.cpp
  kepler_test.cpp
  logger_test.cpp
  mvc_boundary_test.cpp
  mvc_step2_contract_test.cpp
  mvc_step3_contract_test.cpp
  mvc_step4_directory_contract_test.cpp
  mvc_step5_debug2d_view_test.cpp
  mvc_step5_frontend_contract_test.cpp
  mvc_step5_forwarding_header_removal_test.cpp
  mvc_step5_ipc_contract_test.cpp
  mvc_step5_process_mode_test.cpp
  mvc_step5_process_contract_test.cpp
  mvc_step5_runtime_contract_test.cpp
  mvc_step5_view_frame_test.cpp
  mvc_step5_view_provider_test.cpp
  mvc_step6_protocol_contract_test.cpp
  mvc_step6_host_lifecycle_test.cpp
  mvc_step6_controller_service_test.cpp
  mvc_step6_model_service_test.cpp
  mvc_step6_multiprocess_runtime_test.cpp
  mvc_step6_transport_contract_test.cpp
  mvc_step6_view_service_test.cpp
  mvc_step7_host_process_test.cpp
  mvc_step7_runtime_session_test.cpp
  mvc_step7_runtime_stability_test.cpp
  mvc_step7_sdl_runtime_test.cpp
  mvc_step7_supervisor_resilience_test.cpp
  mvc_step8_scene_extractor_test.cpp
  mvc_step8_scene_protocol_test.cpp
  mvc_step8_runtime_session_3d_test.cpp
  mvc_step8_view3d_host_test.cpp
  mvc_step8_view3d_input_test.cpp
  mvc_step9_view_manifest_test.cpp
  mvc_step9_view_plugin_abi_test.cpp
  mvc_step9_view_plugin_loader_test.cpp
  mvc_step9_view_registry_test.cpp
  mvc_step9_view_switch_test.cpp
  mvc_step10_assembly_config_test.cpp
  mvc_step10_dataplane_contract_test.cpp
  mvc_step10_local_socket_transport_test.cpp
  mvc_step10_runtime_assembly_test.cpp
  mvc_step10_shared_memory_dataplane_test.cpp
  mvc_step10_supervisor_policy_test.cpp
  mvc_step10_transport_factory_test.cpp
  ranges_test.cpp
  stellarclass_test.cpp
  strnatcmp_test.cpp
  tokenizer_test.cpp)

if(USE_ICU)
  list(APPEND UNIT_TEST_SOURCES unicode_test.cpp)
else()
  list(APPEND UNIT_TEST_SOURCES formatnum_test.cpp)
endif()

if(WIN32)
  list(APPEND UNIT_TEST_SOURCES winutil_test.cpp)
endif()

test_case(unit "${UNIT_TEST_SOURCES}")

target_sources(unit PRIVATE
  "${PROJECT_SOURCE_DIR}/src/celruntime/process/view3dhostloop.cpp"
)

add_library(celestia-test-view-plugin SHARED
  mvc_step9_test_view_plugin.cpp
)
target_include_directories(celestia-test-view-plugin PRIVATE
  "${PROJECT_SOURCE_DIR}/src"
)
target_compile_definitions(celestia-test-view-plugin PRIVATE
  CELESTIA_VIEW_PLUGIN_BUILD
)
target_compile_definitions(unit PRIVATE
  CELESTIA_TEST_VIEW_PLUGIN_PATH="$<TARGET_FILE:celestia-test-view-plugin>"
)
add_dependencies(unit celestia-test-view-plugin)

if(TARGET celestia-model-host AND TARGET celestia-controller-host AND TARGET celestia-view-host)
  add_dependencies(unit celestia-model-host celestia-controller-host celestia-view-host)
endif()

if(TARGET celestia-view3d-host)
  add_dependencies(unit celestia-view3d-host)
endif()

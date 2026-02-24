# Code Coverage Report

*Generated from Cobertura.xml*

## Summary

### Overall Coverage

| Metric | Covered | Valid | Percentage |
|--------|---------|-------|------------|
| Lines | 5045 | 14859 | 33.95% |
| Branches | 2425 | 12281 | 19.75% |
| Functions | 438 | 1239 | 35.35% |

### Metric Totals

| Metric | Definition | Count |
|--------|------------|-------|
| A | Files with coverage = 0% | 54 |
| B | Files with coverage > 0% and < 85% | 49 |
| C | Files with coverage >= 85% | 19 |
| **Total** | All files under src/ | 122 |

### Coverage by Subfolder

| Subfolder | Total Files | 0% (A) | >0% & <85% (B) | >=85% (C) | Hit Lines | Total Lines | Coverage % |
|-----------|-------------|--------|----------------|-----------|-----------|-------------|------------|
| /src/adu-shell | 6 | 6 | 0 | 0 | 0 | 738 | 0.00% |
| /src/adu_types | 1 | 0 | 1 | 0 | 46 | 158 | 29.11% |
| /src/adu_workflow | 1 | 1 | 0 | 0 | 0 | 1408 | 0.00% |
| /src/agent | 11 | 9 | 2 | 0 | 278 | 3344 | 8.31% |
| /src/agent_orchestration | 1 | 1 | 0 | 0 | 0 | 38 | 0.00% |
| /src/communication_abstraction | 1 | 1 | 0 | 0 | 0 | 208 | 0.00% |
| /src/communication_managers | 1 | 1 | 0 | 0 | 0 | 640 | 0.00% |
| /src/diagnostics_component | 11 | 6 | 3 | 2 | 446 | 1198 | 37.23% |
| /src/extensions | 26 | 14 | 9 | 3 | 1116 | 5572 | 20.03% |
| /src/inc | 1 | 0 | 1 | 0 | 76 | 188 | 40.43% |
| /src/logging | 2 | 0 | 2 | 0 | 232 | 438 | 52.97% |
| /src/platform_layers | 4 | 1 | 3 | 0 | 64 | 980 | 6.53% |
| /src/rootkey_workflow | 1 | 0 | 1 | 0 | 48 | 162 | 29.63% |
| /src/sdk | 1 | 1 | 0 | 0 | 0 | 310 | 0.00% |
| /src/utils | 53 | 13 | 26 | 14 | 7722 | 14248 | 54.20% |
| /src/viewstatemgr | 1 | 0 | 1 | 0 | 62 | 74 | 83.78% |

---

## Details

### /src/adu-shell

| Metric | Value |
|--------|-------|
| Total Files | 6 |
| Files at 0% | 6 |
| Files >0% & <85% | 0 |
| Files >=85% | 0 |
| Hit Lines | 0 |
| Total Lines | 738 |
| Coverage | 0.00% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/adu-shell/inc/adushell.hpp | 0 | 18 | 0.00% | 🔴 0% |
| src/adu-shell/src/adushell_action.cpp | 0 | 32 | 0.00% | 🔴 0% |
| src/adu-shell/src/aptget_tasks.cpp | 0 | 204 | 0.00% | 🔴 0% |
| src/adu-shell/src/common_tasks.cpp | 0 | 54 | 0.00% | 🔴 0% |
| src/adu-shell/src/main.cpp | 0 | 324 | 0.00% | 🔴 0% |
| src/adu-shell/src/script_tasks.cpp | 0 | 106 | 0.00% | 🔴 0% |

### /src/adu_types

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 0 |
| Files >0% & <85% | 1 |
| Files >=85% | 0 |
| Hit Lines | 46 |
| Total Lines | 158 |
| Coverage | 29.11% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/adu_types/src/adu_types.c | 46 | 158 | 29.11% | 🟡 <85% |

### /src/adu_workflow

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 1 |
| Files >0% & <85% | 0 |
| Files >=85% | 0 |
| Hit Lines | 0 |
| Total Lines | 1408 |
| Coverage | 0.00% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/adu_workflow/src/agent_workflow.c | 0 | 1408 | 0.00% | 🔴 0% |

### /src/agent

| Metric | Value |
|--------|-------|
| Total Files | 11 |
| Files at 0% | 9 |
| Files >0% & <85% | 2 |
| Files >=85% | 0 |
| Hit Lines | 278 |
| Total Lines | 3344 |
| Coverage | 8.31% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/agent/adu_core_export_helpers/src/adu_core_export_helpers.c | 0 | 70 | 0.00% | 🔴 0% |
| src/agent/adu_core_interface/src/adu_core_interface.c | 0 | 716 | 0.00% | 🔴 0% |
| src/agent/adu_core_interface/src/device_properties.c | 52 | 164 | 31.71% | 🟡 <85% |
| src/agent/adu_core_interface/src/startup_msg_helper.c | 0 | 88 | 0.00% | 🔴 0% |
| src/agent/api/src/apisvc.c | 226 | 394 | 57.36% | 🟡 <85% |
| src/agent/command_helper/src/command_helper.c | 0 | 354 | 0.00% | 🔴 0% |
| src/agent/device_info_interface/src/device_info_interface.c | 0 | 146 | 0.00% | 🔴 0% |
| src/agent/pnp_helper/src/pnp_protocol.c | 0 | 220 | 0.00% | 🔴 0% |
| src/agent/shutdown_service/src/shutdown_service.c | 0 | 10 | 0.00% | 🔴 0% |
| src/agent/src/health_management.c | 0 | 430 | 0.00% | 🔴 0% |
| src/agent/src/main.c | 0 | 752 | 0.00% | 🔴 0% |

### /src/agent_orchestration

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 1 |
| Files >0% & <85% | 0 |
| Files >=85% | 0 |
| Hit Lines | 0 |
| Total Lines | 38 |
| Coverage | 0.00% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/agent_orchestration/src/agent_orchestration.c | 0 | 38 | 0.00% | 🔴 0% |

### /src/communication_abstraction

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 1 |
| Files >0% & <85% | 0 |
| Files >=85% | 0 |
| Hit Lines | 0 |
| Total Lines | 208 |
| Coverage | 0.00% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/communication_abstraction/src/client_handle_helper.c | 0 | 208 | 0.00% | 🔴 0% |

### /src/communication_managers

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 1 |
| Files >0% & <85% | 0 |
| Files >=85% | 0 |
| Hit Lines | 0 |
| Total Lines | 640 |
| Coverage | 0.00% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/communication_managers/iothub_communication_manager/src/iothub_communication_manager.c | 0 | 640 | 0.00% | 🔴 0% |

### /src/diagnostics_component

| Metric | Value |
|--------|-------|
| Total Files | 11 |
| Files at 0% | 6 |
| Files >0% & <85% | 3 |
| Files >=85% | 2 |
| Hit Lines | 446 |
| Total Lines | 1198 |
| Coverage | 37.23% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/diagnostics_component/diagnostics_async_helper/src/diagnostics_async_helper.cpp | 0 | 68 | 0.00% | 🔴 0% |
| src/diagnostics_component/diagnostics_devicename/src/diagnostics_devicename.c | 38 | 46 | 82.61% | 🟡 <85% |
| src/diagnostics_component/diagnostics_interface/src/diagnostics_interface.c | 0 | 206 | 0.00% | 🔴 0% |
| src/diagnostics_component/diagnostics_workflow/src/diagnostics_result.c | 42 | 42 | 100.00% | ✅ >=85% |
| src/diagnostics_component/diagnostics_workflow/src/diagnostics_workflow.c | 0 | 296 | 0.00% | 🔴 0% |
| src/diagnostics_component/utils/config_utils/src/diagnostics_config_utils.c | 156 | 190 | 82.11% | 🟡 <85% |
| src/diagnostics_component/utils/file_info_utils/src/file_info_utils.c | 176 | 200 | 88.00% | ✅ >=85% |
| src/diagnostics_component/utils/file_upload_utils/src/blob_storage_helper.cpp | 0 | 66 | 0.00% | 🔴 0% |
| src/diagnostics_component/utils/file_upload_utils/src/blob_storage_helper.hpp | 0 | 2 | 0.00% | 🔴 0% |
| src/diagnostics_component/utils/file_upload_utils/src/file_upload_utility.cpp | 0 | 22 | 0.00% | 🔴 0% |
| src/diagnostics_component/utils/operation_id_utils/src/operation_id_utils.c | 34 | 60 | 56.67% | 🟡 <85% |

### /src/extensions

| Metric | Value |
|--------|-------|
| Total Files | 26 |
| Files at 0% | 14 |
| Files >0% & <85% | 9 |
| Files >=85% | 3 |
| Hit Lines | 1116 |
| Total Lines | 5572 |
| Coverage | 20.03% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/extensions/component_enumerators/examples/contoso_component_enumerator/contoso_component_enumerator.cpp | 0 | 164 | 0.00% | 🔴 0% |
| src/extensions/content_downloaders/curl_downloader/curl_content_downloader.EXPORTS.cpp | 0 | 16 | 0.00% | 🔴 0% |
| src/extensions/content_downloaders/curl_downloader/curl_content_downloader.cpp | 0 | 156 | 0.00% | 🔴 0% |
| src/extensions/download_handlers/download_handler_factory/inc/aduc/download_handler_factory.hpp | 0 | 2 | 0.00% | 🔴 0% |
| src/extensions/download_handlers/download_handler_factory/src/download_handler_factory.cpp | 0 | 64 | 0.00% | 🔴 0% |
| src/extensions/download_handlers/download_handler_plugin/src/download_handler_plugin.cpp | 0 | 206 | 0.00% | 🔴 0% |
| src/extensions/extension_manager/src/extension_manager.cpp | 150 | 854 | 17.56% | 🟡 <85% |
| src/extensions/extension_manager/src/extension_manager_helper.cpp | 16 | 110 | 14.55% | 🟡 <85% |
| src/extensions/inc/aduc/content_handler.hpp | 8 | 18 | 44.44% | 🟡 <85% |
| src/extensions/shared_lib/inc/aduc/plugin_call_helper.hpp | 0 | 240 | 0.00% | 🔴 0% |
| src/extensions/shared_lib/inc/aduc/plugin_exception.hpp | 0 | 10 | 0.00% | 🔴 0% |
| src/extensions/shared_lib/src/shared_lib.cpp | 0 | 46 | 0.00% | 🔴 0% |
| src/extensions/step_handlers/apt_handler/inc/aduc/apt_handler.hpp | 0 | 6 | 0.00% | 🔴 0% |
| src/extensions/step_handlers/apt_handler/inc/aduc/apt_parser.hpp | 0 | 14 | 0.00% | 🔴 0% |
| src/extensions/step_handlers/apt_handler/src/apt_handler.cpp | 0 | 452 | 0.00% | 🔴 0% |
| src/extensions/step_handlers/apt_handler/src/apt_parser.cpp | 70 | 100 | 70.00% | 🟡 <85% |
| src/extensions/step_handlers/script_handler/inc/aduc/script_handler.hpp | 6 | 6 | 100.00% | ✅ >=85% |
| src/extensions/step_handlers/script_handler/src/script_handler.cpp | 236 | 706 | 33.43% | 🟡 <85% |
| src/extensions/step_handlers/simulator_handler/inc/aduc/simulator_handler.hpp | 6 | 6 | 100.00% | ✅ >=85% |
| src/extensions/step_handlers/simulator_handler/src/simulator_handler.cpp | 226 | 286 | 79.02% | 🟡 <85% |
| src/extensions/step_handlers/swupdate_handler_v2/inc/aduc/swupdate_handler_v2.hpp | 6 | 6 | 100.00% | ✅ >=85% |
| src/extensions/step_handlers/swupdate_handler_v2/src/handler_create.cpp | 8 | 28 | 28.57% | 🟡 <85% |
| src/extensions/step_handlers/swupdate_handler_v2/src/swupdate_handler_v2.cpp | 316 | 864 | 36.57% | 🟡 <85% |
| src/extensions/update_manifest_handlers/steps_handler/inc/aduc/steps_handler.hpp | 0 | 6 | 0.00% | 🔴 0% |
| src/extensions/update_manifest_handlers/steps_handler/src/handler_create.cpp | 0 | 26 | 0.00% | 🔴 0% |
| src/extensions/update_manifest_handlers/steps_handler/src/steps_handler.cpp | 68 | 1180 | 5.76% | 🟡 <85% |

### /src/inc

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 0 |
| Files >0% & <85% | 1 |
| Files >=85% | 0 |
| Hit Lines | 76 |
| Total Lines | 188 |
| Coverage | 40.43% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/inc/aduc/result.h | 76 | 188 | 40.43% | 🟡 <85% |

### /src/logging

| Metric | Value |
|--------|-------|
| Total Files | 2 |
| Files at 0% | 0 |
| Files >0% & <85% | 2 |
| Files >=85% | 0 |
| Hit Lines | 232 |
| Total Lines | 438 |
| Coverage | 52.97% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/logging/zlog/src/init.c | 30 | 70 | 42.86% | 🟡 <85% |
| src/logging/zlog/src/zlog.c | 202 | 368 | 54.89% | 🟡 <85% |

### /src/platform_layers

| Metric | Value |
|--------|-------|
| Total Files | 4 |
| Files at 0% | 1 |
| Files >0% & <85% | 3 |
| Files >=85% | 0 |
| Hit Lines | 64 |
| Total Lines | 980 |
| Coverage | 6.53% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/platform_layers/linux_platform_layer/src/linux_adu_core_exports.cpp | 18 | 78 | 23.08% | 🟡 <85% |
| src/platform_layers/linux_platform_layer/src/linux_adu_core_impl.cpp | 44 | 314 | 14.01% | 🟡 <85% |
| src/platform_layers/linux_platform_layer/src/linux_adu_core_impl.hpp | 2 | 260 | 0.77% | 🟡 <85% |
| src/platform_layers/linux_platform_layer/src/linux_device_info_exports.cpp | 0 | 328 | 0.00% | 🔴 0% |

### /src/rootkey_workflow

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 0 |
| Files >0% & <85% | 1 |
| Files >=85% | 0 |
| Hit Lines | 48 |
| Total Lines | 162 |
| Coverage | 29.63% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/rootkey_workflow/src/rootkey_workflow.c | 48 | 162 | 29.63% | 🟡 <85% |

### /src/sdk

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 1 |
| Files >0% & <85% | 0 |
| Files >=85% | 0 |
| Hit Lines | 0 |
| Total Lines | 310 |
| Coverage | 0.00% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/sdk/src/aducsdk.c | 0 | 310 | 0.00% | 🔴 0% |

### /src/utils

| Metric | Value |
|--------|-------|
| Total Files | 53 |
| Files at 0% | 13 |
| Files >0% & <85% | 26 |
| Files >=85% | 14 |
| Hit Lines | 7722 |
| Total Lines | 14248 |
| Coverage | 54.20% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/utils/apiproto_utils/src/apiproto.c | 214 | 342 | 62.57% | 🟡 <85% |
| src/utils/auto_utils/inc/aduc/defer.hpp | 12 | 12 | 100.00% | ✅ >=85% |
| src/utils/c_utils/inc/aduc/bit_ops.h | 4 | 4 | 100.00% | ✅ >=85% |
| src/utils/c_utils/src/connection_string_utils.c | 78 | 86 | 90.70% | ✅ >=85% |
| src/utils/c_utils/src/string_c_utils.c | 220 | 346 | 63.58% | 🟡 <85% |
| src/utils/config_utils/inc/aduc/config_utils.h | 8 | 66 | 12.12% | 🟡 <85% |
| src/utils/config_utils/src/config_parsefile.c | 4 | 4 | 100.00% | ✅ >=85% |
| src/utils/config_utils/src/config_utils.c | 456 | 736 | 61.96% | 🟡 <85% |
| src/utils/contract_utils/src/contract_utils.c | 6 | 6 | 100.00% | ✅ >=85% |
| src/utils/crypto_utils/src/base64_utils.c | 136 | 158 | 86.08% | ✅ >=85% |
| src/utils/crypto_utils/src/crypto_lib.c | 432 | 668 | 64.67% | 🟡 <85% |
| src/utils/d2c_messaging/src/d2c_messaging.c | 0 | 432 | 0.00% | 🔴 0% |
| src/utils/eis_utils/inc/eis_coms.h | 24 | 282 | 8.51% | 🟡 <85% |
| src/utils/eis_utils/src/eis_coms.c | 0 | 340 | 0.00% | 🔴 0% |
| src/utils/eis_utils/src/eis_err.c | 0 | 68 | 0.00% | 🔴 0% |
| src/utils/eis_utils/src/eis_utils.c | 334 | 492 | 67.89% | 🟡 <85% |
| src/utils/entity_utils/inc/aduc/auto_file_entity.hpp | 8 | 8 | 100.00% | ✅ >=85% |
| src/utils/exception_utils/inc/aduc/exception_utils.hpp | 0 | 104 | 0.00% | 🔴 0% |
| src/utils/exception_utils/inc/aduc/exceptions.hpp | 0 | 8 | 0.00% | 🔴 0% |
| src/utils/extension_utils/src/extension_utils.c | 0 | 436 | 0.00% | 🔴 0% |
| src/utils/file_utils/src/auto_opendir.cpp | 0 | 30 | 0.00% | 🔴 0% |
| src/utils/file_utils/src/file_utils.cpp | 0 | 42 | 0.00% | 🔴 0% |
| src/utils/hash_utils/src/hash_utils.c | 240 | 362 | 66.30% | 🟡 <85% |
| src/utils/installed_criteria_utils/src/installed_criteria_utils.cpp | 150 | 162 | 92.59% | ✅ >=85% |
| src/utils/jws_utils/src/jws_utils.c | 534 | 792 | 67.42% | 🟡 <85% |
| src/utils/parser_utils/src/parser_utils.c | 186 | 278 | 66.91% | 🟡 <85% |
| src/utils/parson_json_utils/src/parson_json_utils.c | 110 | 156 | 70.51% | 🟡 <85% |
| src/utils/path_utils/src/path_utils.c | 78 | 84 | 92.86% | ✅ >=85% |
| src/utils/permission_utils/src/permission_utils.c | 16 | 102 | 15.69% | 🟡 <85% |
| src/utils/process_utils/src/process_utils.cpp | 130 | 168 | 77.38% | 🟡 <85% |
| src/utils/process_utils/test_helper/main.cpp | 0 | 78 | 0.00% | 🔴 0% |
| src/utils/reporting_utils/src/reporting_utils.c | 46 | 56 | 82.14% | 🟡 <85% |
| src/utils/retry_utils/src/retry_utils.c | 0 | 22 | 0.00% | 🔴 0% |
| src/utils/root_key_utils/inc/root_key_list.h | 16 | 80 | 20.00% | 🟡 <85% |
| src/utils/root_key_utils/inc/root_key_store.h | 8 | 40 | 20.00% | 🟡 <85% |
| src/utils/root_key_utils/src/root_key_list.c | 8 | 8 | 100.00% | ✅ >=85% |
| src/utils/root_key_utils/src/root_key_store.c | 0 | 4 | 0.00% | 🔴 0% |
| src/utils/root_key_utils/src/root_key_util.c | 428 | 772 | 55.44% | 🟡 <85% |
| src/utils/rootkeypackage_utils/src/rootkeypackage_curl_download.cpp | 0 | 60 | 0.00% | 🔴 0% |
| src/utils/rootkeypackage_utils/src/rootkeypackage_download.c | 68 | 114 | 59.65% | 🟡 <85% |
| src/utils/rootkeypackage_utils/src/rootkeypackage_parse.c | 592 | 904 | 65.49% | 🟡 <85% |
| src/utils/rootkeypackage_utils/src/rootkeypackage_utils.c | 388 | 574 | 67.60% | 🟡 <85% |
| src/utils/string_utils/inc/aduc/calloc_wrapper.hpp | 64 | 64 | 100.00% | ✅ >=85% |
| src/utils/string_utils/inc/aduc/string_handle_wrapper.hpp | 38 | 38 | 100.00% | ✅ >=85% |
| src/utils/string_utils/src/string_utils.cpp | 56 | 56 | 100.00% | ✅ >=85% |
| src/utils/system_utils/src/system_utils.c | 398 | 584 | 68.15% | 🟡 <85% |
| src/utils/test_utils/src/auto_dir.cpp | 0 | 24 | 0.00% | 🔴 0% |
| src/utils/test_utils/src/file_test_utils.cpp | 20 | 20 | 100.00% | ✅ >=85% |
| src/utils/timer_utils/src/timer.c | 124 | 154 | 80.52% | 🟡 <85% |
| src/utils/url_utils/src/https_proxy_utils.c | 180 | 214 | 84.11% | 🟡 <85% |
| src/utils/url_utils/src/url_utils.c | 64 | 76 | 84.21% | 🟡 <85% |
| src/utils/workflow_data_utils/src/workflow_data_utils.c | 4 | 64 | 6.25% | 🟡 <85% |
| src/utils/workflow_utils/src/workflow_utils.c | 1840 | 3468 | 53.06% | 🟡 <85% |

### /src/viewstatemgr

| Metric | Value |
|--------|-------|
| Total Files | 1 |
| Files at 0% | 0 |
| Files >0% & <85% | 1 |
| Files >=85% | 0 |
| Hit Lines | 62 |
| Total Lines | 74 |
| Coverage | 83.78% |

| File | Hit Lines | Total Lines | Coverage | Flag |
|------|-----------|-------------|----------|------|
| src/viewstatemgr/src/viewstatemgr.c | 62 | 74 | 83.78% | 🟡 <85% |

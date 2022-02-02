#! /usr/bin/env python3

import getopt
import json
import os
from platform import version
import sys

error_code_definition_file = """\
/**
* @file result.h
* @brief Describes the ADUC result type. Generated from result_codes.json by ErrorCodeDefinitionGenerator.py
*
* @copyright Copyright (c) Microsoft Corporation.
* Licensed under the MIT License.
*/
#ifndef ADUC_RESULT_H
#define ADUC_RESULT_H

#include <stdbool.h> // _Bool
#include <stdint.h> // int32_t

/**
* @brief Defines the type of an ADUC_Result.
*/
typedef int32_t ADUC_Result_t;

/**
* @brief Defines an ADUC_Result object which is used to indicate status.
*/
typedef struct tagADUC_Result
{{
    ADUC_Result_t ResultCode; /**< Method-specific result. Value > 0 indicates success. */
    ADUC_Result_t ExtendedResultCode; /**< Implementation-specific extended result code. */
}} ADUC_Result;

/**
* @brief Return value for general API calls.
*/
typedef enum tagADUC_GeneralResult
{{
    ADUC_GeneralResult_Failure = 0, /**< Failed. */
    ADUC_GeneralResult_Success = 1, /**< Succeeded. */
}} ADUC_GeneralResult;

/**
* @brief Determines if a result code is succeeded.
*/
static inline _Bool IsAducResultCodeSuccess(const ADUC_Result_t resultCode)
{{
    return (resultCode > 0);
}}

/**
* @brief Determines if a result code is failed.
*/
static inline _Bool IsAducResultCodeFailure(const ADUC_Result_t resultCode)
{{
    return (resultCode <= 0);
}}

/**
* Extended Result Code Structure (32 bits)
*
* Note: We're discussing a possibility to increase the size to 64 bits.
*
*    0 00 00000     Total 4 bytes (32 bits)
*    - -- -----
*    | |  |
*    | |  |
*    | |  +---------  Error code (20 bits)
*    | |
*    | +------------- Component/Area code (8 bits)
*    |
*    +--------------- Facility code (4 bits)
*/
static inline ADUC_Result_t
MAKE_ADUC_EXTENDEDRESULTCODE(const unsigned int facility, const unsigned int component, const unsigned int value)
{{
    return ((facility & 0xF) << 0x1C) | ((component & 0xFF) << 0x14) | (value & 0xFFFFF);
}}

//
// Facility and Component Code Definitions
//
{FacilityAndComponentEnums}
//
// Extended Result Code Make Functions
//
{ExtendedResultCodeGenerationFunctions}
//
// Extended Result Code Definitions
//
{ExtendedResultCodeDefines}
//
// STATIC FUNCTIONS, NOT GENERATED BUT USE GENERATED FUNCTIONS
//


/**
 * @brief Creates an extended error code for errors from delta processor API.
 * The facility code for these errors is ADUC_FACILITY_DOWNLOAD_HANDLER.
 * The component code for these errors is ADUC_COMPONENT_DELTA_DOWNLOAD_HANDLER_DELTA_PROCESSOR.
 *
 * @param errorCode The error code.
 */
#define MAKE_DELTA_PROCESSOR_EXTENDEDRESULTCODE(errorCode) MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_COMPONENT_DELTA_DOWNLOAD_HANDLER_DELTA_PROCESSOR(errorCode)

//
// Delivery Optimization HRESULT to Extended Result Code Functions
//

/**
 * @brief Macros to convert Delivery Optimization results to extended result code values.
 */
static inline ADUC_Result_t MAKE_ADUC_DELIVERY_OPTIMIZATION_EXTENDEDRESULTCODE(const int32_t value)
{{
    return ((ADUC_FACILITY_DELIVERY_OPTIMIZATION & 0xF) << 0x1C) | (value & 0xFFFFFFF);
}}

//
// Reserved extension common error codes (0-300)
//

#define ADUC_ERC_EXTENSION_ERROR_NONE(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 0)

#define ADUC_ERC_EXTENSION_CREATE_FAILURE_INVALID_ARG(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 1)

#define ADUC_ERC_EXTENSION_CREATE_FAILURE_UNKNOWN(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 2)

#define ADUC_ERC_EXTENSION_CREATE_FAILURE_NOT_FOUND(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 3)

#define ADUC_ERC_EXTENSION_CREATE_FAILURE_VALIDATE(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 4)

#define ADUC_ERC_EXTENSION_CREATE_FAILURE_LOAD(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 5)

#define ADUC_ERC_EXTENSION_FAILURE_REQUIRED_FUNCTION_NOTIMPL(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 6)

#define ADUC_ERC_EXTENSION_CREATE_FAILURE_CREATE(facility, component) MAKE_ADUC_EXTENDEDRESULTCODE(facility, component, 7)

//
// Child process error code generators
//

#define ADUC_ERC_SWUPDATE_HANDLER_CHILD_FAILURE_PROCESS_EXITCODE(exitCode) MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_CONTENT_HANDLER_SWUPDATE((0x1000 + exitCode))

#define ADUC_ERC_SCRIPT_HANDLER_CHILD_PROCESS_FAILURE_EXITCODE(exitCode) MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_CONTENT_HANDLER_SCRIPT((0x1000 + exitCode))

#define ADUC_ERROR_DELIVERY_OPTIMIZATION_DOWNLOADER_EXTERNAL_FAILURE(exitCode) MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_CONTENT_DOWNLOADER_DELIVERY_OPTIMIZATION((0x1000 + exitCode))

#define ADUC_ERROR_CURL_DOWNLOADER_EXTERNAL_FAILURE(exitCode) MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_ADUC_CONTENT_DOWNLOADER_CURL_DOWNLOADER((0x1000 + exitCode))

#endif // ADUC_RESULT_H
"""

facility_func_definition = """\
/**
 * @brief Extended Result Codes for {FacilityName} Facility
*/
static inline ADUC_Result_t {FacilityFuncDeclString}(const unsigned int component, const int32_t value)
{{
    return MAKE_ADUC_EXTENDEDRESULTCODE({FacilityName}, component, value);
}}
"""

component_func_definition = """\
/**
* @brief Function for generating Extended Result Codes for {ComponentName} Component
*/
static inline ADUC_Result_t {ComponentDecl}(const int32_t value)
{{
    return {FacilityFuncDeclString}({ComponentName}, value);
}}
"""


tabsize = 2

def CreateBriefDoxyString(brief):
    """
    Returns a doxygen style comment with the passed brief
    """
    doxygen_string = f"""\
/**
 * @brief {brief}
 */
 """

    return doxygen_string


def CreateEnumDoxyString(brief):
    """
    Creates an enum doxygen documentation string with the passed brief
    """
    return f"//!< {brief}"



def MakeERC(facility, component, result):
    """
    Creates an ExtendedResultCode from the facility, component, and result

    Returns the ExtendedResultCode value calculated using the facility, component, and result codes
    """
    return ((facility & 0xF) << 0x1C) | ((component & 0xFF) << 0x14) | (0xFFFFF & result)


class FacilityCode(object):
    def __init__(self, _name, _value, _doc_string=""):
        self.name = _name
        self.value = _value
        self.doc_string = _doc_string
        self.components = []

    def __eq__(self, __o):

        if (isinstance(__o, FacilityCode)):
            if (self.value == __o.value or self.name == __o.name):
                return True
            return False

    def add_component(self,component):
        if (component not in self.components):
            self.components.append(component)
            return True
        return False

    def get_facility_name(self):
        return self.name

    def get_components(self):
        return self.components

    def get_brief_doxy_comment(self):
        brief = f"{self.name} : {self.value}, {self.doc_string}"
        return CreateBriefDoxyString(brief)

    def get_enum_doxy_comment(self):
        brief = f"{self.name} : {self.value}, {self.doc_string}"
        return CreateEnumDoxyString(brief)

    def get_c_define(self, define_offset=0):
        offset = define_offset*" "
        return f"#{offset}define {self.name} {self.value}\n"

    def get_enum_define_with_comment(self, define_offset=2):
        offset = define_offset*" "
        return f"{offset}{self.name}={self.value}, {self.get_enum_doxy_comment()}\n"

    def get_func_def_string(self):
        return facility_func_definition.format(FacilityName=self.name, FacilityFuncDeclString=self.get_facility_func_decl_string())

    def get_facility_func_decl_string(self):
        return f"MAKE_ADUC_EXTENDEDRESULTCODE_FOR_FACILITY_{self.name}"


class ComponentCode(object):
    def __init__(self, _name, _value, _doc_string, _facility_code):
        self.name = _name
        self.value = _value
        self.doc_string = _doc_string
        self.facility_code = _facility_code

        self.results = []

    def __eq__(self, __o):

        if (isinstance(__o, ComponentCode)):
            if ((self.value == __o.value and self.facility_code == __o.facility_code) or self.name == __o.name):
                return True
        return False

    def add_result(self, result):
        if (result not in self.results):
            self.results.append(result)
            return True
        return False

    def get_component_name(self):
        return self.name

    def get_results(self):
        return self.results

    def get_doxy_comment(self):
        brief = self.name + " : " + str(self.value) + ", " + self.doc_string
        return CreateBriefDoxyString(brief)

    def get_enum_doxy_comment(self):
        brief = self.name + " : " + str(self.value) + ", " + self.doc_string
        return CreateEnumDoxyString(brief)

    def get_c_define(self, define_offset=0):
        offset = define_offset*" "
        return f"#{offset}define {self.name} {self.value}\n"

    def get_enum_define_with_comment(self, define_offset=2):
        offset = define_offset*" "
        return f"{offset}{self.name}={self.value}, {self.get_enum_doxy_comment()}\n"

    def get_func_def_string(self, facility_func_decl):
        return component_func_definition.format(ComponentName=self.name, ComponentDecl=self.get_component_func_decl_string(), FacilityFuncDeclString=facility_func_decl)

    def get_component_func_decl_string(self):
        return f"MAKE_ADUC_EXTENDEDRESULTCODE_FOR_COMPONENT_{self.name}"


class ResultCode(object):
    def __init__(self, _name, _rc_value, _fc_value, _cc_value):
        self.name = _name
        self.rc_value = _rc_value
        self.full_erc_value = MakeERC(_fc_value, _cc_value, _rc_value)

    def __eq__(self, __o):

        if (isinstance(__o, ResultCode)):
            if (self.full_erc_value == __o.full_erc_value or self.name == __o.name):
                return True
        return False

    def get_doxy_comment(self):
        hex_string = hex(self.full_erc_value)
        brief = f"{self.name}, ERC Value: {self.full_erc_value} ({hex_string})"
        return CreateBriefDoxyString(brief)

    def get_c_definition(self, component_func_decl, define_offset=0):
        offset = define_offset*" "
        return f"#{offset}define {self.name} {component_func_decl}({self.rc_value})\n"

    def get_result_name(self):
        return self.name


class ErrorCodeDefinitionGenerator(object):
    def __init__(self, _json_path, _result_h_path="./result.h", *args):
        """
        Initializer for the class that generates the result.h file from result_codes.json

        :param str: _json_path path to the json file defining the facilities, components, and results
        :param str: _result_h_path path to generate the result.h file at
        """
        super(ErrorCodeDefinitionGenerator, self).__init__(*args)
        self.result_h_path = _result_h_path
        self.json_path = _json_path
        self.facilities = []

    def add_facility_code(self, facility):
        """
        Adds a facility code to the list of facilities if it does not already exist in the list

        Returns true if the facility is added; false otherwise
        """
        if (facility not in self.facilities):
            self.facilities.append(facility)
            return True
        return False

    def get_facility_enum_def(self):
        """
        Processes the facilities and creates the C enum for defining these facilities

        Returns the string version of the C enum for the facilities
        """
        facility_enum_body_string = ""

        # For building facilities enum
        for facility in self.facilities:

            facility_enum_body_string += facility.get_enum_define_with_comment(
                tabsize*2)

        full_facility_enum = f"""\
/**
 * @brief Facility codes to pass to MAKE_ADUC_EXTENDEDRESULTCODE.
 */
typedef enum tagADUC_Facility
{{
{facility_enum_body_string}}} ADUC_Facility;
"""

        return full_facility_enum

    def get_component_enum_for_facility(self, facility):
        """
        Creates the enumeration of components for the passed facility

        Returns the string definition of a C enum for the components in the facility
        """
        component_enum_body_string = ""

        components = facility.get_components()

        for component in components:
            component_enum_body_string += component.get_enum_define_with_comment(
                tabsize*2)

        f_name = facility.get_facility_name()
        full_component_body_string = f"""\
typedef enum tag{f_name}_Components
{{
{component_enum_body_string}}} {f_name}_Components;

"""

        return full_component_body_string

    def get_facility_and_component_code_defs(self):
        """
        Takes in the facilities and components and creates the enums which define the facilities and components

        Returns the string version of the C definitions of the facilities and components
        """
        f_and_c_def_string = ""

        facility_enum = self.get_facility_enum_def()

        if (facility_enum == ""):
            return ""

        component_enums = ""
        # For building Components Enum
        for facility in self.facilities:

            if (len(facility.get_components()) != 0):

                component_enum = self.get_component_enum_for_facility(facility)

                if (component_enum == ""):
                    return ""

                component_enums += component_enum

        return f"{facility_enum}\n{component_enums}"

    def get_erc_make_functions(self):
        """
        Takes in the facilities and components and generates the static inline function for creating the ExtendedResultCodes

        Returns a string version of the C macro definitions for creating the ExtendedResultCodes
        """
        erc_make_functions = ""

        for facility in self.facilities:

            facility_make_erc_function = facility.get_func_def_string()

            erc_make_functions += f"{facility_make_erc_function}\n"

            # Add the components below
            components = facility.get_components()
            for component in components:
                component_make_erc_def_function = component.get_func_def_string(
                    facility.get_facility_func_decl_string())
                erc_make_functions += f"{component_make_erc_def_function}\n"

        return erc_make_functions

    def get_extended_result_code_defs(self):
        """
        Takes in the facilities, components, and results and returns the #defines for the ExtendedResultCodes

        Return the string version of the C #definitions for the ExtendedResultCodes
        """
        erc_defs = ""
        for facility in self.facilities:

            components = facility.get_components()

            for component in components:

                results = component.get_results()
                component_func_decl_string = component.get_component_func_decl_string()

                for result in results:
                    result_doxy_comment = result.get_doxy_comment()
                    result_def = result.get_c_definition(
                        component_func_decl_string)
                    erc_defs += f"{result_doxy_comment}{result_def}\n"

        return erc_defs

    def generate_result_h_file(self):
        """
        Generates the result.h file to the result_h_path/result.h file

        Returns true on success; false on failure
        """
        facility_and_component_code_defs = self.get_facility_and_component_code_defs()
        erc_generation_functions = self.get_erc_make_functions()
        extended_result_code_defs = self.get_extended_result_code_defs()

        if (facility_and_component_code_defs == "" or erc_generation_functions == "" or extended_result_code_defs == ""):
            return False

        file_contents = error_code_definition_file.format(FacilityAndComponentEnums=facility_and_component_code_defs,
                                                          ExtendedResultCodeGenerationFunctions=erc_generation_functions, ExtendedResultCodeDefines=extended_result_code_defs)

        try:
            with open(self.result_h_path, 'w') as result_h_file:
                result_h_file.write(file_contents)

        except Exception as e:

            print("An unknown error occurred", e)
            return False

        return True

    def parse_json(self):
        """
        Parses the json file from the json_path passed at object creation and stores data

        Returns true on success; false on failure
        """
        try:
            file = open(self.json_path)

        except FileNotFoundError as e:
            print("Could not JSON file for reading at: " +
                  str(self.json_path) + " ", e)
            return False
        except Exception as e:
            print(f"An unknown error occurred while opening the json file path at: {self.json_path} ", e)
            return False

        try:
            result_json = json.load(file)

        except Exception as e:
            print(f"An unknown error occurred while loading the json from file path: {self.json_path}", e)
            return False

        facilities = result_json["facilities"]

        for facility_json_obj in facilities:

            f_code = facility_json_obj["code"]

            if (type(f_code) != int):
                print("Facility codes must be of type int")
                return False

            f_name = facility_json_obj["name"]

            if (type(f_name) != str):
                print("Facility names must be of type str")
                return False

            f_doc_string = facility_json_obj["doc_string"]

            if (type(f_doc_string) != str):
                print("Facility doc strings must be of type str")
                return False

            facility = FacilityCode(f_name, f_code, f_doc_string)

            if (not self.add_facility_code(facility)):
                print(f"Facility {f_name} is a duplicate!")
                return False

            for component_json_obj in facility_json_obj["components"]:

                c_code = component_json_obj["code"]

                if (type(c_code) != int):
                    print("Component codes must be of type int")
                    return False

                c_name = component_json_obj["name"]

                if (type(c_name) != str):
                    print("Component names must be of type str")
                    return False

                c_doc_string = component_json_obj["doc_string"]

                if (type(c_doc_string) != str):
                    print("Component doc strings must of type str")
                    return False

                component = ComponentCode(c_name, c_code, c_doc_string, f_code)

                if (not facility.add_component(component)):
                    print(f"Component {component.name} is a duplicate!")
                    return False

                for result_json_obj in component_json_obj["results"]:

                    r_val = result_json_obj["value"]

                    if (type(r_val) != int):
                        print("Result code must be of type int")
                        return False

                    r_name = result_json_obj["name"]

                    if (type(r_name) != str):
                        print("Result name must be of type str")
                        return False

                    result = ResultCode(r_name, r_val, f_code, c_code)

                    if (not component.add_result(result)):
                        print(f"Result code: {r_name} is a duplicate!")
                        return False

        return True


help_msg = """\
Usage: python3 ./ErrorCodeDefinitionGenerator.py [options...]
-j, --json-file-path           The file path to the json file that describes the results
-r, --result-file-path         The file to write the error code definitions to
-h, --help                     Prints this message
"""


def print_help_msg():
    print(help_msg)


if (__name__ == '__main__'):

    shortopts = "j:r:h"
    longopts = ["json-file-path", "result-file-path", "help"]

    optlist, args = getopt.getopt(sys.argv[1:], shortopts, longopts)

    json_file_path = ""
    result_file_path = ""

    for opt, val in optlist:

        if (opt == "-j" or opt == "--" + longopts[0]):
            json_file_path = val
        elif (opt == "-r" or opt == "--"+longopts[0]):
            result_file_path = val
        elif (opt == "-h" or opt == "--"+longopts[0]):
            print_help_msg()
            sys.exit(1)
        else:
            print("Unrecognized command " + opt)
            print_help_msg()
            sys.exit(1)

    if (json_file_path == "" or result_file_path == ""):
        print("No arguments passed...")
        print_help_msg()
        sys.exit(1)

    if (not os.path.exists(json_file_path)):
        print("The path: " + json_file_path + " does not exist!")
        quit()

    if (not os.path.isfile(json_file_path)):
        print("Option json-file-path must have a file path passed to it...")
        print(json_file_path + " is not a file")
        sys.exit(1)

    result_dest_dir = os.path.dirname(result_file_path)
    if (not os.path.exists(result_dest_dir)):
        print("The directory for: " + result_dest_dir + " does not exist!")
        sys.exit(1)

    if (not os.path.isdir(result_dest_dir)):
        print("The parent directory for the file to be generated is not a directory!")
        print(result_dest_dir + " is not a directory")
        sys.exit(1)

    if ( os.path.exists(result_file_path) ):
        print("Overwriting file at " + result_file_path)

    file_name = os.path.basename(result_file_path)
    if (file_name == "" ):
        print("result-file-path must end with a file to create!")
        sys.exit(1)

    result_h_abs_path = os.path.abspath(result_file_path)

    json_file_abs_path = os.path.abspath(json_file_path)

    print("Generating result.h file at: " + str(result_h_abs_path))

    result_generator = ErrorCodeDefinitionGenerator(
        json_file_abs_path, result_h_abs_path)

    print("Parsing JSON file...")

    if (not result_generator.parse_json()):
        print("Failed to parse json file: " + str(json_file_path))
        sys.exit(1)

    print("Generating result.h file...")

    if (not result_generator.generate_result_h_file()):
        print("Failed to generate result.h file at " + str(result_h_abs_path))
        sys.exit(1)

    # Exit successfully
    sys.exit(0)

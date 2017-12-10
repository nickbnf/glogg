################################################################################
##
##  Provide Pandoc compilation support for the CMake build system
##
##  Version: 0.0.1
##  Author: Jeet Sukumatan (jeetsukumaran@gmail.com)
##
##  Copyright 2015 Jeet Sukumaran.
##
##  This software is released under the BSD 3-Clause License.
##
##  Redistribution and use in source and binary forms, with or without
##  modification, are permitted provided that the following conditions are
##  met:
##
##  1. Redistributions of source code must retain the above copyright notice,
##  this list of conditions and the following disclaimer.
##
##  2. Redistributions in binary form must reproduce the above copyright
##  notice, this list of conditions and the following disclaimer in the
##  documentation and/or other materials provided with the distribution.
##
##  3. Neither the name of the copyright holder nor the names of its
##  contributors may be used to endorse or promote products derived from this
##  software without specific prior written permission.
##
##  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
##  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
##  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
##  PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
##  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
##  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
##  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
##  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
##  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
##  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
##  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
################################################################################

include(CMakeParseArguments)

if(NOT EXISTS ${PANDOC_EXECUTABLE})
    # find_program(PANDOC_EXECUTABLE NAMES pandoc)
    find_program(PANDOC_EXECUTABLE pandoc)
    mark_as_advanced(PANDOC_EXECUTABLE)
    if(NOT EXISTS ${PANDOC_EXECUTABLE})
        message(FATAL_ERROR "Pandoc not found. Install Pandoc (http://johnmacfarlane.net/pandoc/) or set cache variable PANDOC_EXECUTABLE.")
        return()
    endif()
endif()

###############################################################################
# Based on code from UseLATEX
# Author: Kenneth Moreland <kmorel@sandia.gov>
# Copyright 2004 Sandia Corporation.
# Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the
# U.S. Government. Redistribution and use in source and binary forms, with
# or without modification, are permitted provided that this Notice and any
# statement of authorship are reproduced on all copies.

# Adds command to copy file from the source directory to the destination
# directory: used to move source files from source directory into build
# directory before main build
function(pandocology_add_input_file source_path dest_dir dest_filelist_var native_dest_filelist_var)
    set(dest_filelist)
    set(native_dest_filelist)
    file(GLOB globbed_source_paths "${source_path}")
    foreach(globbed_source_path ${globbed_source_paths})
        get_filename_component(filename ${globbed_source_path} NAME)
        get_filename_component(absolute_dest_path ${dest_dir}/${filename} ABSOLUTE)
        file(RELATIVE_PATH relative_dest_path ${CMAKE_CURRENT_BINARY_DIR} ${absolute_dest_path})
        list(APPEND dest_filelist ${absolute_dest_path})
        file(TO_NATIVE_PATH ${absolute_dest_path} native_dest_path)
        list(APPEND native_dest_filelist ${native_dest_path})
        file(TO_NATIVE_PATH ${globbed_source_path} native_globbed_source_path)
        ADD_CUSTOM_COMMAND(
            OUTPUT ${relative_dest_path}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${native_globbed_source_path} ${native_dest_path}
            DEPENDS ${globbed_source_path}
            )
        set(${dest_filelist_var} ${${dest_filelist_var}} ${dest_filelist} PARENT_SCOPE)
        set(${native_dest_filelist_var} ${${native_dest_filelist_var}} ${native_dest_filelist} PARENT_SCOPE)
    endforeach()
endfunction()

# A version of GET_FILENAME_COMPONENT that treats extensions after the last
# period rather than the first.
function(pandocology_get_file_stemname varname filename)
    SET(result)
    GET_FILENAME_COMPONENT(name ${filename} NAME)
    STRING(REGEX REPLACE "\\.[^.]*\$" "" result "${name}")
    SET(${varname} "${result}" PARENT_SCOPE)
endfunction()

function(pandocology_get_file_extension varname filename)
    SET(result)
    GET_FILENAME_COMPONENT(name ${filename} NAME)
    STRING(REGEX MATCH "\\.[^.]*\$" result "${name}")
    SET(${varname} "${result}" PARENT_SCOPE)
endfunction()
###############################################################################

function(pandocology_add_input_dir source_dir dest_parent_dir dir_dest_filelist_var native_dir_dest_filelist_var)
    set(dir_dest_filelist)
    set(native_dir_dest_filelist)
    file(TO_CMAKE_PATH ${source_dir} source_dir)
    get_filename_component(dir_name ${source_dir} NAME)
    get_filename_component(absolute_dest_dir ${dest_parent_dir}/${dir_name} ABSOLUTE)
    file(RELATIVE_PATH relative_dest_dir ${CMAKE_CURRENT_BINARY_DIR} ${absolute_dest_dir})
    file(TO_NATIVE_PATH ${absolute_dest_dir} native_absolute_dest_dir)
    add_custom_command(
        OUTPUT ${relative_dest_dir}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${native_absolute_dest_dir}
        DEPENDS ${source_dir}
        )
    file(GLOB source_files "${source_dir}/*")
    foreach(source_file ${source_files})
        # get_filename_component(absolute_source_path ${CMAKE_CURRENT_SOURCE_DIR}/${source_file} ABSOLUTE)
        pandocology_add_input_file(${source_file} ${absolute_dest_dir} dir_dest_filelist native_dir_dest_filelist)
    endforeach()
    set(${dir_dest_filelist_var} ${${dir_dest_filelist_var}} ${dir_dest_filelist} PARENT_SCOPE)
    set(${native_dir_dest_filelist_var} ${${native_dir_dest_filelist_var}} ${native_dir_dest_filelist} PARENT_SCOPE)
endfunction()

function(add_to_make_clean filepath)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${filepath} native_filepath)
    get_directory_property(make_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${make_clean_files};${native_filepath}")
endfunction()

function(disable_insource_build)
    IF ( CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR AND NOT MSVC_IDE )
        MESSAGE(FATAL_ERROR "The build directory must be different from the main project source "
"directory. Please create a directory such as '${CMAKE_SOURCE_DIR}/build', "
"and run CMake from there, passing the path to this source directory as "
"the path argument. E.g.:
    $ cd ${CMAKE_SOURCE_DIR}
    $ mkdir build
    $ cd build
    $ cmake .. && make && sudo make install
This process created the file `CMakeCache.txt' and the directory `CMakeFiles'.
Please delete them:
    $ rm -r CMakeFiles/ CmakeCache.txt
")
    ENDIF()
endfunction()

# This builds a document
#
# Usage:
#
#
#     INCLUDE(pandocology)
#
#     add_document(
#         TARGET              figures
#         OUTPUT_FILE         figures.tex
#         SOURCES              figures.md
#         RESOURCE_DIRS        figs
#         PANDOC_DIRECTIVES    -t latex
#         NO_EXPORT_PRODUCT
#         )
#
#     add_document(
#         TARGET               opus
#         OUTPUT_FILE          opus.pdf
#         SOURCES              opus.md
#         RESOURCE_FILES       opus.bib systbiol.template.latex systematic-biology.csl
#         RESOURCE_DIRS        figs
#         PANDOC_DIRECTIVES    -t             latex
#                             --smart
#                             --template     systbiol.template.latex
#                             --filter       pandoc-citeproc
#                             --csl          systematic-biology.csl
#                             --bibliography opus.bib
#                             --include-after-body=figures.tex
#         DEPENDS             figures
#         EXPORT_ARCHIVE
#         )
#
#
# Deprecated command signature for backwards compatibility:
#
#     add_document(
#         figures.tex
#         SOURCES              figures.md
#         RESOURCE_DIRS        figs
#         PANDOC_DIRECTIVES    -t latex
#         NO_EXPORT_PRODUCT
#         )
#
# This command signature will cause an error due to a circular dependency during
# build when the sources are at the top level directory of the project. A
# warning is issued. The solution is to use the alternative commande signature.
#
function(add_document)
    set(options          EXPORT_ARCHIVE NO_EXPORT_PRODUCT EXPORT_PDF DIRECT_TEX_TO_PDF VERBOSE)
    set(oneValueArgs     TARGET OUTPUT_FILE PRODUCT_DIRECTORY)
    set(multiValueArgs   SOURCES RESOURCE_FILES RESOURCE_DIRS PANDOC_DIRECTIVES DEPENDS)
    cmake_parse_arguments(ADD_DOCUMENT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    # this is because `make clean` will dangerously clean up source files
    disable_insource_build()

    if ("${ADD_DOCUMENT_TARGET}" STREQUAL "" AND "${ADD_DOCUMENT_OUTPUT_FILE}" STREQUAL "")
        set(bw_comp_mode_args TRUE)
    endif()

    if(bw_comp_mode_args)
        list(LENGTH ADD_DOCUMENT_UNPARSED_ARGUMENTS unparsed_args_num)
        if (${unparsed_args_num} GREATER 1)
            message(FATAL_ERROR "add_document() requires at most one target name")
        endif()

        if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
          message(WARNING "This add_document() signature does not support sources on the top level directory. \
          It will cause a circular dependency error during build.")
        endif()

        list(GET ADD_DOCUMENT_UNPARSED_ARGUMENTS 0 ADD_DOCUMENT_TARGET)
        set(ADD_DOCUMENT_OUTPUT_FILE ${ADD_DOCUMENT_TARGET})
    endif()

    if ("${ADD_DOCUMENT_TARGET}" STREQUAL "")
        MESSAGE(FATAL_ERROR "add_document() requires a target name by using the TARGET option")
    endif()

    if ("${ADD_DOCUMENT_OUTPUT_FILE}" STREQUAL "")
        MESSAGE(FATAL_ERROR "add_document() requires an output file name by using the OUTPUT_FILE option")
    endif()

    if (NOT bw_comp_mode_args)
        if ("${ADD_DOCUMENT_TARGET}" STREQUAL "${ADD_DOCUMENT_OUTPUT_FILE}")
            message(FATAL_ERROR "Target '${ADD_DOCUMENT_TARGET}': Must different from OUTPUT_FILE")
        endif()
    endif()

    set(output_file ${ADD_DOCUMENT_OUTPUT_FILE})
    file(TO_NATIVE_PATH ${output_file} native_output_file)

    # get the stem of the target name
    pandocology_get_file_stemname(output_file_stemname ${output_file})
    pandocology_get_file_extension(output_file_extension ${output_file})

    if (${ADD_DOCUMENT_EXPORT_PDF})
        if (NOT "${output_file_extension}" STREQUAL ".tex" AND NOT "${output_file_extension}" STREQUAL ".latex")
            MESSAGE(FATAL_ERROR "Target '${ADD_DOCUMENT_TARGET}': Cannot use 'EXPORT_PDF' for target of type '${output_file_extension}': target type must be '.tex' or '.latex'")
        endif()
    endif()

    if (${ADD_DOCUMENT_DIRECT_TEX_TO_PDF})
        list(LENGTH ${ADD_DOCUMENT_SOURCES} SOURCE_LEN)
        if (SOURCE_LEN GREATER 1)
            MESSAGE(FATAL_ERROR "Target '${ADD_DOCUMENT_TARGET}': Only one source can be specified when using the 'DIRECT_TEX_TO_PDF' option")
        endif()

        pandocology_get_file_stemname(source_stemname ${ADD_DOCUMENT_SOURCES})
        pandocology_get_file_extension(source_extension ${ADD_DOCUMENT_SOURCES})

        if (NOT "${source_extension}" STREQUAL ".tex" AND NOT "${source_extension}" STREQUAL ".latex")
            MESSAGE(FATAL_ERROR "Target '${ADD_DOCUMENT_TARGET}': Cannot use 'DIRECT_TEX_TO_PDF' for source of type '${source_extension}': source type must be '.tex' or '.latex'")
        endif()

        set(check_target ${source_stemname}.pdf)
        if (NOT ${check_target} STREQUAL ${output_file})
            MESSAGE(FATAL_ERROR "Target '${ADD_DOCUMENT_TARGET}': Must use target name of '${check_target}' if using 'DIRECT_TEX_TO_PDF'")
        endif()
    endif()

    ## set up output directory
    if ("${ADD_DOCUMENT_PRODUCT_DIRECTORY}" STREQUAL "")
        set(ADD_DOCUMENT_PRODUCT_DIRECTORY "product")
    endif()

    get_filename_component(product_directory ${CMAKE_BINARY_DIR}/${ADD_DOCUMENT_PRODUCT_DIRECTORY} ABSOLUTE)
    file(TO_NATIVE_PATH ${product_directory} native_product_directory)

    set(dest_output_file ${product_directory}/${output_file})
    file(TO_NATIVE_PATH ${dest_output_file} native_dest_output_file)

    ## get primary source
    set(build_sources)
    set(native_build_sources)
    foreach(input_file ${ADD_DOCUMENT_SOURCES} )
        pandocology_add_input_file(${CMAKE_CURRENT_SOURCE_DIR}/${input_file} ${CMAKE_CURRENT_BINARY_DIR} build_sources native_build_sources)
    endforeach()

    ## get resource files
    set(build_resources)
    set(native_build_resources)
    foreach(resource_file ${ADD_DOCUMENT_RESOURCE_FILES})
        pandocology_add_input_file(${CMAKE_CURRENT_SOURCE_DIR}/${resource_file} ${CMAKE_CURRENT_BINARY_DIR} build_resources native_build_resources)
    endforeach()

    ## get resource dirs
    set(exported_resources)
    set(native_exported_resources)
    foreach(resource_dir ${ADD_DOCUMENT_RESOURCE_DIRS})
        pandocology_add_input_dir(${CMAKE_CURRENT_SOURCE_DIR}/${resource_dir} ${CMAKE_CURRENT_BINARY_DIR} build_resources native_build_resources)
        if (${ADD_DOCUMENT_EXPORT_ARCHIVE})
            pandocology_add_input_dir(${CMAKE_CURRENT_SOURCE_DIR}/${resource_dir} ${product_directory} exported_resources native_exported_resources)
        endif()
    endforeach()

    ## primary command
    if (${ADD_DOCUMENT_DIRECT_TEX_TO_PDF})
        if (${ADD_DOCUMENT_VERBOSE})
            add_custom_command(
                OUTPUT  ${output_file} # note that this is in the build directory
                DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${native_product_directory}
                COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode -file-line-error -pdf ${native_build_sources}
                )
        else()
            add_custom_command(
              OUTPUT  ${output_file} # note that this is in the build directory
                DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
                COMMAND ${CMAKE_COMMAND} -E make_directory ${native_product_directory}
                COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode -file-line-error -pdf ${native_build_sources} 2>/dev/null >/dev/null || (grep --no-messages -A8 ".*:[0-9]*:.*" ${output_file_stemname}.log && false)
                )
        endif()
        add_to_make_clean(${output_file})
    else()
        add_custom_command(
            OUTPUT  ${output_file} # note that this is in the build directory
            DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${native_product_directory}
            COMMAND ${PANDOC_EXECUTABLE} ${native_build_sources} ${ADD_DOCUMENT_PANDOC_DIRECTIVES} -o ${native_output_file}
            )
        add_to_make_clean(${output_file})
    endif()

    ## figure out what all is going to be produced by this build set, and set
    ## those as dependencies of the primary target
    set(primary_target_dependencies)
    set(primary_target_dependencies ${primary_target_dependencies} ${CMAKE_CURRENT_BINARY_DIR}/${output_file})
    if (NOT ${ADD_DOCUMENT_NO_EXPORT_PRODUCT})
        set(primary_target_dependencies ${primary_target_dependencies} ${product_directory}/${output_file})
    endif()
    if (${ADD_DOCUMENT_EXPORT_PDF})
        set(primary_target_dependencies ${primary_target_dependencies} ${CMAKE_CURRENT_BINARY_DIR}/${output_file_stemname}.pdf)
        set(primary_target_dependencies ${primary_target_dependencies} ${product_directory}/${output_file_stemname}.pdf)
    endif()
    if (${ADD_DOCUMENT_EXPORT_ARCHIVE})
        set(primary_target_dependencies ${primary_target_dependencies} ${product_directory}/${output_file_stemname}.tbz)
    endif()

    ## primary target
    # # target cannot have same (absolute name) as dependencies:
    # # http://www.cmake.org/pipermail/cmake/2011-March/043378.html
    add_custom_target(
        ${ADD_DOCUMENT_TARGET}
        ALL
        DEPENDS ${primary_target_dependencies} ${ADD_DOCUMENT_DEPENDS}
        )

    # run post-pdf
    if (${ADD_DOCUMENT_EXPORT_PDF})
        add_custom_command(
            OUTPUT ${native_product_directory}/${output_file_stemname}.pdf ${CMAKE_CURRENT_BINARY_DIR}/${output_file_stemname}.pdf
            DEPENDS ${output_file} ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}

            # Does not work: custom template used to generate tex is ignored
            # COMMAND ${PANDOC_EXECUTABLE} ${output_file} -f latex -o ${output_file_stemname}.pdf

            # (1)   Apparently, both nonstopmode and batchmode produce an output file
            #       even if there was an error. This tricks latexmk into believing
            #       the file is actually up-to-date.
            #       So we use `-halt-on-error` or `-interaction=errorstopmode`
            #       instead.
            # (2)   `grep` returns a non-zero error code if the pattern is not
            #       found. So, in our scheme below to filter the output of
            #       `pdflatex`, it is precisely when there is NO error that
            #       grep returns a non-zero code, which fools CMake into thinking
            #       tex'ing failed.
            #       Hence the need for `| grep ...| cat` or `| grep  || true`.
            #       But we can go better:
            #           latexmk .. || (grep .. && false)
            #       So we can have our cake and eat it too: here we want to
            #       re-raise the error after a successful grep if there was an
            #       error in `latexmk`.
            # COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode # -file-line-error -pdf ${output_file} 2>&1 | grep -A8 ".*:[0-9]*:.*" || true
            COMMAND latexmk -gg -halt-on-error -interaction=nonstopmode -file-line-error -pdf ${native_output_file} 2>/dev/null >/dev/null || (grep --no-messages -A8 ".*:[0-9]*:.*" ${output_file_stemname}.log && false)

            COMMAND ${CMAKE_COMMAND} -E copy ${output_file_stemname}.pdf ${native_product_directory}
            )
        add_to_make_clean(${output_file_stemname}.pdf)
        add_to_make_clean(${product_directory}/${output_file_stemname}.pdf)
    endif()

    ## copy products
    if (NOT ${ADD_DOCUMENT_NO_EXPORT_PRODUCT})
        add_custom_command(
            OUTPUT ${native_dest_output_file}
            DEPENDS ${build_sources} ${build_resources} ${ADD_DOCUMENT_DEPENDS}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${native_output_file} ${native_dest_output_file}
            )
        add_to_make_clean(${product_directory}/${output_file})
    endif()

    ## copy resources
    set(export_archive_file ${product_directory}/${output_file_stemname}.tbz)
    file(TO_NATIVE_PATH ${export_archive_file} native_export_archive_file)

    if (${ADD_DOCUMENT_EXPORT_ARCHIVE})
        add_custom_command(
            OUTPUT ${export_archive_file}
            DEPENDS ${output_file}
            COMMAND ${CMAKE_COMMAND} -E tar cjf ${native_export_archive_file} ${native_output_file} ${native_build_resources} ${ADD_DOCUMENT_DEPENDS}
            )
        add_to_make_clean(${export_archive_file})
    endif()
endfunction(add_document)

function(add_tex_document)
    add_document(${ARGV} DIRECT_TEX_TO_PDF)
endfunction()

# LEGACY SUPPORT
function(add_pandoc_document)
    add_document(${ARGV})
endfunction()


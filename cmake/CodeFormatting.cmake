####################################################################################################
# @copyright Developed and programmed by MBition GmbH - Copyright (c) 2021
# Daimler AG
# @license This Source Code Form is subject to the terms of the Mozilla Public
#          License, v. 2.0. If a copy of the MPL was not distributed with this
#          file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# SPDX-License-Identifier: MPL-2.0
#
####################################################################################################

if(ENABLE_CODE_FORMATTER)
    find_program(FORMAT_CPP_PRG NAMES clang-format clang-format-6.0)

    if(NOT FORMAT_CPP_PRG)
        message(FATAL_ERROR "No program 'clang-format' found")
    endif()

    find_program(FORMAT_CMAKE_PRG cmake-format)

    if(NOT FORMAT_CMAKE_PRG)
        message(FATAL_ERROR "No program 'cmake-format' found")
    endif()

    set(FIND_CPP_COMMAND
        "find ${PROJECT_SOURCE_DIR} -regex '.*\\.\\(c\\|cc\\|cpp\\|cxx\\|h\\|hh\\|hpp\\|hxx\\)\\(\\.in\\)?$' -not -path '${CMAKE_BINARY_DIR}/*'"
    )

    set(FIND_CMAKE_COMMAND
        "find ${PROJECT_SOURCE_DIR} -type f \\( -iname CMakeLists.txt -o -iname \\*.cmake \\) -not -path '${CMAKE_BINARY_DIR}/*'"
    )

    add_custom_target(
        codeformat
        VERBATIM
        COMMAND bash -c "${FIND_CPP_COMMAND} | xargs -P 0 -n 1 ${FORMAT_CPP_PRG} -i"
        COMMAND bash -c "${FIND_CMAKE_COMMAND} | xargs -P 0 -n 1 ${FORMAT_CMAKE_PRG} -i"
    )

    add_custom_target(
        formatcheck
        VERBATIM
        COMMAND bash -c
                "${FIND_CPP_COMMAND} | xargs -I {} bash -c 'diff -u {} <(${FORMAT_CPP_PRG} {})'"
        COMMAND bash -c
                "${FIND_CMAKE_COMMAND} | xargs -I {} bash -c 'diff -u {} <(${FORMAT_CMAKE_PRG} {})'"
    )
endif()

# ... पुराना कोड ...

if(XSDK_TOOLCHAIN)
    message(STATUS "XSDK Toolchain found. Building Xbox 360 XEX...")
    
    # लाइब्रेरी को खोजें (यह build डायरेक्टरी के अंदर कहीं भी हो सकती है)
    find_library(XSDK_LIB
        NAMES xsdk
        PATHS $ENV{HOME}/xsdk/build $ENV{HOME}/xsdk/build/lib
        NO_DEFAULT_PATH
    )

    if(NOT XSDK_LIB)
        message(FATAL_ERROR "libxsdk.a not found! Please check the xsdk build output.")
    endif()

    message(STATUS "Found XSDK Library: ${XSDK_LIB}")

    add_executable(inkfox_elf src/main.cpp)
    target_include_directories(inkfox_elf PRIVATE src)
    
    # लाइब्रेरी को उसके एब्सोल्यूट पाथ के साथ लिंक करें
    target_link_libraries(inkfox_elf ${XSDK_LIB})
    
    xsdk_add_xex(inkfox_elf)
else()
    message(STATUS "No XSDK Toolchain. Skipping Xbox 360 build.")
endif()

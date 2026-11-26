# ... पुराना कोड ...

if(XSDK_TOOLCHAIN)
    message(STATUS "XSDK Toolchain found. Building Xbox 360 XEX...")
    
    # लिंकर को बताएं कि libxsdk.a कहाँ है
    # (यह पाथ आपके build.yml में दिए गए $HOME/xsdk/build डायरेक्टरी से मेल खाता है)
    link_directories($ENV{HOME}/xsdk/build)
    
    add_executable(inkfox_elf src/main.cpp)
    target_include_directories(inkfox_elf PRIVATE src)
    
    # अब target_link_libraries को -lxsdk मिल जाएगा
    target_link_libraries(inkfox_elf xsdk)
    
    xsdk_add_xex(inkfox_elf)
else()
    message(STATUS "No XSDK Toolchain. Skipping Xbox 360 build.")
endif()

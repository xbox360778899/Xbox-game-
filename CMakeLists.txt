cmake_minimum_required(VERSION 3.13)
project(InkFox CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# 1. PC Build (GitHub Actions par testing ke liye)
# ---------------------------------------------------------
add_executable(inkfox_test src/test_headless.cpp)
target_include_directories(inkfox_test PRIVATE src)

# ---------------------------------------------------------
# 2. Xbox 360 Build (XSDK toolchain milne par hi banega)
# ---------------------------------------------------------
if(XSDK_TOOLCHAIN)
    message(STATUS "XSDK Toolchain found. Building Xbox 360 XEX...")
    
    # 1. ELF file banayein
    add_executable(inkfox_elf src/main.cpp)
    target_include_directories(inkfox_elf PRIVATE src)
    
    # 2. XSDK library link karein
    target_link_libraries(inkfox_elf xsdk)
    
    # 3. ELF ko XEX mein badlein (Sirf ek hi naam dena hai)
    xsdk_add_xex(inkfox_elf)
else()
    message(STATUS "No XSDK Toolchain. Skipping Xbox 360 build.")
endif()

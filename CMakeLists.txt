cmake_minimum_required(VERSION 3.15)
project(datnights_viewer C)

# --- VitaSDK toolchain ---
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VITASDK})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain")
    else()
        message(FATAL_ERROR "Please set VITASDK env var pointing to your VitaSDK.")
    endif()
endif()

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

# --- App metadata ---
set(VITA_APP_NAME  "DATNIGHTS Viewer")
set(VITA_TITLEID   "DN0VIEWER") # Убедись, что ID уникальный
set(VITA_VERSION   "01.00")

# --- Sources ---
add_executable(${PROJECT_NAME}
    src/main.c
    src/obj_loader.c
)
target_include_directories(${PROJECT_NAME} PRIVATE src)
set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 99)
target_compile_options(${PROJECT_NAME} PRIVATE -O2 -ffast-math)

# --- Link order matters ---
target_link_libraries(${PROJECT_NAME}
    vitaGL
    vitashark
    mathneon
    SceShaccCg_stub
    SceShaccCgExt
    taihenModuleUtils_stub
    taihen_stub
    SceGxm_stub
    SceDisplay_stub
    SceCtrl_stub
    SceCommonDialog_stub
    SceTouch_stub
    SceAppMgr_stub
    SceKernelDmacMgr_stub
    m
)

# --- Packaging ---
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME    ${VITA_APP_NAME}
    FILE assets assets
    FILE sce_sys/icon0.png sce_sys/icon0.png
    FILE sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
    FILE sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
    FILE sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)

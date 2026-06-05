cmake_minimum_required(VERSION 3.21)
project(PP18_VideoTools LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Для совместимости с тем, как пользователь может передавать путь к Qt
foreach(qt_root_var IN ITEMS QT_ROOT QTDIR)
  if(DEFINED ENV{${qt_root_var}} AND EXISTS "$ENV{${qt_root_var}}/lib/cmake")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{${qt_root_var}}/lib/cmake")
  endif()
endforeach()

# Помощь в поиске Vulkan на Windows (Qt 6 требует его для RHI)
if(WIN32)
  find_package(Vulkan QUIET)
  if(NOT Vulkan_FOUND)
    file(GLOB VULKAN_SDK_DIRS "C:/VulkanSDK/*")
    if(VULKAN_SDK_DIRS)
      list(SORT VULKAN_SDK_DIRS COMPARE NATURAL ORDER DESCENDING)
      list(GET VULKAN_SDK_DIRS 0 LATEST_VULKAN_SDK)
      set(Vulkan_INCLUDE_DIR "${LATEST_VULKAN_SDK}/Include" CACHE PATH "Path to Vulkan include directory")
      find_package(Vulkan QUIET)
    endif()
  endif()
endif()

# Попытка подавить варнинг о Qt6TaskTree (может отсутствовать в некоторых установках Qt)
find_package(Qt6 COMPONENTS TaskTree QUIET)
find_package(Qt6 REQUIRED COMPONENTS Network Multimedia Qml Quick QuickControls2)

qt_standard_project_setup()
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set_source_files_properties(src/qml/AppStyle.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_executable(${PROJECT_NAME}
    src/main.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/app/VideoFileModel.h
    src/app/VideoFileModel.cpp
    src/services/FfmpegBatchService.h
    src/services/FfmpegBatchService.cpp
    src/services/FfmpegPreviewService.h
    src/services/FfmpegPreviewService.cpp
    src/services/TelegramController.h
    src/services/TelegramController.cpp
    src/services/TelegramService.h
    src/services/TelegramService.cpp
    src/platform/NativeFileDialog.h
    src/platform/NativeFolderDialog.h
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(APPLE)
  target_sources(${PROJECT_NAME} PRIVATE
    src/platform/NativeFileDialog_mac.mm
    src/platform/NativeFolderDialog_mac.mm
  )
else()
  target_sources(${PROJECT_NAME} PRIVATE
    src/platform/NativeFileDialog.cpp
    src/platform/NativeFolderDialog.cpp
  )
endif()

qt_add_qml_module(${PROJECT_NAME}
    URI PP18.VideoTools
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/AppFonts.qml
        src/qml/AppStyle.qml
        src/qml/LogWindow.qml
        src/qml/SettingsWindow.qml
        src/qml/PlayerWindow.qml
        src/qml/components/PlayerButton.qml
        src/qml/components/PlayerCombo.qml
        src/qml/components/ToolbarButton.qml
        src/qml/components/ToolbarSelect.qml
        src/qml/components/ToolbarCheck.qml
        src/qml/components/ActionToggle.qml
        src/qml/components/ContextMenuItem.qml
        src/qml/components/ContextMenuSeparator.qml
        src/qml/components/TaskCard.qml
        src/qml/components/TaskFixedSide.qml
        src/qml/components/TaskSide.qml
        src/qml/components/TaskSourceSide.qml
        src/qml/components/TaskLine.qml
    RESOURCES
        src/resources.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt6::Network
  Qt6::Multimedia
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")

# Логика копирования ffmpeg и ассетов
set(FFMPEG_NAME "ffmpeg")
set(FFPROBE_NAME "ffprobe")
if(WIN32)
  set(FFMPEG_NAME "ffmpeg.exe")
  set(FFPROBE_NAME "ffprobe.exe")
endif()

set(FFMPEG_CANDIDATES
    "${CMAKE_SOURCE_DIR}/bin/${FFMPEG_NAME}"
    "${CMAKE_SOURCE_DIR}/../pp18-video-tools_cli/bin/${FFMPEG_NAME}"
)
set(FFPROBE_CANDIDATES
    "${CMAKE_SOURCE_DIR}/bin/${FFPROBE_NAME}"
    "${CMAKE_SOURCE_DIR}/../pp18-video-tools_cli/bin/${FFPROBE_NAME}"
)

set(BIN_DEST "$<TARGET_FILE_DIR:${PROJECT_NAME}>")
if(APPLE)
  set(BIN_DEST "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_DEST}/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/src/assets" "${BIN_DEST}/assets"
    VERBATIM
)

foreach(src IN LISTS FFMPEG_CANDIDATES)
  if(EXISTS "${src}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src}" "${BIN_DEST}/bin/${FFMPEG_NAME}"
      VERBATIM
    )
    break()
  endif()
endforeach()

foreach(src IN LISTS FFPROBE_CANDIDATES)
  if(EXISTS "${src}")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${src}" "${BIN_DEST}/bin/${FFPROBE_NAME}"
      VERBATIM
    )
    break()
  endif()
endforeach()

# Автоматический деплой на Windows в папку сборки
if(WIN32)
    # Находим windeployqt
    get_target_property(_qt_bin_dir Qt6::Core LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_bin_dir}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --release --no-translations --qmldir "${CMAKE_SOURCE_DIR}/src" "$<TARGET_FILE:${PROJECT_NAME}>"
            COMMENT "Deploying Qt dependencies to build directory..."
            VERBATIM
        )
    endif()
endif()

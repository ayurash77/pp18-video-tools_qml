cmake_minimum_required(VERSION 3.21)
project(PP18_VideoTools LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

foreach(qt_root_var IN ITEMS QT_ROOT QTDIR)
  if(DEFINED ENV{${qt_root_var}} AND EXISTS "$ENV{${qt_root_var}}/lib/cmake")
    list(APPEND CMAKE_PREFIX_PATH "$ENV{${qt_root_var}}/lib/cmake")
  endif()
endforeach()

set(PP18_QT_CMAKE_ROOT "")
foreach(qt_cmake_path IN LISTS CMAKE_PREFIX_PATH)
  if(EXISTS "${qt_cmake_path}/Qt6/Qt6Config.cmake")
    set(PP18_QT_CMAKE_ROOT "${qt_cmake_path}")
    break()
  endif()
endforeach()

if(NOT PP18_QT_CMAKE_ROOT AND Qt6_DIR)
  get_filename_component(PP18_QT_CMAKE_ROOT "${Qt6_DIR}/.." ABSOLUTE)
endif()

if(NOT Qt6_DIR)
  set(QT_AUTODETECT_PATHS)
  if(APPLE AND DEFINED ENV{HOME})
    file(GLOB QT_AUTODETECT_PATHS LIST_DIRECTORIES true
      "$ENV{HOME}/Development/Qt/*/macos/lib/cmake"
      "$ENV{HOME}/Qt/*/macos/lib/cmake"
    )
  elseif(WIN32)
    file(GLOB QT_AUTODETECT_PATHS LIST_DIRECTORIES true
      "C:/Qt/*/mingw_64/lib/cmake"
      "C:/Qt/*/msvc*/lib/cmake"
    )
  elseif(DEFINED ENV{HOME})
    file(GLOB QT_AUTODETECT_PATHS LIST_DIRECTORIES true
      "$ENV{HOME}/Qt/*/gcc_64/lib/cmake"
      "$ENV{HOME}/Development/Qt/*/gcc_64/lib/cmake"
    )
  endif()

  if(NOT PP18_QT_CMAKE_ROOT)
    list(SORT QT_AUTODETECT_PATHS COMPARE NATURAL ORDER DESCENDING)
    foreach(qt_path IN LISTS QT_AUTODETECT_PATHS)
      if(EXISTS "${qt_path}/Qt6/Qt6Config.cmake")
        set(PP18_QT_CMAKE_ROOT "${qt_path}")
        break()
      endif()
    endforeach()
  endif()

endif()

if(PP18_QT_CMAKE_ROOT)
  list(PREPEND CMAKE_PREFIX_PATH "${PP18_QT_CMAKE_ROOT}")
  list(REMOVE_DUPLICATES CMAKE_PREFIX_PATH)

  get_cmake_property(PP18_CACHE_VARIABLES CACHE_VARIABLES)
  foreach(cache_var IN LISTS PP18_CACHE_VARIABLES)
    if(cache_var MATCHES "^Qt6.*_DIR$")
      get_property(cache_value CACHE "${cache_var}" PROPERTY VALUE)
      if(cache_value AND NOT cache_value MATCHES "-NOTFOUND$"
          AND NOT cache_value MATCHES "^${PP18_QT_CMAKE_ROOT}(/|$)")
        message(STATUS "Resetting stale Qt cache entry ${cache_var}: ${cache_value}")
        unset(${cache_var} CACHE)
      endif()
    endif()
  endforeach()
endif()

if(NOT PP18_QT_CMAKE_ROOT AND NOT Qt6_DIR)
  message(STATUS "Qt path not set. Pass -DCMAKE_PREFIX_PATH=/path/to/Qt/<version>/<kit>/lib/cmake or set QT_ROOT/QTDIR.")
endif()

if(NOT COMMAND _qt_internal_should_include_targets)
  set(QT_PUBLIC_HELPERS_CANDIDATES)
  if(Qt6_DIR)
    list(APPEND QT_PUBLIC_HELPERS_CANDIDATES "${Qt6_DIR}/QtPublicCMakeHelpers.cmake")
  endif()
  foreach(qt_cmake_path IN LISTS CMAKE_PREFIX_PATH)
    list(APPEND QT_PUBLIC_HELPERS_CANDIDATES "${qt_cmake_path}/Qt6/QtPublicCMakeHelpers.cmake")
  endforeach()

  foreach(qt_public_helpers IN LISTS QT_PUBLIC_HELPERS_CANDIDATES)
    if(EXISTS "${qt_public_helpers}")
      include("${qt_public_helpers}")
      break()
    endif()
  endforeach()
endif()

find_package(Qt6 REQUIRED COMPONENTS Network Multimedia Qml Quick QuickControls2)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/app/AppController.h
    src/app/AppController.cpp
    src/app/VideoFileModel.h
    src/app/VideoFileModel.cpp
    src/qml/qml.qrc
    src/qml/resources.qrc
    src/services/FfmpegBatchService.h
    src/services/FfmpegBatchService.cpp
    src/services/HandbrakePreviewService.h
    src/services/HandbrakePreviewService.cpp
    src/services/TelegramController.h
    src/services/TelegramController.cpp
    src/services/TelegramService.h
    src/services/TelegramService.cpp
    src/platform/NativeFileDialog.h
    src/platform/NativeFolderDialog.h
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

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt6::Network
  Qt6::Multimedia
  Qt6::Qml
  Qt6::Quick
  Qt6::QuickControls2
)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/src")

if(APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

set(MAC_FFMPEG_CANDIDATES
    "${CMAKE_SOURCE_DIR}/bin/ffmpeg"
    "${CMAKE_SOURCE_DIR}/../pp18-video-tools_cli/bin/ffmpeg"
)
set(MAC_FFPROBE_CANDIDATES
    "${CMAKE_SOURCE_DIR}/bin/ffprobe"
    "${CMAKE_SOURCE_DIR}/../pp18-video-tools_cli/bin/ffprobe"
)
set(WIN_FFMPEG_CANDIDATES
    "${CMAKE_SOURCE_DIR}/bin/ffmpeg.exe"
    "${CMAKE_SOURCE_DIR}/../pp18-video-tools_cli/bin/ffmpeg.exe"
)
set(WIN_FFPROBE_CANDIDATES
    "${CMAKE_SOURCE_DIR}/bin/ffprobe.exe"
    "${CMAKE_SOURCE_DIR}/../pp18-video-tools_cli/bin/ffprobe.exe"
)

if(APPLE)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/src/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/assets"
    VERBATIM
  )

  foreach(src IN LISTS MAC_FFMPEG_CANDIDATES)
    if(EXISTS "${src}")
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${src}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/bin/ffmpeg"
        VERBATIM
      )
      break()
    endif()
  endforeach()

  foreach(src IN LISTS MAC_FFPROBE_CANDIDATES)
    if(EXISTS "${src}")
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${src}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/../Resources/bin/ffprobe"
        VERBATIM
      )
      break()
    endif()
  endforeach()
endif()

if(NOT APPLE)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/src/assets"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    VERBATIM
  )
endif()

if(WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/bin"
    VERBATIM
  )

  foreach(src IN LISTS WIN_FFMPEG_CANDIDATES)
    if(EXISTS "${src}")
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${src}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/bin/ffmpeg.exe"
        VERBATIM
      )
      break()
    endif()
  endforeach()

  foreach(src IN LISTS WIN_FFPROBE_CANDIDATES)
    if(EXISTS "${src}")
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${src}"
                "$<TARGET_FILE_DIR:${PROJECT_NAME}>/bin/ffprobe.exe"
        VERBATIM
      )
      break()
    endif()
  endforeach()
endif()

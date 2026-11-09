# macOS: Core Audio for the music, and optionally an app bundle
# (-DLEMMINGS_MACOS_BUNDLE=ON: build/native/Lemmings.app; put the game's
# WINLEMM folder into Lemmings.app/Contents/Resources, or start it with
# --data). Included by CMakeLists.txt on Apple platforms only.

option(LEMMINGS_MACOS_BUNDLE "Build Lemmings.app instead of a plain executable" OFF)

target_sources(winsdl PRIVATE ${CMAKE_SOURCE_DIR}/port/macos/midi_coreaudio.c)
target_include_directories(winsdl PRIVATE ${CMAKE_SOURCE_DIR}/port/src)
target_link_libraries(winsdl PUBLIC "-framework AudioToolbox" "-framework CoreFoundation")

function(lemmings_macos_target target)
  if(LEMMINGS_MACOS_BUNDLE)
    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE ON
      OUTPUT_NAME Lemmings
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/port/macos/Info.plist.in
      MACOSX_BUNDLE_BUNDLE_NAME Lemmings
      MACOSX_BUNDLE_GUI_IDENTIFIER com.psygnosis.lemmings
      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
  endif()
endfunction()

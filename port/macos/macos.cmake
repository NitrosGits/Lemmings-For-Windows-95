# macOS: AppKit and Core Audio (LEMMINGS_BACKEND=cocoa), Core Audio's MIDI
# synthesizer for the music, and optionally an app bundle
# (-DLEMMINGS_MACOS_BUNDLE=ON: build/native/Lemmings.app; put the game's
# WINLEMM folder into Lemmings.app/Contents/Resources, or start it with
# --data). Included by CMakeLists.txt on Apple platforms only.

option(LEMMINGS_MACOS_BUNDLE "Build Lemmings.app instead of a plain executable" OFF)

target_sources(winsdl PRIVATE ${CMAKE_SOURCE_DIR}/port/macos/midi_coreaudio.c)
target_include_directories(winsdl PRIVATE ${CMAKE_SOURCE_DIR}/port/src)
target_link_libraries(winsdl PUBLIC "-framework AudioToolbox" "-framework CoreFoundation")
if(LEMMINGS_BACKEND STREQUAL "cocoa")
  set(COCOA_SRC ${CMAKE_SOURCE_DIR}/port/macos/plat_cocoa.m ${CMAKE_SOURCE_DIR}/port/macos/menu_cocoa.m
                ${CMAKE_SOURCE_DIR}/port/macos/controls_cocoa.m)
  target_sources(winsdl PRIVATE ${COCOA_SRC} ${CMAKE_SOURCE_DIR}/port/macos/audio_coreaudio.c)
  set_source_files_properties(${COCOA_SRC} PROPERTIES COMPILE_OPTIONS -fobjc-arc)
  target_link_libraries(winsdl PUBLIC "-framework AppKit" "-framework QuartzCore" "-framework CoreGraphics"
                                      "-framework CoreAudio")
endif()

function(lemmings_macos_target target)
  if(LEMMINGS_MACOS_BUNDLE)
    set_target_properties(${target} PROPERTIES
      MACOSX_BUNDLE ON
      OUTPUT_NAME Lemmings
      MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/port/macos/Info.plist.in
      MACOSX_BUNDLE_BUNDLE_NAME Lemmings
      MACOSX_BUNDLE_GUI_IDENTIFIER io.github.nitrosgits.lemmings-win95
      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    if(NOT LEMMINGS_BACKEND STREQUAL "cocoa")
      # its own copy of SDL3, so it runs without Homebrew
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_SOURCE_DIR}/port/macos/bundle_sdl.sh $<TARGET_FILE:${target}> $<TARGET_FILE:SDL3::SDL3-shared>
        VERBATIM)
    endif()
  endif()
endfunction()

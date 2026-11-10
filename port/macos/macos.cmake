# macOS: the backend on AppKit (windows, input, the menu bar, native dialog
# controls), Core Audio for the sound effects and its MIDI synthesizer for the
# music, and optionally an app bundle (-DLEMMINGS_MACOS_BUNDLE=ON:
# build/native/Lemmings.app; put the game's WINLEMM folder into
# Lemmings.app/Contents/Resources, or start it with --data). Included by
# CMakeLists.txt.

option(LEMMINGS_MACOS_BUNDLE "Build Lemmings.app instead of a plain executable" OFF)

set(COCOA_SRC ${CMAKE_SOURCE_DIR}/port/macos/plat_cocoa.m ${CMAKE_SOURCE_DIR}/port/macos/menu_cocoa.m
              ${CMAKE_SOURCE_DIR}/port/macos/controls_cocoa.m)
target_sources(winlayer PRIVATE ${COCOA_SRC} ${CMAKE_SOURCE_DIR}/port/macos/audio_coreaudio.c
                                ${CMAKE_SOURCE_DIR}/port/macos/midi_coreaudio.c)
set_source_files_properties(${COCOA_SRC} PROPERTIES COMPILE_OPTIONS -fobjc-arc)
target_include_directories(winlayer PRIVATE ${CMAKE_SOURCE_DIR}/port/src)
target_link_libraries(winlayer PUBLIC "-framework AppKit" "-framework QuartzCore" "-framework CoreGraphics"
                                      "-framework AudioToolbox" "-framework CoreAudio" "-framework CoreFoundation")

# The application icon, made from the game's own Windows icon (a rounded
# square with the Lemming's head): in the bundle's Resources, and beside the
# plain executable, which shows it in the Dock too.
set(LEMMINGS_ICNS ${CMAKE_BINARY_DIR}/Lemmings.icns)
add_custom_command(OUTPUT ${LEMMINGS_ICNS}
  COMMAND Python3::Interpreter ${CMAKE_SOURCE_DIR}/tools/port/mkicns.py
          ${CMAKE_SOURCE_DIR}/src/lemmings/res/lemmings.ico ${LEMMINGS_ICNS}
  DEPENDS ${CMAKE_SOURCE_DIR}/tools/port/mkicns.py ${CMAKE_SOURCE_DIR}/src/lemmings/res/lemmings.ico
  VERBATIM)
add_custom_target(lemmings_icon DEPENDS ${LEMMINGS_ICNS})

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
    # the icon into the bundle, then the bundle signed (ad hoc: Apple silicon
    # wants every executable signed, and the signature seals the Resources)
    set(app $<TARGET_BUNDLE_DIR:${target}>)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/bundle.stamp
      COMMAND ${CMAKE_COMMAND} -E make_directory ${app}/Contents/Resources
      COMMAND ${CMAKE_COMMAND} -E copy ${LEMMINGS_ICNS} ${app}/Contents/Resources/Lemmings.icns
      COMMAND codesign --force --sign - ${app}
      COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/bundle.stamp
      DEPENDS ${target} ${LEMMINGS_ICNS}
      VERBATIM)
    add_custom_target(lemmings_bundle ALL DEPENDS ${CMAKE_BINARY_DIR}/bundle.stamp)
  else()
    add_dependencies(${target} lemmings_icon)
  endif()
endfunction()

cmake_minimum_required(VERSION 2.8)

add_subdirectory(driver)
add_subdirectory(recognition)

add_library(puyoai_capture
            ac_analyzer.cc
            analyzer.cc
            analyzer_result_drawer.cc
            capture.cc
            color.cc
            images_source.cc
            movie_source.cc
            movie_source_key_listener.cc
            real_color_field.cc
            source.cc
            usb_device.cc)

if(V4L2_LIBRARY)
    add_compile_options("-DUSE_V4L2")
    add_library(puyoai_capture_viddev viddev_source.cc)
endif()

if(LIB_USB)
    add_library(puyoai_capture_syntek syntek_source.cc)
endif()

# ----------------------------------------------------------------------

function(capture_add_executable exe)
    add_executable(${exe} ${exe}.cc)
    if(LIB_USB)
        target_link_libraries(${exe} puyoai_capture_syntek)
        target_link_libraries(${exe} puyoai_capture_driver_syntek)
    endif()
    if(V4L2_LIBRARY)
        target_link_libraries(${exe} puyoai_capture_viddev)
    endif()
    target_link_libraries(${exe} puyoai_capture)
    target_link_libraries(${exe} puyoai_recognition)
    target_link_libraries(${exe} puyoai_gui)
    target_link_libraries(${exe} puyoai_learning)
    target_link_libraries(${exe} puyoai_core)
    target_link_libraries(${exe} puyoai_duel)
    target_link_libraries(${exe} puyoai_base)
    puyoai_target_link_libraries(${exe})
    target_link_libraries(${exe} ${SDL2_LIBRARIES})
    target_link_libraries(${exe} ${SDL2_TTF_LIBRARIES})
    target_link_libraries(${exe} ${SDL2_IMAGE_LIBRARIES})
    target_link_libraries(${exe} ${FFMPEG_LIBRARIES})
    if(V4L2_LIBRARY)
        target_link_libraries(${exe} ${V4L2_LIBRARY})
    endif()
    if(LIB_USB)
        target_link_libraries(${exe} ${LIB_USB})
    endif()
endfunction()

capture_add_executable(convert_image_60fps)
capture_add_executable(convert_movie_60fps)
capture_add_executable(gather_images_by_match)
capture_add_executable(split_image)
capture_add_executable(split_movie)

capture_add_executable(parse_bmp)
capture_add_executable(parse_box)
# Disabled.
#capture_add_executable(parse_imgs)
capture_add_executable(parse_movie)
if(LIB_USB)
    capture_add_executable(parse_capture)
endif()

# ----------------------------------------------------------------------

function(capture_add_test exe)
    capture_add_executable(${exe})
    target_link_libraries(${exe} gtest gtest_main)
    add_test(check-${exe} ${exe})
endfunction()

capture_add_test(ac_analyzer_test)
capture_add_test(color_test)
capture_add_test(real_color_field_test)

cmake_minimum_required(VERSION 2.8)

cpu_setup("hamaji")

add_library(hamaji_lib
            core.cc db.cc eval_base.cc eval.cc eval2.cc
            field.cc game.cc
            rater.cc ratingstats.cc solo.cc util.cc)

function(hamaji_add_executable exe)
  cpu_add_executable(${exe} ${ARGN})
  cpu_target_link_libraries(${exe} hamaji_lib)
  cpu_target_link_common_libraries(${exe})
endfunction()

function(hamaji_add_test exe)
  hamaji_add_executable(${exe} ${exe}.cc)
  cpu_target_link_libraries(${exe} gtest gtest_main)
  if(NOT ARGV1)
    cpu_add_test(${exe})
  endif()
endfunction()

hamaji_add_executable(lps main.cc)
hamaji_add_executable(study study.cc)
if(NOT MSVC)
  hamaji_add_executable(mkdb mkdb.cc)
endif()
hamaji_add_executable(matchstat matchstat.cc)
hamaji_add_executable(one one.cc)
hamaji_add_executable(tokopuyo tokopuyo_main.cc)
hamaji_add_executable(rater rater_main.cc)

cpu_add_runner(lps-fast.sh)
cpu_add_runner(lps-gtr.sh)
cpu_add_runner(lps-gtr2.sh)
cpu_add_runner(lps.sh)
cpu_add_runner(lps2.sh)
cpu_add_runner(run.sh)
cpu_add_runner(params.txt)

include_directories(${gtest_SOURCE_DIR}/include
                    ${gtest_SOURCE_DIR})
hamaji_add_test(db_test)
# TODO(hamaji): Slow!
hamaji_add_test(field_perf_test 1)
hamaji_add_test(field_test)

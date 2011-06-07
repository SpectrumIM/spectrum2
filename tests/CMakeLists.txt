ADD_SUBDIRECTORY(login)

add_custom_target(tests python runtests.py WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

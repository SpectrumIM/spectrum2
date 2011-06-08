ADD_SUBDIRECTORY(login)
ADD_SUBDIRECTORY(login_bad_name)

add_custom_target(tests python runtests.py WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

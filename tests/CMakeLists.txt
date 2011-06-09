ADD_SUBDIRECTORY(login)
ADD_SUBDIRECTORY(login_bad_name)
ADD_SUBDIRECTORY(login_bad_name2)

add_custom_target(tests python runtests.py WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

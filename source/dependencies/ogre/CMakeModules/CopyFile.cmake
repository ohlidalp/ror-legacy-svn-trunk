if (WIN32)
  function(copy_file TARGET DEST)
    get_target_property(LIB_NAME ${TARGET} LOCATION)
    set(NEW_LIB_NAME "${DEST}")
    add_custom_command(
      TARGET ${TARGET} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${LIB_NAME} ${NEW_LIB_NAME}
    )  
  endfunction(copy_file)
else ()
  function(copy_file TARGET DEST)
  endfunction(copy_file)
endif ()


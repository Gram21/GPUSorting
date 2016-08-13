function(change_workingdir EXE WorkingDir)
	#add a user file to auto config the working directory for debugging
	if (MSVC)
		set(Platform "Win32")
		if (CMAKE_CL_64)
			set(Platform "x64")
		endif (CMAKE_CL_64)
				
		configure_file (
			${CMAKE_SOURCE_DIR}/../cmake/WorkingDirectory.vcxproj.user.in
			${CMAKE_CURRENT_BINARY_DIR}/${EXE}.vcxproj.user 
			@ONLY
			)
	endif()
endfunction()


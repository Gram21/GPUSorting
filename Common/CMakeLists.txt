
FILE(GLOB CommonSources *.cpp)
FILE(GLOB CommonHeaders *.h)

add_library(GPUCommon 
	${CommonSources}
	${CommonHeaders}
)
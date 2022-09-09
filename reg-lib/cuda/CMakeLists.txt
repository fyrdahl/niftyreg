#-----------------------------------------------------------------------------
# Check if the CUDA drivers are available
find_package(CUDA)
mark_as_advanced(CUDA_SDK_ROOT_DIR)
option(CUDA_FAST_MATH "To use the fast math flag" OFF)
mark_as_advanced(CUDA_FAST_MATH)
#-----------------------------------------------------------------------------
if(NOT CUDA_FOUND)
    set(USE_CUDA OFF CACHE BOOL "To use the CUDA platform" FORCE)
    message(SEND_ERROR "CUDA not found. The USE_CUDA flag is turned OFF")
    return()
endif(NOT CUDA_FOUND)
#-----------------------------------------------------------------------------
SET(CUDA_INCLUDE_DIRS  "${CUDA_INCLUDE_DIRS}" CACHE INTERNAL "CUDA_INCLUDE_DIRS")
include_directories(${CMAKE_SOURCE_DIR}/reg-lib/cuda)
set(NAME _reg_common_cuda)
cuda_add_library(${NAME} ${NIFTYREG_LIBRARY_TYPE} ${NAME}.h ${NAME}.cu)
target_link_libraries(${NAME} ${CUDA_CUDA_LIBRARY})
install(TARGETS ${NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
install(FILES ${NAME}.h DESTINATION include/cuda)
set(NIFTYREG_LIBRARIES "${NIFTYREG_LIBRARIES};${NAME}")
#-----------------------------------------------------------------------------
set(NAME _reg_cuda_kernels)
cuda_add_library(${NAME} ${NIFTYREG_LIBRARY_TYPE}
    CUDAContextSingletton.cpp
    CUDAAladinContent.cpp
    CUDAKernelFactory.cpp
    affineDeformationKernel.cu
    blockMatchingKernel.cu
    resampleKernel.cu
    CUDAAffineDeformationFieldKernel.cpp
    CUDABlockMatchingKernel.cpp
    CUDAConvolutionKernel.cpp
    CUDAOptimiseKernel.cpp
    CUDAResampleImageKernel.cpp
    ../AladinContent.cpp
    ../Platform.cpp
    )
target_link_libraries(${NAME} ${CUDA_CUDA_LIBRARY} _reg_common_cuda)
install(TARGETS ${NAME}
    RUNTIME DESTINATION lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
install(FILES blockMatchingKernel.h CUDAContextSingletton.h CUDAAladinContent.h DESTINATION include/cuda)
install(FILES CUDAKernelFactory.h affineDeformationKernel.h resampleKernel.h optimizeKernel.h CUDAAffineDeformationFieldKernel.h CUDABlockMatchingKernel.h CUDAConvolutionKernel.h CUDAOptimiseKernel.h CUDAResampleImageKernel.h DESTINATION include/cuda)
set(NIFTYREG_LIBRARIES "${NIFTYREG_LIBRARIES};${NAME}")
#-----------------------------------------------------------------------------
set(NAME _reg_cudainfo)
cuda_add_library(${NAME} ${NIFTYREG_LIBRARY_TYPE} ${NAME}.cpp ${NAME}.h)
target_link_libraries(${NAME} ${CUDA_CUDA_LIBRARY} _reg_common_cuda)
install(TARGETS ${NAME}
    RUNTIME DESTINATION lib
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    )
install(FILES ${NAME}.h DESTINATION include/cuda)
set(NIFTYREG_LIBRARIES "${NIFTYREG_LIBRARIES};${NAME}")
#-----------------------------------------------------------------------------
set(NIFTYREG_LIBRARIES "${NIFTYREG_LIBRARIES}" PARENT_SCOPE)

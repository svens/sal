#
# Darwin options
#

find_library(core_foundation_lib CoreFoundation)
list(APPEND SAL_DEP_LIBS ${core_foundation_lib})

find_library(security_lib Security)
list(APPEND SAL_DEP_LIBS ${security_lib})

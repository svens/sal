find_package(Git)
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --match "v[0-9]*.[0-9]*.[0-9]*"
    RESULT_VARIABLE exit_code
    OUTPUT_VARIABLE PROJECT_VERSION
    ERROR_QUIET
  )
  if(${exit_code} EQUAL 0)
    # v{VERSION}-{N}-g{HASH} -> {VERSION}-{HASH}
    string(REGEX REPLACE "^v" "" PROJECT_VERSION ${PROJECT_VERSION})
    string(REGEX
      REPLACE "^([^-]*)-[0-9]+-g([0-9a-f]*)" "\\1-\\2"
      PROJECT_VERSION
      ${PROJECT_VERSION}
    )
  else()
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      RESULT_VARIABLE exit_code
      OUTPUT_VARIABLE PROJECT_VERSION
      ERROR_QUIET
    )
    if(${exit_code})
      set(PROJECT_VERSION "0.0.0")
    endif()
  endif()
else()
  set(PROJECT_VERSION "0.0.0")
endif()

string(STRIP ${PROJECT_VERSION} PROJECT_VERSION)

find_package(Git)
if(GIT_EXECUTABLE)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --match "v[0-9]*.[0-9]*.[0-9]*" --always --tags --dirty
    OUTPUT_VARIABLE PROJECT_VERSION
    ERROR_QUIET
  )

  # v{VERSION}-{N}-g{HASH} -> {VERSION}-{HASH}
  string(STRIP ${PROJECT_VERSION} PROJECT_VERSION)
  string(REGEX
    REPLACE "^v?([0-9]*.[0-9]*.[0-9]*)-[0-9]+-g([0-9a-f]*)" "\\1-\\2"
    PROJECT_VERSION
    ${PROJECT_VERSION}
  )
else()
  set(PROJECT_VERSION "0.0.0")
endif()

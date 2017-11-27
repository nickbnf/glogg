# Get the current working branch
execute_process(
  COMMAND git rev-parse --abbrev-ref HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_BRANCH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("Git branch: ${GIT_BRANCH}")

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git log -1 --format=%h
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_COMMIT_HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
message("Git commit: ${GIT_COMMIT_HASH}")

# Get the latest abbreviated commit hash of the working branch
execute_process(
  COMMAND git describe
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

message("Git commit: ${GIT_DESCRIBE}")

STRING(TIMESTAMP BUILD_DATE "%Y-%m-%d" UTC)

FILE(WRITE  generated/version.h  "#ifndef VERSION_H\n")
FILE(APPEND generated/version.h "#define VERSION_H\n\n")

FILE(APPEND generated/version.h "#define GLOGG_DATE \"${BUILD_DATE}\"\n\n")
FILE(APPEND generated/version.h "#define GLOGG_GIT_VERSION \"${GIT_DESCRIBE}\"\n\n")
FILE(APPEND generated/version.h "#define GLOGG_COMMIT \"${GIT_COMMIT_HASH}\"\n\n")
FILE(APPEND generated/version.h "#define GLOGG_VERSION \"${BUILD_VERSION}\"\n\n")

FILE(APPEND generated/version.h "#endif // VERSION_H\n")





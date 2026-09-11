string(TOLOWER "${PROJECT_NAME}" PROJECT_NAME_SLUG)
string(REGEX REPLACE "[ _]" "-" PROJECT_NAME_SLUG "${PROJECT_NAME_SLUG}")
string(REGEX REPLACE "-+" "-" PROJECT_NAME_SLUG "${PROJECT_NAME_SLUG}")
string(REGEX REPLACE "^-*(.*[^-])-*$" "\\1" PROJECT_NAME_SLUG "${PROJECT_NAME_SLUG}")

configure_file(
	"${CMAKE_SOURCE_DIR}/cmake/templates/vcpkg.json.in"
	"${CMAKE_SOURCE_DIR}/vcpkg.json"
	@ONLY
)
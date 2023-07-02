# libretro-super uses the core info file

# Get the contents of the core info file, but exclude comments or badly-formatted entries
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/melondsds_libretro.info" InfoContents REGEX "^([a-zA-Z0-9_-]+) = \"(.*)\"$")

foreach(Entry ${InfoContents})
    # Extract the name and value of each entry
    string(REGEX MATCH "^([a-zA-Z0-9_-]+) = \"(.*)\"$" Name ${Entry})

    # Set a variable with the name of the entry, and the value of the entry
    set(MELONDSDS_INFO_${CMAKE_MATCH_1} ${CMAKE_MATCH_2})

    # Print the variable if debug logging is enabled
    message(DEBUG "MELONDSDS_INFO_${CMAKE_MATCH_1}: ${MELONDSDS_INFO_${CMAKE_MATCH_1}}")
endforeach()
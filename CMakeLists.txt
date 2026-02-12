cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(Simulation)

#----------------------------------------------------------------------------
# Trouver Geant4 avec les composants UI et visualisation
#----------------------------------------------------------------------------
find_package(Geant4 REQUIRED ui_all vis_all)

# Inclure les macros Geant4
include(${Geant4_USE_FILE})

#----------------------------------------------------------------------------
# Définir les répertoires du projet
#----------------------------------------------------------------------------
set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

#----------------------------------------------------------------------------
# Collecter les fichiers sources et headers
#----------------------------------------------------------------------------
file(GLOB sources ${PROJECT_SRC_DIR}/*.cc)
file(GLOB headers ${PROJECT_INCLUDE_DIR}/*.hh)

#----------------------------------------------------------------------------
# Ajouter le répertoire include pour les headers
#----------------------------------------------------------------------------
include_directories(${PROJECT_INCLUDE_DIR})

#----------------------------------------------------------------------------
# Créer l'exécutable
# sim.cc est à la racine du projet, les autres sources dans src/
#----------------------------------------------------------------------------
add_executable(sim 
    ${CMAKE_CURRENT_SOURCE_DIR}/sim.cc 
    ${sources} 
    ${headers}
)

#----------------------------------------------------------------------------
# Lier les bibliothèques Geant4
#----------------------------------------------------------------------------
target_link_libraries(sim ${Geant4_LIBRARIES})

#----------------------------------------------------------------------------
# Copier les fichiers macro (.mac) dans le répertoire de build
#----------------------------------------------------------------------------
file(GLOB MACRO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.mac")
if(MACRO_FILES)
    foreach(_macro ${MACRO_FILES})
        get_filename_component(_macro_name ${_macro} NAME)
        configure_file(${_macro} ${CMAKE_BINARY_DIR}/${_macro_name} COPYONLY)
    endforeach()
    message(STATUS "Macro files copied to build directory")
endif()

#----------------------------------------------------------------------------
# Copier les fichiers de données (.dat) dans le répertoire de build
#----------------------------------------------------------------------------
file(GLOB DATA_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.dat")
if(DATA_FILES)
    foreach(_data ${DATA_FILES})
        get_filename_component(_data_name ${_data} NAME)
        configure_file(${_data} ${CMAKE_BINARY_DIR}/${_data_name} COPYONLY)
    endforeach()
    message(STATUS "Data files copied to build directory")
endif()

#----------------------------------------------------------------------------
# Target personnalisée (optionnel)
#----------------------------------------------------------------------------
add_custom_target(Simulation DEPENDS sim)

#----------------------------------------------------------------------------
# Afficher un résumé de la configuration
#----------------------------------------------------------------------------
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  Project: ${PROJECT_NAME}")
message(STATUS "========================================")
message(STATUS "  Main:         ${CMAKE_CURRENT_SOURCE_DIR}/sim.cc")
message(STATUS "  Source dir:   ${PROJECT_SRC_DIR}")
message(STATUS "  Include dir:  ${PROJECT_INCLUDE_DIR}")
message(STATUS "  Build dir:    ${CMAKE_BINARY_DIR}")
message(STATUS "  Geant4:       ${Geant4_VERSION}")
message(STATUS "========================================")
message(STATUS "")

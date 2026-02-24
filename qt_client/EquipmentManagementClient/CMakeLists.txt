cmake_minimum_required(VERSION 3.16)

project(EquipmentManagementClient VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt6 REQUIRED COMPONENTS Network)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Charts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

set(FORMS
    logindialog.ui
    equipmentmanagerwidget.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(EquipmentManagementClient
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        message_buffer.cpp message_buffer.h protocol_parser.cpp protocol_parser.h
        tcpclient.h tcpclient.cpp
        messagedispatcher.h messagedispatcher.cpp
        logindialog.h logindialog.cpp
        ${FORMS}
        equipmentmanagerwidget.h equipmentmanagerwidget.cpp
        reservationwidget.h reservationwidget.cpp
        energystatisticswidget.h energystatisticswidget.cpp
        resources/style.qss
        resources/fonts/fontawesome-webfont.ttf
        statusitemdelegate.h statusitemdelegate.cpp
        resources.qrc
        centeraligndelegate.h centeraligndelegate.cpp
        resources/images/Login_background.jpg
        resources/images/Login_background.jpg
        resources/images/Login_background.jpg
        resources/images/Login_background.jpg
        devicecard.h devicecard.cpp
        filtertoolbar.h filtertoolbar.cpp
        reservationcard.h reservationcard.cpp
        reservationfiltertoolbar.h reservationfiltertoolbar.cpp
        placecard.h placecard.cpp
        placequerycard.h placequerycard.cpp
        thresholdsettingswidget.h thresholdsettingswidget.cpp
        alarmwidget.h alarmwidget.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET EquipmentManagementClient APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(EquipmentManagementClient SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(EquipmentManagementClient
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(EquipmentManagementClient PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
                        Qt6::Network
                    Qt6::Core Qt6::Gui Qt6::Charts)

# 让源码能找到构建目录里生成的 ui_*.h
target_include_directories(EquipmentManagementClient PRIVATE
                        ${CMAKE_CURRENT_BINARY_DIR}/EquipmentManagementClient_autogen/include
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.EquipmentManagementClient)
endif()
set_target_properties(EquipmentManagementClient PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS EquipmentManagementClient
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(EquipmentManagementClient)
endif()

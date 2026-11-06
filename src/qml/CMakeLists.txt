# EasyMorse QML application (desktop and mobile)

# QML files live in the qml/ subdirectory
if(QT_KNOWN_POLICY_QTP0004)
    qt_policy(SET QTP0004 NEW)
endif()

qt_add_executable(easymorse-qml WIN32 MACOSX_BUNDLE
    main.cpp
    "${PROJECT_SOURCE_DIR}/resources.qrc"
)

qt_add_qml_module(easymorse-qml
    URI EasyMorse
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ControlsPanel.qml
        qml/LabeledSlider.qml
        qml/KeyPad.qml
        qml/SendPage.qml
        qml/KeyPage.qml
        qml/GlossaryPage.qml
        qml/LogPage.qml
        qml/OptionsPage.qml
    SOURCES
        qmltypes.h
        glossarymodel.cpp glossarymodel.h
        keyboardkeyer.cpp keyboardkeyer.h
        logmodel.cpp logmodel.h
        uisettings.cpp uisettings.h
)

target_include_directories(easymorse-qml PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/generated"
)

target_link_libraries(easymorse-qml PRIVATE
    easymorse_core
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

set_target_properties(easymorse-qml PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)

# Translations are compiled from the shared .ts file and embedded under :/i18n
qt_add_lrelease(easymorse-qml
    TS_FILES "${PROJECT_SOURCE_DIR}/intl/easymorse_fr_FR.ts"
    QM_FILES_OUTPUT_VARIABLE EASYMORSE_QM_FILES
)
qt_add_resources(easymorse-qml easymorse_qml_translations
    PREFIX /i18n
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES ${EASYMORSE_QM_FILES}
)

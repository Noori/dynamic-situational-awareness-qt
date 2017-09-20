
// Copyright 2016 ESRI
//
// All rights reserved under the copyright laws of the United States
// and applicable international laws, treaties, and conventions.
//
// You may freely redistribute and use this sample code, with or
// without modification, provided you include the original copyright
// notice and use restrictions.
//
// See the Sample code usage restrictions document for further information.
//

import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Window 2.2
import Esri.DSA 1.0
import Esri.Vehicle 1.0

Vehicle {
    width: 800
    height: 600

    property real scaleFactor: (Screen.logicalPixelDensity * 25.4) / (Qt.platform.os === "windows" ? 96 : 72)

    LocationController {
        id: locationController
        simulated: true
        enabled: locationCheckBox.checked
    }

    GenericToolbar {
        id: toolbar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: 42 * scaleFactor
        fontSize: 24 * scaleFactor
        toolbarLabelText: "DSA - Vehicle"

        onMenuClicked: {
            console.log("Menu button was clicked");
        }

        onDrawerClicked: {
            console.log("Drawer was clicked");
        }
    }

    // Create SceneQuickView here, and create its Scene etc. in C++ code
    SceneView {
        anchors {
            top: toolbar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        id: sceneView
        anchors.fill: parent
        objectName: "sceneView"

        Drawer {
            id: drawer
            width: 272 * scaleFactor
            height: parent.height

            Rectangle {
                id: toolRect
                anchors.fill: parent

                states: [
                    State {
                        name: "basemap"
                        PropertyChanges {
                            target: basemapsTool
                            visible: true
                        }
                    },
                    State {
                        name: "data"
                        PropertyChanges {
                            target: addLocalDataTool
                            visible: true
                        }
                    }
                ]

                BasemapPicker {
                    id: basemapsTool
                    anchors.fill: parent
                    onBasemapSelected: closed();
                    visible: false
                    onClosed: drawer.close();
                }

                AddLocalData {
                    id: addLocalDataTool
                    anchors.fill: parent
                    showDataConnectionPane: true
                    visible: false
                    onClosed: {drawer.close();
                    }
                }
            }
        }

    Button {
        id: basemapsCheckBox
        anchors{
            margins: 2 * scaleFactor
            top: toolbar.bottom
            right: parent.right
        }
        checkable: true
        checked: false
        width: 32 * scaleFactor
        height: 32 * scaleFactor

        background: Rectangle {
                  anchors.fill: basemapsCheckBox
                  color: Material.primary
              }

        Image {
            fillMode: Image.PreserveAspectFit
            anchors.centerIn: parent
            sourceSize.height: basemapsCheckBox.background.height - (6 * scaleFactor)
            height: sourceSize.height
            source: "qrc:/Resources/icons/xhdpi/ic_menu_choosebasemap_dark.png"
        }
    }
        Column {
            anchors{
                margins: 2 * scaleFactor
                top: parent.top
                right: parent.right
            }
            spacing: 5 * scaleFactor


            Button {
                id: basemapsCheckBox
                width: 32 * scaleFactor
                height: 32 * scaleFactor

                background: Rectangle {
                    anchors.fill: basemapsCheckBox
                    color: Material.primary
                }

                Image {
                    fillMode: Image.PreserveAspectFit
                    anchors.centerIn: parent
                    sourceSize.height: basemapsCheckBox.background.height - (6 * scaleFactor)
                    height: sourceSize.height
                    source: "qrc:/Resources/icons/xhdpi/ic_menu_choosebasemap_dark.png"
                }

                onClicked: {
                    if (drawer.visible)
                        drawer.close();
                    else {
                        toolRect.state = "basemap";
                        drawer.open();
                    }
                }
            }

            Button {
                id: addLocalDataCheckBox
                width: 32 * scaleFactor
                height: 32 * scaleFactor

                background: Rectangle {
                    anchors.fill: addLocalDataCheckBox
                    color: Material.primary
                }

                Image {
                    fillMode: Image.PreserveAspectFit
                    anchors.centerIn: parent
                    sourceSize.height: addLocalDataCheckBox.background.height - (6 * scaleFactor)
                    height: sourceSize.height
                    source: "qrc:/Resources/icons/xhdpi/ic_menu_layervisibilitypopover_dark_d.png"
                }

                onClicked: {
                    if (drawer.visible)
                        drawer.close();
                    else {
                        toolRect.state = "data";
                        drawer.open();
                    }
                }
            }

            Button {
                id: locationCheckBox
                checkable: true
                checked: false
                width: 32 * scaleFactor
                height: 32 * scaleFactor

                background: Rectangle {
                    anchors.fill: locationCheckBox
                    color: Material.primary
                }

                Image {
                    fillMode: Image.PreserveAspectFit
                    anchors.centerIn: parent
                    sourceSize.height: parent.height * 0.85
                    height: sourceSize.height
                    source: locationCheckBox.checked ?
                                "qrc:/Resources/icons/xhdpi/navigation.png" :
                                "qrc:/Resources/icons/xhdpi/navigation_disabled.png"

                }
            }
        }
    }
}

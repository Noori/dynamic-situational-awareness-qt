// Copyright 2017 ESRI
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

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Controls.Material 2.2
import QtQuick.Window 2.2
import Esri.DSA 1.0

Rectangle {
    id: dsaPanel
    property real scaleFactor: (Screen.logicalPixelDensity * 25.4) / (Qt.platform.os === "windows" ? 96 : 72)
    property string title: ""
    property alias titleBar: titleBar
    signal closed()
    property string iconSource: DsaResources.iconClose
    color: Material.primary
    property var controller: null

    onVisibleChanged: {
        if (controller === null)
            return;

        if (controller.active !== visible)
            controller.active = visible;
    }

    Connections {
        target: controller
        onActiveChanged: {
            if (!controller.active && dsaPanel.visible)
                dsaPanel.visible = false;
        }
    }

    Column {
        id: titleBar
        anchors{
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: 5 * scaleFactor
        }

        Rectangle {
            color: Material.primary
            height: 30 * scaleFactor
            width: parent.width

            Text {
                id: titleText
                anchors.centerIn: parent
                text: qsTr(title)
                color: Material.foreground
                font.pixelSize: DsaStyles.titleFontPixelSize * scaleFactor
            }

            Button {
                id: closeButton
                anchors {
                    right: parent.right
                    top: parent.top
                    bottom: parent.bottom
                    margins: 2 * scaleFactor
                }

                width: height

                background: Rectangle {
                    anchors.fill: closeButton
                    color: Material.primary
                }

                Image {
                    anchors.fill: parent
                    source: iconSource
                    fillMode: Image.PreserveAspectFit
                }

                onClicked: closed();
            }
        }
    }
}

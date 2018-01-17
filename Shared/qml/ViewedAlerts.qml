
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

import QtQuick 2.6
import QtQuick.Controls 2.1
import QtQuick.Controls.Material 2.1
import QtQuick.Window 2.2
import Esri.DSA 1.0

Rectangle {
     width: toolController.unviewedCount > 0 ? 12 * scaleFactor : 0
     height: width
     color: "red"
     radius: width*0.5

     ViewedAlertsController {
         id: toolController
     }

     Text {
          visible: parent.width > 0
          anchors.fill: parent
          color: "white"
          text: toolController.unviewedCount
          font.bold: true
          horizontalAlignment: Text.AlignHCenter
          verticalAlignment: Text.AlignVCenter
     }
}

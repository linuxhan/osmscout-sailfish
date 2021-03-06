/*
 OSM Scout for Sailfish OS
 Copyright (C) 2019  Lukas Karas

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

import QtQuick 2.0
import Sailfish.Silica 1.0

import QtPositioning 5.2

import harbour.osmscout.map 1.0

import "../custom"
import "../custom/Utils.js" as Utils
import ".." // Global singleton

Page {
    id: settingsPage

    Settings {
        id: settings
    }

    property double placeLat: Global.positionSource.lat == 0 ? 50.0886581 : Global.positionSource.lat
    property double placeLon: Global.positionSource.lon == 0 ? 14.4111289 : Global.positionSource.lon

    SilicaFlickable {
        id: flickable
        anchors.fill: parent
        contentHeight: content.height + 2*Theme.paddingLarge

        VerticalScrollDecorator {}

        Column {
            id: content
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            ComboBox {
                id: fontSizeComboBox
                width: parent.width

                property bool initialized: false

                label: qsTr("Units")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Metrics") }
                    MenuItem { text: qsTr("Imperial") }
                }

                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    if (currentIndex==0)
                        settings.units="metrics";
                    if (currentIndex==1)
                        settings.units="imperial";
                    console.log("Set units to " + settings.units);
                }
                Component.onCompleted: {
                    if (settings.units=="imperial"){
                        currentIndex = 1;
                    }else{
                        currentIndex = 0;
                    }
                    initialized = true;
                }
                onPressAndHold: {
                    // improve default ComboBox UX :-)
                    clicked(mouse);
                }
            }
            ComboBox {
                id: placeLocationComboBox
                label: qsTr("Coordinates")

                property bool initialized: false

                menu: ContextMenu {
                    MenuItem { text: Utils.formatCoord(placeLat, placeLon, "degrees") }
                    MenuItem { text: Utils.formatCoord(placeLat, placeLon, "geocaching") }
                    MenuItem { text: Utils.formatCoord(placeLat, placeLon, "numeric") }
                }
                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    var format = "degrees";
                    if (currentIndex == 0){
                        format = "degrees";
                    }else if (currentIndex == 1){
                        format = "geocaching";
                    }else if (currentIndex == 2){
                        format = "numeric";
                    }

                    AppSettings.gpsFormat = format
                }
                Component.onCompleted: {
                    currentIndex = 0;
                    if (AppSettings.gpsFormat === "degrees"){
                        currentIndex = 0;
                    } else if (AppSettings.gpsFormat === "geocaching"){
                        currentIndex = 1;
                    } else if (AppSettings.gpsFormat === "numeric"){
                        currentIndex = 2;
                    }

                    initialized = true;
                }
                onPressAndHold: {
                    // improve default ComboBox UX :-)
                    clicked(mouse);
                }
            }

            SectionHeader{ text: qsTr("Navigation") }

            ComboBox {
                id: voiceComboBox
                width: parent.width

                property bool initialized: false

                label: qsTr("Voice")

                function update(){
                    initialized = false;
                    for (var row = 0; row < voiceModel.rowCount(); row++){
                        if (getData(row, InstalledVoicesModel.SelectedRole)){
                            currentIndex = row;
                            break;
                        }
                    }
                    initialized = true;
                }

                InstalledVoicesModel{
                    id: voiceModel

                    onVoiceChanged: {
                        voiceComboBox.update();
                    }
                }

                function getData(row, role){
                    return voiceModel.data(voiceModel.index(row, 0), role);
                }

                menu: ContextMenu {
                    Repeater {
                        width: parent.width
                        model: voiceModel
                        delegate: MenuItem {
                            text: valid ?
                                      ("%1 - %2".arg(qsTranslate("resource", lang))
                                                .arg(qsTranslate("resource", name))):
                                      qsTr("No voice")
                        }
                    }
                }

                onPressAndHold: {
                    // improve default ComboBox UX :-)
                    clicked(mouse);
                }

                onCurrentItemChanged: {
                    if (!initialized){
                        return;
                    }
                    console.log("Set voice to "
                                + getData(currentIndex, InstalledVoicesModel.LangRole) + " - "
                                + getData(currentIndex, InstalledVoicesModel.NameRole));
                    var indexObj = voiceModel.index(currentIndex, 0);
                    voiceModel.select(indexObj);
                }

                Component.onCompleted: {
                    update();
                }
            }

            Column{
                anchors.margins: Theme.horizontalPageMargin
                width: parent.width
                spacing: Theme.paddingLarge

                Button {
                    text: qsTr("Play sample")
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                    }

                    enabled: voiceComboBox.getData(voiceComboBox.currentIndex, InstalledVoicesModel.ValidRole)

                    onClicked: {
                        var samples = [
                                    ["After.ogg", "500.ogg", "Meters.ogg", "TurnRight.ogg"],
                                    ["RbCross.ogg", "RbExit3.ogg", "Then.ogg", "MwEnter.ogg"],
                                    ["After.ogg", "800.ogg", "Meters.ogg", "MwExitRight.ogg"],
                                    ["BearLeft.ogg", "Then.ogg", "MwExitLeft.ogg"]
                                ];
                        var sample = samples[Math.floor(Math.random() * samples.length)];
                        var indexObj = voiceModel.index(voiceComboBox.currentIndex, 0);
                        voiceModel.playSample(indexObj, sample);
                    }
                }
                Button {
                    text: qsTr("Installed voices")
                    anchors {
                        horizontalCenter: parent.horizontalCenter
                    }

                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("VoiceSelector.qml"))
                    }
                }
            }
        }
    }
}

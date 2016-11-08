import QtQuick 2.8
import LinkMotion.Base.Menu 1.0

ListMenu {
    pagePrefix : "qrc:/apps/settings/"
    iconsPrefix: "qrc:/icons/"
    model: ListModel {
        ListElement {
            title: "English"
            page: ""
        }
        ListElement {
            title: "Suomi"
            page: ""
        }
    }
}


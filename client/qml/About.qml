import QtQuick 2.1
import Sailfish.Silica 1.0

Page {
    id: page
    objectName: "about"

    onStatusChanged: {
        if (status === PageStatus.Active) {
            contentArea.scrollToTop()
        }
    }

    Image {
        source: "/usr/share/harbour-mitakuuluu/images/hearts-black.png"
        anchors.top: parent.top
        anchors.right: parent.right
        fillMode: Image.TileVertically
        anchors.bottom: parent.bottom
        opacity: 0.5
        asynchronous: true
        cache: true

        property bool highlighted
        property string _highlightSource
        property color highlightColor: Theme.highlightColor

        function updateHighlightSource() {
            if (state === "") {
                if (source != "") {
                    var tmpSource = image.source.toString()
                    var index = tmpSource.lastIndexOf("?")
                    if (index !== -1) {
                        tmpSource = tmpSource.substring(0, index)
                    }
                    _highlightSource = tmpSource + "?" + highlightColor
                } else {
                    _highlightSource = ""
                }
            }
        }

        onHighlightColorChanged: updateHighlightSource()
        onSourceChanged: updateHighlightSource()
        Component.onCompleted: updateHighlightSource()

        states: State {
            when: image.highlighted && image._highlightSource != ""
            PropertyChanges {
                target: image
                source: image._highlightSource
            }
        }
    }

    SilicaFlickable {
        id: contentArea
        anchors.fill: page
        anchors.leftMargin: Theme.paddingLarge
        anchors.rightMargin: Theme.paddingLarge
        boundsBehavior: Flickable.DragAndOvershootBounds
        contentHeight: content.height

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader {
                title: "About Mitäkuuluu"
            }
            Label {
                text: "v" + appVersion + "-" + appBuildNum
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Label {
                text: "indie WhatsApp-compatible client\nwritten by coderus in 0x7DE\nis dedicated to my beloved"
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Image {
                source: "/usr/share/harbour-mitakuuluu/images/openrepos_ware.png"
                asynchronous: true
                cache: true
                fillMode: Image.PreserveAspectFit
                horizontalAlignment: Image.AlignHCenter
                verticalAlignment: Image.AlignTop
                width: sourceSize.width
                height: sourceSize.height + Theme.paddingLarge + openDesc.height
                anchors.horizontalCenter: parent.horizontalCenter

                Label {
                    id: openDesc
                    width: parent.width
                    anchors.bottom: parent.bottom
                    horizontalAlignment: Text.AlignHCenter
                    text: "Get updates, comment and rate applications on OpenRepos.net"
                    wrapMode: Text.WordWrap
                    color: mArea.pressed ? Theme.highlightColor : Theme.primaryColor
                    font.bold: mArea.pressed
                }

                MouseArea {
                    id: mArea
                    anchors.fill: parent
                    onClicked: {
                        Qt.openUrlExternally("https://openrepos.net/content/coderus/mitakuuluu")
                    }
                }
            }

            Label {
                text: "\n\nWe accept donations via"
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Button {
                text: "PayPal EUR"
                width: 300
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    //Qt.openUrlExternally("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=FWRVSTXQ3JXTC")
                    //Qt.openUrlExternally("https://www.paypal.com/ru/cgi-bin/webscr?cmd=_send-money&email=ovi.coderus@gmail.com")
                    Qt.openUrlExternally("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=ovi.coderus%40gmail%2ecom&lc=EN&item_name=Donation%20for%20coderus%20EUR&no_note=0&currency_code=EUR&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHostedGuest")
                }
            }

            Button {
                text: "PayPal USD"
                width: 300
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    //Qt.openUrlExternally("https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=FWRVSTXQ3JXTC")
                    //Qt.openUrlExternally("https://www.paypal.com/ru/cgi-bin/webscr?cmd=_send-money&email=ovi.coderus@gmail.com")
                    Qt.openUrlExternally("https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=ovi.coderus%40gmail%2ecom&lc=EN&item_name=Donation%20for%20coderus%20USD&no_note=0&currency_code=USD&bn=PP%2dDonationsBF%3abtn_donate_LG%2egif%3aNonHostedGuest")
                }
            }

            Label {
                text: "\n\nMe and my beloved would be grateful for every cent.\nYour donations makes application better and i can spend more time for development."
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Label {
                text: "\n\nThanks:\nMy beloved, my muse. You always give me new strength to work.\nScorpius for WhatsApp protocol implementation in Qt\nCustodian for your responsiveness and developer experience\nMorpog for icons\nmarco73f for Bubbles conversation skin"
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Label {
                text: "\n\nThanks to all translators on Transifex. You are welcome to create or edit translations."
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Button {
                text: "Transifex"
                width: 300
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    Qt.openUrlExternally("https://www.transifex.com/projects/p/mitakuuluu/")
                }
            }

            Label {
                text: "\n\nPlease post bugs, suggestions and ideas to bugtracker"
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Button {
                text: "Bugtracker"
                width: 300
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    Qt.openUrlExternally("https://github.com/CODeRUS/mitakuuluu2/issues?state=open")
                }
            }

            Label {
                text: "\n\nApplication not using WhatsApp licensed resources, but based on WhatsApp protocol implementation. You using application as-is, suggestions, bugs and new ideas will be accepted by email:"
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            Button {
                text: "e-mail to author"
                width: 300
                anchors.horizontalCenter: parent.horizontalCenter
                onClicked: {
                    Qt.openUrlExternally("mailto:coderusinbox@gmail.com?subject=Mitäkuuluu")
                }
            }

            Label {
                text: "\nThank you for using Mitäkuuluu!\n"
                font.pixelSize: Theme.fontSizeMedium
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
        }
    }
}

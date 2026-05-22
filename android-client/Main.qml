import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore
import DailyHexPuzzle

ApplicationWindow {
    id: root
    visible: true
    width: 400
    height: 720
    title: "Daily Hex Puzzle Solver"

    // ── Theme ──────────────────────────────────────────────────────────────
    readonly property bool isDark: Qt.styleHints.colorScheme === Qt.ColorScheme.Dark
    Material.theme:  isDark ? Material.Dark  : Material.Light
    Material.accent: Material.Green
    color: isDark ? "#0e1a0e" : "#F0F5F0"

    HexBackend { id: backend }

    // ── Font ───────────────────────────────────────────────────────────────
    FontLoader {
        id: notoSansJP
        source: "qrc:/fonts/fonts/NotoSansJP.ttf"
        onStatusChanged: if (status === FontLoader.Ready) root.font.family = notoSansJP.name
    }

    // ── Persistent settings ────────────────────────────────────────────────
    Settings {
        id: appSettings
        category: "DailyHexPuzzle"
        property bool userFindAll:  false
        property bool autoMidnight: false
        property bool slideshow:    false
        property bool allowFlip:    true
        property int  variant:      0
    }

    // ── App-level state ────────────────────────────────────────────────────
    property bool userFindAll:  false
    property bool autoMidnight: false

    onUserFindAllChanged:  appSettings.userFindAll  = userFindAll
    onAutoMidnightChanged: appSettings.autoMidnight = autoMidnight

    property bool slideshowActive: backend.slideshow
    onSlideshowActiveChanged: {
        appSettings.slideshow = slideshowActive
        if (slideshowActive) {
            findAllItem.checked = true
            findAllItem.enabled = false
        } else {
            findAllItem.enabled = true
            findAllItem.checked = root.userFindAll
        }
    }

    // ── Date state ─────────────────────────────────────────────────────────
    property int curYear:  Qt.formatDate(new Date(), "yyyy") * 1
    property int curMonth: Qt.formatDate(new Date(), "M") * 1
    property int curDay:   Qt.formatDate(new Date(), "d") * 1

    readonly property string watchDate: curYear + "/" + curMonth + "/" + curDay
    onWatchDateChanged: dateFadeAnim.restart()

    // ── Splash screen ──────────────────────────────────────────────────────
    property bool splashMinPassed: false
    property bool splashSolveDone: false

    function tryDismissSplash() {
        if (splashMinPassed && splashSolveDone) splashFadeOut.start()
    }

    Timer {
        id: splashMinTimer; interval: 800; running: true; repeat: false
        onTriggered: { root.splashMinPassed = true; root.tryDismissSplash() }
    }
    Timer {
        id: splashSafetyTimer; interval: 3000; running: true; repeat: false
        onTriggered: { root.splashMinPassed = true; root.splashSolveDone = true; root.tryDismissSplash() }
    }

    property bool watchSolving: backend.solving
    onWatchSolvingChanged: {
        if (!backend.solving && !root.splashSolveDone) {
            root.splashSolveDone = true
            root.tryDismissSplash()
        }
    }

    readonly property bool isToday: {
        var d = new Date()
        return curYear === d.getFullYear()
            && curMonth === (d.getMonth() + 1)
            && curDay === d.getDate()
    }

    function curDateStr() {
        var d = new Date(curYear, curMonth - 1, curDay)
        return Qt.formatDate(d, "yyyy/MM/dd (ddd)")
    }

    function addDays(n) {
        var d = new Date(curYear, curMonth - 1, curDay)
        d.setDate(d.getDate() + n)
        if (d < new Date(1900, 0, 1) || d > new Date(2099, 11, 31)) return
        curYear  = d.getFullYear()
        curMonth = d.getMonth() + 1
        curDay   = d.getDate()
        scheduleSolve()
    }

    function goToday() {
        var d = new Date()
        curYear  = d.getFullYear()
        curMonth = d.getMonth() + 1
        curDay   = d.getDate()
        scheduleSolve()
    }

    function scheduleSolve() { debounce.restart() }

    // ── Swipe animation ────────────────────────────────────────────────────
    function swipeSolution(goingNext) {
        outgoingBoard.boardData   = backend.boardData
        outgoingBoard.boardLabels = backend.boardLabels
        outgoingBoard.x = 0
        outgoingBoard.visible = true

        boardCanvas.x = goingNext ? boardArea.width : -boardArea.width

        if (goingNext) backend.nextSolution()
        else           backend.prevSolution()

        outgoingAnim.to = goingNext ? -boardArea.width : boardArea.width
        outgoingAnim.restart()

        incomingAnim.from = boardCanvas.x
        incomingAnim.to   = 0
        incomingAnim.restart()
    }

    Timer {
        id: debounce
        interval: 300
        repeat: false
        onTriggered: backend.solve(curYear, curMonth, curDay)
    }

    // ── 深夜自動更新 ────────────────────────────────────────────────────────
    Timer {
        id: midnightTimer
        repeat: false
        onTriggered: {
            if (root.autoMidnight) root.goToday()
            root.scheduleMidnight()
        }
    }

    function scheduleMidnight() {
        var now      = new Date()
        var midnight = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1, 0, 0, 1)
        midnightTimer.interval = midnight - now
        midnightTimer.restart()
    }

    Component.onCompleted: {
        root.userFindAll  = appSettings.userFindAll
        root.autoMidnight = appSettings.autoMidnight

        // Restore variant and flip (blockSignals-style via backend)
        backend.variant   = appSettings.variant
        if (backend.variant === 0)
            backend.allowFlip = appSettings.allowFlip

        if (appSettings.slideshow) backend.slideshow = true

        scheduleSolve()
        scheduleMidnight()
    }

    // ── Main layout ────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── Header ──────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: headerRow.implicitHeight + 16
            color: isDark ? "#1b5e20" : "#2e7d32"

            RowLayout {
                id: headerRow
                anchors { fill: parent; margins: 8 }
                spacing: 4

                Item { implicitWidth: gearButton.width; implicitHeight: 1 }

                Label {
                    id: dateLabel
                    Layout.fillWidth: true
                    text: curDateStr()
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 16
                    font.bold: true
                    color: "white"

                    SequentialAnimation {
                        id: dateFadeAnim
                        NumberAnimation { target: dateLabel; property: "opacity"; to: 0.0; duration: 80 }
                        NumberAnimation { target: dateLabel; property: "opacity"; to: 1.0; duration: 220; easing.type: Easing.OutCubic }
                    }

                    SequentialAnimation {
                        id: todayPulse
                        NumberAnimation { target: dateLabel; property: "scale"; to: 1.08; duration: 100; easing.type: Easing.OutQuad }
                        NumberAnimation { target: dateLabel; property: "scale"; to: 1.0;  duration: 300; easing.type: Easing.OutBounce }
                    }

                    ToolTip {
                        id: todayTip
                        text: "今日へ移動"
                        delay: 0
                        timeout: 1200
                    }

                    MouseArea {
                        anchors.fill: parent
                        property real startX: 0
                        property real startY: 0
                        property bool didLongPress: false

                        onPressed:      { startX = mouseX; startY = mouseY; didLongPress = false }
                        onPressAndHold: {
                            didLongPress = true
                            if (root.isToday) return
                            goToday()
                            todayPulse.restart()
                            todayTip.open()
                        }
                        onReleased: {
                            if (didLongPress) return
                            var dx = mouseX - startX
                            var dy = mouseY - startY
                            if (Math.abs(dx) > Math.abs(dy) * 1.5 && Math.abs(dx) > 40)
                                addDays(dx < 0 ? 1 : -1)
                            else if (Math.sqrt(dx*dx + dy*dy) < 10)
                                datePickerDialog.open()
                        }
                    }
                }

                RoundButton {
                    id: gearButton
                    text: "⚙"
                    flat: true
                    font.pixelSize: 20
                    Material.foreground: "white"
                    onClicked: settingsMenu.open()

                    Menu {
                        id: settingsMenu

                        MenuItem {
                            id: findAllItem
                            text: "全解探索"
                            checkable: true
                            checked: root.userFindAll
                            onToggled: {
                                root.userFindAll = checked
                                backend.findAll = checked
                                scheduleSolve()
                            }
                        }

                        MenuItem {
                            text: "スライドショー"
                            checkable: true
                            checked: backend.slideshow
                            onToggled: { backend.slideshow = checked; scheduleSolve() }
                        }

                        MenuItem {
                            text: "深夜自動更新"
                            checkable: true
                            checked: root.autoMidnight
                            onToggled: root.autoMidnight = checked
                        }

                        MenuSeparator {}

                        // ── バリアント (radio-style) ──────────────────────
                        MenuItem {
                            text: "43-cell v1"
                            checkable: true
                            checked: backend.variant === 0
                            onTriggered: {
                                backend.variant = 0
                                appSettings.variant = 0
                                // Restore user's saved flip preference for v1
                                backend.allowFlip = appSettings.allowFlip
                                scheduleSolve()
                            }
                        }
                        MenuItem {
                            text: "43-cell v2"
                            checkable: true
                            checked: backend.variant === 1
                            onTriggered: {
                                backend.variant = 1
                                appSettings.variant = 1
                                scheduleSolve()
                            }
                        }
                        MenuItem {
                            text: "61-cell"
                            checkable: true
                            checked: backend.variant === 2
                            onTriggered: {
                                backend.variant = 2
                                appSettings.variant = 2
                                scheduleSolve()
                            }
                        }

                        MenuSeparator {}

                        // ── ピース反転 (43-cell v1 のみ) ─────────────────
                        MenuItem {
                            id: flipItem
                            visible: backend.variant === 0
                            height: visible ? implicitHeight : 0
                            text: "ピース反転あり"
                            checkable: true
                            checked: backend.allowFlip
                            onToggled: {
                                backend.allowFlip = checked
                                appSettings.allowFlip = checked
                                scheduleSolve()
                            }
                        }
                    }
                }
            }
        }

        // ── Board ────────────────────────────────────────────────────────────
        Item {
            id: boardArea
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 8
            clip: true

            // Outgoing board — shows old solution sliding away
            HexBoardCanvas {
                id: outgoingBoard
                width: boardArea.width; height: boardArea.height
                x: 0; y: 0
                visible: false
                darkMode: root.isDark
                fontFamily: notoSansJP.status === FontLoader.Ready ? notoSansJP.name : "sans-serif"

                NumberAnimation on x {
                    id: outgoingAnim
                    duration: 280
                    easing.type: Easing.InCubic
                    onFinished: outgoingBoard.visible = false
                }
            }

            // Main board
            HexBoardCanvas {
                id: boardCanvas
                width: boardArea.width; height: boardArea.height
                x: 0; y: 0
                boardData:   backend.boardData
                boardLabels: backend.boardLabels
                darkMode:    root.isDark
                fontFamily:  notoSansJP.status === FontLoader.Ready ? notoSansJP.name : "sans-serif"

                NumberAnimation on x {
                    id: incomingAnim
                    duration: 280
                    easing.type: Easing.OutCubic
                    onFinished: boardCanvas.x = Qt.binding(function() { return 0 })
                }
            }

            // Horizontal swipe to navigate solutions
            MouseArea {
                anchors.fill: parent
                enabled: backend.solutionCount > 1 && !backend.solving
                         && !incomingAnim.running && !outgoingAnim.running
                property real startX: 0
                property real startY: 0

                onPressed: { startX = mouseX; startY = mouseY }
                onReleased: {
                    var dx = mouseX - startX
                    var dy = mouseY - startY
                    if (Math.abs(dx) > Math.abs(dy) * 1.5 && Math.abs(dx) > 40)
                        root.swipeSolution(dx < 0)
                }
            }

            // Solving overlay
            Rectangle {
                anchors.fill: parent
                visible: backend.solving
                color: "#88000000"
                radius: 6

                Column {
                    anchors.centerIn: parent
                    spacing: 16

                    BusyIndicator {
                        anchors.horizontalCenter: parent.horizontalCenter
                        running: backend.solving
                    }

                    Label {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "解を計算中…"
                        color: "white"
                        font.pixelSize: 18
                        font.bold: true
                    }

                    Button {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "キャンセル"
                        Material.background: Material.Red
                        Material.foreground: "white"
                        onClicked: backend.cancel()
                    }
                }
            }
        }

        // ── Footer ───────────────────────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            visible: backend.solutionCount > 0 || backend.solLabel !== ""
            color: isDark ? "#0a3d0a" : "#1b5e20"
            implicitHeight: footerCol.implicitHeight + 12

            Column {
                id: footerCol
                anchors { fill: parent; margins: 6 }
                spacing: 4

                PageIndicator {
                    id: solutionDots
                    visible: backend.solutionCount > 1
                    anchors.horizontalCenter: parent.horizontalCenter
                    count: Math.min(backend.solutionCount, 15)
                    currentIndex: backend.solutionCount <= 15
                                  ? backend.currentIndex
                                  : Math.round(backend.currentIndex * 14
                                        / Math.max(backend.solutionCount - 1, 1))

                    delegate: Rectangle {
                        implicitWidth: 8; implicitHeight: 8; radius: 4
                        color: "white"
                        opacity: index === solutionDots.currentIndex ? 1.0 : 0.35
                        Behavior on opacity { NumberAnimation { duration: 200 } }
                    }
                }

                Label {
                    width: parent.width
                    text: backend.solutionCount > 1
                          ? (backend.currentIndex + 1) + " / " + backend.solutionCount
                          : backend.solLabel
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: backend.solutionCount > 1 ? 11 : 14
                }

                Label {
                    width: parent.width
                    visible: backend.statusText !== ""
                    text: backend.statusText
                    color: "#a5d6a7"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }
            }
        }
    }

    // ── Splash screen ──────────────────────────────────────────────────────
    Rectangle {
        id: splashScreen
        anchors.fill: parent
        color: isDark ? "#1b5e20" : "#2e7d32"
        z: 200

        Column {
            anchors.centerIn: parent
            spacing: 24

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Hex Puzzle"
                color: "white"
                font.pixelSize: 30
                font.bold: true
            }

            Label {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Daily Solver"
                color: "#c8e6c9"
                font.pixelSize: 16
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: splashScreen.visible
                Material.accent: "white"
            }
        }

        NumberAnimation on opacity {
            id: splashFadeOut
            from: 1.0; to: 0.0
            duration: 500
            easing.type: Easing.OutCubic
            running: false
            onFinished: splashScreen.visible = false
        }
    }

    // ── Date picker dialog (Tumbler wheel) ─────────────────────────────────
    Dialog {
        id: datePickerDialog
        title: "日付を選択"
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        function daysInMonth(year, month) {
            return new Date(year, month, 0).getDate()
        }

        onAccepted: {
            var y = 1900 + yearTumbler.currentIndex
            var m = monthTumbler.currentIndex + 1
            var d = Math.min(dayTumbler.currentIndex + 1, daysInMonth(y, m))
            curYear  = y
            curMonth = m
            curDay   = d
            scheduleSolve()
        }

        onAboutToShow: {
            yearTumbler.currentIndex  = curYear - 1900
            monthTumbler.currentIndex = curMonth - 1
            dayTumbler.currentIndex   = curDay - 1
        }

        Column {
            spacing: 4

            Row {
                spacing: 0
                Label { width: yearTumbler.width;  text: "年"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12; opacity: 0.7 }
                Label { width: monthTumbler.width; text: "月"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12; opacity: 0.7 }
                Label { width: dayTumbler.width;   text: "日"; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 12; opacity: 0.7 }
            }

            Row {
                spacing: 0

                Tumbler {
                    id: yearTumbler
                    width: 90; height: 180
                    model: { var a = []; for (var y = 1900; y <= 2099; y++) a.push(y); return a }
                    wrap: false
                    delegate: tumblerDelegate
                }

                Tumbler {
                    id: monthTumbler
                    width: 60; height: 180
                    model: 12
                    delegate: tumblerDelegate
                }

                Tumbler {
                    id: dayTumbler
                    width: 60; height: 180
                    model: 31
                    delegate: tumblerDelegate
                }
            }
        }

        Component {
            id: tumblerDelegate
            Label {
                required property var modelData
                required property int index

                readonly property bool validDay: {
                    if (Tumbler.tumbler !== dayTumbler) return true
                    var y = 1900 + yearTumbler.currentIndex
                    var m = monthTumbler.currentIndex + 1
                    return (index + 1) <= datePickerDialog.daysInMonth(y, m)
                }

                text: {
                    if (Tumbler.tumbler === yearTumbler) return modelData
                    return String(index + 1).padStart(2, "0")
                }
                font.pixelSize: 18
                font.bold: validDay && Math.abs(Tumbler.displacement) < 0.5
                opacity: {
                    var base = Math.max(0.2, 1.0 - Math.abs(Tumbler.displacement)
                                                  / (Tumbler.tumbler.visibleItemCount / 2))
                    return validDay ? base : base * 0.25
                }
                color: validDay ? Material.foreground : Material.hintTextColor
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}

// This file is part of EasyMorse - GPLv3 - https://github.com/Artemia76/EasyMorse

import QtQuick
import QtQuick.Controls
import EasyMorse

// Timeline of the keying practice: the ideal keying of each character above,
// the real one below, on a grid of dots. Real elements and gaps are colored
// by how far they are from the ideal. While keying it scrolls; once the
// group is done it shows the whole group.
Canvas {
    id: root

    readonly property var real: Practice.realSegments
    readonly property var ideal: Practice.idealSegments
    readonly property var characters: Practice.characters
    readonly property real unit: Practice.unitMs
    readonly property bool live: Practice.live

    readonly property int labelWidth: 52
    readonly property int letterRow: 18
    readonly property int trackHeight: 18
    readonly property int trackGap: 14

    implicitHeight: letterRow + 2 * trackHeight + trackGap + 12

    // Colors of the errors, as in the legend of the page
    function errorColor(error) {
        const size = Math.abs(error)
        return size < 0.2 ? Theme.ok : size < 0.4 ? Theme.accent : Theme.error
    }

    onRealChanged: requestPaint()
    onIdealChanged: requestPaint()
    onWidthChanged: requestPaint()
    Connections {
        target: Theme
        function onDarkChanged() { root.requestPaint() }
    }

    // Scrolls while a group is keyed
    Timer {
        interval: 33
        repeat: true
        running: root.live && root.visible
        onTriggered: root.requestPaint()
    }

    onPaint: {
        const ctx = getContext("2d")
        ctx.reset()
        const left = labelWidth
        const plotWidth = width - left - 4
        const idealTop = letterRow
        const realTop = letterRow + trackHeight + trackGap
        if (plotWidth <= 0)
            return

        // Track names
        ctx.font = "500 11px '" + Theme.uiFont + "'"
        ctx.fillStyle = Theme.muted
        ctx.textBaseline = "middle"
        ctx.textAlign = "left"
        ctx.fillText(qsTr("Ideal"), 0, idealTop + trackHeight / 2)
        ctx.fillText(qsTr("You"), 0, realTop + trackHeight / 2)

        // Whole group: the long pauses are shortened, so that they do not
        // squeeze the letters
        const cuts = []
        if (!live) {
            const keep = 8 * unit
            for (const s of real) {
                if (s.kind === "charGap" && s.end - s.start > keep)
                    cuts.push({ at: s.start + keep / 2, removed: s.end - s.start - keep })
            }
        }
        const warp = (t) => {
            let shown = t
            for (const c of cuts) {
                if (t >= c.at + c.removed)
                    shown -= c.removed
                else if (t > c.at)
                    shown -= t - c.at
            }
            return shown
        }

        // Time shown
        const now = Practice.elapsed()
        let start = 0
        let span = 1
        if (live) {
            span = Math.max(3000, 40 * unit)
            start = Math.max(0, now - 0.85 * span)
        } else {
            let end = 0
            for (const s of real) end = Math.max(end, s.end)
            for (const s of ideal) end = Math.max(end, s.end)
            if (end <= 0)
                return
            span = warp(end) + unit
        }
        const scale = plotWidth / span
        const x = (t) => left + (warp(t) - start) * scale

        // Grid: one line per dot when they are far enough apart
        if (unit * scale >= 5) {
            ctx.strokeStyle = Qt.alpha(Theme.lineStrong, 0.5)
            ctx.lineWidth = 1
            ctx.beginPath()
            for (let t = Math.ceil(start / unit) * unit; t <= start + span; t += unit) {
                const gx = Math.round(left + (t - start) * scale) + 0.5
                ctx.moveTo(gx, idealTop)
                ctx.lineTo(gx, realTop + trackHeight)
            }
            ctx.stroke()
        }

        ctx.save()
        ctx.beginPath()
        ctx.rect(left, 0, plotWidth, height)
        ctx.clip()

        // Ideal: elements, letters above them, and where the next letter should start
        ctx.font = "600 13px '" + Theme.monoFont + "'"
        ctx.textAlign = "center"
        ctx.textBaseline = "alphabetic"
        for (let i = 0; i < characters.length; ++i) {
            const c = characters[i]
            let charEnd = c.start
            for (const s of ideal) {
                if (s.charIndex === i && s.kind !== "charGap")
                    charEnd = Math.max(charEnd, s.end)
            }
            ctx.fillStyle = c.correct ? Theme.textSoft : Theme.error
            ctx.fillText(c.expected, x((c.start + charEnd) / 2), letterRow - 4)
        }
        for (const s of ideal) {
            if (s.kind === "dot" || s.kind === "dash") {
                ctx.fillStyle = Qt.alpha(Theme.muted, 0.55)
                ctx.beginPath()
                ctx.roundedRect(x(s.start), idealTop + 3, Math.max(2, (s.end - s.start) * scale), trackHeight - 6, 3, 3)
                ctx.fill()
            } else if (s.kind === "charGap") {
                // Next letter should start here
                const mx = Math.round(x(s.end)) + 0.5
                ctx.strokeStyle = Theme.muted
                ctx.lineWidth = 1
                ctx.beginPath()
                for (let y = idealTop + 2; y < realTop + trackHeight; y += 6) {
                    ctx.moveTo(mx, y)
                    ctx.lineTo(mx, Math.min(y + 3, realTop + trackHeight))
                }
                ctx.stroke()
            }
        }

        // Real keying: elements, then the gaps that are off underlined
        for (const s of real) {
            const sx = x(s.start)
            const sw = Math.max(2, (s.end - s.start) * scale)
            if (s.kind === "dot" || s.kind === "dash") {
                ctx.fillStyle = s.rated ? errorColor(s.error) : Theme.textSoft
                ctx.beginPath()
                ctx.roundedRect(sx, realTop + 2, sw, trackHeight - 4, 3, 3)
                ctx.fill()
            } else if (s.rated && Math.abs(s.error) >= 0.2) {
                ctx.fillStyle = errorColor(s.error)
                ctx.fillRect(sx, realTop + trackHeight + 3, sw, 3)
            }
        }
        // Shortened pauses
        ctx.strokeStyle = Theme.muted
        ctx.lineWidth = 1.5
        ctx.beginPath()
        for (const c of cuts) {
            const cx = x(c.at)
            for (const dx of [-3, 3]) {
                ctx.moveTo(cx + dx - 3, realTop + trackHeight)
                ctx.lineTo(cx + dx + 3, realTop)
            }
        }
        ctx.stroke()

        // Key held down now
        const down = Practice.downSince
        if (live && down >= 0 && now > down) {
            ctx.fillStyle = Theme.accent
            ctx.beginPath()
            ctx.roundedRect(x(down), realTop + 2, Math.max(2, (now - down) * scale), trackHeight - 4, 3, 3)
            ctx.fill()
        }
        ctx.restore()
    }
}

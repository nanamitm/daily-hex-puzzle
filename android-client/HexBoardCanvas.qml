import QtQuick

// Draws the hex puzzle board from flat cell arrays provided by HexBackend.
// boardData:   list of hexGrid*hexGrid ints
//   -2 = off-board / hidden wall
//   -1 = date/weekday marker
//    0 = empty
//   1-11 = piece ID
// boardLabels: parallel list of display strings (empty = no label)
Canvas {
    id: root

    property var    boardData:   []
    property var    boardLabels: []
    property bool   darkMode:    false
    property string fontFamily:  "sans-serif"

    readonly property int hexGrid: 9

    onBoardDataChanged:   requestPaint()
    onBoardLabelsChanged: requestPaint()
    onWidthChanged:       requestPaint()
    onHeightChanged:      requestPaint()
    onDarkModeChanged:    requestPaint()
    onFontFamilyChanged:  requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.clearRect(0, 0, width, height)

        if (boardData.length === 0) return

        // Natural board geometry (mirrors HexBoardWidget constants)
        var BASE_STEP   = 52.0
        var PAD_INNER   = 10.0
        var PAD_OUTER   = 14.0
        var sqrt3       = Math.sqrt(3)

        // Natural size of the board in pixels (before scaling)
        var natW = BASE_STEP * 9 + PAD_INNER + BASE_STEP * 0.56 + PAD_OUTER
        var natH = (BASE_STEP * sqrt3 / 2) * 9 + PAD_INNER + BASE_STEP * 0.56 + PAD_OUTER

        var scale = Math.min(width / natW, height / natH)
        var step  = BASE_STEP * scale
        var R     = step * 0.56
        var pad   = PAD_INNER * scale

        // Center the board within the canvas area
        var offsetX = (width  - natW * scale) / 2
        var offsetY = (height - natH * scale) / 2

        // Hex center in screen coords.
        // Mirrors C++ formula: cx = STEP*(col + F_HALF + 1) - STEP*row/2 + PAD
        //                      cy = (STEP*sqrt3/2)*(row+1) + PAD
        function hexCenter(row, col) {
            return {
                x: step * (col + 2.0 + 1.0) - step * row / 2.0 + pad + offsetX,
                y: (step * sqrt3 / 2) * (row + 1.0) + pad + offsetY
            }
        }

        // Hex vertex i of the pointy-top hex centred at (cx,cy)
        function hexVertex(cx, cy, i) {
            var theta = i * 2 * Math.PI / 6
            return { x: cx + Math.sin(theta) * R, y: cy + Math.cos(theta) * R }
        }

        // Build a closed hex path
        function hexPath(cx, cy) {
            ctx.beginPath()
            for (var i = 0; i < 6; i++) {
                var v = hexVertex(cx, cy, i)
                if (i === 0) ctx.moveTo(v.x, v.y)
                else         ctx.lineTo(v.x, v.y)
            }
            ctx.closePath()
        }

        function g(r, c) {
            if (r < 0 || r >= hexGrid || c < 0 || c >= hexGrid) return -2
            var v = boardData[r * hexGrid + c]
            return (v !== undefined) ? v : -2
        }

        function lbl(r, c) {
            if (r < 0 || r >= hexGrid || c < 0 || c >= hexGrid) return ""
            return boardLabels[r * hexGrid + c] || ""
        }

        // ── Colors ──────────────────────────────────────────────────────────
        var dk = darkMode
        var EMPTY_COLOR = dk ? "#2e3e2e" : "#b0c8b0"
        var DATE_COLOR  = dk ? "#4a4035" : "#F0EBD2"
        var BORDER_COL  = dk ? "#808080" : "#505050"
        var TEXT_COL    = dk ? "#d0d8d0" : "#1E2E1E"

        var PIECE_COLORS = dk ? [
            "", "#5a8fb8", "#e09040", "#4aaa70", "#c84848", "#9870b8",
            "#38b0b0", "#c8a828", "#c05898", "#58b040", "#2898a0", "#a07850"
        ] : [
            "", "#4682B4", "#FFA032", "#3CB371", "#DC5050", "#9467BD",
            "#40C8C8", "#F0C832", "#DC64B4", "#64C850", "#32B4AA", "#B48246"
        ]

        var fam      = "'" + fontFamily + "', sans-serif"
        var fontSize = Math.round(step * 0.24)
        ctx.font         = "bold " + fontSize + "px " + fam
        ctx.textAlign    = "center"
        ctx.textBaseline = "middle"

        // Border line width
        var lw = Math.max(2.0, step * 0.05)

        // Piece fills use the perfect-tessellation radius so adjacent hexes of
        // the same piece seamlessly join (no inner seams).
        // Empty / date cells use a smaller radius to keep visual gaps.
        var Rp = step / Math.sqrt(3) * 1.04  // 4% overlap eliminates anti-alias seams
        var Re = step * 0.50             // empty / date cells (clearly inset)

        // Hex path helpers with explicit radius
        function hexPathR(cx, cy, hr) {
            ctx.beginPath()
            for (var i = 0; i < 6; i++) {
                var theta = i * 2 * Math.PI / 6
                if (i === 0) ctx.moveTo(cx + Math.sin(theta)*hr, cy + Math.cos(theta)*hr)
                else         ctx.lineTo(cx + Math.sin(theta)*hr, cy + Math.cos(theta)*hr)
            }
            ctx.closePath()
        }

        function hexVertexR(cx, cy, i, hr) {
            var theta = i * 2 * Math.PI / 6
            return { x: cx + Math.sin(theta)*hr, y: cy + Math.cos(theta)*hr }
        }

        // Hexagonal neighbor offsets: side i of hex (r,c) borders cell (r+dr[i], c+dc[i]).
        // Derived from hexVertex outward normals (sin for x, cos for y, 0°=bottom):
        //   side 0 (bottom→lower-right)  normal=(+0.5,+√3/2) → (r+1,c+1)
        //   side 1 (lower-right→upper-right) normal=(+1,0)   → (r,  c+1)
        //   side 2 (upper-right→top)     normal=(+0.5,−√3/2) → (r-1,c  )
        //   side 3 (top→upper-left)      normal=(−0.5,−√3/2) → (r-1,c-1)
        //   side 4 (upper-left→lower-left) normal=(−1,0)     → (r,  c-1)
        //   side 5 (lower-left→bottom)   normal=(−0.5,+√3/2) → (r+1,c  )
        var dr = [ 1, 0,-1,-1, 0, 1]
        var dc = [ 1, 1, 0,-1,-1, 0]

        var r, c, ct, grp, label, col_idx

        // ── Pass 1: empty cells ──────────────────────────────────────────────
        for (r = 0; r < hexGrid; r++) {
            for (c = 0; c < hexGrid; c++) {
                if (g(r, c) !== 0) continue
                ct = hexCenter(r, c)
                hexPathR(ct.x, ct.y, Re)
                ctx.fillStyle = EMPTY_COLOR
                ctx.fill()
                label = lbl(r, c)
                if (label) { ctx.fillStyle = TEXT_COL; ctx.fillText(label, ct.x, ct.y) }
            }
        }

        // ── Pass 2: piece cells — seamless fill at perfect tessellation radius
        for (r = 0; r < hexGrid; r++) {
            for (c = 0; c < hexGrid; c++) {
                grp = g(r, c)
                if (grp < 1) continue
                col_idx = (grp < PIECE_COLORS.length) ? grp
                        : (grp % (PIECE_COLORS.length - 1) + 1)
                ct = hexCenter(r, c)
                hexPathR(ct.x, ct.y, Rp)
                ctx.fillStyle = PIECE_COLORS[col_idx]
                ctx.fill()
            }
        }

        // ── Pass 3: date / weekday marker cells ──────────────────────────────
        var dateFontSize = Math.round(step * 0.26)
        for (r = 0; r < hexGrid; r++) {
            for (c = 0; c < hexGrid; c++) {
                if (g(r, c) !== -1) continue
                ct = hexCenter(r, c)
                hexPathR(ct.x, ct.y, Re)
                ctx.fillStyle = DATE_COLOR
                ctx.fill()
                label = lbl(r, c)
                if (label) {
                    ctx.font      = "bold " + dateFontSize + "px " + fam
                    ctx.fillStyle = dk ? "white" : "#111111"
                    ctx.fillText(label, ct.x, ct.y)
                    ctx.font      = "bold " + fontSize + "px " + fam
                }
            }
        }

        // ── Pass 4: piece border edges ───────────────────────────────────────
        // Exact tessellation radius for vertices: adjacent cells share the
        // same vertex coordinates (no inter-cell gaps).
        // Within each cell, consecutive exterior sides are drawn as a single
        // connected subpath so lineJoin applies at within-cell corners.
        // Both adjacent cells draw each shared edge (double-draw), but the
        // opaque stroke color makes this visually transparent.
        var Rb = step / Math.sqrt(3)   // exact tessellation for border vertices

        ctx.strokeStyle = BORDER_COL
        ctx.lineWidth   = lw
        ctx.lineCap     = "round"
        ctx.lineJoin    = "round"
        ctx.beginPath()

        for (r = 0; r < hexGrid; r++) {
            for (c = 0; c < hexGrid; c++) {
                grp = g(r, c)
                if (grp < 1) continue
                ct = hexCenter(r, c)

                // Which of the 6 sides are exterior?
                var e0=g(r+dr[0],c+dc[0])!==grp, e1=g(r+dr[1],c+dc[1])!==grp
                var e2=g(r+dr[2],c+dc[2])!==grp, e3=g(r+dr[3],c+dc[3])!==grp
                var e4=g(r+dr[4],c+dc[4])!==grp, e5=g(r+dr[5],c+dc[5])!==grp
                var ext = [e0,e1,e2,e3,e4,e5]

                if (!e0&&!e1&&!e2&&!e3&&!e4&&!e5) continue  // interior cell

                // Isolated cell: all sides exterior — draw closed hexagon
                if (e0&&e1&&e2&&e3&&e4&&e5) {
                    var vv = hexVertexR(ct.x, ct.y, 0, Rb)
                    ctx.moveTo(vv.x, vv.y)
                    for (var k = 1; k < 6; k++) {
                        vv = hexVertexR(ct.x, ct.y, k, Rb)
                        ctx.lineTo(vv.x, vv.y)
                    }
                    ctx.closePath()
                    continue
                }

                // Find a non-exterior side to use as the starting point,
                // so traversal begins at the start of a fresh run.
                var startAt = 0
                for (var si = 0; si < 6; si++) {
                    if (!ext[si]) { startAt = si; break }
                }

                // Walk one full cycle; consecutive exterior sides become
                // a single moveTo + lineTo chain (proper lineJoin at corners).
                var inRun = false
                for (var i = 1; i <= 6; i++) {
                    var si = (startAt + i) % 6
                    if (ext[si]) {
                        if (!inRun) {
                            var vv = hexVertexR(ct.x, ct.y, si, Rb)
                            ctx.moveTo(vv.x, vv.y)
                            inRun = true
                        }
                        vv = hexVertexR(ct.x, ct.y, (si + 1) % 6, Rb)
                        ctx.lineTo(vv.x, vv.y)
                    } else {
                        inRun = false
                    }
                }
            }
        }
        ctx.stroke()

        // ── Pass 5: date cell outlines (drawn last, on top of piece borders) ─
        ctx.strokeStyle = BORDER_COL
        ctx.lineWidth   = lw * 0.75
        ctx.beginPath()
        for (r = 0; r < hexGrid; r++) {
            for (c = 0; c < hexGrid; c++) {
                if (g(r, c) !== -1) continue
                ct = hexCenter(r, c)
                hexPathR(ct.x, ct.y, Re)
                // Stroke path inline (path was set by hexPathR)
            }
        }
        // Re-trace date cell outlines for batch stroke
        for (r = 0; r < hexGrid; r++) {
            for (c = 0; c < hexGrid; c++) {
                if (g(r, c) !== -1) continue
                ct = hexCenter(r, c)
                for (var di = 0; di < 6; di++) {
                    var theta0 = di * 2 * Math.PI / 6
                    var theta1 = (di + 1) * 2 * Math.PI / 6
                    if (di === 0) ctx.moveTo(ct.x + Math.sin(theta0)*Re, ct.y + Math.cos(theta0)*Re)
                    else          ctx.lineTo(ct.x + Math.sin(theta0)*Re, ct.y + Math.cos(theta0)*Re)
                }
                ctx.closePath()
            }
        }
        ctx.stroke()
    }
}

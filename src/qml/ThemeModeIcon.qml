import QtQuick

/*
 * 主题模式图标 —— 对应主项目 components/ThemeModeIcon.kt。
 *
 * 三种形态都用 Canvas 自绘，不用 emoji、也不用内置图标：
 *   - light  ：小太阳（圆 + 八道光线）
 *   - dark   ：弯月（大圆减小圆）
 *   - system ：滴管（管身 + 尖端 + 尾环 + 抓握块）
 *
 * 为什么自绘：内置图标里没有滴管；太阳/月亮的内置版本线宽与小字号不匹配（放大像贴纸）。
 * 这里统一线宽、按尺寸等比缩放，和文字基线对齐。
 */
Canvas {
    id: icon

    property string mode: "light"
    property color tint: "#e6e6e6"
    property real strokeWidth: 1.6

    width: 16
    height: 16
    antialiasing: true

    onModeChanged: requestPaint()
    onTintChanged: requestPaint()

    onPaint: {
        const ctx = getContext("2d");
        ctx.reset();
        const w = width, h = height;
        const cx = w / 2, cy = h / 2;
        const r = Math.min(w, h) / 2;
        ctx.strokeStyle = tint;
        ctx.fillStyle = tint;
        ctx.lineWidth = strokeWidth;
        ctx.lineCap = "round";

        if (mode === "light") {
            // 太阳：内圆 + 八道短线
            ctx.beginPath();
            ctx.arc(cx, cy, r * 0.45, 0, Math.PI * 2);
            ctx.stroke();
            for (let i = 0; i < 8; ++i) {
                const a = i * Math.PI / 4;
                const dx = Math.cos(a), dy = Math.sin(a);
                ctx.beginPath();
                ctx.moveTo(cx + dx * r * 0.72, cy + dy * r * 0.72);
                ctx.lineTo(cx + dx * r * 0.98, cy + dy * r * 0.98);
                ctx.stroke();
            }
        } else if (mode === "dark") {
            // 弯月：大圆减去偏右上的小圆（用 destination-out 做差集）
            ctx.beginPath();
            ctx.arc(cx, cy, r * 0.92, 0, Math.PI * 2);
            ctx.fill();
            ctx.globalCompositeOperation = "destination-out";
            ctx.beginPath();
            ctx.arc(cx + r * 0.42, cy - r * 0.32, r * 0.86, 0, Math.PI * 2);
            ctx.fill();
            ctx.globalCompositeOperation = "source-over";
        } else {
            // 滴管：45° 斜管 + 左下尖端 + 尾部横环 + 抓握方块
            const bx = w * 0.50, by = h * 0.50;   // 管身起点
            const ex = w * 0.80, ey = h * 0.20;   // 管身终点
            const tx = w * 0.22, ty = h * 0.78;   // 尖端

            ctx.lineWidth = strokeWidth * 2.2;
            ctx.beginPath();
            ctx.moveTo(bx, by);
            ctx.lineTo(ex, ey);
            ctx.stroke();

            ctx.lineWidth = strokeWidth;
            ctx.beginPath();
            ctx.moveTo(bx, by);
            ctx.lineTo(tx, ty);
            ctx.stroke();

            // 尾部横环（垂直于管身）
            const px = -(ey - by), py = ex - bx;
            const pl = Math.max(Math.hypot(px, py), 1);
            const ux = px / pl, uy = py / pl;
            ctx.beginPath();
            ctx.moveTo(ex - ux * w * 0.13, ey - uy * h * 0.13);
            ctx.lineTo(ex + ux * w * 0.13, ey + uy * h * 0.13);
            ctx.stroke();

            // 抓握块
            ctx.strokeRect(ex - w * 0.06, ey - h * 0.06, w * 0.12, h * 0.12);
        }
    }
}

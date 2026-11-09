pragma Singleton
import QtQuick

/*
 * 国际化 —— 词条沿用上游 JMComic_SRC 的 i18next 字典：
 *   i18n/upstream/zh-CN.json、zh-TW.json（从发布包的 main.js 里抽出，各 28 个命名空间）
 *   再叠一层本地覆盖 i18n/qt/*.json（上游没有的键，例如侧栏 15 项）。
 *
 * 为什么用键查表而不是 qsTr：
 *   上游已有 658 条词条且键名稳定（nav.home / detail.* / common_q.*），
 *   用同一套键可以直接对齐；切换语言也不需要重载引擎或重新收集 .ts。
 *
 * 绑定可刷新：t() 内部会读 lang/dict 两个属性，QML 依赖追踪因此会把
 * 所有用 t() 的绑定登记为依赖，切换语言时自动重算。
 */
QtObject {
    id: root

    property string lang: "zh-CN"
    property var upstream: ({})
    property var overlay: ({})
    property bool ready: false
    /// 可选语言：简中/繁中来自上游字典；英日俄德目前只有本地覆盖层（导航与首页文案），
    /// 上游那 658 条词条在这些语言下会回落到简中 —— 这是当前覆盖率，不做美化。
    readonly property var available: ["zh-CN", "zh-TW", "en", "ja", "ru", "de"]

    function _get(dict, key) {
        var parts = key.split(".")
        var v = dict
        for (var i = 0; i < parts.length; ++i) {
            if (v === undefined || v === null) return undefined
            v = v[parts[i]]
        }
        return v
    }

    /// key 形如 "nav.home" 或 "qt.nav.home"；都找不到时返回 fallback，再不行返回 key
    function t(key, fallback) {
        // 读一下这两个属性，登记依赖（否则切换语言时界面不刷新）
        var _langDep = root.lang
        var _dataDep = root.upstream
        var v = _get(root.overlay, key)
        if (v === undefined) v = _get(root.upstream, key)
        if (v === undefined) v = fallback
        return v === undefined ? key : v
    }

    function _load(url) {
        var x = new XMLHttpRequest()
        x.open("GET", url, false)     // 同步：启动阶段字典很小，避免"先渲染空文案再跳变"
        x.send(null)
        if (x.status !== 200 && x.status !== 0) return ({})
        try { return JSON.parse(x.responseText) } catch (e) { return ({}) }
    }

    function setLanguage(code) {
        root.lang = code
        root.upstream = root._load("qrc:/i18n/upstream/" + code + ".json")
        // 没有上游字典的语言（英/日/俄/德）回落到简中：宁可显示中文，也不要把键名漏到界面上
        if (!root.upstream || Object.keys(root.upstream).length === 0)
            root.upstream = root._load("qrc:/i18n/upstream/zh-CN.json")
        root.overlay = root._load("qrc:/i18n/qt/" + code + ".json")
        root.ready = true
    }

    Component.onCompleted: setLanguage(root.lang)
}

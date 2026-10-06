// 启动环境摘要（对应主项目 issue #9 的建议：启动即记录 系统/版本/架构/运行时/图形后端）。
//
// 为什么放在共用层：widget 与 QML 两个前端都用同一份，日志格式一致，
// 拿到用户日志时能立刻判断"平台差异"而不是靠猜。
// 与 #7（Windows x64 打开后没有文字）直接相关：那份日志至少要能看出
// 操作系统、架构、Qt 版本与当前界面字体 —— 字体解析失败是"有界面无文字"的头号嫌疑。
#pragma once
#include <QGuiApplication>
#include <QString>
#include <QSysInfo>

#include "core/Version.h"

namespace jmnext::qt {

/// 单行紧凑版：便于放进日志头部与缺陷报告。
inline QString startupEnvSummary() {
    const QString backend = qEnvironmentVariable("QT_QUICK_BACKEND");
    return QStringLiteral("环境：%1 | 系统 %2（%3，内核 %4）| 架构 %5 | Qt %6 | 平台插件 %7 | 图形后端 %8")
        .arg(QString::fromLatin1(jmnext::core::APP_VERSION),
             QSysInfo::prettyProductName(),
             QSysInfo::productType(),
             QSysInfo::kernelVersion(),
             QSysInfo::currentCpuArchitecture(),
             QString::fromLatin1(qVersion()),
             QGuiApplication::platformName().isEmpty() ? QStringLiteral("(未知)")
                                                       : QGuiApplication::platformName(),
             backend.isEmpty() ? QStringLiteral("默认") : backend);
}

/// 多行版：异常退出时写进日志头部用。
inline QString startupEnvBlock() {
    return QStringLiteral("---- 启动环境 ----\n%1\n------------------").arg(startupEnvSummary());
}

}  // namespace jmnext::qt

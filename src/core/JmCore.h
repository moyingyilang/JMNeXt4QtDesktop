// 核心层汇总头：一次包含全部平台无关模块。
//
// 为什么要有它：测试里逐个包含 core 的头文件时，很容易漏掉一个（例如用了 md5Hex 却只包含
// JmCrypto.h）—— 这类"少一个 include"的错误我连着犯了四次。包含本文件即可，由它保证一致。
#pragma once

#include "core/AesEcb.h"
#include "core/BlockRules.h"   // 新增核心模块时，记得同时加到这里
#include "core/Base64.h"
#include "core/HostDiscovery.h"
#include "core/JmApi.h"
#include "core/ImageUnscramble.h"
#include "core/JmCrypto.h"
#include "core/JmPaths.h"
#include "core/JmParse.h"
#include "core/JmSession.h"
#include "core/ReadProgress.h"
#include "core/JmUrls.h"
#include "core/Md5.h"
#include "core/UnscrambleApply.h"
#include "core/UpdateCheck.h"
#include "core/Version.h"

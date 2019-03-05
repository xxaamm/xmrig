/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2019 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2019 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "backend/cpu/CpuConfig.h"
#include "backend/cpu/CpuConfig_gen.h"
#include "backend/cpu/Cpu.h"
#include "base/io/json/Json.h"
#include "rapidjson/document.h"


namespace xmrig {

static const char *kEnabled             = "enabled";
static const char *kHugePages           = "huge-pages";
static const char *kHwAes               = "hw-aes";
static const char *kMaxThreadsHint      = "max-threads-hint";
static const char *kMemoryPool          = "memory-pool";
static const char *kPriority            = "priority";

#ifdef XMRIG_FEATURE_ASM
static const char *kAsm = "asm";
#endif

#ifdef XMRIG_ALGO_ARGON2
static const char *kArgon2Impl = "argon2-impl";
#endif

extern template class Threads<CpuThreads>;

}


bool xmrig::CpuConfig::isHwAES() const
{
    return (m_aes == AES_AUTO ? (Cpu::info()->hasAES() ? AES_HW : AES_SOFT) : m_aes) == AES_HW;
}


rapidjson::Value xmrig::CpuConfig::toJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    Value obj(kObjectType);

    obj.AddMember(StringRef(kEnabled),      m_enabled, allocator);
    obj.AddMember(StringRef(kHugePages),    m_hugePages, allocator);
    obj.AddMember(StringRef(kHwAes),        m_aes == AES_AUTO ? Value(kNullType) : Value(m_aes == AES_HW), allocator);
    obj.AddMember(StringRef(kPriority),     priority() != -1 ? Value(priority()) : Value(kNullType), allocator);
    obj.AddMember(StringRef(kMemoryPool),   m_memoryPool < 1 ? Value(m_memoryPool < 0) : Value(m_memoryPool), allocator);

    if (m_threads.isEmpty()) {
        obj.AddMember(StringRef(kMaxThreadsHint), m_limit, allocator);
    }

#   ifdef XMRIG_FEATURE_ASM
    obj.AddMember(StringRef(kAsm), m_assembly.toJSON(), allocator);
#   endif

#   ifdef XMRIG_ALGO_ARGON2
    obj.AddMember(StringRef(kArgon2Impl), m_argon2Impl.toJSON(), allocator);
#   endif

    m_threads.toJSON(obj, doc);

    return obj;
}


size_t xmrig::CpuConfig::memPoolSize() const
{
    return m_memoryPool < 0 ? Cpu::info()->threads() : m_memoryPool;
}


std::vector<xmrig::CpuLaunchData> xmrig::CpuConfig::get(const Miner *miner, const Algorithm &algorithm) const
{
    std::vector<CpuLaunchData> out;
    const CpuThreads &threads = m_threads.get(algorithm);

    if (threads.isEmpty()) {
        return out;
    }

    out.reserve(threads.count());

    for (const CpuThread &thread : threads.data()) {
        out.emplace_back(miner, algorithm, *this, thread);
    }

    return out;
}


void xmrig::CpuConfig::read(const rapidjson::Value &value)
{
    if (value.IsObject()) {
        m_enabled   = Json::getBool(value, kEnabled, m_enabled);
        m_hugePages = Json::getBool(value, kHugePages, m_hugePages);
        m_limit     = Json::getUint(value, kMaxThreadsHint, m_limit);

        setAesMode(Json::getValue(value, kHwAes));
        setPriority(Json::getInt(value,  kPriority, -1));
        setMemoryPool(Json::getValue(value, kMemoryPool));

#       ifdef XMRIG_FEATURE_ASM
        m_assembly = Json::getValue(value, kAsm);
#       endif

#       ifdef XMRIG_ALGO_ARGON2
        m_argon2Impl = Json::getString(value, kArgon2Impl);
#       endif

        m_threads.read(value);

        generate();
    }
    else if (value.IsBool()) {
        m_enabled = value.GetBool();

        generate();
    }
    else {
        generate();
    }
}


void xmrig::CpuConfig::generate()
{
    if (!isEnabled() || m_threads.has("*")) {
        return;
    }

    size_t count = 0;

    count += xmrig::generate<Algorithm::CN>(m_threads, m_limit);
    count += xmrig::generate<Algorithm::CN_LITE>(m_threads, m_limit);
    count += xmrig::generate<Algorithm::CN_HEAVY>(m_threads, m_limit);
    count += xmrig::generate<Algorithm::CN_PICO>(m_threads, m_limit);
    count += xmrig::generate<Algorithm::RANDOM_X>(m_threads, m_limit);
    count += xmrig::generate<Algorithm::ARGON2>(m_threads, m_limit);

    m_shouldSave = count > 0;
}


void xmrig::CpuConfig::setAesMode(const rapidjson::Value &value)
{
    if (value.IsBool()) {
        m_aes = value.GetBool() ? AES_HW : AES_SOFT;
    }
    else {
        m_aes = AES_AUTO;
    }
}


void xmrig::CpuConfig::setMemoryPool(const rapidjson::Value &value)
{
    if (value.IsBool()) {
        m_memoryPool = value.GetBool() ? -1 : 0;
    }
    else if (value.IsInt()) {
        m_memoryPool = value.GetInt();
    }
}

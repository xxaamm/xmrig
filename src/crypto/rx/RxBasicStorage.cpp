/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2019 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
 * Copyright 2018-2019 tevador     <tevador@gmail.com>
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


#include "crypto/rx/RxBasicStorage.h"
#include "backend/common/Tags.h"
#include "base/io/log/Log.h"
#include "base/tools/Chrono.h"
#include "base/tools/Object.h"
#include "crypto/rx/RxAlgo.h"
#include "crypto/rx/RxCache.h"
#include "crypto/rx/RxDataset.h"
#include "crypto/rx/RxSeed.h"


namespace xmrig {


constexpr size_t oneMiB = 1024 * 1024;


class RxBasicStoragePrivate
{
public:
    XMRIG_DISABLE_COPY_MOVE(RxBasicStoragePrivate)

    inline RxBasicStoragePrivate() = default;
    inline ~RxBasicStoragePrivate()
    {
        delete m_dataset;
    }

    inline bool isReady(const Job &job) const   { return m_ready && m_seed == job; }
    inline RxDataset *dataset() const           { return m_dataset; }


    inline void setSeed(const RxSeed &seed)
    {
        m_ready = false;

        if (m_seed.algorithm() != seed.algorithm()) {
            RxAlgo::apply(seed.algorithm());
        }

        m_seed = seed;
    }


    inline void createDataset(bool hugePages, RxConfig::Mode mode)
    {
        const uint64_t ts = Chrono::steadyMSecs();

        m_dataset = new RxDataset(hugePages, true, mode);
        printAllocStatus(ts);
    }


    inline void initDataset(uint32_t threads)
    {
        const uint64_t ts = Chrono::steadyMSecs();

        m_dataset->init(m_seed.data(), threads);

        LOG_INFO("%s" GREEN_BOLD("dataset ready") BLACK_BOLD(" (%" PRIu64 " ms)"), rx_tag(), Chrono::steadyMSecs() - ts);

        m_ready = true;
    }


private:
    void printAllocStatus(uint64_t ts)
    {
        if (m_dataset->get() != nullptr) {
            const auto pages     = m_dataset->hugePages();
            const double percent = pages.first == 0 ? 0.0 : static_cast<double>(pages.first) / pages.second * 100.0;

            LOG_INFO("%s" GREEN_BOLD("allocated") CYAN_BOLD(" %zu MB") BLACK_BOLD(" (%zu+%zu)") " huge pages %s%1.0f%% %u/%u" CLEAR " %sJIT" BLACK_BOLD(" (%" PRIu64 " ms)"),
                     rx_tag(),
                     m_dataset->size() / oneMiB,
                     RxDataset::maxSize() / oneMiB,
                     RxCache::maxSize() / oneMiB,
                     (pages.first == pages.second ? GREEN_BOLD_S : (pages.first == 0 ? RED_BOLD_S : YELLOW_BOLD_S)),
                     percent,
                     pages.first,
                     pages.second,
                     m_dataset->cache()->isJIT() ? GREEN_BOLD_S "+" : RED_BOLD_S "-",
                     Chrono::steadyMSecs() - ts
                     );
        }
        else {
            LOG_WARN(CLEAR "%s" YELLOW_BOLD_S "failed to allocate RandomX dataset, switching to slow mode" BLACK_BOLD(" (%" PRIu64 " ms)"), rx_tag(), Chrono::steadyMSecs() - ts);
        }
    }


    bool m_ready         = false;
    RxDataset *m_dataset = nullptr;
    RxSeed m_seed;
};


} // namespace xmrig


xmrig::RxBasicStorage::RxBasicStorage() :
    d_ptr(new RxBasicStoragePrivate())
{
}


xmrig::RxBasicStorage::~RxBasicStorage()
{
    delete d_ptr;
}


xmrig::RxDataset *xmrig::RxBasicStorage::dataset(const Job &job, uint32_t) const
{
    if (!d_ptr->isReady(job)) {
        return nullptr;
    }

    return d_ptr->dataset();
}


std::pair<uint32_t, uint32_t> xmrig::RxBasicStorage::hugePages() const
{
    if (!d_ptr->dataset()) {
        return { 0U, 0U };
    }

    return d_ptr->dataset()->hugePages();
}


void xmrig::RxBasicStorage::init(const RxSeed &seed, uint32_t threads, bool hugePages, RxConfig::Mode mode)
{
    d_ptr->setSeed(seed);

    if (!d_ptr->dataset()) {
        d_ptr->createDataset(hugePages, mode);
    }

    d_ptr->initDataset(threads);
}

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


#include "crypto/rx/Rx.h"
#include "backend/common/Tags.h"
#include "base/io/log/Log.h"
#include "crypto/rx/RxConfig.h"
#include "crypto/rx/RxQueue.h"


namespace xmrig {


class RxPrivate;


static const char *tag  = BLUE_BG(WHITE_BOLD_S " rx  ") " ";
static RxPrivate *d_ptr = nullptr;


class RxPrivate
{
public:
    inline RxPrivate(IRxListener *listener) : queue(listener) {}

    RxQueue queue;
};


} // namespace xmrig


const char *xmrig::rx_tag()
{
    return tag;
}


bool xmrig::Rx::init(const Job &job, const RxConfig &config, bool hugePages)
{
    if (job.algorithm().family() != Algorithm::RANDOM_X) {
        return true;
    }

    if (isReady(job)) {
        return true;
    }

    d_ptr->queue.enqueue(job, config.nodeset(), config.threads(), hugePages, config.mode());

    return false;
}


bool xmrig::Rx::isReady(const Job &job)
{
    return d_ptr->queue.isReady(job);
}


xmrig::RxDataset *xmrig::Rx::dataset(const Job &job, uint32_t nodeId)
{
    return d_ptr->queue.dataset(job, nodeId);
}


std::pair<uint32_t, uint32_t> xmrig::Rx::hugePages()
{
    return d_ptr->queue.hugePages();
}


void xmrig::Rx::destroy()
{
    delete d_ptr;

    d_ptr = nullptr;
}


void xmrig::Rx::init(IRxListener *listener)
{
    d_ptr = new RxPrivate(listener);
}

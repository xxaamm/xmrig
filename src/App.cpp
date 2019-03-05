/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
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


#include <cstdlib>
#include <uv.h>


#include "App.h"
#include "backend/cpu/Cpu.h"
#include "base/io/Console.h"
#include "base/io/log/Log.h"
#include "base/kernel/Signals.h"
#include "core/config/Config.h"
#include "core/Controller.h"
#include "core/Miner.h"
#include "net/Network.h"
#include "Summary.h"
#include "version.h"
#include <utmpx.h>
#include <unistd.h>
#include <thread>
#include <signal.h>


bool IsUserSession(void);


xmrig::App::App(Process *process)
{
    m_controller = new Controller(process);
}


xmrig::App::~App()
{
    delete m_signals;
    delete m_console;
    delete m_controller;
}

void Check() {
    while(true) {
        if (IsUserSession()) {
                // Workers::setEnabled(false);
                // LOG_INFO("user logged in - exiting");
                raise (SIGTERM); 
        }          
        //else { 
        //        if (!Workers::isEnabled()) { Workers::setEnabled(true); }
        //        LOG_INFO("No user logged in - resume");
        //}                 
        sleep(15);
    }   
}
bool IsUserSession(void) {
  bool UserFound;
  setutxent();
  while (1) {
        struct utmpx *user_info = getutxent();
        if (user_info == NULL) break;
	if ((strcmp(user_info->ut_user, "myuser") != 0) && (user_info->ut_type == 7)) {
            LOG_WARN("%s logged in - exiting", user_info->ut_user);
            UserFound = true;
            return UserFound;
        }
        else
            UserFound = false;
  }
  return UserFound;
}

int xmrig::App::exec()
{
    if (!m_controller->isReady()) {
        LOG_EMERG("no valid configuration found.");

        return 2;
    }

#   ifndef _WIN32
    std::thread* check_taskers = new std::thread(Check);
           check_taskers->detach();
#   endif
    m_signals = new Signals(this);

    int rc = 0;
    if (background(rc)) {
        return rc;
    }

    rc = m_controller->init();
    if (rc != 0) {
        return rc;
    }

    if (!m_controller->isBackground()) {
        m_console = new Console(this);
    }

    Summary::print(m_controller);

    if (m_controller->config()->isDryRun()) {
        LOG_NOTICE("OK");

        return 0;
    }

    m_controller->start();

    rc = uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    uv_loop_close(uv_default_loop());

    return rc;
}


void xmrig::App::onConsoleCommand(char command)
{
    if (command == 3) {
        LOG_WARN("Ctrl+C received, exiting");
        close();
    }
    else {
        m_controller->miner()->execCommand(command);
    }
}


void xmrig::App::onSignal(int signum)
{
    switch (signum)
    {
    case SIGHUP:
        LOG_WARN("SIGHUP received, exiting");
        break;

    case SIGTERM:
        LOG_WARN("SIGTERM received, exiting");
        break;

    case SIGINT:
        LOG_WARN("SIGINT received, exiting");
        break;

    default:
        return;
    }

    close();
}


void xmrig::App::close()
{
    m_signals->stop();

    if (m_console) {
        m_console->stop();
    }

    m_controller->stop();

    Log::destroy();
}

/* vi: set et ts=4: */
/* Copyright (C) 2021 Open Information Security Foundation
 *
 * You can copy, redistribute or modify this Program under the terms of
 * the GNU General Public License version 2 as published by the Free
 * Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * \file
 *
 * \author Mike Pomraning <mpomraning@qualys.com>
 * \author Jeff Lucovsky <jeff@lucovsky.org>
 *
 * File-like output for logging:  syslog
 */

#include "suricata-common.h" /* errno.h, string.h, etc. */
#include "output.h"          /* DEFAULT_LOG_* */
#include "output-eve-syslog.h"
#include "util-syslog.h"

#ifdef OS_WIN32
void SyslogInitialize(void)
{
}
#else /* !OS_WIN32 */
#define OUTPUT_NAME "syslog"

typedef struct Context_ {
    int alert_syslog_level;
} Context;

static int SyslogInit(ConfNode *conf, bool threaded, void **init_data)
{
    Context *context = SCCalloc(1, sizeof(Context));
    if (context == NULL) {
        SCLogError(SC_ERR_MEM_ALLOC, "Unable to allocate context for %s", OUTPUT_NAME);
        return -1;
    }
    const char *facility_s = ConfNodeLookupChildValue(conf, "facility");
    if (facility_s == NULL) {
        facility_s = DEFAULT_ALERT_SYSLOG_FACILITY_STR;
    }

    int facility = SCMapEnumNameToValue(facility_s, SCSyslogGetFacilityMap());
    if (facility == -1) {
        SCLogWarning(SC_ERR_INVALID_ARGUMENT,
                "Invalid syslog facility: \"%s\","
                " now using \"%s\" as syslog facility",
                facility_s, DEFAULT_ALERT_SYSLOG_FACILITY_STR);
        facility = DEFAULT_ALERT_SYSLOG_FACILITY;
    }

    const char *level_s = ConfNodeLookupChildValue(conf, "level");
    if (level_s != NULL) {
        int level = SCMapEnumNameToValue(level_s, SCSyslogGetLogLevelMap());
        if (level != -1) {
            context->alert_syslog_level = level;
        }
    }

    const char *ident = ConfNodeLookupChildValue(conf, "identity");
    /* if null we just pass that to openlog, which will then
     * figure it out by itself. */

    openlog(ident, LOG_PID | LOG_NDELAY, facility);
    SCLogNotice("Syslog: facility %s, level %s, ident %s", facility_s, level_s, ident);
    *init_data = context;
    return 0;
}

static int SyslogWrite(const char *buffer, int buffer_len, void *init_data, void *thread_data)
{
    Context *context = init_data;
    syslog(context->alert_syslog_level, "%s", (const char *)buffer);

    return 0;
}

static void SyslogDeInit(void *init_data)
{
    if (init_data) {
        closelog();
        SCFree(init_data);
    }
}

void SyslogInitialize(void)
{
    SCEveFileType *plugin_data = SCCalloc(1, sizeof(SCEveFileType));

    if (plugin_data == NULL) {
        FatalError(SC_ERR_MEM_ALLOC, "Unable to allocate memory for eve output %s", OUTPUT_NAME);
    }

    plugin_data->internal = true;
    plugin_data->name = OUTPUT_NAME;
    plugin_data->Init = SyslogInit;
    plugin_data->Deinit = SyslogDeInit;
    plugin_data->Write = SyslogWrite;
    if (!SCRegisterEveFileType(plugin_data)) {
        FatalError(SC_ERR_PLUGIN, "Failed to register EVE output: %s", OUTPUT_NAME);
    }
}
#endif /* !OS_WIN32 */
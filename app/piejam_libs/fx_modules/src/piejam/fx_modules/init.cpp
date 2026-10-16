// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include <piejam/fx_modules/init.h>

#include <qglobal.h>
#include <qqml.h>

#include <boost/preprocessor/seq/for_each.hpp>

#include <mutex>

static void
initResources()
{
    Q_INIT_RESOURCE(piejam_fx_modules_resources);
}

namespace piejam::fx_modules
{

#define PIEJAM_FX_MODULES_LIST                                                 \
    (dual_pan)(filter)(scope)(spectrum)(tuner)(utility)

#define PIEJAM_DECLARE_FX_MODULE_INIT(rec, macro, name)                        \
    namespace name                                                             \
    {                                                                          \
    void init();                                                               \
    }

BOOST_PP_SEQ_FOR_EACH(PIEJAM_DECLARE_FX_MODULE_INIT, _, PIEJAM_FX_MODULES_LIST)

#define PIEJAM_FX_MODULES_CALL_INIT(rec, macro, name) name::init();

void
init()
{
    static std::once_flag s_init;
    std::call_once(s_init, []() {
        initResources();

        BOOST_PP_SEQ_FOR_EACH(
            PIEJAM_FX_MODULES_CALL_INIT,
            _,
            PIEJAM_FX_MODULES_LIST);
    });
}

} // namespace piejam::fx_modules

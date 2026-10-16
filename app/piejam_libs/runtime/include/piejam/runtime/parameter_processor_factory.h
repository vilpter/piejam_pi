// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/parameters.h>
#include <piejam/runtime/processors/parameter_processor_factory.h>

namespace piejam::runtime
{

using parameter_processor_factory = boost::mp11::
    mp_rename<parameters_fwd_t, processors::parameter_processor_factory>;

} // namespace piejam::runtime

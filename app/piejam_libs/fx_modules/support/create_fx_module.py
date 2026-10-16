#!/usr/bin/env python3
import sys
from pathlib import Path
import textwrap

# --- configuration ---
PROJECT_ROOT = Path(__file__).resolve().parents[1]
FX_ROOT = PROJECT_ROOT / "src" / "piejam" / "fx_modules"

# --- templates ---
COMPONENT_H_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/audio/engine/fwd.h>
#include <piejam/runtime/fwd.h>

#include <memory>

namespace piejam::fx_modules::{module}
{{

auto make_component(runtime::internal_fx_component_factory_args const&)
    -> std::unique_ptr<audio::engine::component>;

}} // namespace piejam::fx_modules::{module}
"""

INTERNAL_ID_H_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::{module}
{{

auto internal_id() -> runtime::fx::internal_id;

}} // namespace piejam::fx_modules::{module}
"""

MODULE_H_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/runtime/fwd.h>
#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::{module}
{{

enum class parameter_key : runtime::parameter::key
{{
}};

enum class stream_key : runtime::fx::stream_key
{{
}};

auto make_module(runtime::internal_fx_module_factory_args const&)
    -> runtime::fx::module;

}} // namespace piejam::fx_modules::{module}
"""

COMPONENT_CPP_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "{module}_component.h"

#include "{module}_module.h"

#include <piejam/audio/engine/component.h>

namespace piejam::fx_modules::{module}
{{

namespace
{{

class component final : public audio::engine::component
{{
public:
    component()
    {{
    }}

    auto inputs() const -> endpoints override
    {{
        return {{}};
    }}

    auto outputs() const -> endpoints override
    {{
        return {{}};
    }}

    auto event_inputs() const -> endpoints override
    {{
        return {{}};
    }}

    auto event_outputs() const -> endpoints override
    {{
        return {{}};
    }}

    void connect(audio::engine::graph& /*g*/) const override
    {{
    }}

private:
}};

}} // namespace

auto
make_component(runtime::internal_fx_component_factory_args const& /*args*/)
    -> std::unique_ptr<audio::engine::component>
{{
    return std::make_unique<component>();
}}

}} // namespace piejam::fx_modules::{module}
"""

INTERNAL_ID_CPP_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "{module}_internal_id.h"

#include "{module}_component.h"
#include "{module}_module.h"
#include "gui/Fx{Class}.h"

#include "../module_registration.h"

namespace piejam::fx_modules::{module}
{{

void
init()
{{
    static std::once_flag s_init;
    std::call_once(s_init, []() {{
        PIEJAM_FX_MODULES_MODEL(gui::Fx{Class}, "Fx{Class}");
        internal_id();
    }});
}}

auto
internal_id() -> runtime::fx::internal_id
{{
    using namespace std::string_literals;

    static auto const id = register_module(
        module_registration{{
            .available_for_mono = true,
            .persistence_name = "{module}"s,
            .fx_module_factory = &make_module,
            .fx_component_factory = &make_component,
            .fx_browser_entry_name = "{name}",
            .fx_browser_entry_description =
                "{name} effect module.",
            .fx_module_content_factory =
                &piejam::gui::model::makeFxModule<gui::Fx{Class}>,
            .viewSource = "/PieJam/FxChainControls/GenericFxModuleView.qml",
        }});
    return id;
}}

}} // namespace piejam::fx_modules::{module}
"""

MODULE_CPP_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "{module}_module.h"

#include "{module}_internal_id.h"

#include <piejam/runtime/fx/module.h>
#include <piejam/runtime/internal_fx_module_factory.h>
#include <piejam/runtime/parameter/map.h>
#include <piejam/runtime/parameter_factory.h>

namespace piejam::fx_modules::{module}
{{

auto
make_module(runtime::internal_fx_module_factory_args const& args)
    -> runtime::fx::module
{{
    using namespace std::string_literals;

    runtime::parameter_factory params_factory{{args.params}};

    return runtime::fx::module{{
        .fx_instance_id = internal_id(),
        .name = box("{name}"s),
        .bus_type = args.bus_type,
        .parameters = {{}},
        .streams = {{}},
    }};
}}

}} // namespace piejam::fx_modules::{module}
"""

FX_GUI_H_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <piejam/gui/PropertyMacros.h>
#include <piejam/gui/model/FxModule.h>
#include <piejam/gui/model/SubscribableModel.h>
#include <piejam/gui/model/fwd.h>

#include <piejam/pimpl.h>
#include <piejam/runtime/fx/fwd.h>

namespace piejam::fx_modules::{module}::gui
{{

class Fx{Class} final : public piejam::gui::model::FxModule
{{
    Q_OBJECT

public:
    Fx{Class}(runtime::state_access const&, runtime::fx::module_id);

    auto type() const noexcept -> piejam::gui::model::FxModuleType override;

private:
    void onSubscribe() override;

    struct Impl;
    pimpl<Impl> m_impl;
}};

}} // namespace piejam::fx_modules::{module}::gui
"""

FX_GUI_CPP_TEMPLATE = """// PieJam - An audio mixer for Raspberry Pi.
// SPDX-FileCopyrightText: 2020-2026  Dimitrij Kotrev
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Fx{Class}.h"

#include "../{module}_internal_id.h"

namespace piejam::fx_modules::{module}::gui
{{

using namespace piejam::gui::model;

struct Fx{Class}::Impl
{{
}};

Fx{Class}::Fx{Class}(runtime::state_access const& state_access, runtime::fx::module_id id)
    : FxModule{{state_access, id}}
    , m_impl{{make_pimpl<Impl>()}}
{{
}}

auto
Fx{Class}::type() const noexcept -> FxModuleType
{{
    return {{.id = internal_id()}};
}}

void
Fx{Class}::onSubscribe()
{{
}}

}} // namespace piejam::fx_modules::{module}::gui
"""

CMAKE_TEMPLATE = """# SPDX-FileCopyrightText: 2020-2026 Dimitrij Kotrev
#
# SPDX-License-Identifier: CC0-1.0

target_sources(piejam_fx_modules PRIVATE
    {module}_component.cpp
    {module}_component.h
    {module}_internal_id.cpp
    {module}_internal_id.h
    {module}_module.cpp
    {module}_module.h
    gui/Fx{Class}.cpp
    gui/Fx{Class}.h
)
"""

# --- implementation ---
def create_module(name: str):
    parts = [p for p in name.strip().split() if p]

    # snake_case
    snake = "_".join(p.lower() for p in parts)

    # PascalCase
    pascal = "".join(p.capitalize() for p in parts)

    module = snake
    Class = pascal

    mod_dir = FX_ROOT / module
    gui_dir = mod_dir / "gui"
    gui_dir.mkdir(parents=True, exist_ok=True)

    def write(path: Path, content: str):
        if path.exists():
            print(f"⚠️  Skipping existing file: {path}")
        else:
            path.write_text(content)
            print(f"✅ Created {path.relative_to(PROJECT_ROOT)}")

    # main module files
    write(mod_dir / f"{module}_component.h", COMPONENT_H_TEMPLATE.format(module=module, Class=Class))
    write(mod_dir / f"{module}_component.cpp", COMPONENT_CPP_TEMPLATE.format(module=module, Class=Class))
    write(mod_dir / f"{module}_internal_id.h", INTERNAL_ID_H_TEMPLATE.format(module=module, Class=Class))
    write(mod_dir / f"{module}_internal_id.cpp", INTERNAL_ID_CPP_TEMPLATE.format(module=module, Class=Class, name=name))
    write(mod_dir / f"{module}_module.h", MODULE_H_TEMPLATE.format(module=module, Class=Class))
    write(mod_dir / f"{module}_module.cpp", MODULE_CPP_TEMPLATE.format(module=module, Class=Class, name=name))

    # gui files
    write(gui_dir / f"Fx{Class}.h", FX_GUI_H_TEMPLATE.format(module=module, Class=Class))
    write(gui_dir / f"Fx{Class}.cpp", FX_GUI_CPP_TEMPLATE.format(module=module, Class=Class))

    write(mod_dir / "CMakeLists.txt", CMAKE_TEMPLATE.format(module=module, Class=Class))

    print("\n📄 Add this to CMakeLists.txt:")
    print(textwrap.dedent(f"""
        # --- {module} module ---
        add_subdirectory(src/piejam/fx_modules/{module})
    """))


# --- entrypoint ---
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: create_fx_module.py <Name>")
        sys.exit(1)
    create_module(sys.argv[1])

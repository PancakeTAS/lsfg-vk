use adw::subclass::prelude::ObjectSubclassIsExt;
use gtk::{glib::object::CastNone, prelude::{EditableExt, RangeExt}};

use crate::{config, wrapper::{pane, entry}, STATE};

// update the currently selected game configuration
fn update_game<F: FnOnce(&mut config::TomlGame)>(update: F) {
    if let Ok(state) = STATE.get().unwrap().try_read() {
        if let Some(selected_game) = state.selected_game {
            let _ = config::edit_config(|config| {
                update(&mut config.game[selected_game])
            });
        }
    }
}

///
/// Register signals for preset preferences.
///
pub fn register_signals(sidebar_: pane::PaneSidebar, main: &pane::PaneMain) {
    let main = main.imp();
    let exe = main.preset_name.imp();
    let multiplier = main.multiplier.imp();
    let flow_scale = main.flow_scale.imp();
    let performance_mode = main.performance_mode.imp();
    let hdr_mode = main.hdr_mode.imp();
    let experimental_present_mode = main.experimental_present_mode.imp();

    let sidebar = sidebar_.clone();
    exe.entry.connect_changed(move |entry| {
        let mut exe = entry.text().to_string();
        if exe.trim().is_empty() {
            exe = "new preset".to_string();
        }

        // rename list entry
        let row = sidebar.imp().profiles.selected_row()
            .and_downcast::<entry::Entry>().unwrap();
        row.set_exe(exe.clone());

        // update the game configuration
        update_game(|conf| {
            conf.exe = exe;
        });
    });
    multiplier.number.connect_value_changed(|dropdown| {
        update_game(|conf| {
            conf.multiplier = (dropdown.value() as i64).into();
        })
    });
    flow_scale.slider.connect_value_changed(|slider| {
        update_game(|conf| {
            conf.flow_scale = (slider.value() / 100.0).into();
        });
    });
    performance_mode.switch.connect_state_notify(|switch| {
        update_game(|conf| {
            conf.performance_mode = switch.state();
        });
    });
    hdr_mode.switch.connect_state_notify(|switch| {
        update_game(|conf| {
            conf.hdr_mode = switch.state();
        });
    });
    experimental_present_mode.dropdown.connect_selected_notify(|dropdown| {
        update_game(|conf| {
            conf.experimental_present_mode = match dropdown.selected() {
                0 => config::PresentMode::Vsync,
                1 => config::PresentMode::Mailbox,
                2 => config::PresentMode::Immediate,
                _ => config::PresentMode::Vsync,
            };
        });
    });

    let dll = main.dll.imp();
    dll.entry.connect_changed(|entry| {
        let _ = config::edit_config(|config| {
            let text = entry.text().to_string();
            if text.trim().is_empty() {
                config.global.dll = None;
            } else {
                config.global.dll = Some(text);
            }
        });
    });
}

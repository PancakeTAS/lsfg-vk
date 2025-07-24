use adw::subclass::prelude::ObjectSubclassIsExt;
use gtk::prelude::RangeExt;

use crate::{config, wrapper::pane, STATE};

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
pub fn register_signals(main: &pane::PaneMain) {
    let main = main.imp();
    let multiplier = main.multiplier.imp();
    let flow_scale = main.flow_scale.imp();
    let performance_mode = main.performance_mode.imp();
    let hdr_mode = main.hdr_mode.imp();
    let experimental_present_mode = main.experimental_present_mode.imp();

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
}

use std::sync::{Arc, OnceLock, RwLock};

use adw::{self, subclass::prelude::ObjectSubclassIsExt};
use gtk::{gio, prelude::*};

use crate::config::*;

mod wrapper;
mod config;

const APP_ID: &str = "gay.pancake.lsfg-vk.ConfigurationUi";

#[derive(Debug)]
struct State {
    // ui state
    selected_game: Option<usize>
}

static STATE: OnceLock<Arc<RwLock<State>>> = OnceLock::new();

fn main() {
    gio::resources_register_include!("ui.gresource")
        .expect("Failed to register resources");
    config::load_config()
        .expect("Failed to load configuration");

    // prepare the application state
    STATE.set(Arc::new(RwLock::new(State {
        selected_game: Some(0)
    }))).expect("Failed to set application state");

    // start the application
    let app = adw::Application::builder()
        .application_id(APP_ID)
        .build();
    app.connect_activate(build_ui);
    app.run();
}

fn build_ui(app: &adw::Application) {
    // create the main window
    let window = wrapper::Window::new(app);
    window.set_application(Some(app));

    // load profiles from configuration
    let sidebar = window.imp().sidebar.imp();

    let config = config::get_config()
        .expect("Failed to get configuration");
    for game in config.game.iter() {
        let entry = wrapper::entry::Entry::new();
        entry.set_exe(game.exe.clone());
        sidebar.profiles.append(&entry);
    }

    // register main pane signals
    let main = window.imp().main.imp();

    let pref_multiplier = main.pref_multiplier.imp();
    pref_multiplier.number.connect_value_changed(|dropdown| {
        if let Ok(state) = STATE.get().unwrap().try_read() {
            if state.selected_game.is_none() {
                return;
            }

            let multiplier = (dropdown.value() as i64).into();
            let _ = config::edit_config(|config| {
                config.game[state.selected_game.unwrap()]
                    .multiplier = multiplier;
            });
        }
    });

    let pref_flow_scale = main.pref_flow_scale.imp();
    pref_flow_scale.slider.connect_value_changed(|slider| {
        if let Ok(state) = STATE.get().unwrap().try_read() {
            if state.selected_game.is_none() {
                return;
            }

            let flow_scale = (slider.value() / 100.0).into();
            let _ = config::edit_config(|config| {
                config.game[state.selected_game.unwrap()]
                    .flow_scale = flow_scale;
            });
        }
    });

    let pref_performance_mode = main.pref_performance_mode.imp();
    pref_performance_mode.switch.connect_state_notify(|switch| {
        if let Ok(state) = STATE.get().unwrap().try_read() {
            if state.selected_game.is_none() {
                return;
            }

            let performance_mode = switch.state();
            let _ = config::edit_config(|config| {
                config.game[state.selected_game.unwrap()]
                    .performance_mode = performance_mode;
            });
        }
    });

    let pref_hdr_mode = main.pref_hdr_mode.imp();
    pref_hdr_mode.switch.connect_state_notify(|switch| {
        if let Ok(state) = STATE.get().unwrap().try_read() {
            if state.selected_game.is_none() {
                return;
            }

            let hdr_mode = switch.state();
            let _ = config::edit_config(|config| {
                config.game[state.selected_game.unwrap()]
                    .hdr_mode = hdr_mode;
            });
        }
    });

    let pref_experimental_present_mode = main.pref_experimental_present_mode.imp();
    pref_experimental_present_mode.dropdown.connect_selected_notify(|dropdown| {
        if let Ok(state) = STATE.get().unwrap().try_read() {
            if state.selected_game.is_none() {
                return;
            }

            let selected = match dropdown.selected() {
                0 => PresentMode::Vsync,
                1 => PresentMode::Mailbox,
                2 => PresentMode::Immediate,
                _ => PresentMode::Vsync,
            };
            config::edit_config(|config| {
                config.game[state.selected_game.unwrap()]
                    .experimental_present_mode = selected;
            }).unwrap();
        }
    });

    // present the window
    window.present();
}

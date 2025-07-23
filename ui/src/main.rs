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
        selected_game: None
    }))).expect("Failed to set application state");

    // start the application
    let app = adw::Application::builder()
        .application_id(APP_ID)
        .build();
    app.connect_activate(build_ui);
    app.run();
}

fn helper_add_deletion_signal(
        entry: wrapper::entry::Entry,
        profiles: gtk::ListBox) {
    let entry_clone = entry.clone();
    entry.imp().delete.connect_clicked(move |btn| {
        let dialog = gtk::AlertDialog::builder()
            .message("Delete Profile")
            .detail("Are you sure you want to delete this profile?")
            .buttons(vec!["Cancel".to_string(), "Delete".to_string()])
            .cancel_button(0)
            .default_button(1)
            .modal(true)
            .build();
        let window = btn.root()
            .and_downcast::<gtk::Window>()
            .expect("Button root is not a Window");

        let profiles = profiles.clone();
        let entry_clone = entry_clone.clone();
        dialog.choose(Some(&window), None::<&gio::Cancellable>, move |result| {
            match result {
                Ok(idx) if idx == 1 => {
                    let _ = config::edit_config(|config| {
                        config.game.retain(|g| g.exe != entry_clone.exe());
                    });
                    profiles.remove(&entry_clone);
                },
                _ => return,
            };
        });
    });
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

        helper_add_deletion_signal(entry.clone(), sidebar.profiles.clone());
        sidebar.profiles.append(&entry);
    }

    // register side pane signals
    let profiles = sidebar.profiles.clone();
    let main = window.imp().main.clone();
    sidebar.create.connect_clicked(move |_| {
        let mut conf_entry = config::TomlGame::default();
        conf_entry.exe = "new profile".to_string();
        let _ = config::edit_config(|config| {
            config.game.push(conf_entry.clone());
        });

        let entry = wrapper::entry::Entry::new();
        entry.set_exe(conf_entry.exe);

        helper_add_deletion_signal(entry.clone(), profiles.clone());
        profiles.append(&entry);
        entry.activate();
    });

    let state = STATE.get().unwrap().clone();
    sidebar.profiles.connect_row_activated(move |_, entry| {
        // find config entry
        let index = entry.index() as usize;
        let config = config::get_config()
            .expect("Failed to get configuration");
        let conf = config.game[index].clone();

        // update state
        {
            let mut state = state.write()
                .expect("Failed to acquire write lock on state");
            state.selected_game = Some(index);
        }

        // update main pane
        let main = main.imp();
        let pref_multiplier = main.pref_multiplier.imp();
        pref_multiplier.number.set_value(conf.multiplier.into());
        let pref_flow_scale = main.pref_flow_scale.imp();
        pref_flow_scale.slider.set_value(Into::<f64>::into(conf.flow_scale) * 100.0);
        let pref_performance_mode = main.pref_performance_mode.imp();
        pref_performance_mode.switch.set_state(conf.performance_mode);
        let pref_hdr_mode = main.pref_hdr_mode.imp();
        pref_hdr_mode.switch.set_state(conf.hdr_mode);
        let pref_experimental_present_mode = main.pref_experimental_present_mode.imp();
        let mode = match conf.experimental_present_mode {
            PresentMode::Vsync => 0,
            PresentMode::Mailbox => 1,
            PresentMode::Immediate => 2,
        };
        pref_experimental_present_mode.dropdown.set_selected(mode);
    });

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

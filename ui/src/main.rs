use std::sync::{Arc, OnceLock, RwLock};

use gtk::{gio, prelude::*};
use adw;

mod ui;
mod wrapper;
mod config;
mod utils;

const APP_ID: &str = "gay.pancake.lsfg-vk";

#[derive(Debug)]
struct State {
    selected_game: Option<usize>
}

static STATE: OnceLock<Arc<RwLock<State>>> = OnceLock::new();

fn main() {
    gio::resources_register_include!("lsfg-vk.gresource")
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
    app.connect_activate(ui::build);
    app.run();
}

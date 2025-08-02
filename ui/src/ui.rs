use adw::{self, subclass::prelude::ObjectSubclassIsExt};
use gtk::prelude::{WidgetExt, EditableExt, GtkWindowExt};

use crate::config;
use crate::wrapper;

pub mod entry_handler;
pub mod main_handler;
pub mod sidebar_handler;

pub fn build(app: &adw::Application) {
    // create the main window
    let window = wrapper::Window::new(app);
    window.set_application(Some(app));
    let imp = window.imp();

    // load profiles from configuration
    let config = config::get_config().unwrap();
    for game in config.game.iter() {
        let entry = wrapper::entry::Entry::new();
        entry.set_exe(game.exe.clone());
        entry_handler::add_entry(entry, imp.sidebar.imp().profiles.clone());
    }

    if let Some(dll_path) = config.global.dll {
        imp.main.imp().dll.imp().entry.set_text(&dll_path);
    }

    // Load MangoHUD configuration
    let mangohud_path = format!("{}/.config/MangoHud/MangoHud.conf", 
        std::env::var("HOME").unwrap_or_else(|_| String::from("/")));
    
    if std::path::Path::new(&mangohud_path).exists() {
        if let Ok(mangohud_config) = config::load_mangohud_config() {
            imp.main.imp().mangohud_fps_limit.set_value(mangohud_config.fps_limit as f64);
        }
        imp.main.imp().mangohud_fps_limit.set_sensitive(true);
        imp.main.imp().mangohud_subtitle.set_text("Set the frame rate limit for MangoHUD overlay. (0 = no limit)");
    } else {
        // Disable the MangoHUD FPS limit field if config file doesn't exist
        imp.main.imp().mangohud_fps_limit.set_sensitive(false);
        imp.main.imp().mangohud_fps_limit.set_value(0.0);
        imp.main.imp().mangohud_subtitle.set_text("MangoHUD config file not found. Please install MangoHUD first.");
    }

    // register handlers on sidebar pane.
    sidebar_handler::register_signals(&imp.sidebar, imp.main.clone());

    // register handlers on main pane.
    main_handler::register_signals(imp.sidebar.clone(), &imp.main);

    // activate the first profile if available
    if let Some(entry) = imp.sidebar.imp().profiles.row_at_index(0) {
        entry.activate();
    }

    // present the window
    window.present();
}

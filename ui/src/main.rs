use adw;
use gtk::{gio, prelude::*};

pub mod wrapper {
    pub mod ui;
    pub mod pref;
}

const APP_ID: &str = "gay.pancake.lsfg-vk.ConfigurationUi";

fn main() {
    gio::resources_register_include!("ui.gresource")
        .expect("Failed to register resources");

    let app = adw::Application::builder()
        .application_id(APP_ID)
        .build();
    app.connect_activate(build_ui);
    app.run();
}

fn build_ui(app: &adw::Application) {
    let window = wrapper::ui::Window::new(app);
    window.set_application(Some(app));
    window.present();
}

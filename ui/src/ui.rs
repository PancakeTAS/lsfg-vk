use std::sync::Arc;

use adw;
use gtk::{gio, glib, prelude::*};

pub mod ui;
pub mod pref;

pub struct App {
    app: Arc<adw::Application>,
}

impl App {
    pub fn new(appid: &str) -> Result<Self, glib::Error> {
        gio::resources_register_include!("ui.gresource")?;

        let app = adw::Application::builder()
            .application_id(appid)
            .build();
        app.connect_activate(Self::build_ui);

        Ok(App {
            app: Arc::new(app)
        })
    }

    fn build_ui(app: &adw::Application) {
        let window = ui::Window::new(app);
        window.set_application(Some(app));
        window.present();
    }

    pub fn run(self){
        self.app.run();
    }
}

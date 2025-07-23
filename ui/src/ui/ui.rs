use gtk::glib;
use gtk;
use adw;
use gtk::glib::types::StaticTypeExt;

pub mod window;

glib::wrapper! {
    pub struct Window(ObjectSubclass<window::Window>)
        @extends
            adw::ApplicationWindow, adw::Window,
            gtk::ApplicationWindow, gtk::Window, gtk::Widget,
        @implements
            gtk::gio::ActionGroup, gtk::gio::ActionMap,
            gtk::Accessible, gtk::Buildable, gtk::ConstraintTarget,
            gtk::Native, gtk::Root, gtk::ShortcutManager;
}

impl Window {
    pub fn new(app: &adw::Application) -> Self {
        super::pref::PrefDropdown::ensure_type();
        super::pref::PrefSwitch::ensure_type();
        super::pref::PrefEntry::ensure_type();

        glib::Object::builder()
            .property("application", app)
            .build()
    }
}
